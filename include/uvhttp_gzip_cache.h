/**
 * @file uvhttp_gzip_cache.h
 * @brief Gzip response compression cache (LRU, keyed by body content hash)
 *
 * Caches the compressed gzip stream for identical response bodies so the
 * deflate pass runs once per unique body instead of once per request. The
 * cache key is (body content xxhash64, body length).
 *
 * @note Only compiled when UVHTTP_FEATURE_COMPRESSION is enabled.
 * @note Instances are owned by the caller (typically uvhttp_server_t) and
 *   must be released with uvhttp_gzip_cache_free() before shutdown.
 * @note All allocations use the unified allocator (uvhttp_alloc), keeping the
 *   module ASan/UBSan-clean by construction.
 * @note Pointers returned by uvhttp_gzip_cache_find() are owned by the cache
 *   and must NOT be freed by the caller; they remain valid until the entry is
 *   evicted or uvhttp_gzip_cache_clear()/free().
 */

#ifndef UVHTTP_GZIP_CACHE_H
#define UVHTTP_GZIP_CACHE_H

#include "uvhttp_error.h"
#include <stddef.h>
#include <stdint.h>

#if UVHTTP_FEATURE_COMPRESSION

#ifdef __cplusplus
extern "C" {
#endif

/* Default cache limits */
#define UVHTTP_GZIP_CACHE_DEFAULT_MAX_ENTRIES 64
#define UVHTTP_GZIP_CACHE_DEFAULT_MAX_MEMORY (1u << 20) /* 1 MiB */
#define UVHTTP_GZIP_CACHE_DEFAULT_TTL 0                  /* 0 = never expire */

typedef struct uvhttp_gzip_cache uvhttp_gzip_cache_t;

/**
 * Create a gzip compression cache.
 *
 * @param max_memory_usage Maximum total memory (bytes) the cache may hold
 * @param max_entries Maximum number of cached bodies
 * @param cache_ttl Entry time-to-live in seconds (0 = never expire)
 * @param cache Output parameter, receives the created cache
 * @return UVHTTP_OK on success, otherwise an error code
 */
uvhttp_error_t uvhttp_gzip_cache_create(size_t max_memory_usage, int max_entries,
                                        int cache_ttl,
                                        uvhttp_gzip_cache_t** cache);

/**
 * Release a gzip compression cache and all cached entries.
 *
 * @param cache Cache to release (may be NULL)
 */
void uvhttp_gzip_cache_free(uvhttp_gzip_cache_t* cache);

/**
 * Remove all cached entries.
 *
 * @param cache Cache to clear
 */
void uvhttp_gzip_cache_clear(uvhttp_gzip_cache_t* cache);

/**
 * Look up a cached compressed body.
 *
 * @param cache Cache to query
 * @param hash Body content hash (uvhttp_hash_default)
 * @param body_len Original body length
 * @param out_len Output parameter, receives compressed length on hit
 * @return Pointer to cached compressed stream (cache-owned), or NULL on miss
 */
const char* uvhttp_gzip_cache_find(uvhttp_gzip_cache_t* cache, uint64_t hash,
                                   size_t body_len, size_t* out_len);

/**
 * Store a compressed body in the cache (internally copied).
 *
 * @param cache Cache to update
 * @param hash Body content hash (uvhttp_hash_default)
 * @param body_len Original body length
 * @param compressed Compressed gzip stream
 * @param compressed_len Compressed stream length
 * @return UVHTTP_OK on success, otherwise an error code
 */
uvhttp_error_t uvhttp_gzip_cache_put(uvhttp_gzip_cache_t* cache, uint64_t hash,
                                     size_t body_len, const char* compressed,
                                     size_t compressed_len);

/**
 * Set the maximum number of cached entries.
 *
 * @param cache Cache to configure
 * @param max_entries Maximum entry count
 */
void uvhttp_gzip_cache_set_max_entries(uvhttp_gzip_cache_t* cache,
                                       int max_entries);

/**
 * Set the maximum total memory usage in bytes.
 *
 * @param cache Cache to configure
 * @param max_memory_usage Maximum memory usage in bytes
 */
void uvhttp_gzip_cache_set_max_memory_usage(uvhttp_gzip_cache_t* cache,
                                            size_t max_memory_usage);

/**
 * Set the entry time-to-live in seconds (0 = never expire).
 *
 * @param cache Cache to configure
 * @param cache_ttl Cache TTL in seconds
 */
void uvhttp_gzip_cache_set_cache_ttl(uvhttp_gzip_cache_t* cache, int cache_ttl);

/**
 * Retrieve cache statistics.
 *
 * @param cache Cache to inspect
 * @param total_memory_usage Output total memory used (bytes)
 * @param entry_count Output number of valid entries
 * @param hit_count Output cache hit count
 * @param miss_count Output cache miss count
 */
void uvhttp_gzip_cache_get_stats(uvhttp_gzip_cache_t* cache,
                                 size_t* total_memory_usage, int* entry_count,
                                 int* hit_count, int* miss_count);

#endif /* UVHTTP_FEATURE_COMPRESSION */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_GZIP_CACHE_H */
