/* 内存分配器模块 - 简化版本 */

#ifndef UVHTTP_ALLOCATOR_H
#define UVHTTP_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

/* 防止与 uvhttp_features.h 中的宏定义冲突 */
#ifndef UVHTTP_MALLOC_DEFINED

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

/* 定义宏已定义标志 */
#define UVHTTP_MALLOC_DEFINED
#endif /* UVHTTP_MALLOC_DEFINED */

/* 统一分配器接口 */
static inline void* uvhttp_malloc(size_t size) {
    return UVHTTP_MALLOC(size);
}

static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return UVHTTP_REALLOC(ptr, size);
}

static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
#ifndef UVHTTP_CALLOC
    return calloc(nmemb, size);
#else
    return UVHTTP_CALLOC(nmemb, size);
#endif
}

static inline void uvhttp_free(void* ptr) {
    if (!ptr) return;
    UVHTTP_FREE(ptr);
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