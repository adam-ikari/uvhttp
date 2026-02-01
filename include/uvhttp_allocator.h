/**
 * @file uvhttp_allocator.h
 * @brief Unified memory allocator with compile-time optimization
 *
 * This module provides a unified memory allocation interface that can be
 * configured at compile time to use either system allocator or mimalloc.
 * All functions are inline-optimized for zero runtime overhead.
 *
 * @note Allocator type is selected at compile time via UVHTTP_ALLOCATOR_TYPE
 * @note All functions are inline for zero runtime overhead
 */

#ifndef UVHTTP_ALLOCATOR_H
#define UVHTTP_ALLOCATOR_H

#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 编译期选择分配器类型 */
#ifndef UVHTTP_ALLOCATOR_TYPE
#    define UVHTTP_ALLOCATOR_TYPE 0 /* 0=system, 1=mimalloc */
#endif

/* ========== 编译期分配器选择 ========== */

#if UVHTTP_ALLOCATOR_TYPE == 1 /* mimalloc */
#    ifdef UVHTTP_ENABLE_MIMALLOC
#        include "mimalloc.h"

/**
 * @brief Allocate memory using mimalloc
 *
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory, or NULL on failure
 *
 * @note Uses mimalloc's mi_malloc() for high-performance allocation
 * @note This function is inline-optimized
 */
static inline void* uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}

/**
 * @brief Free memory allocated with uvhttp_alloc
 *
 * @param ptr Pointer to memory to free (can be NULL)
 *
 * @note Uses mimalloc's mi_free()
 * @note This function is inline-optimized
 */
static inline void uvhttp_free(void* ptr) {
    mi_free(ptr);
}

/**
 * @brief Reallocate memory
 *
 * @param ptr Pointer to previously allocated memory (can be NULL)
 * @param size New size in bytes
 * @return void* Pointer to reallocated memory, or NULL on failure
 *
 * @note Uses mimalloc's mi_realloc()
 * @note This function is inline-optimized
 */
static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
}

/**
 * @brief Allocate and zero-initialize memory
 *
 * @param nmemb Number of elements
 * @param size Size of each element in bytes
 * @return void* Pointer to allocated and zeroed memory, or NULL on failure
 *
 * @note Uses mimalloc's mi_calloc()
 * @note This function is inline-optimized
 */
static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    return mi_calloc(nmemb, size);
}
#    else
/* mimalloc 不可用，回退到系统分配器 */
static inline void* uvhttp_alloc(size_t size) {
    return malloc(size);
}
static inline void uvhttp_free(void* ptr) {
    free(ptr);
}
static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}
static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}
#    endif

#else /* 系统分配器（默认） */
/* 系统分配器模式 - 使用内联函数确保可以用作函数指针 */

/**
 * @brief Allocate memory using system allocator
 *
 * @param size Number of bytes to allocate
 * @return void* Pointer to allocated memory, or NULL on failure
 *
 * @note Uses standard malloc()
 * @note This function is inline-optimized
 */
static inline void* uvhttp_alloc(size_t size) {
    return malloc(size);
}

/**
 * @brief Free memory allocated with uvhttp_alloc
 *
 * @param ptr Pointer to memory to free (can be NULL)
 *
 * @note Uses standard free()
 * @note This function is inline-optimized
 */
static inline void uvhttp_free(void* ptr) {
    free(ptr);
}

/**
 * @brief Reallocate memory
 *
 * @param ptr Pointer to previously allocated memory (can be NULL)
 * @param size New size in bytes
 * @return void* Pointer to reallocated memory, or NULL on failure
 *
 * @note Uses standard realloc()
 * @note This function is inline-optimized
 */
static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

/**
 * @brief Allocate and zero-initialize memory
 *
 * @param nmemb Number of elements
 * @param size Size of each element in bytes
 * @return void* Pointer to allocated and zeroed memory, or NULL on failure
 *
 * @note Uses standard calloc()
 * @note This function is inline-optimized
 */
static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}
#endif

/* ========== 分配器信息 ========== */

/**
 * @brief Get the name of the current allocator
 *
 * @return const char* Name of the allocator ("mimalloc" or "system")
 *
 * @note Returns "system (mimalloc not available)" if mimalloc was requested but
 * not available
 */
static inline const char* uvhttp_allocator_name(void) {
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

/**
 * @page memory_allocator_usage Memory Allocator Usage Guide
 *
 * @section allocator_selection Allocator Selection
 *
 * Choose allocator type at compile time:
 * @code
 * cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..  # System allocator (default)
 * cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..  # mimalloc allocator
 * @endcode
 *
 * @section allocator_usage Usage
 *
 * @code
 * void* ptr = uvhttp_alloc(size);     // Allocate memory
 * ptr = uvhttp_realloc(ptr, new_size); // Reallocate
 * uvhttp_free(ptr);                    // Free memory
 * ptr = uvhttp_calloc(count, size);   // Allocate and initialize
 * @endcode
 *
 * @section allocator_performance Performance Characteristics
 *
 * - System allocator: Stable and reliable, no extra dependencies
 * - mimalloc: High performance, optimized for multi-threading, built-in small
 * object optimization
 *
 * @section allocator_optimization Compile-time Optimization
 *
 * - All functions are inline
 * - Zero runtime overhead
 * - Compiler can fully optimize
 *
 * @section allocator_notes Notes
 *
 * - mimalloc already optimizes small object allocation, no extra memory pool
 * layer needed
 * - If application needs memory pool, implement at application layer based on
 * specific requirements
 * - Choose appropriate allocator type for best performance
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ALLOCATOR_H */