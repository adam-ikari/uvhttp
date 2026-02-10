#if UVHTTP_FEATURE_ROUTER_CACHE

#    include "uvhttp_allocator.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_hash.h"
#    include "uvhttp_router.h"
#    include "uvhttp_utils.h"

#    include <ctype.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

/* Hash bucket structure - optimized for cache locality */
typedef struct __attribute__((packed)) {
    char path[UVHTTP_MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
    uint32_t access_count;
    uint8_t distance;
    uint8_t _padding[3];
} hash_entry_t;

/* Dynamic hash table with auto-resizing */
typedef struct {
    hash_entry_t* entries;
    size_t size;
    size_t count;
    size_t threshold;
} hash_table_t;

/* Simple router structure - uses hash table only
 *
 * Thread safety note:
 * This implementation is designed for single-threaded event loop architecture
 * (based on libuv):
 * - The router must be used in a single event loop thread
 * - Does not support multi-threaded concurrent access
 * - If you need to use it in a multi-threaded environment, create independent
 * server instances and routers for each thread
 *
 * Performance optimization:
 * - Uses hash table with open addressing (O(1) average lookup)
 * - Lock-free design, avoids mutex overhead
 * - No extra caching layers, zero overhead
 */
typedef struct {
    /* Hash table: dynamic sizing with open addressing */
    hash_table_t hash_table;

    /* Route statistics */
    size_t total_routes;
} cache_optimized_router_t;

/* Use unified hash function (inline function) */
static inline uint32_t route_hash(const char* str) {
    if (!str)
        return 0;

    size_t len = strlen(str);
    if (len > UVHTTP_MAX_ROUTE_PATH_LEN - 1) {
        len = UVHTTP_MAX_ROUTE_PATH_LEN - 1;
    }

    return (uint32_t)XXH64(str, len, UVHTTP_HASH_DEFAULT_SEED);
}

/* Fast method parsing (inline function) */
static inline uvhttp_method_t fast_method_parse(const char* method) {
    if (!method)
        return UVHTTP_ANY;

    /* Use prefix for fast determination */
    switch (method[0]) {
    case 'G':
        return (method[1] == 'E' && method[2] == 'T' && method[3] == '\0')
                   ? UVHTTP_GET
                   : UVHTTP_ANY;
    case 'P':
        if (method[1] == 'O') {
            return (method[2] == 'S' && method[3] == 'T' && method[4] == '\0')
                       ? UVHTTP_POST
                       : UVHTTP_ANY;
        } else if (method[1] == 'U') {
            return (method[2] == 'T' && method[3] == '\0') ? UVHTTP_PUT
                                                           : UVHTTP_ANY;
        }
        break;
    case 'D':
        return (method[1] == 'E' && method[2] == 'L' && method[3] == 'E' &&
                method[4] == 'T' && method[5] == 'E' && method[6] == '\0')
                   ? UVHTTP_DELETE
                   : UVHTTP_ANY;
    case 'H':
        return (method[1] == 'E' && method[2] == 'A' && method[3] == 'D' &&
                method[4] == '\0')
                   ? UVHTTP_HEAD
                   : UVHTTP_ANY;
    case 'O':
        return (method[1] == 'P' && method[2] == 'T' && method[3] == 'I' &&
                method[4] == 'O' && method[5] == 'N' && method[6] == 'S' &&
                method[7] == '\0')
                   ? UVHTTP_OPTIONS
                   : UVHTTP_ANY;
    case 'P':
        return (method[1] == 'A' && method[2] == 'T' && method[3] == 'C' &&
                method[4] == 'H' && method[5] == '\0')
                   ? UVHTTP_PATCH
                   : UVHTTP_ANY;
    case 'T':
        return (method[1] == 'R' && method[2] == 'A' && method[3] == 'C' &&
                method[4] == 'E' && method[5] == '\0')
                   ? UVHTTP_TRACE
                   : UVHTTP_ANY;
    case 'C':
        return (method[1] == 'O' && method[2] == 'N' && method[3] == 'N' &&
                method[4] == 'E' && method[5] == 'C' && method[6] == 'T' &&
                method[7] == '\0')
                   ? UVHTTP_CONNECT
                   : UVHTTP_ANY;
    }
    return UVHTTP_ANY;
}

/* Initialize hash table */
static uvhttp_error_t hash_table_init(hash_table_t* table, size_t initial_size) {
    if (!table || initial_size == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    table->entries = uvhttp_calloc(initial_size, sizeof(hash_entry_t));
    if (!table->entries) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    table->size = initial_size;
    table->count = 0;
    table->threshold = (size_t)(initial_size * UVHTTP_ROUTER_HASH_LOAD_FACTOR);

    return UVHTTP_OK;
}

/* Resize hash table */
static uvhttp_error_t hash_table_resize(hash_table_t* table, size_t new_size) {
    if (!table || new_size == 0 || new_size > UVHTTP_ROUTER_HASH_MAX_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    hash_entry_t* new_entries = uvhttp_calloc(new_size, sizeof(hash_entry_t));
    if (!new_entries) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* Rehash all entries using linear probing */
    for (size_t i = 0; i < table->size; i++) {
        hash_entry_t* old_entry = &table->entries[i];
        if (old_entry->path[0] == '\0') {
            continue;
        }

        uint32_t hash = route_hash(old_entry->path);
        uint32_t index = hash % new_size;
        uint32_t start_index = index;

        /* Find empty slot using linear probing */
        while (new_entries[index].path[0] != '\0') {
            index = (index + 1) % new_size;

            /* Prevent infinite loop */
            if (index == start_index) {
                uvhttp_free(new_entries);
                return UVHTTP_ERROR_OUT_OF_MEMORY;
            }
        }

        /* Copy entry */
        memcpy(&new_entries[index], old_entry, sizeof(hash_entry_t));
        new_entries[index].distance = 0;
    }

    uvhttp_free(table->entries);
    table->entries = new_entries;
    table->size = new_size;
    table->threshold = (size_t)(new_size * UVHTTP_ROUTER_HASH_LOAD_FACTOR);

    return UVHTTP_OK;
}

/* Find slot in hash table (open addressing with linear probing) */
static uint32_t find_slot(hash_table_t* table, const char* path,
                          uvhttp_method_t method) {
    uint32_t hash = route_hash(path);
    uint32_t index = hash % table->size;
    uint32_t start_index = index;

    while (1) {
        hash_entry_t* entry = &table->entries[index];

        if (entry->path[0] == '\0') {
            return index;
        }

        if (strcmp(entry->path, path) == 0 && entry->method == method) {
            return index;
        }

        index = (index + 1) % table->size;

        /* Prevent infinite loop */
        if (index == start_index) {
            return UINT32_MAX;
        }
    }
}

/* Add to hash table */
static uvhttp_error_t add_to_hash_table(cache_optimized_router_t* cr,
                                        const char* path,
                                        uvhttp_method_t method,
                                        uvhttp_request_handler_t handler) {
    if (!cr || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (strlen(path) >= UVHTTP_MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_HEADER_TOO_LARGE;
    }

    hash_table_t* table = &cr->hash_table;

    /* Check if resize is needed */
    if (table->count >= table->threshold) {
        size_t new_size = table->size * 2;
        if (new_size > UVHTTP_ROUTER_HASH_MAX_SIZE) {
            new_size = UVHTTP_ROUTER_HASH_MAX_SIZE;
        }
        if (new_size > table->size) {
            uvhttp_error_t err = hash_table_resize(table, new_size);
            if (err != UVHTTP_OK) {
                return err;
            }
        }
    }

    /* Find slot */
    uint32_t slot = find_slot(table, path, method);
    hash_entry_t* entry = &table->entries[slot];

    /* Check if already exists */
    if (entry->path[0] != '\0') {
        if (strcmp(entry->path, path) == 0 && entry->method == method) {
            return UVHTTP_ERROR_ALREADY_EXISTS;
        }
    }

    /* Insert new entry */
    uvhttp_safe_strncpy(entry->path, path, UVHTTP_MAX_ROUTE_PATH_LEN);
    entry->method = method;
    entry->handler = handler;
    entry->access_count = 0;
    entry->distance = 0;

    table->count++;
    cr->total_routes++;

    return UVHTTP_OK;
}

/* Find in hash table */
static uvhttp_request_handler_t find_in_hash_table(cache_optimized_router_t* cr,
                                                   const char* path,
                                                   uvhttp_method_t method) {
    if (!cr || !path) {
        return NULL;
    }

    hash_table_t* table = &cr->hash_table;
    uint32_t hash = route_hash(path);
    uint32_t index = hash % table->size;
    uint32_t start_index = index;

    while (1) {
        hash_entry_t* entry = &table->entries[index];

        if (entry->path[0] == '\0') {
            return NULL;
        }

        if (strcmp(entry->path, path) == 0 && entry->method == method) {
            if (entry->access_count < UVHTTP_ACCESS_COUNTER_MAX) {
                entry->access_count++;
            }
            return entry->handler;
        }

        index = (index + 1) % table->size;

        /* Prevent infinite loop */
        if (index == start_index) {
            return NULL;
        }
    }
}

/* ========== Public API Functions ========== */

uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router) {
    if (!router) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr =
        uvhttp_calloc(1, sizeof(cache_optimized_router_t));
    if (!cr) {
        *router = NULL;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(cr, 0, sizeof(cache_optimized_router_t));

    uvhttp_error_t err;

    /* Initialize hash table */
    err = hash_table_init(&cr->hash_table, UVHTTP_ROUTER_HASH_BASE_SIZE);
    if (err != UVHTTP_OK) {
        uvhttp_free(cr);
        *router = NULL;
        return err;
    }

    *router = (uvhttp_router_t*)cr;
    return UVHTTP_OK;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (!router) {
        return;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* Free hash table */
    if (cr->hash_table.entries) {
        uvhttp_free(cr->hash_table.entries);
        cr->hash_table.entries = NULL;
    }

    uvhttp_free(cr);
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    return uvhttp_router_add_route_method(router, path, UVHTTP_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(
    uvhttp_router_t* router, const char* path, uvhttp_method_t method,
    uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* Add to hash table */
    uvhttp_error_t err = add_to_hash_table(cr, path, method, handler);
    if (err != UVHTTP_OK) {
        return err;
    }

    return UVHTTP_OK;
}

uvhttp_request_handler_t uvhttp_router_find_handler(
    const uvhttp_router_t* router, const char* path, const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);

    /* Search in hash table only */
    return find_in_hash_table(cr, path, method_enum);
}

uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router,
                                   const char* path, const char* method,
                                   uvhttp_route_match_t* match) {
    if (!router || !path || !method || !match) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);

    /* Search in hash table only */
    uvhttp_request_handler_t handler = find_in_hash_table(cr, path, method_enum);

    if (!handler) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    match->handler = handler;
    match->param_count = 0;

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                        uvhttp_param_t* params,
                                        size_t* param_count) {
    if (!path || !params || !param_count) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *param_count = 0;
    return UVHTTP_OK;
}

uvhttp_method_t uvhttp_method_from_string(const char* method) {
    return fast_method_parse(method);
}

const char* uvhttp_method_to_string(uvhttp_method_t method) {
    switch (method) {
    case UVHTTP_GET:
        return "GET";
    case UVHTTP_POST:
        return "POST";
    case UVHTTP_PUT:
        return "PUT";
    case UVHTTP_DELETE:
        return "DELETE";
    case UVHTTP_HEAD:
        return "HEAD";
    case UVHTTP_OPTIONS:
        return "OPTIONS";
    case UVHTTP_PATCH:
        return "PATCH";
    case UVHTTP_TRACE:
        return "TRACE";
    case UVHTTP_CONNECT:
        return "CONNECT";
    default:
        return "UNKNOWN";
    }
}

#endif /* UVHTTP_FEATURE_ROUTER_CACHE */