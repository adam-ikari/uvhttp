/* UVHTTP LRU cache module implementation - based on uthash, single-threaded
 * version */

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_lru_cache.h"
#    include "uvhttp_constants.h"

#    include "uvhttp_allocator.h"
#    include "uvhttp_error.h"
#    include "uvhttp_error_handler.h"
#    include "uvhttp_error_helpers.h"
#    include "uvhttp_logging.h"

#    include <stdlib.h>
#    include <string.h>
#    include <time.h>

/* Include uthash header file */
#    include "uthash.h"

/**
 * Create LRU cache manager
 */
uvhttp_error_t uvhttp_lru_cache_create(size_t max_memory_usage, int max_entries,
                                       int cache_ttl, cache_manager_t** cache) {
    UVHTTP_LOG_DEBUG(
        "Creating LRU cache: max_memory=%zu, max_entries=%d, ttl=%d",
        max_memory_usage, max_entries, cache_ttl);

    if (!cache) {
        UVHTTP_LOG_ERROR(
            "Failed to create LRU cache: invalid output parameter");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (max_memory_usage == 0) {
        UVHTTP_LOG_ERROR(
            "Failed to create LRU cache: max_memory_usage cannot be zero");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (max_entries <= 0) {
        UVHTTP_LOG_ERROR(
            "Failed to create LRU cache: max_entries must be positive");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_manager_t* cache_ptr = uvhttp_alloc(sizeof(cache_manager_t));
    if (!cache_ptr) {
        UVHTTP_LOG_ERROR("Failed to create LRU cache: memory allocation error");
        uvhttp_handle_memory_failure("cache_manager", NULL, NULL);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(cache_ptr, 0, sizeof(cache_manager_t));
    cache_ptr->max_memory_usage = max_memory_usage;
    cache_ptr->max_entries = max_entries;
    cache_ptr->cache_ttl = cache_ttl;
    cache_ptr->max_file_size = 1024 * 1024 * 1024; /* Default: 1GB */

    /* Single-threaded version: no need to initialize lock */

    UVHTTP_LOG_INFO("LRU cache created successfully");
    *cache = cache_ptr;
    return UVHTTP_OK;
}

/**
 * Free cache entry
 */
static void free_cache_entry(cache_entry_t* entry) {
    if (!entry)
        return;

    if (entry->content) {
        uvhttp_free(entry->content);
    }
    uvhttp_free(entry);
}

/**
 * Free LRU cache manager
 */
void uvhttp_lru_cache_free(cache_manager_t* cache) {
    if (!cache)
        return;

    UVHTTP_LOG_DEBUG("Freeing LRU cache: current entries=%d, memory usage=%zu",
                     cache->entry_count, cache->total_memory_usage);

    uvhttp_lru_cache_clear(cache);

    /* Single-threaded version: no need to destroy lock */

    uvhttp_free(cache);
    UVHTTP_LOG_DEBUG("LRU cache freed");
}

/**
 * Find cache entry - single-threaded version
 */
cache_entry_t* uvhttp_lru_cache_find(cache_manager_t* cache,
                                     const char* file_path) {
    if (!cache || !file_path) {
        UVHTTP_LOG_WARN(
            "Invalid cache lookup parameters: cache=%p, file_path=%p", cache,
            file_path);
        return NULL;
    }

    UVHTTP_LOG_DEBUG("Looking up cache entry: %s", file_path);

    cache_entry_t* entry = NULL;

    /* single-thread version: no need to add locks */

    HASH_FIND_STR(cache->hash_table, file_path, entry);

    if (entry) {
        /* check if expired */
        if (uvhttp_lru_cache_is_expired(entry, cache->cache_ttl)) {
            UVHTTP_LOG_DEBUG("Cache entry expired: %s", file_path);
            /* remove expired entry */
            uvhttp_lru_cache_remove(cache, file_path);
            cache->miss_count++;
            return NULL;
        }

        /* update access time and move to LRU header */
        entry->access_time = time(NULL);
        uvhttp_lru_cache_move_to_head(cache, entry);
        cache->hit_count++;
        UVHTTP_LOG_DEBUG("Cache hit: %s (size: %zu)", file_path,
                         entry->content_length);
        return entry;
    }

    cache->miss_count++;
    UVHTTP_LOG_DEBUG("Cache miss: %s", file_path);
    return NULL;
}

/**
 * move entry to LRU list header
 */
void uvhttp_lru_cache_move_to_head(cache_manager_t* cache,
                                   cache_entry_t* entry) {
    if (!cache || !entry)
        return;

    /* if already header, no need to move */
    if (entry == cache->lru_head)
        return;

    /* check if entry is in list (prevent wild pointer) */
    if (entry->lru_prev == NULL && cache->lru_head != NULL) {
        /* entry not in list, directly add to header */
        entry->lru_prev = NULL;
        entry->lru_next = cache->lru_head;

        if (cache->lru_head) {
            cache->lru_head->lru_prev = entry;
        }

        cache->lru_head = entry;

        /* if list is null, tail also points to entry */
        if (!cache->lru_tail) {
            cache->lru_tail = entry;
        }
        return;
    }

    /* remove from current position */
    if (entry->lru_prev) {
        entry->lru_prev->lru_next = entry->lru_next;
    } else {
        /* entry is header */
        cache->lru_head = entry->lru_next;
    }

    if (entry->lru_next) {
        entry->lru_next->lru_prev = entry->lru_prev;
    } else {
        /* entry is tail */
        cache->lru_tail = entry->lru_prev;
    }

    /* move to header */
    entry->lru_prev = NULL;
    entry->lru_next = cache->lru_head;

    if (cache->lru_head) {
        cache->lru_head->lru_prev = entry;
    }

    cache->lru_head = entry;

    /* if list is null, tail also points to entry */
    if (!cache->lru_tail) {
        cache->lru_tail = entry;
    }
}

/**
 * remove entry from LRU list tail
 */
cache_entry_t* uvhttp_lru_cache_remove_tail(cache_manager_t* cache) {
    if (!cache || !cache->lru_tail)
        return NULL;

    cache_entry_t* entry = cache->lru_tail;

    /* update tail pointer */
    cache->lru_tail = entry->lru_prev;

    if (cache->lru_tail) {
        cache->lru_tail->lru_next = NULL;
    } else {
        /* list is null, header also set to null */
        cache->lru_head = NULL;
    }

    /* cleanlistpointer */
    entry->lru_prev = NULL;
    entry->lru_next = NULL;

    return entry;
}

/**
 * check if cache entry has expired
 */
/* time Mock function for testing (weak symbol) */
__attribute__((weak)) time_t get_current_time() {
    return time(NULL);
}

int uvhttp_lru_cache_is_expired(cache_entry_t* entry, int cache_ttl) {
    if (!entry)
        return 1; /* NULL entry considered expired */
    if (cache_ttl <= 0)
        return 0; /* TTL of 0 means never expires */

    /* use get_current_time() to support Mock time system (for testing) */
    time_t now = get_current_time();
    return (now - entry->cache_time) > cache_ttl;
}

/**
 * add or update cache entry - single-thread version
 */
uvhttp_error_t uvhttp_lru_cache_put(cache_manager_t* cache,
                                    const char* file_path, char* content,
                                    size_t content_length,
                                    const char* mime_type, time_t last_modified,
                                    const char* etag) {
    if (!cache || !file_path || !content) {
        UVHTTP_LOG_WARN(
            "Invalid cache add parameters: cache=%p, file_path=%p, content=%p",
            cache, file_path, content);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check if file size exceeds limit */
    if (content_length > cache->max_file_size) {
        UVHTTP_LOG_WARN("File size exceeds limit: %s (size: %zu, limit: %zu)",
                        file_path, content_length, cache->max_file_size);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* calculate memory usage, check integer overflow */
    if (content_length > SIZE_MAX - sizeof(cache_entry_t)) {
        UVHTTP_LOG_ERROR(
            "Content length too large: %zu (causes integer overflow)",
            content_length);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    size_t memory_usage = sizeof(cache_entry_t) + content_length;

    UVHTTP_LOG_DEBUG("Adding cache entry: %s (size: %zu, type: %s)", file_path,
                     content_length, mime_type ? mime_type : "unknown");

    /* single-thread version: no need to add locks */

    /* check if need to evict entries - batch eviction optimization */
    int eviction_count = 0;
    /* Use small batch size (2 entries) to minimize blocking */
    int batch_size = 2;

    while (
        (cache->max_memory_usage > 0 &&
         cache->total_memory_usage + memory_usage > cache->max_memory_usage * 0.9) ||
        (cache->max_entries > 0 && cache->entry_count >= cache->max_entries)) {

        /* if cache is null but still need to evict, it means cannot satisfy
         * condition, return error */
        if (!cache->lru_tail) {
            UVHTTP_LOG_ERROR("Cannot evict more entries: cache is empty but "
                             "still needs space");
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }

        /* batch evict multiple entries to reduce loop count */
        for (int i = 0;
             i < batch_size && ((cache->max_memory_usage > 0 &&
                                 cache->total_memory_usage + memory_usage >
                                     cache->max_memory_usage * 0.9) ||
                                (cache->max_entries > 0 &&
                                 cache->entry_count >= cache->max_entries));
             i++) {

            /* LRU: evict least recently used */
            cache_entry_t* evicted = uvhttp_lru_cache_remove_tail(cache);

            if (!evicted)
                break;

            UVHTTP_LOG_DEBUG(
                "Evicting cache entry: %s (freeing memory: %zu)",
                evicted->file_path, evicted->memory_usage);

            /* remove from hash table */
            HASH_DEL(cache->hash_table, evicted);

            /* updatestatistics */
            cache->total_memory_usage -= evicted->memory_usage;
            cache->entry_count--;
            cache->eviction_count++;
            eviction_count++;

            /* releasememory */
            free_cache_entry(evicted);
        }

        /* if still not enough space after one batch eviction, continue to next
         * batch */
        if (eviction_count > 0 && eviction_count % batch_size == 0) {
            UVHTTP_LOG_DEBUG(
                "Batch eviction completed: %d entries evicted so far",
                eviction_count);
        }
    }

    if (eviction_count > 0) {
        UVHTTP_LOG_INFO("Evicted %d cache entries (batch size: %d) to make "
                        "room for new entry",
                        eviction_count, batch_size);
    }

    /* find if already exists */
    cache_entry_t* entry = NULL;
    HASH_FIND_STR(cache->hash_table, file_path, entry);

    if (entry) {
        /* updateexistingentry */
        UVHTTP_LOG_DEBUG(
            "Updating existing cache entry: %s (old size: %zu, new size: %zu)",
            file_path, entry->content_length, content_length);
        uvhttp_free(entry->content);

        /* update memory usage */
        cache->total_memory_usage -= entry->memory_usage;
    } else {
        /* create new entry */
        entry = uvhttp_alloc(sizeof(cache_entry_t));
        if (!entry) {
            UVHTTP_LOG_ERROR(
                "Failed to create cache entry: memory allocation error");
            uvhttp_handle_memory_failure("cache_entry", NULL, NULL);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }

        memset(entry, 0, sizeof(cache_entry_t));

        /* verify file path length, prevent buffer overflow */
        if (strlen(file_path) >= sizeof(entry->file_path)) {
            UVHTTP_LOG_ERROR("File path too long: %s (max: %zu)", file_path,
                             sizeof(entry->file_path) - 1);
            uvhttp_free(entry);
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
        entry->file_path[sizeof(entry->file_path) - 1] = '\0';

        /* initializeLRUlistpointer */
        entry->lru_prev = NULL;
        entry->lru_next = NULL;

        /* add to hash table */
        HASH_ADD_STR(cache->hash_table, file_path, entry);
        cache->entry_count++;

        UVHTTP_LOG_DEBUG("Created new cache entry: %s", file_path);
    }

    /* allocate and copy content */
    entry->content = (char*)uvhttp_alloc(content_length + 1);
    if (!entry->content) {
        UVHTTP_LOG_ERROR("Failed to allocate content buffer: size=%zu",
                         content_length);
        if (!entry->file_path[0]) {
            /* new entry, need to remove from hash table and release */
            HASH_DEL(cache->hash_table, entry);
            cache->entry_count--;
            uvhttp_free(entry);
        }
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(entry->content, content, content_length);
    entry->content[content_length] = '\0';

    /* setentrycontent */
    entry->content_length = content_length;
    entry->memory_usage = memory_usage;
    entry->last_modified = last_modified;
    entry->access_time = get_current_time();
    entry->cache_time = entry->access_time;
    entry->is_compressed = 0;

    /* setMIMEtype */
    if (mime_type) {
        strncpy(entry->mime_type, mime_type, sizeof(entry->mime_type) - 1);
        entry->mime_type[sizeof(entry->mime_type) - 1] = '\0';
    } else {
        strncpy(entry->mime_type, "application/octet-stream",
                sizeof(entry->mime_type) - 1);
        entry->mime_type[sizeof(entry->mime_type) - 1] = '\0';
    }

    /* setETag */
    if (etag) {
        strncpy(entry->etag, etag, sizeof(entry->etag) - 1);
        entry->etag[sizeof(entry->etag) - 1] = '\0';
    } else {
        entry->etag[0] = '\0';
    }

    /* update memory usage */
    cache->total_memory_usage += memory_usage;

    /* move to LRU header */
    uvhttp_lru_cache_move_to_head(cache, entry);

    UVHTTP_LOG_DEBUG(
        "Cache entry added successfully: %s (total memory: %zu, entries: %d)",
        file_path, cache->total_memory_usage, cache->entry_count);

    return UVHTTP_OK;
}

/**
 * delete cache entry - single-thread version
 */
uvhttp_error_t uvhttp_lru_cache_remove(cache_manager_t* cache,
                                       const char* file_path) {
    if (!cache || !file_path) {
        UVHTTP_LOG_WARN(
            "Invalid cache remove parameters: cache=%p, file_path=%p", cache,
            file_path);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    UVHTTP_LOG_DEBUG("Removing cache entry: %s", file_path);

    /* single-thread version: no need to add locks */

    cache_entry_t* entry = NULL;
    HASH_FIND_STR(cache->hash_table, file_path, entry);

    if (!entry) {
        UVHTTP_LOG_WARN("Attempting to remove non-existent cache entry: %s",
                        file_path);
        return UVHTTP_ERROR_NOT_FOUND;
    }

    /* remove from hash table */
    HASH_DEL(cache->hash_table, entry);

    /* remove from LRU list */
    if (entry->lru_prev) {
        entry->lru_prev->lru_next = entry->lru_next;
    } else {
        cache->lru_head = entry->lru_next;
    }

    if (entry->lru_next) {
        entry->lru_next->lru_prev = entry->lru_prev;
    } else {
        cache->lru_tail = entry->lru_prev;
    }

    /* updatestatistics */
    cache->total_memory_usage -= entry->memory_usage;
    cache->entry_count--;

    UVHTTP_LOG_DEBUG("Cache entry removed successfully: %s (freed memory: %zu)",
                     file_path, entry->memory_usage);

    /* releasememory */
    free_cache_entry(entry);

    return UVHTTP_OK;
}

/**
 * clear all cache - single-thread version
 */
void uvhttp_lru_cache_clear(cache_manager_t* cache) {
    if (!cache)
        return;

    UVHTTP_LOG_INFO(
        "Clearing all cache entries: current entries=%d, total memory=%zu",
        cache->entry_count, cache->total_memory_usage);

    /* single-thread version: no need to add locks */

    int cleared_count = 0;
    size_t freed_memory = 0;

    cache_entry_t *entry, *tmp;
    HASH_ITER(hh, cache->hash_table, entry, tmp) {
        HASH_DEL(cache->hash_table, entry);
        freed_memory += entry->memory_usage;
        free_cache_entry(entry);
        cleared_count++;
    }

    cache->hash_table = NULL;
    cache->lru_head = NULL;
    cache->lru_tail = NULL;
    cache->total_memory_usage = 0;
    cache->entry_count = 0;

    UVHTTP_LOG_INFO("Cache cleared: removed %d entries, freed %zu bytes",
                    cleared_count, freed_memory);
}

/**
 * get cache statistics info - single-thread version
 */
void uvhttp_lru_cache_get_stats(cache_manager_t* cache,
                                size_t* total_memory_usage, int* entry_count,
                                int* hit_count, int* miss_count,
                                int* eviction_count) {
    if (!cache)
        return;

    /* single-thread version: no need to add locks */

    if (total_memory_usage)
        *total_memory_usage = cache->total_memory_usage;
    if (entry_count)
        *entry_count = cache->entry_count;
    if (hit_count)
        *hit_count = cache->hit_count;
    if (miss_count)
        *miss_count = cache->miss_count;
    if (eviction_count)
        *eviction_count = cache->eviction_count;
}

/**
 * reset statistics info - single-thread version
 */
void uvhttp_lru_cache_reset_stats(cache_manager_t* cache) {
    if (!cache)
        return;

    /* single-thread version: no need to add locks */

    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->eviction_count = 0;
}

/**
 * set maximum file size for cache
 */
void uvhttp_lru_cache_set_max_file_size(cache_manager_t* cache,
                                        size_t max_file_size) {
    if (!cache)
        return;

    cache->max_file_size = max_file_size;
}

/**
 * set maximum memory usage for cache
 */
void uvhttp_lru_cache_set_max_memory_usage(cache_manager_t* cache,
                                           size_t max_memory_usage) {
    if (!cache)
        return;

    cache->max_memory_usage = max_memory_usage;
}

/**
 * set maximum entries for cache
 */
void uvhttp_lru_cache_set_max_entries(cache_manager_t* cache, int max_entries) {
    if (!cache)
        return;

    cache->max_entries = max_entries;
}

/**
 * set cache TTL
 */
void uvhttp_lru_cache_set_cache_ttl(cache_manager_t* cache, int cache_ttl) {
    if (!cache)
        return;

    cache->cache_ttl = cache_ttl;
}

/**
 * clean expired entries - single-thread version
 */
int uvhttp_lru_cache_cleanup_expired(cache_manager_t* cache) {
    if (!cache || cache->cache_ttl <= 0)
        return 0;

    UVHTTP_LOG_DEBUG(
        "Starting cleanup of expired cache entries (TTL: %d seconds)",
        cache->cache_ttl);

    /* single-thread version: no need to add locks */

    int cleaned_count = 0;
    size_t freed_memory = 0;
    time_t now = time(NULL);

    cache_entry_t* entry = cache->lru_tail;
    while (entry) {
        cache_entry_t* next = entry->lru_prev;

        if ((now - entry->cache_time) > cache->cache_ttl) {
            UVHTTP_LOG_DEBUG(
                "Cleaning up expired entry: %s (expired %ld seconds ago)",
                entry->file_path, now - entry->cache_time);

            /* remove from hash table */
            HASH_DEL(cache->hash_table, entry);

            /* remove from LRU list */
            if (entry->lru_prev) {
                entry->lru_prev->lru_next = entry->lru_next;
            } else {
                cache->lru_tail = entry->lru_next;
            }

            if (entry->lru_next) {
                entry->lru_next->lru_prev = entry->lru_prev;
            } else {
                cache->lru_head = entry->lru_prev;
            }

            /* updatestatistics */
            cache->total_memory_usage -= entry->memory_usage;
            cache->entry_count--;
            cleaned_count++;
            freed_memory += entry->memory_usage;

            /* releasememory */
            free_cache_entry(entry);
        }

        entry = next;
    }

    if (cleaned_count > 0) {
        UVHTTP_LOG_INFO("Expired cache cleanup completed: removed %d entries, "
                        "freed %zu bytes",
                        cleaned_count, freed_memory);
    } else {
        UVHTTP_LOG_DEBUG("No expired cache entries found");
    }

    return cleaned_count;
}

/**
 * calculate cache hit rate - single-thread version
 */
double uvhttp_lru_cache_get_hit_rate(cache_manager_t* cache) {
    if (!cache || (cache->hit_count + cache->miss_count) == 0) {
        UVHTTP_LOG_DEBUG("Cache hit rate calculation: no statistics available");
        return 0.0;
    }

    /* single-thread version: no need to add locks */

    double hit_rate =
        (double)cache->hit_count / (cache->hit_count + cache->miss_count);

    UVHTTP_LOG_DEBUG("Cache hit rate: %.2f%% (hits: %d, misses: %d)",
                     hit_rate * 100.0, cache->hit_count, cache->miss_count);

    return hit_rate;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
