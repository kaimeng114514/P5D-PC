// PSV原生移植 - 纹理格式与SceGxmTexture/GXP sampler/DDS 解析实现
#include "texture_format.h"
#include <vulkan/vulkan.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <algorithm>

namespace p5d {
namespace tex {

// ---------------------------------------------------------------------------
// GXP sampler 参数表解析（SceGxmProgramParameter，每项 16 字节，Vita3K 权威布局）
//   off0 name_offset(int32, 相对本参数起始)
//   off4 category:4 | type:4 (uint8)
//   off5 component_count:4 | container_index:4
//   off6 semantic(uint8)  off7 semantic_index(uint8)
//   off8 array_size(u32)  off12 resource_index(u32)
// SceGxmProgram: parameter_count@0x24, parameters_offset@0x28（相对该字段自身）
// ---------------------------------------------------------------------------
std::vector<GxpSamplerParam> parse_gxp_samplers(const SceUInt8* gxp, SceSize gxp_size) {
    std::vector<GxpSamplerParam> out;
    if (!gxp || gxp_size < 0x30 || std::memcmp(gxp, "GXP\0", 4) != 0) return out;
    auto rd32 = [&](SceUInt32 o) { SceUInt32 v; std::memcpy(&v, gxp + o, 4); return v; };
    SceUInt32 pcount = rd32(0x24);
    SceUInt32 poff   = rd32(0x28);
    const SceUInt8* base = gxp + 0x28 + poff;
    if (pcount > 256) return out;
    // container19(TEXTURE) 的 base_sa_offset：sampler SA 号 = base + 2*序号（Vita3K decode_src12 double_reg: num<<1）
    SceUInt32 tex_base_sa = 0;
    if (gxp_size >= 0x9c) {
        SceUInt32 ccnt = rd32(0x90), coff = rd32(0x94);
        for (SceUInt32 c = 0; c < ccnt && c < 16; c++) {
            SceUInt32 co = 0x94 + coff + c * 8;
            if (co + 8 > gxp_size) break;
            SceUInt16 ci=0, bsa=0; std::memcpy(&ci, gxp + co + 0, 2); std::memcpy(&bsa, gxp + co + 4, 2);
            if (ci == 19) { tex_base_sa = bsa; break; }
        }
    }
    for (SceUInt32 k = 0; k < pcount; k++) {
        const SceUInt8* p = base + k * 16;
        if (p + 16 > gxp + gxp_size) break;
        SceInt32 name_off; std::memcpy(&name_off, p + 0, 4);
        SceUInt8 catbyte = p[4];
        SceUInt8 category = catbyte & 0xF;      // 2 = SAMPLER
        if (category != 2) continue;
        GxpSamplerParam s;
        s.semantic_index = p[7];
        s.is_cube = (s.semantic_index >> 4) & 1; // Vita3K is_sampler_cube()
        std::memcpy(&s.resource_index, p + 12, 4);
        const SceUInt8* nm = p + name_off;
        SceUInt32 n = 0;
        while (nm + n < gxp + gxp_size && nm[n] != 0 && n < sizeof(s.name) - 1) { s.name[n] = (char)nm[n]; n++; }
        s.name[n] = 0;
        s.sa_num = tex_base_sa + 2u * (SceUInt32)out.size();  // double-reg：每 sampler 占 2 个 SA 槽
        out.push_back(s);
    }
    return out;
}

// ---------------------------------------------------------------------------
// SceGxmTexture 16 字节位域
// ---------------------------------------------------------------------------
bool parse_sce_gxm_texture(const SceUInt8* p, SceGxmTextureInfo& o) {
    if (!p) return false;
    auto rd = [&](int i) { SceUInt32 v; std::memcpy(&v, p + i*4, 4); return v; };
    SceUInt32 w0 = rd(0), w1 = rd(1), w2 = rd(2), w3 = rd(3);
    // Control Word 0（位域逐字对照 Vita3K SceGxmTexture）
    o.vaddr_mode = (w0 >> 3) & 0x7;
    o.uaddr_mode = (w0 >> 6) & 0x7;
    o.mip_filter = (w0 >> 9) & 0x1;
    o.min_filter = (w0 >> 10) & 0x3;
    o.mag_filter = (w0 >> 12) & 0x3;
    SceUInt32 mc  = (w0 >> 17) & 0xF;
    SceUInt32 cnt = (mc + 1) & 0xF;
    o.mip_count   = cnt ? cnt : 1;
    o.lod_bias    = (w0 >> 21) & 0x3F;
    o.gamma_mode  = (w0 >> 27) & 0x3;
    o.format0     = (w0 >> 31) & 0x1;
    // Control Word 1：base 占 bits28:24(5bit) + bit31(format0)，掩码 0x9F000000
    o.texture_type = (w1 >> 29) & 0x7;
    o.base_format  = (((w1 >> 24) & 0x1F) << 24) | (o.format0 << 31);
    o.is_cube = (o.texture_type == 2 || o.texture_type == 7); // CUBE / CUBE_ARBITRARY
    if (o.texture_type == 1 || o.texture_type == 2) {
        // h143av 权威(Vita3K textures.cpp get_width)：SWIZZLED=1/CUBE=2 宽高字段为 log2
        o.height = 1u << (w1 & 0xF);
        o.width  = 1u << ((w1 >> 12) & 0xF);
    } else {
        // LINEAR / TILED / ...：字段为 (尺寸-1)
        o.height = (w1 & 0xFFF) + 1;
        o.width  = ((w1 >> 12) & 0xFFF) + 1;
    }
    // Control Word 2：字地址 -> 字节虚拟地址 = word<<2
    o.data_addr_word = (w2 >> 2) & 0x3FFFFFFFu;
    o.data_byte_addr = o.data_addr_word << 2;
    // Control Word 3
    o.palette_word      = w3 & 0x3FFFFFFu;   // palette_addr（26bit）
    o.palette_byte_addr = o.palette_word << 6;  // h143ay 权威: byte=addr<<6（Vita3K get_texture_palette）
    o.swizzle       = (w3 >> 28) & 0x7;
    o.normalize_mode= (w3 >> 31) & 0x1;
    return true;
}

SceUInt32 component_count_for_base_format(SceUInt32 base) {
    switch (base) {
    // 1 通道：U8/S8/U16/S16/F16/F32/F32M/X8U24/U32/S32/UBC4/SBC4
    case 0x00000000: case 0x01000000: case 0x09000000: case 0x0A000000:
    case 0x0B000000: case 0x12000000: case 0x13000000: case 0x15000000:
    case 0x17000000: case 0x18000000: case 0x88000000: case 0x89000000:
        return 1;
    // 2 通道：U8U8/S8S8/U16U16/S16S16/F16F16/F32F32/U32U32/UBC5/SBC5
    case 0x07000000: case 0x08000000: case 0x0F000000: case 0x10000000:
    case 0x11000000: case 0x1E000000: case 0x1F000000: case 0x8A000000:
    case 0x8B000000:
        return 2;
    // 3 通道：U5U6U5/S5S5U6/X8S8S8U8/SE5M9M9/F11F11F10/YUV/U8U8U8/S8S8S8
    case 0x05000000: case 0x06000000: case 0x14000000: case 0x19000000:
    case 0x1A000000: case 0x90000000: case 0x91000000: case 0x92000000:
    case 0x98000000: case 0x99000000:
        return 3;
    default:
        return 4; // U8U8U8U8 / UBC1/UBC2/UBC3 / P4 / P8 / PVRTC 等
    }
}

bool resolve_gxm_texture(const SceUInt8* control16, ResolvedGxmTexture& out) {
    if (!parse_sce_gxm_texture(control16, out.info)) return false;
    out.vk_format = sce_base_format_to_vk(out.info.base_format);
    out.component_count = component_count_for_base_format(out.info.base_format);
    out.swizzle_map = texture_swizzle(out.info.swizzle, out.component_count);
    return true;
}

// ---------------------------------------------------------------------------
// SceGxmTextureBaseFormat -> VkFormat
// ---------------------------------------------------------------------------
VkFmt sce_base_format_to_vk(SceUInt32 base_format) {
    switch (base_format) {
    // 未压缩
    case 0x00000000: return VK_FORMAT_R8_UNORM;                       // U8
    case 0x05000000: return VK_FORMAT_R5G6B5_UNORM_PACK16;            // U5U6U5
    case 0x04000000: return VK_FORMAT_A1R5G5B5_UNORM_PACK16;          // U1U5U5U5
    case 0x07000000: return VK_FORMAT_R8G8_UNORM;                     // U8U8
    case 0x09000000: return VK_FORMAT_R16_UNORM;                      // U16
    case 0x0B000000: return VK_FORMAT_R16_SFLOAT;                     // F16
    // ★★★ h145k297【已尝试并**实测否定**：把 U8U8U8U8 的 R/B 换过来 —— 无效，故保持原映射】★★★
    //   起点：用户实测反馈窗口里 ATLUS logo 是**金色+紫色**，而原版应为**蓝色 + 中间红色的 T**。
    //   对渲染帧做像素统计（_h145k296_colors.py，p5d_seq_2800.bmp）：
    //       主色 #FFE00E(金) / #FFDF00(金) / #4B00F4(紫) / #D6A407(暗金) / #FFFFFF(白)
    //   每个色做 R↔B 交换后恰好变成 #0EE0FF(亮蓝) / #F4004B(绯红) / #FFFFFF —— 与描述完全吻合，
    //   因此一度判为"本格式字节序是 BGRA"。**h145k297 实测推翻了该结论**：
    //       tag=h145k297_rbswap 把 0x0C000000 改为 VK_FORMAT_B8G8R8A8_UNORM（stderr 确有
    //       `[TEXFMT] U8U8U8U8(0x0C000000) -> VK_FORMAT_B8G8R8A8_UNORM`，即该分支**确实被执行**），
    //       但 p5d_seq_2800 的主色**逐字节不变**（仍是 #FFE00E / #4B00F4）。
    //   ⇒ **R/B 交换不在这个格式映射里**。可能的位置（尚未验证，留给下一轮）：
    //       (a) 产出该 logo 的那张纹理根本不是 0x0C000000（`[TEXMAT]` 只打了 tp=3，没打 base_format）；
    //       (b) 颜色是 **app 自己的片元着色器**染出来的，而 `P5D_DIAG_FS_ALL=1` 把它的着色器整个替换掉了，
    //           所以我们看到的是**未着色的原始纹理**（金/紫），app 真实着色后才是蓝/红；
    //       (c) 交换发生在 RT/呈现/截屏链路的某一环（但白色描边不变、且用户是在**窗口里**看到金色的，
    //           所以这一项可能性较低）。
    //   撤销理由：该改动会全局影响所有 U8U8U8U8 纹理，却**没有任何实测收益** ⇒ 按项目规范回退，避免引入未验证行为。
    case 0x0C000000: return VK_FORMAT_R8G8B8A8_UNORM;                 // U8U8U8U8（保持原映射）
    // ★★★ h145k623【1 通道格式：LOGO 画布就是 U8（0x15000000）—— 黑屏的直接根因】★★★
    //   实证链条：
    //     · LOGO 画布 1024x512 占 512KB ⇒ 1 字节/像素；其 fmt=0x95001000
    //       ⇒ base_format = fmt & 0x1F000000 = 0x15000000；
    //     · `component_count_for_base_format(0x15000000)` 返回 1 ⇒ 单通道，吻合；
    //     · 但本表原先**没有** 0x15000000 ⇒ 走 default ⇒ 返回 FMT_UNDEFINED；
    //     · `vulkan_backend.cpp:2046` 对 FMT_UNDEFINED 直接 `return -2`（"needs transcode"）
    //       ⇒ **画布纹理从未上传** ⇒ 采样返回 0（alpha=0）；
    //     · 而 `[BLENDMAP]` 实测 frag_0084 的混合是 `enable=1 cs=4(SRC_ALPHA) cd=5(ONE_MINUS_SRC_ALPHA)`
    //       ⇒ alpha=0 ⇒ 这一发 draw 什么都不写 ⇒ **LOGO 相位全黑**。
    //   交叉印证（两处独立实验都出图）：
    //     · 把 u0 强行指向 A6B37280（U8U8U8U8=0x0C000000，**表里有**）⇒ mean=0.57；
    //     · `P5D_FORCE_DUMMY_TEX=1`（1x1 白色 dummy）⇒ mean=0.66。
    case 0x15000000: return VK_FORMAT_R8_UNORM;                       // U8（单通道）
    // h145k724【编译修复】此处原先重复定义了 0x00000000 / 0x09000000 / 0x0B000000，
    //   与上面（本 switch 开头）的同名 case **映射到完全相同的格式值** ⇒ 冗余，
    //   编译器报 `error C2196: case 值已使用`，导致本 TU **自 2026-09-22 起再也编不过**，
    //   链接的一直是陈旧 obj。删除重复项属**行为等价**改动（同 label 同值）。
    case 0x01000000: return VK_FORMAT_R8_SNORM;                       // S8
    case 0x0A000000: return VK_FORMAT_R16_SNORM;                      // S16
    case 0x13000000: return VK_FORMAT_R32_UINT;                       // U32
    case 0x17000000: return VK_FORMAT_R32_SFLOAT;                     // F32M
    case 0x18000000: return VK_FORMAT_R32_SINT;                       // S32
    case 0x0E000000: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;       // U2U10U10U10
    case 0x0F000000: return VK_FORMAT_R16G16_UNORM;                   // U16U16
    case 0x11000000: return VK_FORMAT_R16G16_SFLOAT;                  // F16F16
    case 0x12000000: return VK_FORMAT_R32_SFLOAT;                     // F32
    case 0x1A000000: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;        // F11F11F10
    case 0x1B000000: return VK_FORMAT_R16G16B16A16_SFLOAT;            // F16x4
    case 0x1C000000: return VK_FORMAT_R16G16B16A16_UNORM;             // U16x4
    case 0x1E000000: return VK_FORMAT_R32G32_SFLOAT;                  // F32F32
    // 块压缩（PSV BC = 桌面 DXTn，Vulkan 原生支持）
    case 0x85000000: return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;           // UBC1 / DXT1
    case 0x86000000: return VK_FORMAT_BC2_UNORM_BLOCK;                // UBC2 / DXT3
    case 0x87000000: return VK_FORMAT_BC3_UNORM_BLOCK;                // UBC3 / DXT5
    case 0x88000000: return VK_FORMAT_BC4_UNORM_BLOCK;                // UBC4
    case 0x89000000: return VK_FORMAT_BC4_SNORM_BLOCK;                // SBC4
    case 0x8A000000: return VK_FORMAT_BC5_UNORM_BLOCK;                // UBC5
    case 0x8B000000: return VK_FORMAT_BC5_SNORM_BLOCK;                // SBC5
    // PVRTC(0x80-0x83)、YUV、P4/P8 调色板、3通道U8U8U8(0x98) 等需转码/补通道，暂不支持
    default: return FMT_UNDEFINED;
    }
}

BlockInfo block_info_for_vk(VkFmt f) {
    switch (f) {
    case VK_FORMAT_BC1_RGBA_UNORM_BLOCK:
    case VK_FORMAT_BC1_RGB_UNORM_BLOCK:
    case VK_FORMAT_BC4_UNORM_BLOCK:
    case VK_FORMAT_BC4_SNORM_BLOCK:
        return { 4, 4, 8, true };
    case VK_FORMAT_BC2_UNORM_BLOCK:
    case VK_FORMAT_BC3_UNORM_BLOCK:
    case VK_FORMAT_BC5_UNORM_BLOCK:
    case VK_FORMAT_BC5_SNORM_BLOCK:
    case VK_FORMAT_BC6H_UFLOAT_BLOCK:
    case VK_FORMAT_BC6H_SFLOAT_BLOCK:
    case VK_FORMAT_BC7_UNORM_BLOCK:
        return { 4, 4, 16, true };
    case VK_FORMAT_R8_UNORM:            return { 1,1,1,false };
    case VK_FORMAT_R8G8_UNORM:          return { 1,1,2,false };
    case VK_FORMAT_R5G6B5_UNORM_PACK16:
    case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
    case VK_FORMAT_R16_UNORM:
    case VK_FORMAT_R16_SFLOAT:
        return { 1,1,2,false };
    case VK_FORMAT_R8G8B8A8_UNORM:
    case VK_FORMAT_R16G16_UNORM:
    case VK_FORMAT_R16G16_SFLOAT:
    case VK_FORMAT_R32_SFLOAT:
        return { 1,1,4,false };
    case VK_FORMAT_R16G16B16A16_UNORM:
    case VK_FORMAT_R16G16B16A16_SFLOAT:
    case VK_FORMAT_R32G32_SFLOAT:
        return { 1,1,8,false };
    default:                            return { 1,1,4,false };
    }
}

SceUInt32 texture_byte_size(VkFmt f, SceUInt32 w, SceUInt32 h, SceUInt32 layers) {
    BlockInfo bi = block_info_for_vk(f);
    SceUInt32 bx = (w + bi.block_w - 1)/bi.block_w;
    SceUInt32 by = (h + bi.block_h - 1)/bi.block_h;
    return bx*by*bi.block_bytes*(layers ? layers : 1u);
}

// ---------------------------------------------------------------------------
// PSV 显存布局(SWIZZLED Morton / TILED 32x32) -> Vulkan 线性
// 算法逐字对照 Vita3K renderer/src/texture/format.cpp
// ---------------------------------------------------------------------------
namespace {
SceUInt32 tf_compact1by1(SceUInt32 x) { // 保留偶数位并压紧（Morton 解码）
    x &= 0x55555555u;
    x = (x ^ (x >> 1)) & 0x33333333u;
    x = (x ^ (x >> 2)) & 0x0F0F0F0Fu;
    x = (x ^ (x >> 4)) & 0x00FF00FFu;
    x = (x ^ (x >> 8)) & 0x0000FFFFu;
    return x;
}
inline SceUInt32 tf_mx(SceUInt32 c){ return tf_compact1by1(c >> 1); }
inline SceUInt32 tf_my(SceUInt32 c){ return tf_compact1by1(c); }
SceUInt32 tf_bit_width(SceUInt32 v){ SceUInt32 n=0; while(v){++n;v>>=1;} return n; }

// 压缩块(4x4)的 Morton z-order 反排：dst 线性块网格 <- src Morton 块序列
void tf_zorder_blocks(SceUInt8* dst,const SceUInt8* src,SceUInt32 w,SceUInt32 h,SceUInt32 blockBytes){
    SceUInt32 bcx=(w+3)/4, bcy=(h+3)/4;
    SceUInt32 mn = bcx<bcy?bcx:bcy, k = tf_bit_width(mn)-1;
    for(SceUInt32 i=0;i<bcx*bcy;++i){
        SceUInt32 x=tf_mx(i)&(mn-1), y=tf_my(i)&(mn-1);
        SceUInt32 upper=(i>>(2*k))<<k;
        if(bcx>=bcy)x|=upper; else y|=upper;
        std::memcpy(dst+(y*bcx+x)*blockBytes, src+i*blockBytes, blockBytes);
    }
}
// 非压缩像素 Morton 反排
void tf_swizzled_px(SceUInt8* dst,const SceUInt8* src,SceUInt32 w,SceUInt32 h,SceUInt32 bpp){
    SceUInt32 mn=w<h?w:h, k=tf_bit_width(mn)-1;
    for(SceUInt32 i=0;i<w*h;++i){
        SceUInt32 x=tf_mx(i)&(mn-1), y=tf_my(i)&(mn-1);
        SceUInt32 upper=(i>>(2*k))<<k;
        if(w>=h)x|=upper; else y|=upper;
        std::memcpy(dst+(y*w+x)*bpp, src+i*bpp, bpp);
    }
}
// TILED：32x32 块，块内 (x&31)|((y&31)<<5)
void tf_tiled_px(SceUInt8* dst,const SceUInt8* src,SceUInt32 w,SceUInt32 h,SceUInt32 bpp){
    SceUInt32 wt=(w+31)>>5;
    for(SceUInt32 y=0;y<h;++y)for(SceUInt32 x=0;x<w;++x){
        SceUInt32 inTile=(x&31)|((y&31)<<5);
        SceUInt32 tile=(x>>5)+wt*(y>>5);
        std::memcpy(dst+((y*w)+x)*bpp, src+(((tile<<10)|inTile))*bpp, bpp);
    }
}
} // anon

std::vector<SceUInt8> linearize_psv_texture(const SceUInt8* src,SceUInt32 w,SceUInt32 h,VkFmt vkfmt,SceUInt32 type_index){
    BlockInfo bi=block_info_for_vk(vkfmt);
    SceUInt32 total=texture_byte_size(vkfmt,w,h,1);
    std::vector<SceUInt8> dst(total);
    bool isSwz=(type_index==0||type_index==2||type_index==5||type_index==7);
    bool isTiled=(type_index==4);
    if(!src) return dst;
    if(bi.compressed){
        if(isSwz) tf_zorder_blocks(dst.data(),src,w,h,bi.block_bytes);
        else std::memcpy(dst.data(),src,total);
    } else {
        SceUInt32 bpp=bi.block_bytes; // 1x1 块，block_bytes 即每像素字节
        if(isSwz) tf_swizzled_px(dst.data(),src,w,h,bpp);
        else if(isTiled) tf_tiled_px(dst.data(),src,w,h,bpp);
        else std::memcpy(dst.data(),src,total);
    }
    return dst;
}

// 正向（线性 -> SWIZZLED Morton），linearize 的逆，仅供往返验证
std::vector<SceUInt8> swizzle_linear_texture(const SceUInt8* src,SceUInt32 w,SceUInt32 h,VkFmt vkfmt){
    BlockInfo bi=block_info_for_vk(vkfmt);
    SceUInt32 total=texture_byte_size(vkfmt,w,h,1);
    std::vector<SceUInt8> dst(total);
    if(!src) return dst;
    if(bi.compressed){
        SceUInt32 bcx=(w+3)/4,bcy=(h+3)/4,mn=bcx<bcy?bcx:bcy,k=tf_bit_width(mn)-1;
        for(SceUInt32 i=0;i<bcx*bcy;++i){
            SceUInt32 x=tf_mx(i)&(mn-1),y=tf_my(i)&(mn-1),upper=(i>>(2*k))<<k;
            if(bcx>=bcy)x|=upper;else y|=upper;
            std::memcpy(dst.data()+i*bi.block_bytes, src+(y*bcx+x)*bi.block_bytes, bi.block_bytes);
        }
    } else {
        SceUInt32 bpp=bi.block_bytes,mn=w<h?w:h,k=tf_bit_width(mn)-1;
        for(SceUInt32 i=0;i<w*h;++i){
            SceUInt32 x=tf_mx(i)&(mn-1),y=tf_my(i)&(mn-1),upper=(i>>(2*k))<<k;
            if(w>=h)x|=upper;else y|=upper;
            std::memcpy(dst.data()+i*bpp, src+(y*w+x)*bpp, bpp);
        }
    }
    return dst;
}

// ---------------------------------------------------------------------------
// DDS 解析
// ---------------------------------------------------------------------------
static VkFmt dds_fourcc_to_vk(const SceUInt8* cc) {
    if (std::memcmp(cc, "DXT1", 4) == 0) return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
    if (std::memcmp(cc, "DXT3", 4) == 0) return VK_FORMAT_BC2_UNORM_BLOCK;
    if (std::memcmp(cc, "DXT5", 4) == 0) return VK_FORMAT_BC3_UNORM_BLOCK;
    if (std::memcmp(cc, "ATI1", 4) == 0 || std::memcmp(cc, "BC4U", 4) == 0) return VK_FORMAT_BC4_UNORM_BLOCK;
    if (std::memcmp(cc, "ATI2", 4) == 0 || std::memcmp(cc, "BC5U", 4) == 0) return VK_FORMAT_BC5_UNORM_BLOCK;
    return FMT_UNDEFINED;
}

// ---------------------------------------------------------------------------
// PSV swizzle -> VkComponentSwizzle（ID=0 ZERO=1 ONE=2 R=3 G=4 B=5 A=6）
// 数值表逐字对照 Vita3K gxm_to_vulkan.cpp（texture 命名空间）
// ---------------------------------------------------------------------------
namespace {
constexpr SceUInt32 SW_ID=0, SW_ZERO=1, SW_ONE=2, SW_R=3, SW_G=4, SW_B=5, SW_A=6;
}

TexComponentMap texture_swizzle4(SceUInt32 ix) {
    switch (ix & 0x7) {
    case 0: return { SW_R, SW_G, SW_B, SW_A }; // ABGR
    case 1: return { SW_B, SW_G, SW_R, SW_A }; // ARGB
    case 2: return { SW_A, SW_B, SW_G, SW_R }; // RGBA
    case 3: return { SW_G, SW_B, SW_A, SW_R }; // BGRA
    case 4: return { SW_R, SW_G, SW_B, SW_ONE }; // 1BGR
    case 5: return { SW_B, SW_G, SW_R, SW_ONE }; // 1RGB
    case 6: return { SW_A, SW_B, SW_G, SW_ONE }; // RGB1
    default:return { SW_G, SW_B, SW_A, SW_ONE }; // BGR1
    }
}

static TexComponentMap texture_swizzle1(SceUInt32 ix) {
    switch (ix & 0x7) {
    case 0: return { SW_R, SW_ZERO, SW_ZERO, SW_ONE  }; // R
    case 1: return { SW_R, SW_ZERO, SW_ZERO, SW_ZERO }; // 000R
    case 2: return { SW_R, SW_ONE,  SW_ONE,  SW_ONE  }; // 111R
    case 3: return { SW_R, SW_R,    SW_R,    SW_R    }; // RRRR
    case 4: return { SW_R, SW_R,    SW_R,    SW_ZERO }; // 0RRR
    case 5: return { SW_R, SW_R,    SW_R,    SW_ONE  }; // 1RRR
    case 6: return { SW_ZERO, SW_ZERO, SW_ZERO, SW_R }; // R000
    default:return { SW_ONE, SW_ONE, SW_ONE, SW_R    }; // R111
    }
}

static TexComponentMap texture_swizzle2(SceUInt32 ix) {
    switch (ix & 0x7) {
    case 0: return { SW_R, SW_G, SW_ZERO, SW_ONE  }; // GR
    case 1: return { SW_R, SW_G, SW_ZERO, SW_ZERO }; // 00GR
    case 2: return { SW_R, SW_R, SW_R,    SW_G    }; // GRRR
    case 3: return { SW_G, SW_G, SW_G,    SW_R    }; // RGGG
    case 4: return { SW_R, SW_G, SW_R,    SW_G    }; // GRGR
    default:return { SW_G, SW_R, SW_ZERO, SW_ZERO }; // 00RG
    }
}

static TexComponentMap texture_swizzle3(SceUInt32 ix) {
    if ((ix & 0x7) == 0) return { SW_R, SW_G, SW_B, SW_ONE }; // BGR
    return { SW_B, SW_G, SW_R, SW_ONE };                       // RGB
}

TexComponentMap texture_swizzle(SceUInt32 ix, SceUInt32 component_count) {
    if (component_count <= 1) return texture_swizzle1(ix);
    if (component_count == 2) return texture_swizzle2(ix);
    if (component_count == 3) return texture_swizzle3(ix);
    return texture_swizzle4(ix); // 4 通道及 BC 块压缩
}

bool load_dds(const SceUInt8* d, SceSize size, DdsImage& o) {
    if (!d || size < 128 || std::memcmp(d, "DDS ", 4) != 0) return false;
    auto u32 = [&](SceUInt32 off) { SceUInt32 v; std::memcpy(&v, d + off, 4); return v; };
    o.height = u32(12); o.width = u32(16);
    SceUInt32 mips = u32(28);
    o.mip_count = (mips == 0) ? 1 : mips;
    SceUInt32 pf_flags = u32(80);
    SceUInt32 caps2 = u32(116);
    o.is_cube = (caps2 & 0x200) != 0;          // DDSCAPS2_CUBEMAP
    o.array_layers = o.is_cube ? 6 : 1;

    if (pf_flags & 0x4) {                      // DDPF_FOURCC
        o.vk_format = dds_fourcc_to_vk(d + 84);
    } else if (pf_flags & 0x40) {              // DDPF_RGB 未压缩
        SceUInt32 bits = u32(88);
        SceUInt32 rm = u32(92), gm = u32(96), bm = u32(100), am = u32(104);
        if (bits == 32 && rm == 0xFF && gm == 0xFF00 && bm == 0xFF0000 && am == 0xFF000000)
            o.vk_format = VK_FORMAT_R8G8B8A8_UNORM;
        else if (bits == 32 && bm == 0xFF && gm == 0xFF00 && rm == 0xFF0000 && am == 0xFF000000)
            o.vk_format = VK_FORMAT_B8G8R8A8_UNORM;
        else if (bits == 16 && rm == 0xF800 && gm == 0x7E0 && bm == 0x1F)
            o.vk_format = VK_FORMAT_R5G6B5_UNORM_PACK16;
        else
            o.vk_format = FMT_UNDEFINED;
    }
    if (o.vk_format == FMT_UNDEFINED) return false;
    o.block = block_info_for_vk(o.vk_format);

    // 仅取顶层(mip0)；cube 时 6 面连续
    SceUInt32 bw = (o.width + o.block.block_w - 1) / o.block.block_w;
    SceUInt32 bh = (o.height + o.block.block_h - 1) / o.block.block_h;
    SceUInt32 face_bytes = bw * bh * o.block.block_bytes;
    SceUInt32 need = face_bytes * o.array_layers;
    if (SceUInt32(size) < 128 + need) {
        // 数据不足时截断到可用长度
        need = (SceUInt32(size) > 128) ? (SceUInt32(size) - 128) : 0;
    }
    o.pixels.assign(d + 128, d + 128 + need);
    return true;
}

bool load_dds_file(const char* path, DdsImage& o) {
    FILE* fp = std::fopen(path, "rb");
    if (!fp) return false;
    std::fseek(fp, 0, SEEK_END); long n = std::ftell(fp); std::fseek(fp, 0, SEEK_SET);
    if (n <= 0 || n > 256*1024*1024) { std::fclose(fp); return false; }
    std::vector<SceUInt8> buf(n);
    size_t rd = std::fread(buf.data(), 1, n, fp); std::fclose(fp);
    return load_dds(buf.data(), (SceSize)rd, o);
}

} // namespace tex
} // namespace p5d
