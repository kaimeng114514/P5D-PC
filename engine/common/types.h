// SPDX-License-Identifier: MIT
// PSV原生移植 - 公共类型定义
// 对应PSV的系统类型，确保二进制兼容
#pragma once
#include <cstdint>
#include <cstddef>

// PSV基本类型
typedef int8_t   SceInt8;
typedef uint8_t  SceUInt8;
typedef int16_t  SceInt16;
typedef uint16_t SceUInt16;
typedef int32_t  SceInt32;
typedef uint32_t SceUInt32;
typedef int64_t  SceInt64;
typedef uint64_t SceUInt64;
typedef int32_t  SceBool;
typedef float    SceFloat;
typedef double   SceDouble;
typedef void*    ScePVoid;
typedef const void* ScePCVoid;

// PSV句柄和UID
typedef int32_t  SceUID;
typedef int32_t  SceModule;
typedef int32_t  SceSize;
typedef int32_t  SceSSize;

// 错误码
typedef int32_t  SceErrorCode;

// PSV时间类型
typedef int64_t  SceKernelSysClock;
typedef int64_t  SceKernelMicroTime;
typedef int64_t  SceKernelWideTime;

// 函数指针类型
typedef int (*SceKernelThreadEntry)(SceSize args, void* argp);

// 通用回调
typedef void (*SceKernelCallbackFunction)(int notifyId, int notifyCount, void* arg);

// 内存属性标志
enum SceKernelMemoryAttr {
    SCE_KERNEL_MEMORY_ATTR_FINE          = 0x00000000,
    SCE_KERNEL_MEMORY_ATTR_CACHEABLE     = 0x00000001,
    SCE_KERNEL_MEMORY_ATTR_NONCACHEABLE  = 0x00000002,
    SCE_KERNEL_MEMORY_ATTR_WRITEBACK     = 0x00000004,
    SCE_KERNEL_MEMORY_ATTR_WRITEALLOCATE = 0x00000008,
    SCE_KERNEL_MEMORY_ATTR_PHYSICAL      = 0x00000010,
    SCE_KERNEL_MEMORY_ATTR_NOALLOC       = 0x00000020,
};

// 线程优先级范围
#define SCE_KERNEL_HIGHEST_PRIORITY   0
#define SCE_KERNEL_LOWEST_PRIORITY    255
#define SCE_KERNEL_DEFAULT_PRIORITY   100

// 模块信息结构体（PSV原生格式）
typedef struct SceModuleInfo {
    SceUInt16    attributes;
    SceUInt16    version;
    char         name[27];
    char         type;
    SceUInt32    gp_value;
    void*        ent_top;
    void*        ent_end;
    void*        stub_top;
    void*        stub_end;
    SceUInt32    dbg_info_top;
    SceUInt32    dbg_info_end;
} SceModuleInfo;

// 大小断言，确保与PSV原生结构一致（32位PSV上为64字节）
// 注意：64位编译时指针为8字节，大小会不同，这是预期的
#if defined(__arm__) || defined(__i386__) || defined(_M_IX86)
static_assert(sizeof(SceModuleInfo) == 64, "SceModuleInfo size mismatch");
#endif
