// PSV 原生移植 - 全局虚拟地址空间注册表
// 把 PSV 32位虚拟地址区间映射到宿主 malloc 的 buffer。
// 用途：sceKernel 分配的内存块 / ELF 段 / sceGxmMapMemory 的显存块统一在此登记，
//       运行时 SceGxmTexture 控制块里的 data_byte_addr 经 translate() 得到宿主像素指针。
#pragma once
#include "../common/types.h"

namespace p5d {
namespace mem {

// 登记一段 [vaddr, vaddr+size) -> host（host 由调用方持有，注册表不负责释放）
void register_region(SceUInt32 vaddr, SceUInt32 size, void* host, const char* name = 0);
// 解除登记（不释放 host）
void unregister_region(SceUInt32 vaddr);
// 释放托管堆 region：tombstone + 增量 unmap + 进 free-list 复用（host 由 owner 自行释放）
void release_region(SceUInt32 vaddr);
// 虚拟地址 -> 宿主指针；未命中返回 0
SceUInt8* translate(SceUInt32 vaddr);
// [vaddr, vaddr+bytes) 是否完全落在某个已登记区间内
bool contains(SceUInt32 vaddr, SceUInt32 bytes);
// 已登记区间数（诊断/测试）
SceUInt32 region_count();
// 诊断枚举：idx 0..region_count-1，回填 vaddr/size，返回 1=有 backing / 0=tombstone
SceUInt32 region_bounds(SceUInt32 idx, SceUInt32* vaddr, SceUInt32* size);
// 页表全量重建次数（诊断：稳态应只在启动期少量，频繁增长说明有 unregister/clear）
SceUInt64 pt_rebuild_count();
// 诊断：打印覆盖 addr 或距其 ±window 内的全部已登记区间（BADMEM 时定位 backing 去向）
void dump_regions_around(SceUInt32 addr, SceUInt32 window);
// 在所有已登记区间里逐 4 字节查找等于 val 的虚拟地址，最多收集 maxout 个，返回个数（诊断用）
SceUInt32 find_u32(SceUInt32 val, SceUInt32* out, SceUInt32 maxout);
// 托管堆：从固定基址向上分配一段不与任何已登记区间重叠的 4KB 对齐 vaddr（模拟 sceKernelAllocMemBlock 拿到的 PSV 地址）
SceUInt32 allocate_vaddr(SceUInt32 size);
// h145k76：按 align 对齐的托管堆分配（PSV 的 CDRAM 内存块需 1MB 对齐，sceKernelAllocMemBlock 真机语义）
SceUInt32 allocate_vaddr_aligned(SceUInt32 size, SceUInt32 align);
// 重置托管堆游标（测试复位用）
void reset_heap_cursor();
// 清空（测试间复位）
void clear();

} // namespace mem
} // namespace p5d
