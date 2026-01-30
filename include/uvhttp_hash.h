/**
 * @file uvhttp_hash.h
 * @brief Hash algorithm interface based on xxHash
 *
 * This module provides high-performance hash functions using xxHash algorithm.
 * All functions are inline-optimized for zero runtime overhead.
 *
 * @note These functions are static inline and defined in the header file
 *       to allow compiler optimization and eliminate function call overhead.
 */

#ifndef UVHTTP_HASH_H
#define UVHTTP_HASH_H

#include "uvhttp_defaults.h"

#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* xxHash 头文件 */
#include "xxhash.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct uvhttp_allocator;

/* ========== 主要API ========== */

/**
 * @brief Compute hash value for arbitrary data
 *
 * @param data Pointer to data buffer to hash
 * @param length Length of data in bytes
 * @param seed Seed value for hash computation
 * @return uint64_t Hash value (0 if data is NULL)
 *
 * @note This function is inline-optimized for performance
 * @note Uses xxHash's 64-bit algorithm (XXH64)
 */
static inline uint64_t uvhttp_hash(const void* data, size_t length,
                                   uint64_t seed) {
    if (!data)
        return 0;
    return XXH64(data, length, seed);
}

/**
 * @brief Compute hash value for null-terminated string
 *
 * @param str Null-terminated string to hash
 * @return uint64_t Hash value (0 if str is NULL)
 *
 * @note Uses default seed (UVHTTP_HASH_DEFAULT_SEED)
 * @note String length is computed internally using strlen()
 */
static inline uint64_t uvhttp_hash_string(const char* str) {
    if (!str)
        return 0;
    return XXH64(str, strlen(str), UVHTTP_HASH_DEFAULT_SEED);
}

/* 便捷函数 */

/**
 * @brief Compute hash value using default seed
 *
 * @param data Pointer to data buffer to hash
 * @param length Length of data in bytes
 * @return uint64_t Hash value using UVHTTP_HASH_DEFAULT_SEED
 *
 * @note Convenience wrapper for uvhttp_hash() with default seed
 */
static inline uint64_t uvhttp_hash_default(const void* data, size_t length) {
    return uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
}

/**
 * @brief Compute string hash using default seed
 *
 * @param str Null-terminated string to hash
 * @return uint64_t Hash value using UVHTTP_HASH_DEFAULT_SEED
 *
 * @note Convenience wrapper for uvhttp_hash_string()
 */
static inline uint64_t uvhttp_hash_string_default(const char* str) {
    return uvhttp_hash_string(str);
}

/* 错误处理宏 */

/**
 * @brief Validate pointer is not NULL
 *
 * @param ptr Pointer to validate
 * @return int TRUE if pointer is not NULL, FALSE otherwise
 *
 * @note This is a macro for compile-time optimization
 */
#define UVHTTP_HASH_VALIDATE_PTR(ptr) ((ptr) != NULL)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_HASH_H */