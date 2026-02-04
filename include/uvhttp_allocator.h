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

/* Compile-time allocator type selection */
#ifndef UVHTTP_ALLOCATOR_TYPE
#    define UVHTTP_ALLOCATOR_TYPE 0 /* 0=system, 1=mimalloc, 2=custom */
#endif

/* ========== Compile-time Allocator Selection ========== */

#if UVHTTP_ALLOCATOR_TYPE == 1 /* mimalloc */
#    ifdef UVHTTP_ENABLE_MIMALLOC
#        include "mimalloc.h"

static inline void* uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    mi_free(ptr);
}

static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
}

static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    return mi_calloc(nmemb, size);
}
#    else
/* mimalloc unavailable, fallback to system allocator */
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

#elif UVHTTP_ALLOCATOR_TYPE == 2 /* custom */
/* Custom allocator - application layer must implement these functions */
extern void* uvhttp_custom_alloc(size_t size);
extern void uvhttp_custom_free(void* ptr);
extern void* uvhttp_custom_realloc(void* ptr, size_t size);
extern void* uvhttp_custom_calloc(size_t nmemb, size_t size);

static inline void* uvhttp_alloc(size_t size) {
    return uvhttp_custom_alloc(size);
}

static inline void uvhttp_free(void* ptr) {
    uvhttp_custom_free(ptr);
}

static inline void* uvhttp_realloc(void* ptr, size_t size) {
    return uvhttp_custom_realloc(ptr, size);
}

static inline void* uvhttp_calloc(size_t nmemb, size_t size) {
    return uvhttp_custom_calloc(nmemb, size);
}

#else /* System(Default) - UVHTTP_ALLOCATOR_TYPE == 0 */
/* System allocator mode - use inline functions to ensure they can be used as
 * function pointers */

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
#endif

/* ========== Allocator Information ========== */

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

/* ========== Usage Instructions ========== */

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