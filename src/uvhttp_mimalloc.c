#include "uvhttp_allocator.h"

// 可选的mimalloc支持
// 只有在定义了UVHTTP_ENABLE_MIMALLOC时才编译
#ifdef UVHTTP_ENABLE_MIMALLOC

#include "mimalloc.h"

// mimalloc适配器实现
static void* mi_malloc_wrapper(size_t size) {
    return mi_malloc(size);
}

static void* mi_realloc_wrapper(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
}

static void mi_free_wrapper(void* ptr) {
    mi_free(ptr);
}

static void* mi_calloc_wrapper(size_t nmemb, size_t size) {
    return mi_calloc(nmemb, size);
}

// mimalloc分配器实例
static uvhttp_allocator_t mi_allocator = {
    .malloc = mi_malloc_wrapper,
    .realloc = mi_realloc_wrapper,
    .free = mi_free_wrapper,
    .calloc = mi_calloc_wrapper,
    .data = NULL,
    .type = UVHTTP_ALLOCATOR_CUSTOM
};

// 初始化mimalloc分配器
void uvhttp_mimalloc_init(void) {
    mi_stats_reset();  // 重置统计
    uvhttp_allocator_set_custom(&mi_allocator);
}

// 获取mimalloc统计信息
void uvhttp_mimalloc_stats_get(uvhttp_memory_stats_t* stats) {
    if (!stats) return;
    
    mi_stats_t mi_stats;
    mi_stats_merge(&mi_stats);
    
    // 转换mimalloc统计到我们的格式
    stats->total_allocated = mi_stats.allocated;
    stats->current_usage = mi_stats.allocated - mi_stats.freed;
    stats->peak_usage = mi_stats.peak;
    stats->allocation_count = mi_stats.malloc_count;
    stats->free_count = mi_stats.free_count;
}

// 打印详细的mimalloc统计
void uvhttp_mimalloc_stats_print(void) {
    mi_stats_print();
}

#else

// 当mimalloc未启用时的空实现
void uvhttp_mimalloc_init(void) {
    // 空实现
}

void uvhttp_mimalloc_stats_get(uvhttp_memory_stats_t* stats) {
    if (stats) {
        memset(stats, 0, sizeof(uvhttp_memory_stats_t));
    }
}

void uvhttp_mimalloc_stats_print(void) {
    // 空实现
}

#endif /* UVHTTP_ENABLE_MIMALLOC */