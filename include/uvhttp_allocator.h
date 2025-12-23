/* 内存分配器模块 - 简化版本 */

#ifndef UVHTTP_ALLOCATOR_H
#define UVHTTP_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 编译时分配器选择宏 */
#ifndef UVHTTP_ALLOCATOR_TYPE
#define UVHTTP_ALLOCATOR_TYPE 0  /* 默认为系统分配器 */
#endif

/* 分配器类型枚举 */
typedef enum {
    UVHTTP_ALLOCATOR_DEFAULT,    /* 系统默认 */
    UVHTTP_ALLOCATOR_MIMALLOC,    /* mimalloc */
    UVHTTP_ALLOCATOR_CUSTOM      /* 自定义 */
} uvhttp_allocator_type_t;

/* 编译宏控制的分配器选择 */
#if UVHTTP_ALLOCATOR_TYPE == 1  /* UVHTTP_ALLOCATOR_MIMALLOC */

    /* mimalloc支持检测 */
    #ifndef UVHTTP_ENABLE_MIMALLOC
    #define UVHTTP_ENABLE_MIMALLOC 1
    #endif

    /* 包含mimalloc头文件 */
    #include "mimalloc.h"

    /* 直接映射到mimalloc函数 - 编译时优化 */
    #define UVHTTP_MALLOC(size) mi_malloc(size)
    #define UVHTTP_REALLOC(ptr, size) mi_realloc(ptr, size)
    #define UVHTTP_FREE(ptr) mi_free(ptr)
    #define UVHTTP_CALLOC(nmemb, size) mi_calloc(nmemb, size)

#elif UVHTTP_ALLOCATOR_TYPE == 2  /* UVHTTP_ALLOCATOR_CUSTOM */

    /* 自定义分配器外部函数声明 - 用户需要在外部实现 */
    extern void* uvhttp_custom_malloc(size_t size);
    extern void* uvhttp_custom_realloc(void* ptr, size_t size);
    extern void  uvhttp_custom_free(void* ptr);
    extern void* uvhttp_custom_calloc(size_t nmemb, size_t size);

    /* 映射到自定义分配器外部函数 */
    #define UVHTTP_MALLOC(size) uvhttp_custom_malloc(size)
    #define UVHTTP_REALLOC(ptr, size) uvhttp_custom_realloc(ptr, size)
    #define UVHTTP_FREE(ptr) uvhttp_custom_free(ptr)
    #define UVHTTP_CALLOC(nmemb, size) uvhttp_custom_calloc(nmemb, size)

#else

    /* 系统默认分配器 */
    #define UVHTTP_MALLOC(size) malloc(size)
    #define UVHTTP_REALLOC(ptr, size) realloc(ptr, size)
    #define UVHTTP_FREE(ptr) free(ptr)
    #define UVHTTP_CALLOC(nmemb, size) calloc(nmemb, size)

#endif

/* 内存池管理 */
#define MEMORY_POOL_SIZE 1000
#define MEMORY_POOL_BLOCK_SIZE 1024
static char* memory_pool[MEMORY_POOL_SIZE];
static size_t memory_pool_sizes[MEMORY_POOL_SIZE];
static int memory_pool_used[MEMORY_POOL_SIZE];

// 初始化内存池 - 简化版本（单线程环境）
static void init_memory_pool() {
    static int initialized = 0;
    if (initialized) return;
    
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        memory_pool[i] = NULL;
        memory_pool_sizes[i] = 0;
        memory_pool_used[i] = 0;
    }
    initialized = 1;
}

// 从内存池分配内存
static void* pool_malloc(size_t size) {
    init_memory_pool();
    
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (!memory_pool[i] || !memory_pool_used[i]) {
            if (!memory_pool[i]) {
                memory_pool[i] = UVHTTP_MALLOC(MEMORY_POOL_BLOCK_SIZE);
                if (!memory_pool[i]) {
                    return NULL;
                }
                memory_pool_sizes[i] = MEMORY_POOL_BLOCK_SIZE;
            }
            
            if (size <= memory_pool_sizes[i]) {
                memory_pool_used[i] = 1;
                return memory_pool[i];
            }
        }
    }
    
    // 内存池已满，使用系统分配器
    return UVHTTP_MALLOC(size);
}

// 释放内存到内存池
static void pool_free(void* ptr) {
    if (!ptr) return;
    
    for (int i = 0; i < MEMORY_POOL_SIZE; i++) {
        if (memory_pool[i] == ptr && memory_pool_used[i]) {
            memory_pool_used[i] = 0;
            return;
        }
    }
    
    // 不在内存池中，使用系统释放器
    free(ptr);
}



/* 兼容性函数声明 */
static inline void* uvhttp_malloc(size_t size) {
    if (size <= MEMORY_POOL_BLOCK_SIZE) {
        return pool_malloc(size);
    }
    return UVHTTP_MALLOC(size);
}

static inline void* uvhttp_realloc(void* ptr, size_t size) {
    // 简化实现：直接使用系统分配器
    void* new_ptr = UVHTTP_REALLOC(ptr, size);
    return new_ptr;
}



static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    if (total_size <= MEMORY_POOL_BLOCK_SIZE) {
        void* ptr = pool_malloc(total_size);
        if (ptr) {
            memset(ptr, 0, total_size);
        }
        return ptr;
    }
    return UVHTTP_CALLOC(nmemb, size);
}

static inline void uvhttp_free(void* ptr) {
    if (!ptr) return;
    
    // 检查是否在内存池范围内（简化检查）
    // 这里使用一个简单的方法：尝试从内存池释放
    pool_free(ptr);
}

/* 便捷宏 */
#define uvhttp_alloc(size) UVHTTP_MALLOC(size)
#define uvhttp_dealloc(ptr) UVHTTP_FREE(ptr)

/* 编译时分配器信息 */
#if UVHTTP_ALLOCATOR_TYPE == 1  /* UVHTTP_ALLOCATOR_MIMALLOC */
#define UVHTTP_ALLOCATOR_NAME "mimalloc"
#elif UVHTTP_ALLOCATOR_TYPE == 2  /* UVHTTP_ALLOCATOR_CUSTOM */
#define UVHTTP_ALLOCATOR_NAME "custom"
#else
#define UVHTTP_ALLOCATOR_NAME "default"
#endif

/* 自定义分配器函数需要用户在外部实现 */

/* 获取当前分配器名称 */
static inline const char* uvhttp_allocator_name(void) {
    return UVHTTP_ALLOCATOR_NAME;
}

/*
 * ============================================================================
 * 内存分配器使用说明
 * ============================================================================
 * 
 * 本内存分配器通过编译时宏控制，支持三种分配器类型：
 * 
 * 1. 系统默认分配器（UVHTTP_ALLOCATOR_TYPE=0）
 *    - 直接使用标准库的 malloc/free
 *    - 无额外依赖，性能稳定
 *    - 适用于大多数场景
 * 
 * 2. mimalloc分配器（UVHTTP_ALLOCATOR_TYPE=1）
 *    - 使用高性能的mimalloc库
 *    - 更好的多线程性能和内存碎片管理
 *    - 适用于高并发场景
 *    - 编译时需要链接mimalloc库
 * 
 * 3. 自定义分配器（UVHTTP_ALLOCATOR_TYPE=2）
 *    - 用户通过外部链接实现自定义分配策略
 *    - 适用于特殊内存管理需求
 *    - 需要在外部实现4个分配函数
 *    - 支持内存池、对象池等高级策略
 * 
 * 使用方法：
 * 
 * 编译时指定分配器类型：
 *   cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..  # 默认分配器
 *   cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..  # mimalloc分配器
 *   cmake -DUVHTTP_ALLOCATOR_TYPE=2 ..  # 自定义分配器
 * 
 * 代码中使用：
 *   void* ptr = UVHTTP_MALLOC(size);     // 分配内存
 *   ptr = UVHTTP_REALLOC(ptr, new_size); // 重新分配
 *   UVHTTP_FREE(ptr);                    // 释放内存
 *   ptr = UVHTTP_CALLOC(count, size);   // 分配并初始化
 * 
 * 或者使用内联函数：
 *   void* ptr = uvhttp_malloc(size);
 *   ptr = uvhttp_realloc(ptr, new_size);
 *   uvhttp_free(ptr);
 *   ptr = uvhttp_calloc(count, size);
 * 
 * 性能建议：
 * - 默认场景使用系统分配器
 * - 高并发多线程场景使用mimalloc
 * - 特殊需求时可实现自定义分配器
 * - 所有分配操作都有统一的错误返回值检查
 * 
 * ============================================================================
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ALLOCATOR_H */