#if UVHTTP_FEATURE_ROUTER_CACHE

#    include "uvhttp_allocator.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_hash.h"
#    include "uvhttp_router.h"
#    include "uvhttp_utils.h"

#    include "uvhttp_connection.h"
#    include "uvhttp_server.h"
#    include "uvhttp_static.h"

#    include <ctype.h>
#    include <stdint.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

/* Route count threshold to switch to Trie mode — mirrors uvhttp_router.c so
 * the public struct fields (use_trie/array_route_count/array_capacity) stay
 * consistent between the two implementations. */
#    ifndef HYBRID_THRESHOLD
#        define HYBRID_THRESHOLD 100
#    endif

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
 *
 * ABI note: the public uvhttp_router_t is embedded as the FIRST member so
 * the `(uvhttp_router_t*)cr` cast used throughout this module is valid and
 * every uvhttp_router_t field (static_prefix, static_context, fallback_*,
 * node_pool, array_routes, ...) lives inside this allocation. Previously this
 * struct was smaller than uvhttp_router_t, so reads of router->static_context
 * (uvhttp_request.c:494) dereferenced memory past the allocation and crashed.
 */
typedef struct {
    /* Public router struct prefix — all uvhttp_router_t fields are valid
     * (zero-initialized by calloc) even though this module only uses the
     * hash table below. */
    uvhttp_router_t router;

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
        } else if (method[1] == 'A') {
            return (method[2] == 'T' && method[3] == 'C' && method[4] == 'H' &&
                    method[5] == '\0')
                       ? UVHTTP_PATCH
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
/* Find handler and optionally return matched route path.
 * route_path_out (may be NULL): if non-NULL and a param route matched,
 *   set to the entry's path (e.g. "/items/:item_id").
 * Returns handler or NULL. */
static uvhttp_request_handler_t find_in_hash_table_ex(
    cache_optimized_router_t* cr,
    const char* path, uvhttp_method_t method,
    const char** route_path_out) {
    if (!cr || !path) {
        return NULL;
    }
    hash_table_t* table = &cr->hash_table;
    uint32_t hash = route_hash(path);
    uint32_t index = hash % table->size;
    uint32_t start_index = index;
    /* First pass: exact match */
    while (1) {
        hash_entry_t* entry = &table->entries[index];
        if (entry->path[0] == '\0') break;
        if (strcmp(entry->path, path) == 0 &&
            (entry->method == method || entry->method == UVHTTP_ANY)) {
            if (entry->access_count < UVHTTP_ACCESS_COUNTER_MAX) entry->access_count++;
            if (route_path_out) *route_path_out = entry->path;
            return entry->handler;
        }
        index = (index + 1) % table->size;
        if (index == start_index) break;
    }
    /* Second pass: parameterized route match */
    for (uint32_t i = 0; i < table->size; i++) {
        hash_entry_t* entry = &table->entries[i];
        if (entry->path[0] == '\0') continue;
        if (entry->method != method && entry->method != UVHTTP_ANY) continue;
        if (!strchr(entry->path, ':')) continue;
        const char* rp = entry->path;
        const char* pp = path;
        int matched = 1;
        while (*rp && *pp) {
            if (*rp == ':') {
                while (*rp && *rp != '/') rp++;
                while (*pp && *pp != '/') pp++;
            } else if (*rp == *pp) {
                rp++; pp++;
            } else { matched = 0; break; }
        }
        if (matched && *rp == '\0' && *pp == '\0') {
            if (entry->access_count < UVHTTP_ACCESS_COUNTER_MAX) entry->access_count++;
            if (route_path_out) *route_path_out = entry->path;
            return entry->handler;
        }
    }
    return NULL;
}

static uvhttp_request_handler_t find_in_hash_table(cache_optimized_router_t* cr,
                                                   const char* path,
                                                   uvhttp_method_t method) {
    return find_in_hash_table_ex(cr, path, method, NULL);
}

/* ========== Public API Functions ========== */

/* static file request handler wrapper — mirrors the one in uvhttp_router.c so
 * the router_cache module can serve static files and fallback routes too. */
static int static_file_handler_wrapper(uvhttp_request_t* request,
                                       uvhttp_response_t* response) {
    /* get connection */
    uv_tcp_t* client = request->client;
    if (!client) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }

    /* get connection from client */
    uvhttp_connection_t* conn =
        (uvhttp_connection_t*)uv_handle_get_data((uv_handle_t*)client);

    if (!conn || !conn->server || !conn->server->router) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }

    uvhttp_router_t* router = conn->server->router;

    /* static file processing — prefer static_context, fall back to
     * fallback_context if the static context is not installed */
    void* ctx = router->static_context ? router->static_context
                                       : router->fallback_context;
    if (ctx) {
#ifdef UVHTTP_STATIC_FILES_ENABLED
        uvhttp_result_t result = uvhttp_static_handle_request(
            (uvhttp_static_context_t*)ctx, request, response);

        if (result == UVHTTP_OK) {
            return 0;
        }
#else
        (void)ctx;
#endif
    }

    /* static file service failed, return 404 */
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Not Found", 9);
    uvhttp_response_send(response);
    return -1;
}

/* static prefix match — returns the wrapper handler when the request path is
 * under the configured static prefix and a static context is installed. */
static uvhttp_request_handler_t static_prefix_handler(const uvhttp_router_t* router,
                                                      const char* path) {
    if (router->static_prefix && router->static_context && path) {
        size_t prefix_len = strlen(router->static_prefix);
        if (strncmp(path, router->static_prefix, prefix_len) == 0) {
            return static_file_handler_wrapper;
        }
    }
    return NULL;
}

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

    /* Free static prefix (owned by the router, see add_static_route) */
    if (router->static_prefix) {
        uvhttp_free(router->static_prefix);
        router->static_prefix = NULL;
    }

    uvhttp_free(cr);
}

uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t* router,
                                              const char* prefix_path,
                                              void* static_context) {
    if (!router || !prefix_path || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* release previous prefix */
    if (router->static_prefix) {
        uvhttp_free(router->static_prefix);
    }

    /* copy new prefix (use uvhttp_alloc to avoid mixing allocators) */
    size_t prefix_len = strlen(prefix_path);
    router->static_prefix = (char*)uvhttp_alloc(prefix_len + 1);
    if (!router->static_prefix) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(router->static_prefix, prefix_path, prefix_len + 1);

    router->static_context = static_context;
    router->static_handler = NULL; /* use static file processing logic */

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t* router,
                                                void* static_context) {
    if (!router || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    router->fallback_context = static_context;
    router->fallback_handler = NULL; /* use static file processing logic */

    return UVHTTP_OK;
}

/* binary data route — mirrors uvhttp_router.c. The mime type is stored in
 * router->static_context and the blob in static_data/static_data_len (one
 * binary route per router; use regular handlers for more). */
static int binary_route_handler(uvhttp_request_t* request,
                                uvhttp_response_t* response) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)request->client->data;
    if (!conn || !conn->server || !conn->server->router) {
        return -1;
    }
    uvhttp_router_t* r = conn->server->router;
    if (!r->static_context) return -1;

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type",
                               r->static_context);
    uvhttp_response_set_body(response, r->static_data, r->static_data_len);
    return uvhttp_response_send(response);
}

uvhttp_error_t uvhttp_router_add_binary_route(uvhttp_router_t* router,
                                              const char* path,
                                              const char* mime_type,
                                              const void* data,
                                              size_t data_len) {
    if (!router || !path || !mime_type || !data || data_len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    router->static_context = (void*)mime_type;
    router->static_data = data;
    router->static_data_len = data_len;

    return uvhttp_router_add_route(router, path, binary_route_handler);
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler) {
    return uvhttp_router_add_route_method(router, path, UVHTTP_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(
    uvhttp_router_t* router, const char* path, uvhttp_method_t method,
    uvhttp_request_handler_t handler) {
    /* Validation mirrors uvhttp_router.c: reject null args, empty paths,
     * over-long paths, and query-string paths. */
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (strlen(path) == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (strlen(path) >= MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (strchr(path, '?') != NULL) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;

    /* Add to hash table */
    uvhttp_error_t err = add_to_hash_table(cr, path, method, handler);
    if (err != UVHTTP_OK) {
        return err;
    }

    /* Keep public struct fields in sync with the non-cache router. */
    router->route_count++;
    int has_params = (strchr(path, ':') != NULL);

    if (has_params || router->array_route_count >= HYBRID_THRESHOLD ||
        router->use_trie) {
        /* Trie mode (parameterized route or threshold exceeded) */
        if (!router->use_trie) {
            router->use_trie = 1;
            router->array_route_count = 0;
        }
    } else {
        /* Array mode: track count; double capacity when full */
        if (router->array_route_count >= router->array_capacity) {
            router->array_capacity = (router->array_capacity == 0)
                                         ? HYBRID_THRESHOLD
                                         : router->array_capacity * 2;
        }
        router->array_route_count++;
    }

    return UVHTTP_OK;
}

uvhttp_request_handler_t uvhttp_router_find_handler(
    const uvhttp_router_t* router, const char* path, const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }

    /* static prefix match first (mirrors uvhttp_router.c array mode) */
    uvhttp_request_handler_t static_h = static_prefix_handler(router, path);
    if (static_h) {
        return static_h;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);

    uvhttp_request_handler_t handler =
        find_in_hash_table(cr, path, method_enum);
    if (handler) {
        return handler;
    }

    /* fallback router (mirrors uvhttp_router.c) */
    if (router->fallback_context) {
        return static_file_handler_wrapper;
    }

    return NULL;
}

uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router,
                                   const char* path, const char* method,
                                   uvhttp_route_match_t* match) {
    if (!router || !path || !method || !match) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    cache_optimized_router_t* cr = (cache_optimized_router_t*)router;
    uvhttp_method_t method_enum = fast_method_parse(method);

    /* Static prefix is only checked in array mode (mirrors uvhttp_router.c:
     * uvhttp_router_match checks static prefix only when !use_trie; trie mode
     * routes through parameter matching instead). */
    if (!router->use_trie && router->static_prefix && router->static_context &&
        path) {
        size_t prefix_len = strlen(router->static_prefix);
        if (strncmp(path, router->static_prefix, prefix_len) == 0) {
            match->handler = static_file_handler_wrapper;
            match->param_count = 0;
            return UVHTTP_OK;
        }
    }

    /* Search in hash table */
    const char* route_path = NULL;
    uvhttp_request_handler_t handler = find_in_hash_table_ex(cr, path, method_enum, &route_path);

    if (!handler) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    match->handler = handler;
    match->param_count = 0;

    /* Extract parameters by comparing route template with request path.
     * Route: /items/:item_id  Request: /items/abc123
     * Result: param name="item_id", value="abc123" */
    if (route_path && strchr(route_path, ':')) {
        const char* rp = route_path;
        const char* pp = path;
        while (*rp && *pp && match->param_count < MAX_PARAMS) {
            if (*rp == ':') {
                /* Extract param name from route template */
                const char* name_start = rp + 1;
                const char* name_end = name_start;
                while (*name_end && *name_end != '/') name_end++;
                size_t name_len = name_end - name_start;
                /* Extract param value from request path */
                const char* val_end = pp;
                while (*val_end && *val_end != '/') val_end++;
                if (name_len > 0 && name_len < sizeof(match->params[match->param_count].name)) {
                    strncpy(match->params[match->param_count].name, name_start, name_len);
                    match->params[match->param_count].name[name_len] = '\0';
                    size_t val_len = val_end - pp;
                    if (val_len < sizeof(match->params[match->param_count].value)) {
                        strncpy(match->params[match->param_count].value, pp, val_len);
                        match->params[match->param_count].value[val_len] = '\0';
                        match->param_count++;
                    }
                }
                rp = name_end;
                pp = val_end;
            } else if (*rp == *pp) {
                rp++;
                pp++;
            } else {
                break;
            }
        }
    }

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                        uvhttp_param_t* params,
                                        size_t* param_count) {
    if (!path || !params || !param_count) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    *param_count = 0;
    char path_copy[UVHTTP_MAX_ROUTE_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';
    char* token = strtok(path_copy, "/");
    while (token && *param_count < MAX_PARAMS) {
        if (token[0] == ':') {
            char* colon = strchr(token + 1, ':');
            if (colon) {
                *colon = '\0';
                strncpy(params[*param_count].name, token + 1,
                        sizeof(params[*param_count].name) - 1);
                strncpy(params[*param_count].value, colon + 1,
                        sizeof(params[*param_count].value) - 1);
                params[*param_count]
                    .name[sizeof(params[*param_count].name) - 1] = '\0';
                params[*param_count]
                    .value[sizeof(params[*param_count].value) - 1] = '\0';
                (*param_count)++;
            }
        }
        token = strtok(NULL, "/");
    }
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
    case UVHTTP_ANY:
        return "ANY";
    default:
        return "UNKNOWN";
    }
}

#endif /* UVHTTP_FEATURE_ROUTER_CACHE */