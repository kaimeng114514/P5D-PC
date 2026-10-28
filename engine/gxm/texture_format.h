// PSV原生移植 - 纹理格式与SceGxmTexture/GXP sampler/DDS 解析
// 权威依据：Vita3K vita3k/gxm/include/gxm/types.h (ref 84184a3)
#pragma once
#include "../common/types.h"
#include <vector>
#include <cstddef>

namespace p5d {
namespace tex {

// VkFormat 的承载类型（避免头文件强依赖 vulkan 头；后端使用处 static_cast<VkFormat>）
using VkFmt = SceUInt32;
// 不支持/未知格式的返回值（VkFormat VK_FORMAT_UNDEFINED = 0）
constexpr VkFmt FMT_UNDEFINED = 0;

// ============================================================================
// GXP sampler 参数：来自 SceGxmProgramParameter 表中 category==SAMPLER(2) 的项
// 这是区分 cube/2D、以及多纹理单元绑定的权威来源（SceGxmTexture 对象本身无 cube 位）
// ============================================================================
struct GxpSamplerParam {
    SceUInt32 resource_index = 0;   // texture unit 0..15（sceGxmTextureBind 单元号）
    SceUInt32 semantic_index = 0;
    SceBool   is_cube = 0;          // semantic_index bit4（Vita3K is_sampler_cube）
    SceUInt32 sa_num = 0xFFFFFFFFu;  // USSE double-reg 后的 SA 寄存器号 = container19.base_sa_offset + 2*sampler序号；SMP 用 (src1_n<<1) 精确匹配
    char      name[32] = {0};
};

// 解析 GXP 程序的 sampler 参数表。gxp 指向 GXP magic('GXP\0')起始。
std::vector<GxpSamplerParam> parse_gxp_samplers(const SceUInt8* gxp, SceSize gxp_size);

// ============================================================================
// SceGxmTexture 运行时纹理控制块（16 字节，4 个 control word 位域）
// ============================================================================
// PSV 纹理通道 swizzle -> Vulkan VkComponentSwizzle 数值（IDENTITY=0/ZERO=1/ONE=2/R=3/G=4/B=5/A=6）
// 来源：Vita3K gxm_to_vulkan.cpp translate_swizzle1/2/3/4（BC1/BC2/BC3 压缩固定走 4 通道表）
struct TexComponentMap { SceUInt32 r, g, b, a; };
TexComponentMap texture_swizzle4(SceUInt32 swizzle_index);
TexComponentMap texture_swizzle(SceUInt32 swizzle_index, SceUInt32 component_count);

struct SceGxmTextureInfo {
    SceUInt32 width = 0, height = 0;          // 已按 texture_type 还原的真实像素尺寸
    SceUInt32 mip_count = 1;
    SceUInt32 base_format = 0;      // SCE_GXM_TEXTURE_BASE_FORMAT_*（高8位）
    SceUInt32 swizzle = 0;          // w3 bits30:28，0..7
    SceUInt32 texture_type = 0;     // w1 bits31:29：0 SWIZZLED/2 CUBE/3 LINEAR/4 TILED/6 LINEAR_STRIDED...
    SceUInt32 uaddr_mode = 0, vaddr_mode = 0;
    SceUInt32 mag_filter = 0, min_filter = 0, mip_filter = 0;
    SceUInt32 gamma_mode = 0;
    SceUInt32 format0 = 0;          // w0 bit31
    SceUInt32 normalize_mode = 0;   // w3 bit31
    SceUInt32 lod_bias = 0;         // w0 bits22:17
    SceUInt32 data_addr_word = 0;   // w2 bits31:2（字地址）
    SceUInt32 data_byte_addr = 0;   // = data_addr_word<<2（PSV 虚拟字节地址）
    SceUInt32 palette_word = 0;     // w3 bits25:0
    SceUInt32 palette_byte_addr = 0;
    bool is_cube = false;
};
bool parse_sce_gxm_texture(const SceUInt8* p, SceGxmTextureInfo& out);

// base_format -> 组件数（对照 Vita3K get_num_components；BC1/BC2/BC3=4，BC4=1，BC5=2）
SceUInt32 component_count_for_base_format(SceUInt32 base_format);

// 运行时一站式解析：16 字节 SceGxmTexture 控制块 -> VkFormat + 组件数 + swizzle 通道映射
struct ResolvedGxmTexture {
    SceGxmTextureInfo info;
    VkFmt vk_format = 0;                 // FMT_UNDEFINED 表示需转码(PVRTC/调色板)
    SceUInt32 component_count = 4;
    TexComponentMap swizzle_map{0,0,0,0};
};
bool resolve_gxm_texture(const SceUInt8* control16, ResolvedGxmTexture& out);

// SceGxmTextureBaseFormat（如 U8U8U8U8=0x0C000000 / UBC1=0x85000000 / UBC3=0x87000000）
// -> VkFormat；返回 FMT_UNDEFINED 表示 PSV 专有、需转码（如 PVRTC）。
VkFmt sce_base_format_to_vk(SceUInt32 base_format);

// 块压缩格式信息（BC1..BC5）；非块格式 block=1x1, block_bytes=每像素字节
struct BlockInfo { SceUInt32 block_w, block_h, block_bytes; bool compressed; };
BlockInfo block_info_for_vk(VkFmt vkfmt);
// 整张纹理(layers 层/cube=6)在宿主侧的紧凑字节数（按 block 尺寸对齐，供 vaddr 取像素长度）
SceUInt32 texture_byte_size(VkFmt vkfmt, SceUInt32 w, SceUInt32 h, SceUInt32 layers);
// 把 PSV 显存布局反排为 Vulkan 线性布局。
// type_index = SceGxmTexture.texture_type(w1>>29)：0/2 SWIZZLED(含CUBE单面)、3/6 LINEAR(原样copy)、4 TILED(32x32)。
// 压缩格式(BC1..BC5)按 4x4 块做 Morton z-order 反排；非压缩按像素。返回线性字节缓冲。
std::vector<SceUInt8> linearize_psv_texture(const SceUInt8* src, SceUInt32 w, SceUInt32 h, VkFmt vkfmt, SceUInt32 type_index);
// 测试辅助：线性 -> SWIZZLED(Morton) 正向打乱（linearize 的逆），用于构造"PSV显存布局"样本做往返验证
std::vector<SceUInt8> swizzle_linear_texture(const SceUInt8* src, SceUInt32 w, SceUInt32 h, VkFmt vkfmt);



// ============================================================================
// DDS（游戏 CPK 内真实纹理落地格式，DXT1=BC1 / DXT5=BC3，可能 cube）
// ============================================================================
struct DdsImage {
    SceUInt32 width = 0, height = 0;
    SceUInt32 mip_count = 1;
    SceUInt32 array_layers = 1;    // cube 时为 6
    SceBool   is_cube = 0;
    VkFmt     vk_format = FMT_UNDEFINED;
    BlockInfo block{};
    std::vector<SceUInt8> pixels;  // 顶层（mip0）数据；cube 时 6 面顺序连续
};
bool load_dds(const SceUInt8* data, SceSize size, DdsImage& out);
bool load_dds_file(const char* path, DdsImage& out);

} // namespace tex
} // namespace p5d
