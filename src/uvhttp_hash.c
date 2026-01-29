/* UVHTTP 哈希算法实现 - 基于 xxHash */

#include "uvhttp_hash.h"

/* 包含xxHash库 */
#include "xxhash.h"

/* ========== 基础API实现 ========== */
uint64_t
uvhttp_hash(const void* data, size_t length, uint64_t seed) {
    if (!data)
        return 0;
    return XXH64(data, length, seed);
}

uint64_t
uvhttp_hash_string(const char* str) {
    if (!str)
        return 0;
    return XXH64(str, strlen(str), UVHTTP_HASH_DEFAULT_SEED);
}