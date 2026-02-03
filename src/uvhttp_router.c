#if !UVHTTP_FEATURE_ROUTER_CACHE
#    include "uvhttp_router.h"

#    include "uvhttp_allocator.h"
#    include "uvhttp_connection.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_server.h"
#    include "uvhttp_static.h"
#    include "uvhttp_utils.h"

#    include <ctype.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>

/**
 * Router system hybrid mode threshold
 *
 * When route count reaches this threshold, automatically switch from array mode
 * to Trie mode for better performance.
 *
 * Selection criteria:
 * - Array mode: O(n) lookup, suitable for few routes (low memory, fast lookup)
 * - Trie mode: O(m) lookup (m is path depth), suitable for many routes (stable
 * lookup)
 * - Threshold 100: Based on performance testing, at this count both modes have
 *                  similar performance, but Trie mode remains stable as routes
 * increase
 *
 * Performance test results (1000 lookups):
 * - 50 routes: array 0.02ms, Trie 0.03ms
 * - 100 routes: array 0.04ms, Trie 0.04ms
 * - 200 routes: array 0.08ms, Trie 0.05ms
 * - 500 routes: array 0.20ms, Trie 0.06ms
 *
 * Note: If you need to adjust this value, please re-run performance tests to
 * verify
 */
#    define HYBRID_THRESHOLD 100 /* Route count threshold to switch to Trie */

/* Optimization: fast method parsing (using prefix matching) */
uvhttp_method_t uvhttp_method_from_string(const char* method) {
    if (!method || !method[0]) {
        return UVHTTP_ANY;
    }

    /* fast prefix match */
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
        return UVHTTP_ANY;
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
    default:
        return UVHTTP_ANY;
    }
}

/* optimization: method to string conversion (using direct index) */
const char* uvhttp_method_to_string(uvhttp_method_t method) {
    /* use static constant array, avoid repeated string comparison */
    static const char* method_strings[] = {
        [UVHTTP_GET] = UVHTTP_METHOD_GET,
        [UVHTTP_POST] = UVHTTP_METHOD_POST,
        [UVHTTP_PUT] = UVHTTP_METHOD_PUT,
        [UVHTTP_DELETE] = UVHTTP_METHOD_DELETE,
        [UVHTTP_HEAD] = UVHTTP_METHOD_HEAD,
        [UVHTTP_OPTIONS] = UVHTTP_METHOD_OPTIONS,
        [UVHTTP_PATCH] = UVHTTP_METHOD_PATCH,
        [UVHTTP_ANY] = "ANY"};

    if (method >= 0 &&
        method < (int)(sizeof(method_strings) / sizeof(method_strings[0]))) {
        return method_strings[method];
    }
    return "UNKNOWN";
}

// create new router node - returns index into node pool
static uint32_t create_route_node(uvhttp_router_t* router) {
    if (router->node_pool_used >= router->node_pool_size) {
        // expand node pool
        uint32_t new_size = router->node_pool_size * 2;
        uvhttp_route_node_t* new_pool = uvhttp_realloc(
            router->node_pool, new_size * sizeof(uvhttp_route_node_t));
        if (!new_pool) {
            return UINT32_MAX;
        }

        // initialize newly added nodes
        for (uint32_t i = router->node_pool_size; i < new_size; i++) {
            memset(&new_pool[i], 0, sizeof(uvhttp_route_node_t));
        }

        router->node_pool = new_pool;
        router->node_pool_size = new_size;
    }

    return router->node_pool_used++;
}

// find or create child node - returns index into node pool
static uint32_t find_or_create_child(uvhttp_router_t* router,
                                     uint32_t parent_index,
                                     const char* segment,
                                     int is_param) {
    uvhttp_route_node_t* parent = &router->node_pool[parent_index];

    // find existing child node
    for (size_t i = 0; i < parent->child_count; i++) {
        uint32_t child_index = parent->child_indices[i];
        uvhttp_route_node_t* child = &router->node_pool[child_index];
        if (strncmp(child->segment_data, segment, child->segment_len) == 0 &&
            strlen(segment) == child->segment_len) {
            return child_index;
        }
    }

    // create new child node
    if (parent->child_count >= 12) {
        return UINT32_MAX;
    }

    uint32_t child_index = create_route_node(router);
    if (child_index == UINT32_MAX) {
        return UINT32_MAX;
    }

    uvhttp_route_node_t* child = &router->node_pool[child_index];
    size_t seg_len = strlen(segment);
    child->segment_len = (uint8_t)(seg_len < 32 ? seg_len : 31);
    memcpy(child->segment_data, segment, child->segment_len);
    child->is_param = is_param;

    parent->child_indices[parent->child_count++] = child_index;
    return child_index;
}

// parsepathparameter
static int parse_path_params(const char* path, uvhttp_param_t* params,
                             size_t* param_count) {
    if (!path || !params || !param_count) {
        return -1;
    }

    *param_count = 0;
    char path_copy[MAX_ROUTE_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    char* token = strtok(path_copy, "/");
    while (token && *param_count < MAX_PARAMS) {
        // check if parameter (starts with :)
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

    return 0;
}

uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router) {
    if (!router) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *router = NULL;

    uvhttp_router_t* r = uvhttp_alloc(sizeof(uvhttp_router_t));

    if (!r) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(r, 0, sizeof(uvhttp_router_t));

    // initializearrayrouter
    r->array_routes = uvhttp_calloc(HYBRID_THRESHOLD, sizeof(array_route_t));

    if (!r->array_routes) {
        uvhttp_free(r);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    r->array_capacity = HYBRID_THRESHOLD;

    // initialize node pool (for Trie)
    r->node_pool_size = 64;
    r->node_pool_used = 0;
    r->node_pool =
        uvhttp_calloc(r->node_pool_size, sizeof(uvhttp_route_node_t));

    if (!r->node_pool) {
        uvhttp_free(r->array_routes);
        uvhttp_free(r);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // create root node
    r->root_index = create_route_node(r);
    if (r->root_index == UINT32_MAX) {
        uvhttp_free(r->node_pool);
        uvhttp_free(r->array_routes);
        uvhttp_free(r);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    r->use_trie = 0; /* default use array router */

    *router = r;
    return UVHTTP_OK;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    if (router) {
        if (router->node_pool) {
            uvhttp_free(router->node_pool);
            router->node_pool = NULL;
        }
        if (router->array_routes) {
            uvhttp_free(router->array_routes);
            router->array_routes = NULL;
        }
        if (router->static_prefix) {
            uvhttp_free(router->static_prefix);
            router->static_prefix = NULL;
        }
        uvhttp_free(router);
    }
}

// arrayrouteradd
static uvhttp_error_t add_array_route(uvhttp_router_t* router, const char* path,
                                      uvhttp_method_t method,
                                      uvhttp_request_handler_t handler) {
    if (router->array_route_count >= router->array_capacity) {
        // expand array capacity
        size_t new_capacity = router->array_capacity * 2;
        array_route_t* new_routes = uvhttp_realloc(
            router->array_routes, new_capacity * sizeof(array_route_t));
        if (!new_routes) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        router->array_routes = new_routes;
        router->array_capacity = new_capacity;
    }

    array_route_t* route = &router->array_routes[router->array_route_count];
    strncpy(route->path, path, sizeof(route->path) - 1);
    route->path[sizeof(route->path) - 1] = '\0';
    route->method = method;
    route->handler = handler;
    router->array_route_count++;
    router->route_count++;

    return UVHTTP_OK;
}

// arrayrouterfind
static uvhttp_request_handler_t find_array_route(const uvhttp_router_t* router,
                                                 const char* path,
                                                 uvhttp_method_t method) {
    for (size_t i = 0; i < router->array_route_count; i++) {
        array_route_t* route = &router->array_routes[i];
        if (route->method == method || route->method == UVHTTP_ANY) {
            if (strcmp(route->path, path) == 0) {
                return route->handler;
            }
        }
    }
    return NULL;
}

// migrate array router to Trie
static uvhttp_error_t migrate_to_trie(uvhttp_router_t* router) {
    if (router->use_trie) {
        return UVHTTP_OK;  // already Trie pattern
    }

    // save array router pointer, release later
    array_route_t* old_routes = router->array_routes;
    size_t old_count = router->array_route_count;

    // migrate all array routers to Trie
    for (size_t i = 0; i < old_count; i++) {
        array_route_t* route = &old_routes[i];

        // build router tree
        uint32_t current_index = router->root_index;
        char path_copy[MAX_ROUTE_PATH_LEN];
        strncpy(path_copy, route->path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';

        char* token = strtok(path_copy, "/");
        while (token) {
            int is_param = (token[0] == ':');
            if (is_param) {
                token++;
            }

            current_index = find_or_create_child(router, current_index, token, is_param);
            if (current_index == UINT32_MAX) {
                uvhttp_free(old_routes);  // clean allocated memory
                return UVHTTP_ERROR_OUT_OF_MEMORY;
            }

            token = strtok(NULL, "/");
        }

        // sethandler
        uvhttp_route_node_t* current = &router->node_pool[current_index];
        current->method = route->method;
        current->handler = route->handler;
    }

    // switch to Trie pattern
    router->use_trie = 1;
    router->array_routes = NULL;
    router->array_route_count = 0;
    router->array_capacity = 0;

    // release old array router memory
    uvhttp_free(old_routes);

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler) {
    return uvhttp_router_add_route_method(router, path, UVHTTP_ANY, handler);
}

uvhttp_error_t uvhttp_router_add_route_method(
    uvhttp_router_t* router, const char* path, uvhttp_method_t method,
    uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (strlen(path) == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (strlen(path) >= MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // check if path contains query string (not allowed)
    if (strchr(path, '?') != NULL) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // check if contains path parameter
    int has_params = (strchr(path, ':') != NULL);

    // if has parameter or router count exceeds threshold, use Trie
    if (has_params || router->array_route_count >= HYBRID_THRESHOLD) {
        if (!router->use_trie) {
            uvhttp_error_t err = migrate_to_trie(router);
            if (err != UVHTTP_OK) {
                return err;
            }
        }

        // add to Trie
        uint32_t current_index = router->root_index;
        char path_copy[MAX_ROUTE_PATH_LEN];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';

        char* token = strtok(path_copy, "/");
        while (token) {
            int is_param = (token[0] == ':');
            if (is_param) {
                token++;
                // save parameter name - optimization: only calculate strlen
                // once
                size_t token_len = strlen(token);
                for (size_t i = 0; i < token_len; i++) {
                    if (token[i] == '/') {
                        uvhttp_route_node_t* current = &router->node_pool[current_index];
                        size_t name_len = i < 32 ? i : 31;
                        memcpy(current->param_name_data, token, name_len);
                        current->param_name_data[name_len] = '\0';
                        current->param_name_len = (uint8_t)name_len;
                        break;
                    }
                }
            }

            current_index = find_or_create_child(router, current_index, token, is_param);
            if (current_index == UINT32_MAX) {
                return UVHTTP_ERROR_OUT_OF_MEMORY;
            }

            token = strtok(NULL, "/");
        }

        // sethandler
        uvhttp_route_node_t* current = &router->node_pool[current_index];
        current->method = method;
        current->handler = handler;
        router->route_count++;
    } else {
        // add to array
        return add_array_route(router, path, method, handler);
    }

    return UVHTTP_OK;
}

// match route node recursively - using indices for cache optimization
static int match_route_node(const uvhttp_router_t* router, uint32_t node_index,
                            const char** segments, size_t segment_count,
                            size_t segment_index, uvhttp_method_t method,
                            uvhttp_route_match_t* match) {
    if (node_index == UINT32_MAX || !router || !segments || !match) {
        return -1;
    }

    const uvhttp_route_node_t* node = &router->node_pool[node_index];

    // if is leaf node
    if (segment_index >= segment_count) {
        if (node->handler &&
            (node->method == UVHTTP_ANY || node->method == method)) {
            match->handler = node->handler;
            return 0;
        }
        return -1;
    }

    const char* segment = segments[segment_index];

    // find matching child node - use indices for better cache locality
    for (size_t i = 0; i < node->child_count; i++) {
        uint32_t child_index = node->child_indices[i];
        const uvhttp_route_node_t* child = &router->node_pool[child_index];

        if (child->is_param) {
            // parameter node, match any segment
            size_t name_len = child->param_name_len;
            size_t name_copy_len =
                name_len < sizeof(match->params[match->param_count].name) - 1
                    ? name_len
                    : sizeof(match->params[match->param_count].name) - 1;
            memcpy(match->params[match->param_count].name, child->param_name_data,
                   name_copy_len);
            match->params[match->param_count].name[name_copy_len] = '\0';

            size_t value_len = strlen(segment);
            size_t value_copy_len =
                value_len < sizeof(match->params[match->param_count].value) - 1
                    ? value_len
                    : sizeof(match->params[match->param_count].value) - 1;
            memcpy(match->params[match->param_count].value, segment,
                   value_copy_len);
            match->params[match->param_count].value[value_copy_len] = '\0';

            match->param_count++;

            int result = match_route_node(router, child_index, segments,
                                          segment_count, segment_index + 1,
                                          method, match);
            if (result == 0) {
                return 0;
            }
            match->param_count--;  // backtrack
        } else {
            // exact match - use segment_len for faster comparison
            size_t seg_len = strlen(segment);
            if (seg_len == child->segment_len &&
                strncmp(child->segment_data, segment, seg_len) == 0) {
                int result = match_route_node(router, child_index, segments,
                                              segment_count, segment_index + 1,
                                              method, match);
                if (result == 0) {
                    return 0;
                }
            }
        }
    }

    return -1;
}

/* static file request handler wrapper function */
static int static_file_handler_wrapper(uvhttp_request_t* request,
                                       uvhttp_response_t* response) {
    /* getconnection */
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

    /* get router */
    uvhttp_router_t* router = conn->server->router;

    /* call static file processing function */
    if (router->static_context) {
        uvhttp_result_t result = uvhttp_static_handle_request(
            (uvhttp_static_context_t*)router->static_context, request,
            response);

        if (result == UVHTTP_OK) {
            return 0;
        }
    }

    /* static file service failed, return 404 */
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Not Found", 9);
    uvhttp_response_send(response);
    return -1;
}

uvhttp_request_handler_t uvhttp_router_find_handler(
    const uvhttp_router_t* router, const char* path, const char* method) {
    if (!router || !path || !method) {
        return NULL;
    }

    uvhttp_method_t method_enum = uvhttp_method_from_string(method);

    // choose find method based on current pattern
    if (router->use_trie) {
        // Triefind
        uvhttp_route_match_t match;
        memset(&match, 0, sizeof(match));

        // parse path segments
        char path_copy[MAX_ROUTE_PATH_LEN];
        strncpy(path_copy, path, sizeof(path_copy) - 1);
        path_copy[sizeof(path_copy) - 1] = '\0';

        const char* segments[MAX_ROUTE_PATH_LEN];
        int segment_count = 0;

        char* token = strtok(path_copy, "/");
        while (token && segment_count < MAX_ROUTE_PATH_LEN) {
            segments[segment_count++] = token;
            token = strtok(NULL, "/");
        }

        // first check static router
        if (router->static_prefix && router->static_context) {
            size_t prefix_len = strlen(router->static_prefix);
            if (strncmp(path, router->static_prefix, prefix_len) == 0) {
                // match static router, return static file handler
                return static_file_handler_wrapper;
            }
        }

        // matchrouter - use root index
        if (match_route_node(router, router->root_index, segments,
                             segment_count, 0, method_enum, &match) == 0) {
            return match.handler;
        }
    } else {
        // arrayfind
        uvhttp_request_handler_t handler =
            find_array_route(router, path, method_enum);
        if (handler) {
            return handler;
        }
    }

    // if no matching router, check fallback router
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

    memset(match, 0, sizeof(uvhttp_route_match_t));

    uvhttp_method_t method_enum = uvhttp_method_from_string(method);

    /* optimization 1: fast path - check array router (suitable for few routers)
     */
    if (!router->use_trie) {
        uvhttp_request_handler_t handler =
            find_array_route(router, path, method_enum);
        if (handler) {
            match->handler = handler;
            return UVHTTP_OK;
        }
        return UVHTTP_ERROR_NOT_FOUND;
    }

    /* optimization 2: fast path - check static router (no parameters) */
    /* for paths without parameters, use fast find */
    int has_params = 0;
    for (const char* p = path; *p; p++) {
        if (*p == ':' || *p == '{') {
            has_params = 1;
            break;
        }
    }

    if (!has_params && router->array_routes && router->array_route_count > 0) {
        /* no parameter path, use array router fast find */
        /* but need to check if array_routes is still valid */
        uvhttp_request_handler_t handler =
            find_array_route(router, path, method_enum);
        if (handler) {
            match->handler = handler;
            return UVHTTP_OK;
        }
    }

    /* optimization 3: Trie tree match (supports parameters) */
    /* parse path segments */
    char path_copy[MAX_ROUTE_PATH_LEN];
    strncpy(path_copy, path, sizeof(path_copy) - 1);
    path_copy[sizeof(path_copy) - 1] = '\0';

    const char* segments[MAX_ROUTE_PATH_LEN];
    int segment_count = 0;

    char* token = strtok(path_copy, "/");
    while (token && segment_count < MAX_ROUTE_PATH_LEN) {
        segments[segment_count++] = token;
        token = strtok(NULL, "/");
    }

    return match_route_node(router, router->root_index, segments, segment_count, 0,
                            method_enum, match) == 0
               ? UVHTTP_OK
               : UVHTTP_ERROR_NOT_FOUND;
}

uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                        uvhttp_param_t* params,
                                        size_t* param_count) {
    return parse_path_params(path, params, param_count);
}

/**
 * add static file router
 */
uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t* router,
                                              const char* prefix_path,
                                              void* static_context) {
    if (!router || !prefix_path || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // release previous prefix
    if (router->static_prefix) {
        uvhttp_free(router->static_prefix);
    }

    // copy new prefix (use uvhttp_alloc to avoid mixing allocators)
    size_t prefix_len = strlen(prefix_path);
    router->static_prefix = (char*)uvhttp_alloc(prefix_len + 1);
    if (!router->static_prefix) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(router->static_prefix, prefix_path, prefix_len + 1);

    router->static_context = static_context;
    router->static_handler = NULL;  // will use static file processing logic

    return UVHTTP_OK;
}

/**
 * add fallback router
 */
uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t* router,
                                                void* static_context) {
    if (!router || !static_context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    router->fallback_context = static_context;
    router->fallback_handler = NULL;  // will use static file processing logic

    return UVHTTP_OK;
}

#endif /* !UVHTTP_FEATURE_ROUTER_CACHE */