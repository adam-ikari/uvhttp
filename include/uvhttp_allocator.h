/* 内存分配器模块 - 简化版本 */

#ifndef UVHTTP_ALLOCATOR_H
#define UVHTTP_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

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

/* 分配器接口 */
typedef struct uvhttp_allocator {
    void* (*malloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
    void* (*calloc)(size_t nmemb, size_t size);
    void* data;
    uvhttp_allocator_type_t type;
} uvhttp_allocator_t;

/* 确保 UVHTTP_ALLOCATOR_TYPE 总是有定义 */
#ifndef UVHTTP_ALLOCATOR_TYPE
#define UVHTTP_ALLOCATOR_TYPE 0  /* 默认为系统分配器 */
#endif

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

    /* 自定义分配器 - 运行时选择 */
    extern uvhttp_allocator_t* uvhttp_custom_allocator;

    /* 自定义分配器函数 */
    void* uvhttp_custom_malloc(size_t size);
    void* uvhttp_custom_realloc(void* ptr, size_t size);
    void  uvhttp_custom_free(void* ptr);
    void* uvhttp_custom_calloc(size_t nmemb, size_t size);

    /* 映射到自定义分配器函数 */
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

/* 兼容性函数声明 */
#if UVHTTP_ALLOCATOR_TYPE == 2  /* UVHTTP_ALLOCATOR_CUSTOM */
/* 自定义分配器需要运行时函数 */
void* uvhttp_malloc(size_t size);
void* uvhttp_realloc(void* ptr, size_t size);
void uvhttp_free(void* ptr);
void* uvhttp_calloc(size_t nmemb, size_t size);
#else
/* 其他分配器类型提供内联函数实现 */
static inline void* uvhttp_malloc(size_t size) {
    return UVHTTP_MALLOC(size);
}

static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return UVHTTP_REALLOC(ptr, size);
}

static inline void uvhttp_free(void* ptr) {
    UVHTTP_FREE(ptr);
}

static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    return UVHTTP_CALLOC(nmemb, size);
}
#endif

/* 运行时分配器获取函数（仅用于自定义分配器） */
#if UVHTTP_ALLOCATOR_TYPE == 2  /* UVHTTP_ALLOCATOR_CUSTOM */
uvhttp_allocator_t* uvhttp_allocator_get(void);
void uvhttp_allocator_set(uvhttp_allocator_t* allocator);
#else
/* 编译时确定的分配器不需要运行时查询 */
static inline uvhttp_allocator_t* uvhttp_allocator_get(void) {
    static uvhttp_allocator_t allocator = {0};
    static int initialized = 0;
    
    if (!initialized) {
#if UVHTTP_ALLOCATOR_TYPE == 1  /* UVHTTP_ALLOCATOR_MIMALLOC */
        allocator.malloc = mi_malloc;
        allocator.realloc = mi_realloc;
        allocator.free = mi_free;
        allocator.calloc = mi_calloc;
#elif UVHTTP_ALLOCATOR_TYPE == 2  /* UVHTTP_ALLOCATOR_CUSTOM */
        allocator.malloc = uvhttp_custom_malloc;
        allocator.realloc = uvhttp_custom_realloc;
        allocator.free = uvhttp_custom_free;
        allocator.calloc = uvhttp_custom_calloc;
#else
        allocator.malloc = malloc;
        allocator.realloc = realloc;
        allocator.free = free;
        allocator.calloc = calloc;
#endif
        allocator.data = NULL;
        allocator.type = UVHTTP_ALLOCATOR_TYPE;
        initialized = 1;
    }
    
    return &allocator;
}
#endif

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

/* 获取当前分配器名称 */
const char* uvhttp_allocator_name(void);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ALLOCATOR_H */