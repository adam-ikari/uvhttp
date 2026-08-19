/* UVHTTP gzip compression cache module implementation - single-threaded LRU.
 * Mirrors the uvhttp_lru_cache module discipline: explicit create/free, unified
 * allocator, uvhttp_error_t error codes, feature-gated compilation.
 */

#if UVHTTP_FEATURE_COMPRESSION

#    include "uvhttp_gzip_cache.h"

#    include "uvhttp_allocator.h"
#    include "uvhttp_error.h"
#    include "uvhttp_features.h"

#    include <string.h>
#    include <time.h>

/* A single cached compressed body. */
typedef struct {
    uint64_t hash;          /* body content hash (uvhttp_hash_default) */
    size_t body_len;        /* original (uncompressed) body length */
    char* compressed;       /* cached gzip stream (cache-owned) */
    size_t compressed_len;  /* compressed stream length */
    unsigned char valid;    /* 1 = slot in use */
    time_t stored_at;       /* for TTL eviction */
    unsigned long last_used; /* LRU tick */
} gzip_cache_entry_t;

struct uvhttp_gzip_cache {
    gzip_cache_entry_t* entries; /* dynamic array of slots */
    int capacity;                /* allocated slot count */
    int max_entries;             /* effective max entry count */
    size_t max_memory_usage;     /* total memory budget (bytes) */
    size_t total_memory;         /* current memory in use */
    int cache_ttl;               /* seconds; 0 = never expire */
    unsigned long tick;          /* LRU clock */
    int hit_count;
    int miss_count;
    int eviction_count;
};

/* Number of valid entries currently in the cache. */
static int gzip_cache_entry_count(uvhttp_gzip_cache_t* cache) {
    int n = 0;
    for (int i = 0; i < cache->capacity; i++) {
        if (cache->entries[i].valid) n++;
    }
    return n;
}

/* Evict the least-recently-used entry. Assumes at least one valid entry. */
static void gzip_cache_evict_one(uvhttp_gzip_cache_t* cache) {
    int victim = -1;
    unsigned long oldest = 0;
    for (int i = 0; i < cache->capacity; i++) {
        gzip_cache_entry_t* e = &cache->entries[i];
        if (!e->valid) continue;
        if (victim < 0 || e->last_used < oldest) {
            victim = i;
            oldest = e->last_used;
        }
    }
    if (victim < 0) return;
    gzip_cache_entry_t* e = &cache->entries[victim];
    if (e->compressed) uvhttp_free(e->compressed);
    cache->total_memory -= e->compressed_len;
    e->valid = 0;
    e->compressed = NULL;
    e->compressed_len = 0;
    cache->eviction_count++;
}

uvhttp_error_t uvhttp_gzip_cache_create(size_t max_memory_usage, int max_entries,
                                        int cache_ttl,
                                        uvhttp_gzip_cache_t** cache) {
    if (!cache) return UVHTTP_ERROR_INVALID_PARAM;
    if (max_entries <= 0) max_entries = UVHTTP_GZIP_CACHE_DEFAULT_MAX_ENTRIES;
    if (max_memory_usage == 0) {
        max_memory_usage = UVHTTP_GZIP_CACHE_DEFAULT_MAX_MEMORY;
    }

    uvhttp_gzip_cache_t* c =
        (uvhttp_gzip_cache_t*)uvhttp_alloc(sizeof(uvhttp_gzip_cache_t));
    if (!c) return UVHTTP_ERROR_OUT_OF_MEMORY;
    memset(c, 0, sizeof(*c));

    c->entries = (gzip_cache_entry_t*)uvhttp_alloc(
        sizeof(gzip_cache_entry_t) * (size_t)max_entries);
    if (!c->entries) {
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memset(c->entries, 0, sizeof(gzip_cache_entry_t) * (size_t)max_entries);

    c->capacity = max_entries;
    c->max_entries = max_entries;
    c->max_memory_usage = max_memory_usage;
    c->cache_ttl = cache_ttl;
    *cache = c;
    return UVHTTP_OK;
}

void uvhttp_gzip_cache_free(uvhttp_gzip_cache_t* cache) {
    if (!cache) return;
    uvhttp_gzip_cache_clear(cache);
    uvhttp_free(cache->entries);
    uvhttp_free(cache);
}

void uvhttp_gzip_cache_clear(uvhttp_gzip_cache_t* cache) {
    if (!cache) return;
    for (int i = 0; i < cache->capacity; i++) {
        gzip_cache_entry_t* e = &cache->entries[i];
        if (e->valid && e->compressed) uvhttp_free(e->compressed);
        e->valid = 0;
        e->compressed = NULL;
        e->compressed_len = 0;
    }
    cache->total_memory = 0;
}

const char* uvhttp_gzip_cache_find(uvhttp_gzip_cache_t* cache, uint64_t hash,
                                   size_t body_len, size_t* out_len) {
    if (!cache || !out_len) return NULL;
    time_t now = time(NULL);
    for (int i = 0; i < cache->capacity; i++) {
        gzip_cache_entry_t* e = &cache->entries[i];
        if (!e->valid) continue;
        /* Lazy TTL eviction */
        if (cache->cache_ttl > 0 && (now - e->stored_at) > cache->cache_ttl) {
            if (e->compressed) uvhttp_free(e->compressed);
            cache->total_memory -= e->compressed_len;
            e->valid = 0;
            e->compressed = NULL;
            e->compressed_len = 0;
            cache->eviction_count++;
            continue;
        }
        if (e->hash == hash && e->body_len == body_len) {
            e->last_used = ++cache->tick;
            *out_len = e->compressed_len;
            cache->hit_count++;
            return e->compressed;
        }
    }
    cache->miss_count++;
    return NULL;
}

uvhttp_error_t uvhttp_gzip_cache_put(uvhttp_gzip_cache_t* cache, uint64_t hash,
                                     size_t body_len, const char* compressed,
                                     size_t compressed_len) {
    if (!cache || !compressed || compressed_len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Single entry larger than the whole budget is never cached: keeping it
     * would defeat the memory-budget guarantee entirely. */
    if (compressed_len > cache->max_memory_usage) {
        return UVHTTP_OK;
    }

    /* Replacing an existing entry with the same key: update in place instead
     * of allocating a second slot, so find() always returns the newest value
     * for a given (hash, body_len) pair. */
    for (int i = 0; i < cache->capacity; i++) {
        gzip_cache_entry_t* e = &cache->entries[i];
        if (e->valid && e->hash == hash && e->body_len == body_len) {
            char* copy = (char*)uvhttp_alloc(compressed_len);
            if (!copy) return UVHTTP_ERROR_OUT_OF_MEMORY;
            memcpy(copy, compressed, compressed_len);
            if (e->compressed) uvhttp_free(e->compressed);
            cache->total_memory -= e->compressed_len;
            e->compressed = copy;
            e->compressed_len = compressed_len;
            e->stored_at = time(NULL);
            e->last_used = ++cache->tick;
            cache->total_memory += compressed_len;
            return UVHTTP_OK;
        }
    }

    /* Respect the memory budget: if adding this entry would exceed the budget,
     * evict least-recently-used entries until it fits (or until empty). */
    while (cache->total_memory + compressed_len > cache->max_memory_usage &&
           gzip_cache_entry_count(cache) > 0) {
        gzip_cache_evict_one(cache);
    }
    /* Also respect the entry-count limit. */
    while (gzip_cache_entry_count(cache) >= cache->max_entries) {
        gzip_cache_evict_one(cache);
    }

    /* Pick the first free slot. */
    int slot = -1;
    for (int i = 0; i < cache->capacity; i++) {
        if (!cache->entries[i].valid) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        /* No free slot (should not happen after eviction), skip caching. */
        return UVHTTP_OK;
    }

    char* copy = (char*)uvhttp_alloc(compressed_len);
    if (!copy) return UVHTTP_ERROR_OUT_OF_MEMORY;
    memcpy(copy, compressed, compressed_len);

    gzip_cache_entry_t* e = &cache->entries[slot];
    e->hash = hash;
    e->body_len = body_len;
    e->compressed = copy;
    e->compressed_len = compressed_len;
    e->valid = 1;
    e->stored_at = time(NULL);
    e->last_used = ++cache->tick;
    cache->total_memory += compressed_len;
    return UVHTTP_OK;
}

void uvhttp_gzip_cache_set_max_entries(uvhttp_gzip_cache_t* cache,
                                       int max_entries) {
    if (!cache || max_entries <= 0) return;
    cache->max_entries = max_entries;
    while (gzip_cache_entry_count(cache) > cache->max_entries) {
        gzip_cache_evict_one(cache);
    }
}

void uvhttp_gzip_cache_set_max_memory_usage(uvhttp_gzip_cache_t* cache,
                                            size_t max_memory_usage) {
    if (!cache || max_memory_usage == 0) return;
    cache->max_memory_usage = max_memory_usage;
    while (cache->total_memory > cache->max_memory_usage &&
           gzip_cache_entry_count(cache) > 0) {
        gzip_cache_evict_one(cache);
    }
}

void uvhttp_gzip_cache_set_cache_ttl(uvhttp_gzip_cache_t* cache, int cache_ttl) {
    if (!cache) return;
    cache->cache_ttl = cache_ttl;
}

void uvhttp_gzip_cache_get_stats(uvhttp_gzip_cache_t* cache,
                                 size_t* total_memory_usage, int* entry_count,
                                 int* hit_count, int* miss_count) {
    if (!cache) return;
    if (total_memory_usage) *total_memory_usage = cache->total_memory;
    if (entry_count) *entry_count = gzip_cache_entry_count(cache);
    if (hit_count) *hit_count = cache->hit_count;
    if (miss_count) *miss_count = cache->miss_count;
}

#endif /* UVHTTP_FEATURE_COMPRESSION */
