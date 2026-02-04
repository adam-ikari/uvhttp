/* UVHTTP LRU Cache Module - Implemented using uthash */

#ifndef UVHTTP_LRU_CACHE_H
#define UVHTTP_LRU_CACHE_H

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_config.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_error.h"

#    include <stddef.h>
#    include <time.h>

/* Include uthash header */
#    include "uthash.h"

#    ifdef __cplusplus
extern "C" {
#    endif

/* Forward declarations */
typedef struct cache_entry cache_entry_t;
typedef struct cache_manager cache_manager_t;

/* LRU cache entry structure */
struct cache_entry {
    char file_path[UVHTTP_MAX_FILE_PATH_SIZE];    /* File path */
    char* content;                                /*  */
    size_t content_length;                        /* Content length */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE]; /* MIMEclass */
    time_t last_modified;                         /* lastmodifywhen */
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];      /* ETagvalue */
    time_t access_time;                           /* lastaccesswhen */
    time_t cache_time;                            /* Cachewhen */
    size_t memory_usage;                          /* memoryUse */
    int is_compressed;                            /* whethercompress */

    /* uthash hash handle */
    UT_hash_handle hh;

    /* LRU linked list handle */
    struct cache_entry* lru_prev;
    struct cache_entry* lru_next;
};

/* LRU cache manager */
struct cache_manager {
    cache_entry_t* hash_table;            /* hash */
    cache_entry_t* lru_head;              /* LRU */
    cache_entry_t* lru_tail;              /* LRU */
    size_t total_memory_usage;            /* memoryUse */
    size_t max_memory_usage;              /* memory */
    size_t max_file_size;                 /* maximum file size */
    int entry_count;                      /* Cacheentry */
    int max_entries;                      /* maximum entries */
    int cache_ttl;                        /* CacheTTL(seconds) */
    const uvhttp_config_t* server_config; /* Server */

    /* Statistics */
    int hit_count;      /* times */
    int miss_count;     /* times */
    int eviction_count; /* evicttimes */
};

/**
 * createLRUCachemanage
 *
 * @param max_memory_usage memoryUse(bytes)
 * @param max_entries Cacheentry
 * @param cache_ttl CacheTTL(seconds), 0representsnever
 * @param cache outputParameter, ReturncreateCachemanage
 * @return UVHTTP_OK Success, othervaluerepresentsError
 */
uvhttp_error_t uvhttp_lru_cache_create(size_t max_memory_usage, int max_entries,
                                       int cache_ttl, cache_manager_t** cache);

/**
 * releaseLRUCachemanage
 *
 * @param cache Cachemanage
 */
void uvhttp_lru_cache_free(cache_manager_t* cache);

/**
 * Cacheentry
 *
 * @param cache Cachemanage
 * @param file_path File path
 * @return Cacheentry, toReturnNULL
 */
cache_entry_t* uvhttp_lru_cache_find(cache_manager_t* cache,
                                     const char* file_path);

/**
 * addorupdateCacheentry
 *
 * @param cache Cachemanage
 * @param file_path File path
 * @param content
 * @param content_length Content length
 * @param mime_type MIMEclass
 * @param last_modified lastmodifywhen
 * @param etag ETagvalue
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_error_t uvhttp_lru_cache_put(cache_manager_t* cache,
                                    const char* file_path, char* content,
                                    size_t content_length,
                                    const char* mime_type, time_t last_modified,
                                    const char* etag);

/**
 * deleteCacheentry
 *
 * @param cache Cachemanage
 * @param file_path File path
 * @return UVHTTP_OKSuccess, othervaluerepresentsFailure
 */
uvhttp_error_t uvhttp_lru_cache_remove(cache_manager_t* cache,
                                       const char* file_path);

/**
 * nullofCache
 *
 * @param cache Cachemanage
 */
void uvhttp_lru_cache_clear(cache_manager_t* cache);

/**
 * getCachestatisticsinfo
 *
 * @param cache Cachemanage
 * @param total_memory_usage outputmemoryUse
 * @param entry_count outputentryquantity
 * @param hit_count outputtimes
 * @param miss_count outputtimes
 * @param eviction_count outputevicttimes
 */
void uvhttp_lru_cache_get_stats(cache_manager_t* cache,
                                size_t* total_memory_usage, int* entry_count,
                                int* hit_count, int* miss_count,
                                int* eviction_count);

/**
 * statisticsinfo
 *
 * @param cache Cachemanage
 */
void uvhttp_lru_cache_reset_stats(cache_manager_t* cache);

/**
 * set maximum file size for cache
 *
 * @param cache Cache manager
 * @param max_file_size Maximum file size in bytes
 */
void uvhttp_lru_cache_set_max_file_size(cache_manager_t* cache,
                                        size_t max_file_size);

/**
 * set maximum memory usage for cache
 *
 * @param cache Cache manager
 * @param max_memory_usage Maximum memory usage in bytes
 */
void uvhttp_lru_cache_set_max_memory_usage(cache_manager_t* cache,
                                           size_t max_memory_usage);

/**
 * set maximum entries for cache
 *
 * @param cache Cache manager
 * @param max_entries Maximum number of entries
 */
void uvhttp_lru_cache_set_max_entries(cache_manager_t* cache, int max_entries);

/**
 * set cache TTL
 *
 * @param cache Cache manager
 * @param cache_ttl Cache TTL in seconds (0 means never expires)
 */
void uvhttp_lru_cache_set_cache_ttl(cache_manager_t* cache, int cache_ttl);

/**
 * entry
 *
 * @param cache Cachemanage
 * @return entryquantity
 */
int uvhttp_lru_cache_cleanup_expired(cache_manager_t* cache);

/**
 * Cacheentrywhether
 *
 * @param entry Cacheentry
 * @param cache_ttl CacheTTL
 * @return 1, 0
 */
int uvhttp_lru_cache_is_expired(cache_entry_t* entry, int cache_ttl);

/**
 * entrytoLRUheader
 *
 * @param cache Cachemanage
 * @param entry Cacheentry
 */
void uvhttp_lru_cache_move_to_head(cache_manager_t* cache,
                                   cache_entry_t* entry);

/**
 * LRUremoveentry
 *
 * @param cache Cachemanage
 * @return isremoveentry
 */
cache_entry_t* uvhttp_lru_cache_remove_tail(cache_manager_t* cache);

/**
 * calculateCacherate
 *
 * @param cache Cachemanage
 * @return rate(0.0-1.0)
 */
double uvhttp_lru_cache_get_hit_rate(cache_manager_t* cache);

#    ifdef __cplusplus
}
#    endif

#endif /* UVHTTP_FEATURE_STATIC_FILES */

#endif /* UVHTTP_LRU_CACHE_H */