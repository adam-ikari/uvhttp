/* UVHTTP 哈希算法接口 - 基于 xxHash */

#ifndef UVHTTP_HASH_H
#define UVHTTP_HASH_H

#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "uvhttp_defaults.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct uvhttp_allocator;

/* ========== 主要API ========== */

/* 基础哈希函数 */
uint64_t uvhttp_hash(const void* data, size_t length, uint64_t seed);
uint64_t uvhttp_hash_string(const char* str);

/* 便捷函数 */
static inline uint64_t uvhttp_hash_default(const void* data, size_t length) {
    return uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
}

static inline uint64_t uvhttp_hash_string_default(const char* str) {
    return uvhttp_hash_string(str);
}

/* 错误处理宏 */
#define UVHTTP_HASH_VALIDATE_PTR(ptr) ((ptr) != NULL)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_HASH_H */