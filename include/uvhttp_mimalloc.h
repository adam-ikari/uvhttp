#ifndef UVHTTP_MIMALLOC_H
#define UVHTTP_MIMALLOC_H

#include "uvhttp_allocator.h"

#ifdef __cplusplus
extern "C" {
#endif

// mimalloc支持函数
// 只有在定义了UVHTTP_ENABLE_MIMALLOC时才有实际实现

// 初始化mimalloc分配器
void uvhttp_mimalloc_init(void);

// 获取mimalloc统计信息
void uvhttp_mimalloc_stats_get(uvhttp_memory_stats_t* stats);

// 打印详细的mimalloc统计
void uvhttp_mimalloc_stats_print(void);

// 检查mimalloc是否可用
#ifdef UVHTTP_ENABLE_MIMALLOC
#define UVHTTP_HAS_MIMALLOC 1
#else
#define UVHTTP_HAS_MIMALLOC 0
#endif

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIMALLOC_H */