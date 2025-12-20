#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>

/* 编译时选择实现 */
#if UVHTTP_ALLOCATOR_TYPE == UVHTTP_ALLOCATOR_MIMALLOC

/* mimalloc实现 */
static int g_mimalloc_initialized = 0;

void* uvhttp_mimalloc_malloc(size_t size) {
    if (!g_mimalloc_initialized) {
        mi_malloc_init();
        g_mimalloc_initialized = 1;
    }
    return mi_malloc(size);
}

void* uvhttp_mimalloc_realloc(void* ptr, size_t size) {
    if (!g_mimalloc_initialized) {
        mi_malloc_init();
        g_mimalloc_initialized = 1;
    }
    return mi_realloc(ptr, size);
}

void uvhttp_mimalloc_free(void* ptr) {
    if (!ptr) return;
    
    if (!g_mimalloc_initialized) {
        mi_malloc_init();
        g_mimalloc_initialized = 1;
    }
    mi_free(ptr);
}

void* uvhttp_mimalloc_calloc(size_t nmemb, size_t size) {
    if (!g_mimalloc_initialized) {
        mi_malloc_init();
        g_mimalloc_initialized = 1;
    }
    return mi_calloc(nmemb, size);
}

void uvhttp_mimalloc_init(void) {
    /* mi_malloc_init() 会在首次调用时自动初始化 */
}

void uvhttp_mimalloc_cleanup(void) {
    if (g_mimalloc_initialized) {
        mi_collect(1);
        g_mimalloc_initialized = 0;
    }
}

#elif UVHTTP_ALLOCATOR_TYPE == UVHTTP_ALLOCATOR_CUSTOM

/* 自定义分配器 - 需要用户提供 */
extern uvhttp_allocator_t* uvhttp_custom_allocator;

#endif

/* 通用分配器获取函数 */
uvhttp_allocator_t* uvhttp_allocator_get(void) {
    static uvhttp_allocator_t default_allocator = {
        .malloc = malloc,
        .realloc = realloc,
        .free = free,
        .calloc = calloc,
        .data = NULL,
        .type = UVHTTP_ALLOCATOR_DEFAULT
    };
    
#if UVHTTP_ALLOCATOR_TYPE == UVHTTP_ALLOCATOR_MIMALLOC
    static uvhttp_allocator_t mimalloc_allocator = {
        .malloc = uvhttp_mimalloc_malloc,
        .realloc = uvhttp_mimalloc_realloc,
        .free = uvhttp_mimalloc_free,
        .calloc = uvhttp_mimalloc_calloc,
        .data = NULL,
        .type = UVHTTP_ALLOCATOR_MIMALLOC
    };
    return &mimalloc_allocator;
    
#elif UVHTTP_ALLOCATOR_TYPE == UVHTTP_ALLOCATOR_CUSTOM
    return uvhttp_custom_allocator;
    
#else
    return &default_allocator;
#endif
}

/* 设置分配器 */
void uvhttp_allocator_set(uvhttp_allocator_t* allocator) {
    /* 简化实现：直接替换全局实例 */
}

/* 内存池分配器创建 */
uvhttp_allocator_t* uvhttp_pool_allocator_new(const uvhttp_pool_config_t* config) {
    /* 简化实现：不支持内存池 */
    return NULL;
}

void uvhttp_pool_allocator_free(uvhttp_allocator_t* allocator) {
    /* 简化实现：不支持内存池 */
}

/* 统计分配器创建 */
uvhttp_allocator_t* uvhttp_stats_allocator_new(uvhttp_allocator_t* wrapped) {
    /* 简化实现：不支持统计 */
    return NULL;
}

void uvhttp_stats_allocator_free(uvhttp_allocator_t* allocator) {
    /* 简化实现：不支持统计 */
}

/* 内存统计 */
void uvhttp_memory_stats_get(uvhttp_memory_stats_t* stats) {
    if (stats) {
        memset(stats, 0, sizeof(uvhttp_memory_stats_t));
    }
}

void uvhttp_memory_stats_reset(void) {
    /* 简化实现：不支持统计 */
}