// SPDX-License-Identifier: MIT
// PSV 原生移植 - 全局虚拟地址空间注册表实现
#include "address_space.h"
#include <vector>
#include <set>
#include <utility>
#include <cstring>
#include <cstdint>
#include <cstdio>
#include <unordered_map>
#include "../common/log.h"

namespace p5d {
namespace mem {

struct Region { SceUInt32 vaddr=0, size=0; SceUInt8* host=nullptr; const char* name=nullptr;
                // ★★★ ARCH_405 ★★★
                //   `freed=true` 表示该块已被 free，但**仍然保持映射**。
                //   动机（ARCH_404 §4）：真实平台上 `free` 只把块还给堆管理器，
                //   **地址空间仍然映射**，读已释放内存不会产生访问违例。
                //   我方原实现是"free 即 pt_unmap + host=nullptr"，比真实平台更严格
                //   ⇒ 会把 guest 的"读已释放块"升级成硬 BADMEM。
                //   这里改为贴近平台语义，**同时**用 g_uaf_acc 计数把这种访问变成可观测，
                //   避免"不再报错"沦为掩盖。
                bool freed=false; };
namespace {
std::vector<Region> g_regions;

// ---- O(1) 取指/访存快取（4KB 页表 TLB，索引式、增量维护）----
// 页表存 region 在 g_regions 中的下标（而非指针）：vector 增长重分配不会让下标失效，
// 因此新 region 只需增量填自己的页，不必每次注册都全量重建（托管堆/显存只增不减，
// 全量重建成本随 region 数增长，曾导致稳态 MIPS 随时间下滑）。
// 非对齐/极小 region 共享一页 -> PT_CONFLICT，translate 回退线性扫描，语义与原实现一致。
const SceUInt32 PT_BASE    = 0x81000000u;
// ★★★ ARCH_398【窗口上界修正：0xB4000000 → 0xC0000000】★★★
//   实测（[LINHI] 直方图，见下）：窗口 4 的 `linear=` 从 2,261 暴涨到 9,116,875，
//   其高字节分布是 **0xB4xxxxxx=3,380,862 / 0xB5xxxxxx=5,733,376**（合计 9,114,238，
//   占 99.97%），另有 0x00xxxxxx=2,621、0xFFxxxxxx=16 属边角噪声。
//   ⇒ 游戏后期开始使用 0xB4/0xB5 段内存，而旧上界 0xB4000000 把它们全部逼进
//     `translate()` 的 O(g_regions) 全表线性扫描：9.1e6 × 2.35e4 ≈ 2.1e11 次比较，
//     与窗口 4 实测的 130–183 秒同数量级（MIPS 15.6 → 1.53）。
//   修法：把窗口上界抬到 0xC0000000（用户空间上界），PT_N = (0xC0000000-0x81000000)>>12。
//   内存代价：0x3F000 × 4B = 1,032,192 B ≈ 1.0 MB（原 835 KB），可忽略。
const SceUInt32 PT_N       = 0x3F000u;   // *4KB = 0x3F000000，覆盖到 0xC0000000
const SceUInt32 PT_EMPTY   = 0xFFFFFFFFu;
const SceUInt32 PT_CONFLICT= 0xFFFFFFFEu;
SceUInt32* g_pt = nullptr;
SceUInt64 g_pt_builds = 0;
// ★★★ h145k523【性能取证：translate() 是否在走"线性扫描"慢路径】★★★
//   动机：解释器实测 8.06 MIPS ≈ 每指令 500+ cycles（正常解释器 20–50）。若大量访问命中
//   PT_CONFLICT 或落在页表覆盖范围外（vaddr<PT_BASE 或 >=PT_BASE+PT_N*4K），translate()
//   就会退化成**每次 O(g_regions) 的线性扫描**（实测 regions=13188）⇒ 这一条即可解释全部差距。
SceUInt64 g_tr_calls=0, g_tr_linear=0, g_tr_null=0, g_tr_fast=0;
// ★★★ ARCH_398【"越窗地址"取证】★★★
//   实测：窗口 4 的 `linear=` 从 2,261 暴涨到 **9,032,285**，而同期 `null=` 只 +376
//   ⇒ 这 900 万次线性扫描**几乎全部命中**，即它们是**合法地址**，只是落在页表窗口
//     [PT_BASE, PT_BASE+PT_N*4K) = [0x81000000, 0xB4000000) **之外**，
//     于是每次退化成 O(g_regions)=O(23,512) 的全表扫描。
//     9.03e6 × 2.35e4 ≈ 2.1e11 次比较 ⇒ 与窗口 4 的 154–183 秒**同数量级**。
//   这里按 (vaddr>>24) 统计越窗地址的高字节，用来决定窗口该扩到哪里（先量后改）。
SceUInt64 g_tr_lin_hi[256] = {0};
// ★★★ ARCH_405【"释放后仍被访问"计数器】★★★
//   `g_uaf_regions` = 当前处于"已 free 但仍映射"状态的块数
//   `g_uaf_acc`     = 对这类块的访问次数（translate 命中计一次）
//   作用：把"不再 BADMEM"这件事**变成可观测的数字**，而不是静默放过。
SceUInt64 g_uaf_regions=0, g_uaf_acc=0;
// ARCH_405b：已释放页登记（key = 页对齐 vaddr）。translate 失败时用它识别"访问已释放内存"。
std::unordered_map<SceUInt32,SceUInt32> g_freed_sz;
// 共享毒页：读已释放内存时返回它（全零），而不是 nullptr ⇒ 不再产生 BADMEM。
static SceUInt8 g_poison[0x1000];
// ★★★ h145k729【PT_CONFLICT 快路径 —— 本次最大的单项提速】★★★
//   实测病因（不是推断）：`[MIPS]` 窗口 4（t=23.4s → 83.2s）只有 **3.35 MIPS**，
//   而其 `linear=` 从 2261 **暴涨到 2,884,311**，同期 `regions` 从 17925 涨到 28273。
//   该窗口 HLE 只占 32.5%（19.4s / 59.8s）⇒ 剩下的 ~40s 既不是 guest 指令、也不是 HLE，
//   只能是 `translate()` 在 PT_CONFLICT 页上**每次 O(g_regions) 线性扫描**：
//     2.88e6 次 × 平均 ~14e3 次比较 ≈ 4.1e10 次比较 ≈ 数十秒。数量级完全吻合。
//   修法：为冲突页**懒构建**候选 region 下标列表（升序），命中时只扫这个小列表。
//   与线性扫描**语义完全一致**（线性扫描同样按下标升序返回第一个覆盖者；
//   覆盖某地址的 region 必然覆盖该地址所在页 ⇒ 候选集相同）。
//   失效：任何触及该页的 map/unmap 都 erase 掉该页的条目；pt_invalidate 全清。
std::unordered_map<SceUInt32,std::vector<SceUInt32> > g_pt_cf;
// ARCH_403：vaddr -> 触发 release_region 时的 guest PC（用于判定悬垂指针的来源）
std::unordered_map<SceUInt32,SceUInt32> g_rel_pc;
extern "C" unsigned p5d_last_exec_pc(void);   // 实现在 thumb_interpreter.cpp
// ★★★ ARCH_404【分配返回值总账】★★★
//   动机：致命 BADMEM 的 `r0 = 0xB5DC403C` = 已释放区域基址 + 0x3C。
//   而 `allocate_vaddr` 恒为 4KB 对齐 ⇒ 理论上我方 malloc **不会**返回这种内部偏移。
//   要判定"是我们交给 guest 的、还是 guest 自己在块内切的"，必须有**每次分配返回值**的总账。
//   这里由 eboot_probe.cpp 的 `heap_alloc` 每次登记（含触发它的 guest PC）。
std::unordered_map<SceUInt32,SceUInt32> g_alloc_pc;
// h145k729：vaddr → g_regions 下标 直映，消除 register_region 里每次调用的 O(R) 线性查找。
//   下标在 push_back 时即固定；只有 unregister_region 的 erase（罕见路径）会整体错位 ⇒ 那里整体重建。
std::unordered_map<SceUInt32,SceUInt32> g_va_idx;
inline void va_idx_rebuild(){ g_va_idx.clear();
    for(SceUInt32 i=0;i<(SceUInt32)g_regions.size();++i) g_va_idx[g_regions[i].vaddr]=i; }
inline void pt_cf_kill(SceUInt32 pgi){ if(!g_pt_cf.empty()) g_pt_cf.erase(pgi); }
inline const std::vector<SceUInt32>& pt_cf_list(SceUInt32 pgi){
    std::vector<SceUInt32>& lst=g_pt_cf[pgi];
    lst.clear();
    const SceUInt64 a=(SceUInt64)PT_BASE + ((SceUInt64)pgi<<12);
    for(SceUInt32 i=0;i<(SceUInt32)g_regions.size();++i){
        const Region& r=g_regions[i];
        if(!r.host || r.size==0) continue;
        const SceUInt64 s=r.vaddr & ~SceUInt64(0xFFF);
        const SceUInt64 e=(SceUInt64)r.vaddr + r.size - 1;
        if(a>=s && a<=e) lst.push_back(i);
    }
    return lst;
}
// free-list：已释放的托管堆空洞，key=(页对齐size, vaddr)，best-fit 复用，避免 vaddr/内存只增不减
std::set<std::pair<SceUInt64,SceUInt32> > g_free_set;
void pt_unmap_region(SceUInt32 idx){
    if(!g_pt) return;
    Region& r=g_regions[idx];
    SceUInt64 s = r.vaddr & ~SceUInt64(0xFFF);
    SceUInt64 e = (SceUInt64)r.vaddr + r.size - 1;
    for(SceUInt64 a=s;a<=e;a+=0x1000){
        if(a<PT_BASE) continue;
        SceUInt32 pgi=(SceUInt32)(a-PT_BASE)>>12; if(pgi>=PT_N) continue;
        pt_cf_kill(pgi);                      // h145k729：该页候选集可能变化
        SceUInt32& slot=g_pt[pgi];
        if(slot==idx) slot=PT_EMPTY;          // 独占页直接回收；CONFLICT 保持，线性扫描会跳过 host=null 的 tombstone
    }
}
inline bool region_covers(const Region& r, SceUInt32 vaddr){
    return vaddr>=r.vaddr && (SceUInt64)vaddr < (SceUInt64)r.vaddr+r.size;
}
void pt_map_region(SceUInt32 idx){
    if(!g_pt) return;                       // 尚未首次构建，等 pt_build 全量
    const Region& r=g_regions[idx];
    if(!r.host || r.size==0) return;
    SceUInt64 s = r.vaddr & ~SceUInt64(0xFFF);
    SceUInt64 e = (SceUInt64)r.vaddr + r.size - 1;
    for(SceUInt64 a=s; a<=e; a+=0x1000){
        if(a < PT_BASE) continue;
        SceUInt32 pgi=(SceUInt32)(a-PT_BASE)>>12; if(pgi>=PT_N) continue;
        pt_cf_kill(pgi);                      // h145k729：该页候选集可能变化
        SceUInt32& slot=g_pt[pgi];
        if(slot==idx) continue;
        slot = (slot==PT_EMPTY) ? idx : PT_CONFLICT;
    }
}
void pt_build(){
    g_pt_cf.clear();
    g_pt = new SceUInt32[PT_N];
    for(SceUInt32 i=0;i<PT_N;++i) g_pt[i]=PT_EMPTY;
    for(SceUInt32 i=0;i<g_regions.size();++i) pt_map_region(i);
    ++g_pt_builds;
}
void pt_invalidate(){ if(g_pt){ delete[] g_pt; g_pt=nullptr; } g_pt_cf.clear(); }
} // namespace

void register_region(SceUInt32 vaddr, SceUInt32 size, void* host, const char* name) {
    if (!host || size==0) return;
    // h145k729：原为每次注册都 O(R) 线性找同 VA 项（R 实测达 28273）⇒ 改成直映 O(1)。
    { std::unordered_map<SceUInt32,SceUInt32>::iterator _it=g_va_idx.find(vaddr);
      if(_it!=g_va_idx.end() && _it->second<(SceUInt32)g_regions.size()){
        SceUInt32 qi=_it->second; Region& rr=g_regions[qi];
        if(rr.vaddr==vaddr){ rr.size=size; rr.host=(SceUInt8*)host; rr.name=name; rr.freed=false; pt_map_region(qi); return; } } }
    // 诊断：不同 VA 的宿主内存区间不得重叠。malloc 块在稳态可达数万，全扫是 O(R)；
    // 启动期（前 4096 次）全扫，之后每 4096 次抽查一次（host 由 malloc 返回，本就不会重叠）。
    static SceUInt64 s_reg_seq=0; bool chk=(s_reg_seq<4096 || (s_reg_seq&4095)==0); ++s_reg_seq;
    if(chk) for (auto& r : g_regions) {
        std::uintptr_t a0=(std::uintptr_t)host,a1=a0+size,b0=(std::uintptr_t)r.host,b1=b0+r.size;
        if(a0<b1 && b0<a1 && r.vaddr!=vaddr)
            LOGE("[REGION-OLAP] new va=0x%X sz=0x%X host=%p <-> old va=0x%X sz=0x%X host=%p",vaddr,size,host,r.vaddr,r.size,(void*)r.host);
    }
    SceUInt32 idx=(SceUInt32)g_regions.size();
    g_regions.push_back(Region{vaddr,size,(SceUInt8*)host,name});
    g_va_idx[vaddr]=idx;
    pt_map_region(idx);                     // 增量填页（O(新 region 页数)）
}
void unregister_region(SceUInt32 vaddr) {
    for (auto it=g_regions.begin(); it!=g_regions.end(); ++it) if (it->vaddr==vaddr){g_regions.erase(it);va_idx_rebuild();pt_invalidate();return;} // erase 改下标，罕见路径才全量重建
}
// 释放一个托管堆 region：保留槽位作 tombstone（下标稳定，可增量 unmap），区间进 free-list 供复用。
// host 内存由调用方（owner）释放，注册表只负责 vaddr/页表。
void release_region(SceUInt32 vaddr) {
    // ★★★ ARCH_403b【双重释放 / 外来释放检测】★★★
    //   致命 BADMEM 已定性为"悬垂"：guest 调 libc `free`（NID 5B9BB802）后我们回收了 4KB 块
    //   （如 0xB5DC4000），而 guest 仍持有块内 `+0x3C` 的对象（release 的对象指针 = 0xB5DC403C）。
    //   两种最可能的我方成因：
    //     ① 同一 vaddr 被释放两次（第二次若仍照常回收/复用，就会把在用内存交出去）
    //     ② 释放了从未由我方分配的 vaddr（外来释放）—— 说明 import 绑定或指针语义有误
    //   这里把两种情况都显式打出来（原来 `if(!r.host) return;` 是静默的）。
    bool _found=false;
    for (SceUInt32 i=0;i<g_regions.size();++i){ const Region& r0=g_regions[i];
        if(r0.vaddr==vaddr){ _found=true; break; } }
    if(!_found){
        static int _ff=0; if(_ff++<60)
            printf("[MEMREL-FOREIGN] #%d vaddr=0x%X 从未由我方注册（外来释放）pc=0x%X\n",
                   _ff,vaddr,p5d_last_exec_pc());
    }
    for (SceUInt32 i=0;i<g_regions.size();++i){ Region& r=g_regions[i]; if(r.vaddr!=vaddr) continue;
        if(r.freed){                            // 已释放，避免重复（ARCH_405：改用 freed 标志，host 仍保留）
            static int _df=0; if(_df++<60)
                printf("[MEMREL-DOUBLE] #%d vaddr=0x%X size=0x%X ★重复释放 pc=0x%X\n",
                       _df,vaddr,r.size,p5d_last_exec_pc());
            return;
        }
        // ★★★ ARCH_403【释放流水】★★★
        //   动机：致命 BADMEM 形如 `addr = r0+8`，而 `r0` 指向的地址**曾注册但已被回收**
        //   （悬垂指针）。要判定"是不是我们过早释放了"，必须留下释放流水以便对账，
        //   并且要记下**触发释放时的 guest PC**，否则只知道"释放过"，不知道"谁释放的"。
        //   ★ 注意：**map 写入必须在 printf 限流之外** —— 先前的写法把它放进了
        //     `if(_rl++<4000)` 里面，于是第 4000 次之后的释放全都没被记录，
        //     造成 `p5d_mem_rel_pc()` 对于晚出现的目标恒返回 0（本文件的仪表 bug）。
        { SceUInt32 _pc=p5d_last_exec_pc();
          g_rel_pc[vaddr]=_pc;
          static int _rl=0;
          if(_rl++<4000) printf("[MEMREL] #%d vaddr=0x%X size=0x%X pc=0x%X\n",_rl,vaddr,r.size,_pc); }
        SceUInt32 psz=(r.size+0xFFFu)&~0xFFFu;
        // ★★★ ARCH_405b：方案调整（ARCH_405 的第一版被实测证伪）★★★
        //   v1 做法是"free 后仍保持 pt 映射 + host 保留"，实测**MIPS 从 12–19 崩到 1.5**：
        //   因为跳过 `pt_unmap_region` 后地址空间无法回收，`allocate_vaddr` 只能一路往前找
        //   EMPTY 页，且页表冲突暴增 ⇒ `linear` 从 1,220 涨到 **619 万**（O(R) 线性扫描）。
        //   ⇒ 该实现不可用，回退到"解除映射"。
        //   v2 做法：**仍然解除映射**（地址空间照旧可回收），但把该 region 的每一页登记进
        //   `g_freed_sz`；`translate()` 在失败路径上认出"这是访问已释放内存"时
        //   返回**共享毒页**（全零）而不是 nullptr ⇒ 不再产生 BADMEM，
        //   同时 `g_uaf_acc` 照常计数（可观测，不掩盖）。
        pt_unmap_region(i);
        r.host=nullptr; r.freed=true; r.name="free";
        for(SceUInt32 _p=0;_p<psz;_p+=0x1000u){
            if(g_freed_sz.size()>2000000u) g_freed_sz.clear();
            g_freed_sz[vaddr+_p]=1u;
        }
        g_free_set.insert(std::make_pair((SceUInt64)psz,vaddr));
        return;
    }
}
SceUInt8* translate(SceUInt32 vaddr) {
    ++g_tr_calls;
    if(!g_pt) pt_build();
    // ARCH_405b：失败路径上的"访问已释放内存"识别（只在失败时做，且失败本身极罕见：
    //   g_tr_null 在 12 亿条指令里只有 ~2600 次）
    struct UafProbe {
        static SceUInt8* hit(SceUInt32 va){
            if(va<PT_BASE) return nullptr;
            std::unordered_map<SceUInt32,SceUInt32>::iterator it=g_freed_sz.find(va & ~0xFFFu);
            if(it==g_freed_sz.end()) return nullptr;
            ++g_uaf_acc;
            return g_poison + (va & 0xFFFu);
        }
    };
    SceUInt32 rel = vaddr - PT_BASE;        // vaddr<PT_BASE 时无符号下溢成巨大值，一次比较即回退
    if(rel < (PT_N<<12)){
        SceUInt32 slot = g_pt[rel>>12];
        if(slot<g_regions.size()){
            const Region& r=g_regions[slot];
            if(r.host && region_covers(r,vaddr)){ ++g_tr_fast; return r.host+(vaddr-r.vaddr); }
            ++g_tr_null; { SceUInt8* _q=UafProbe::hit(vaddr); if(_q) return _q; } return nullptr;
        }
        if(slot==PT_EMPTY){ SceUInt8* _q=UafProbe::hit(vaddr); if(_q) return _q; return nullptr; }
        /* PT_CONFLICT：h145k729 走该页候选列表（只扫少数项，语义与线性扫描完全一致）。
           `g_tr_linear` 的语义保持为"进入了冲突页路径"，以便与历史 [MIPS] 数值直接对比。 */
        if(g_pt_cf.size()>65536u) g_pt_cf.clear();
        ++g_tr_linear;
        ++g_tr_lin_hi[(vaddr>>24)&0xFFu];
        { const std::vector<SceUInt32>& cand=pt_cf_list(rel>>12);
          for(size_t k=0;k<cand.size();++k){ const Region& r=g_regions[cand[k]];
              if(r.host && region_covers(r,vaddr)){ ++g_tr_fast; return r.host+(vaddr-r.vaddr); } } }
        ++g_tr_null;
        { SceUInt8* _q=UafProbe::hit(vaddr); if(_q) return _q; }
        return nullptr;
    }
    ++g_tr_linear;
    ++g_tr_lin_hi[(vaddr>>24)&0xFFu];
    for (auto& r : g_regions)
        if (r.host && region_covers(r,vaddr)){ ++g_tr_fast; return r.host+(vaddr-r.vaddr); }
    ++g_tr_null;
    { SceUInt8* _q=UafProbe::hit(vaddr); if(_q) return _q; }
    return nullptr;
}
// h145k523 取证读口（供解释器 [MIPS] 行打印）
extern "C" void p5d_tr_stats(unsigned long long* c,unsigned long long* l,unsigned long long* n,unsigned long long* f){
    *c=g_tr_calls; *l=g_tr_linear; *n=g_tr_null; *f=g_tr_fast;
}
// ARCH_398：越窗地址高字节直方图（供 [MIPS] 行打印）
extern "C" unsigned long long p5d_tr_linhi(int i){ return (i>=0&&i<256)?g_tr_lin_hi[i]:0ull; }
// ARCH_405：UAF 计数导出（"已 free 但仍被访问"的块数与访问次数）
extern "C" void p5d_mem_uaf_stats(unsigned long long* regs,unsigned long long* acc){
    *regs=g_freed_sz.size(); *acc=g_uaf_acc; }
// ★★★ ARCH_403【故障地址成因判定】★★★
//   `release_region` 会保留槽位作 tombstone（`vaddr`/`size` 仍在，只把 `host` 置空、
//   `name` 置 "free"）⇒ **"从未注册" 与 "曾注册后被回收" 是可以区分的**。
//   返回：0 = 从未注册（真实的洞）
//         1 = 曾注册但已被 release_region 回收（★ 悬垂指针 ⇒ 我方生命周期缺陷）
//         2 = 无（留给将来扩展）
//         3 = 当前仍正常映射（不应出现在 mr_fail 里）
extern "C" int p5d_mem_classify(unsigned a,unsigned* o_va,unsigned* o_sz,unsigned* o_has_host){
    *o_va=0; *o_sz=0; *o_has_host=0;
    for(size_t i=0;i<g_regions.size();++i){
        const Region& r=g_regions[i];
        if(r.size==0) continue;
        unsigned long long s=(unsigned long long)r.vaddr;
        unsigned long long e=s+(unsigned long long)r.size;
        if((unsigned long long)a>=s && (unsigned long long)a<e){
            *o_va=r.vaddr; *o_sz=r.size; *o_has_host=r.host?1u:0u;
            // ARCH_405：freed 判定改用标志位（host 现被保留）
            if(r.freed) return 1;
            return r.host?3:0;
        }
    }
    return 0;
}
// ARCH_403：取"该地址被释放时触发它的 guest PC"（0 表示没有记录）
extern "C" unsigned p5d_mem_rel_pc(unsigned a){
    std::unordered_map<SceUInt32,SceUInt32>::iterator it=g_rel_pc.find(a);
    return it==g_rel_pc.end()?0u:it->second;
}
// ARCH_404：登记一次分配返回值（由 heap_alloc 调用）
extern "C" void p5d_mem_note_alloc(unsigned va,unsigned pc){ if(va) g_alloc_pc[va]=pc; }
// ARCH_404：该地址是否曾作为分配返回值被交给 guest；有则返回当时的 guest PC
extern "C" unsigned p5d_mem_alloc_pc(unsigned a){
    std::unordered_map<SceUInt32,SceUInt32>::iterator it=g_alloc_pc.find(a);
    return it==g_alloc_pc.end()?0u:it->second;
}
// ARCH_404：分配总账里是否有"落在 [a, a+len) 内"的返回值（用于判断内部偏移的来源）
extern "C" unsigned p5d_mem_alloc_cover(unsigned a,unsigned len,unsigned* o_va,unsigned* o_pc){
    *o_va=0; *o_pc=0;
    for(std::unordered_map<SceUInt32,SceUInt32>::iterator it=g_alloc_pc.begin();it!=g_alloc_pc.end();++it){
        if(it->first<=a && a<(unsigned long long)it->first+len){ *o_va=it->first; *o_pc=it->second; return 1u; }
    }
    return 0u;
}
bool contains(SceUInt32 vaddr, SceUInt32 bytes) {
    if(bytes==0) bytes=1;
    if(!g_pt) pt_build();
    SceUInt32 rel = vaddr - PT_BASE;
    if(rel < (PT_N<<12)){
        SceUInt32 slot = g_pt[rel>>12];
        if(slot<g_regions.size()){
            const Region& r=g_regions[slot];
            if(r.host && region_covers(r,vaddr) && region_covers(r,vaddr+bytes-1)) return true;
            return false;
        }
        if(slot==PT_EMPTY) return false;
        /* PT_CONFLICT：h145k729 同样走该页候选列表 */
        if(g_pt_cf.size()>65536u) g_pt_cf.clear();
        { const std::vector<SceUInt32>& cand=pt_cf_list(rel>>12);
          for(size_t k=0;k<cand.size();++k){ const Region& r=g_regions[cand[k]];
              if(r.host && vaddr>=r.vaddr && (SceUInt64)vaddr+bytes <= (SceUInt64)r.vaddr+r.size) return true; } }
        return false;
    }
    for (auto& r : g_regions)
        if (r.host && vaddr>=r.vaddr && (SceUInt64)vaddr+bytes <= (SceUInt64)r.vaddr+r.size) return true;
    return false;
}
SceUInt32 region_count(){ return (SceUInt32)g_regions.size(); }
SceUInt32 region_bounds(SceUInt32 idx, SceUInt32* vaddr, SceUInt32* size){
    if(idx>=g_regions.size()) return 0;
    const Region& r=g_regions[idx];
    if(vaddr)*vaddr=r.vaddr;
    if(size)*size=r.size;
    return r.host?1u:0u;
}
SceUInt64 pt_rebuild_count(){ return g_pt_builds; }
void dump_regions_around(SceUInt32 addr, SceUInt32 window){
    std::printf("[REGDUMP] fault=0x%08X region_count=%zu pt_builds=%llu\n",addr,g_regions.size(),(unsigned long long)g_pt_builds);
    bool any=false;
    for(auto&r:g_regions){
        SceUInt64 s=r.vaddr,e=(SceUInt64)r.vaddr+r.size;
        bool cover=(addr>=r.vaddr && (SceUInt64)addr<e);
        bool near=((SceUInt64)addr+window>=s && (SceUInt64)addr<=e+window);
        if(cover||near){
            std::printf("  %s va=0x%08X end=0x%08llX size=0x%X host=%p name=%s\n",
                cover?"[COVERS]":"        ",r.vaddr,(unsigned long long)e,r.size,(void*)r.host,r.name?r.name:"?");
            any=true;
        }
    }
    if(!any) std::printf("  no region within +/-0x%X\n",window);
    std::fflush(stdout);
}
SceUInt32 find_u32(SceUInt32 val, SceUInt32* out, SceUInt32 maxout){
    SceUInt32 n=0;
    for(auto&r:g_regions){ if(!r.host) continue;
        SceUInt32 cnt=r.size/4;
        for(SceUInt32 k=0;k+4<=r.size && n<maxout;k+=4){
            SceUInt32 v=(SceUInt32)r.host[k]|((SceUInt32)r.host[k+1]<<8)|((SceUInt32)r.host[k+2]<<16)|((SceUInt32)r.host[k+3]<<24);
            if(v==val){ out[n++]=r.vaddr+k; if(n>=maxout)break; }
        }
        if(n>=maxout)break;
    }
    return n;
}
void clear(){ g_regions.clear(); g_free_set.clear(); pt_invalidate(); }

namespace { SceUInt32 g_heap_cursor = 0xA0000000u; }
void reset_heap_cursor(){ g_heap_cursor=0xA0000000u; }
SceUInt32 allocate_vaddr(SceUInt32 size){ return allocate_vaddr_aligned(size,0x1000u); }
// h145k76【★ 对齐分配】PSV 的 CDRAM 内存块必须 1MB 对齐（sceKernelAllocMemBlock(CDRAM) 的真机语义）；
//   CRI VavcPsp2 在 0x812A92B8 用 `ubfx r1,r0,#0,#0x14; cbz` 硬性检查解码器工作内存的 1MB 对齐，
//   不满足就静默失败 ⇒ 影片解码器建不出来 ⇒ 标题画面全黑。原 allocate_vaddr 只保证 4KB 对齐。
SceUInt32 allocate_vaddr_aligned(SceUInt32 size, SceUInt32 align){
    if(align<0x1000u) align=0x1000u;
    const SceUInt32 amask=align-1u;
    SceUInt32 sz=(size+0xFFFu)&~0xFFFu; if(sz==0)sz=0x1000;
    if(!g_pt) pt_build();
    // 仅在 align==4KB 时才复用 free-list（空洞未必满足更大对齐）
    if(align==0x1000u){
        auto it=g_free_set.lower_bound(std::make_pair((SceUInt64)sz,(SceUInt32)0));
        if(it!=g_free_set.end()){
            SceUInt32 va=it->second; SceUInt64 fsz=it->first; g_free_set.erase(it);
            if(fsz >= (SceUInt64)sz+0x1000) g_free_set.insert(std::make_pair(fsz-sz,va+sz));
            return va;
        }
    }
    SceUInt32 pages=sz>>12;
    SceUInt32 cand=(g_heap_cursor+amask)&~amask;
    const SceUInt32 lim=PT_BASE+(PT_N<<12);
    // 快路径：沿 cursor 在页表里找连续 EMPTY 页，O(页数)；撞到占用块跳到其末尾并对齐。
    while((SceUInt64)cand+sz <= lim){
        bool ok=true; SceUInt32 jump=0;
        for(SceUInt32 k=0;k<pages;++k){
            SceUInt32 va=cand+(k<<12); SceUInt32 slot=g_pt[(va-PT_BASE)>>12];
            if(slot!=PT_EMPTY){
                ok=false;
                if(slot<g_regions.size()){ const Region&r=g_regions[slot]; jump=(SceUInt32)(((SceUInt64)r.vaddr+r.size+0xFFFu)&~SceUInt64(0xFFF)); }
                else jump=va+0x1000;     // PT_CONFLICT：跳过这一页
                break;
            }
        }
        if(ok){ g_heap_cursor=cand+sz; return cand; }
        if(jump<=cand) jump=cand+0x1000;
        cand=(jump+amask)&~amask;
    }
    // 回退：超范围或异常，原 O(R) 线性避让
    cand=(g_heap_cursor+amask)&~amask;
    for(int t=0;t<200000;++t){
        bool overlap=false; SceUInt32 jump=0;
        for(auto&r:g_regions){
            if((SceUInt64)cand < (SceUInt64)r.vaddr+r.size && (SceUInt64)r.vaddr < (SceUInt64)cand+sz){
                overlap=true; jump=(r.vaddr+r.size+0xFFFu)&~0xFFFu; break;
            }
        }
        if(!overlap){ g_heap_cursor=(SceUInt32)((SceUInt64)cand+sz); return cand; }
        if(jump<=cand) jump=cand+0x1000;
        cand=(jump+amask)&~amask;
    }
    return 0;
}

} // namespace mem
} // namespace p5d
