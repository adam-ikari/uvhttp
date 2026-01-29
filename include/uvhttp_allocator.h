/* UVHTTP 统一内存分配器 - 编译期优化 */

#ifndef UVHTTP_ALLOCATOR_H
#define UVHTTP_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 编译期选择分配器类型 */
#ifndef UVHTTP_ALLOCATOR_TYPE
#    define UVHTTP_ALLOCATOR_TYPE 0 /* 0=系统, 1=mimalloc */
#endif

/* ========== 编译期分配器选择 ========== */

#if UVHTTP_ALLOCATOR_TYPE == 1 /* mimalloc */
#    ifdef UVHTTP_ENABLE_MIMALLOC
#        include "mimalloc.h"
/* mimalloc 模式 - 直接使用 mimalloc API */
static inline void*
uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}
static inline void
uvhttp_free(void* ptr) {
    mi_free(ptr);
}
static inline void*
uvhttp_realloc(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
}
static inline void*
uvhttp_calloc(size_t nmemb, size_t size) {
    return mi_calloc(nmemb, size);
}
#    else
/* mimalloc 不可用，回退到系统分配器 */
static inline void*
uvhttp_alloc(size_t size) {
    return malloc(size);
}
static inline void
uvhttp_free(void* ptr) {
    free(ptr);
}
static inline void*
uvhttp_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}
static inline void*
uvhttp_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}
#    endif

#else /* 系统分配器（默认） */
/* 系统分配器模式 - 使用内联函数确保可以用作函数指针 */
static inline void*
uvhttp_alloc(size_t size) {
    return malloc(size);
}
static inline void
uvhttp_free(void* ptr) {
    free(ptr);
}
static inline void*
uvhttp_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}
static inline void*
uvhttp_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}
#endif

/* ========== 分配器信息 ========== */

static inline const char*
uvhttp_allocator_name(void) {
#if UVHTTP_ALLOCATOR_TYPE == 1
#    ifdef UVHTTP_ENABLE_MIMALLOC
    return "mimalloc";
#    else
    return "system (mimalloc not available)";
#    endif
#else
    return "system";
#endif
}

/* ========== 使用说明 ========== */

/*
 * UVHTTP 内存分配器使用说明
 *
 * 编译时选择分配器类型：
 *   cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..  # 系统分配器（默认）
 *   cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..  # mimalloc 分配器
 *
 * 代码中使用：
 *   void* ptr = uvhttp_alloc(size);     // 分配内存
 *   ptr = uvhttp_realloc(ptr, new_size); // 重新分配
 *   uvhttp_free(ptr);                    // 释放内存
 *   ptr = uvhttp_calloc(count, size);   // 分配并初始化
 *
 * 性能特点：
 * - 系统分配器：稳定可靠，无额外依赖
 * - mimalloc：高性能，适合多线程场景，内置小对象优化
 *
 * 编译期优化：
 * - 所有函数都是内联函数
 * - 零运行时开销
 * - 编译器可以完全优化
 *
 * 注意：
 * - mimalloc 本身已经优化了小对象分配，无需额外内存池层
 * - 如果应用需要内存池，应在应用层根据具体需求实现
 * - 选择合适的分配器类型可以获得最佳性能
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ALLOCATOR_H */