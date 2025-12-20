/* 内存分配器模块 */

#ifndef UVHTTP_ALLOCATOR_H
#define UVHTTP_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 编译时分配器选择 */
#ifndef UVHTTP_ALLOCATOR_TYPE
#define UVHTTP_ALLOCATOR_TYPE UVHTTP_ALLOCATOR_DEFAULT
#endif

/* 分配器类型 */
typedef enum {
    UVHTTP_ALLOCATOR_DEFAULT,    /* 系统默认 */
    UVHTTP_ALLOCATOR_MIMALLOC,     /* mimalloc */
    UVHTTP_ALLOCATOR_CUSTOM      /* 自定义 */
} uvhttp_allocator_type_t;

/* 分配器接口 */
typedef struct uvhttp_allocator {
    void* (*malloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
    void* (*calloc)(size_t nmemb, size_t size);
    void* data;
    uvhttp_allocator_type_t type;
} uvhttp_allocator_t;

/* mimalloc支持检测 */
#ifndef UVHTTP_ENABLE_MIMALLOC
#define UVHTTP_ENABLE_MIMALLOC 0
#endif

/* 编译时选择实现 */
#if UVHTTP_ALLOCATOR_TYPE == UVHTTP_ALLOCATOR_MIMALLOC
    #if UVHTTP_ENABLE_MIMALLOC
        /* mimalloc函数声明 */
        void* uvhttp_mimalloc_malloc(size_t size);
        void* uvhttp_mimalloc_realloc(void* ptr, size_t size);
        void  uvhttp_mimalloc_free(void* ptr);
        void* uvhttp_mimalloc_calloc(size_t nmemb, size_t size);
        void  uvhttp_mimalloc_init(void);
        
        #define UVHTTP_MALLOC(size) uvhttp_mimalloc_malloc(size)
        #define UVHTTP_REALLOC(ptr, size) uvhttp_mimalloc_realloc(ptr, size)
        #define UVHTTP_FREE(ptr) uvhttp_mimalloc_free(ptr)
        #define UVHTTP_CALLOC(nmemb, size) uvhttp_mimalloc_calloc(nmemb, size)
    #else
        /* mimalloc未启用，回退到系统分配器 */
        #define UVHTTP_MALLOC(size) malloc(size)
        #define UVHTTP_REALLOC(ptr, size) realloc(ptr, size)
        #define UVHTTP_FREE(ptr) free(ptr)
        #define UVHTTP_CALLOC(nmemb, size) calloc(nmemb, size)
    #endif
    
#elif UVHTTP_ALLOCATOR_TYPE == UVHTTP_ALLOCATOR_CUSTOM
    /* 自定义分配器 - 需要用户提供实现 */
    extern uvhttp_allocator_t* uvhttp_custom_allocator;
    
    #define UVHTTP_MALLOC(size) uvhttp_custom_allocator->malloc(size)
    #define UVHTTP_REALLOC(ptr, size) uvhttp_custom_allocator->realloc(ptr, size)
    #define UVHTTP_FREE(ptr) uvhttp_custom_allocator->free(ptr)
    #define UVHTTP_CALLOC(nmemb, size) uvhttp_custom_allocator->calloc(nmemb, size)
    
#else
    /* 系统默认分配器 */
    #define UVHTTP_MALLOC(size) malloc(size)
    #define UVHTTP_REALLOC(ptr, size) realloc(ptr, size)
    #define UVHTTP_FREE(ptr) free(ptr)
    #define UVHTTP_CALLOC(nmemb, size) calloc(nmemb, size)
#endif

/* 获取当前分配器 */
uvhttp_allocator_t* uvhttp_allocator_get(void);

/* 设置分配器 */
void uvhttp_allocator_set(uvhttp_allocator_t* allocator);

/* 内联内存分配函数 */
static inline void* uvhttp_malloc(size_t size) {
    uvhttp_allocator_t* allocator = uvhttp_allocator_get();
    if (allocator) {
        return allocator->malloc(size);
    }
    return malloc(size);
}

static inline void* uvhttp_realloc(void* ptr, size_t size) {
    uvhttp_allocator_t* allocator = uvhttp_allocator_get();
    if (allocator) {
        return allocator->realloc(ptr, size);
    }
    return realloc(ptr, size);
}

static inline void uvhttp_free(void* ptr) {
    uvhttp_allocator_t* allocator = uvhttp_allocator_get();
    if (allocator) {
        allocator->free(ptr);
    } else {
        free(ptr);
    }
}

static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    uvhttp_allocator_t* allocator = uvhttp_allocator_get();
    if (allocator) {
        return allocator->calloc(nmemb, size);
    }
    return calloc(nmemb, size);
}

/* 编译时配置参数 */
#ifndef UVHTTP_POOL_SIZE
#define UVHTTP_POOL_SIZE (64 * 1024)  /* 64KB */
#endif

#ifndef UVHTTP_POOL_BLOCK_SIZE
#define UVHTTP_POOL_BLOCK_SIZE 256
#endif

#ifndef UVHTTP_POOL_MAX_BLOCKS
#define UVHTTP_POOL_MAX_BLOCKS 256
#endif

/* 便捷宏 */
#define uvhttp_alloc(size) UVHTTP_MALLOC(size)
#define uvhttp_dealloc(ptr) UVHTTP_FREE(ptr)

/* 内存池配置 */
typedef struct {
    size_t pool_size;
    size_t block_size;
    int max_blocks;
} uvhttp_pool_config_t;

/* 内存池分配器 */
uvhttp_allocator_t* uvhttp_pool_allocator_new(const uvhttp_pool_config_t* config);
void uvhttp_pool_allocator_free(uvhttp_allocator_t* allocator);

/* 统计分配器 */
uvhttp_allocator_t* uvhttp_stats_allocator_new(uvhttp_allocator_t* wrapped);
void uvhttp_stats_allocator_free(uvhttp_allocator_t* allocator);

/* 内存统计结构 */
typedef struct {
    size_t total_allocated;
    size_t total_freed;
    size_t current_allocated;
    size_t allocation_count;
    size_t free_count;
} uvhttp_memory_stats_t;

/* 内存统计 */
void uvhttp_memory_stats_get(uvhttp_memory_stats_t* stats);
void uvhttp_memory_stats_reset(void);

/* 外部分配器包装 */
#define UVHTTP_ALLOCATOR(name, malloc_func, free_func) \
    static uvhttp_allocator_t name##_allocator = { \
        .malloc = malloc_func, \
        .free = free_func, \
        .realloc = realloc, \
        .calloc = calloc, \
        .data = NULL, \
        .type = UVHTTP_ALLOCATOR_CUSTOM \
    };

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ALLOCATOR_H */