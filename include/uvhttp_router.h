#ifndef UVHTTP_ROUTER_H
#define UVHTTP_ROUTER_H

#include "uvhttp_common.h"
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_platform.h"
#include "uvhttp_request.h"

#include <assert.h>
#include <stddef.h>

// Forward declarations
typedef struct uvhttp_response uvhttp_response_t;

// HTTP method enumeration - use definitions in uvhttp_request.h

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROUTES 128
#define MAX_ROUTE_PATH_LEN 256
#define MAX_PARAMS 16
#define MAX_PARAM_NAME_LEN 64
#define MAX_PARAM_VALUE_LEN 256

// Route parameters
typedef struct {
    char name[MAX_PARAM_NAME_LEN];
    char value[MAX_PARAM_VALUE_LEN];
} uvhttp_param_t;

// Route match result
typedef struct {
    uvhttp_request_handler_t handler;
    uvhttp_param_t params[MAX_PARAMS];
    size_t param_count;
} uvhttp_route_match_t;

// Route node - optimized for CPU cache (128 bytes = 2 cache lines)
typedef struct uvhttp_route_node {
    /* Cache line 1: Hot path fields (64 bytes) */
    uvhttp_method_t method;           /* 4 bytes - HTTP method */
    uvhttp_request_handler_t handler; /* 8 bytes - Request handler */
    size_t child_count;               /* 8 bytes - Number of children */
    int is_param;                     /* 4 bytes - Is parameter node */
    uint8_t segment_len;              /* 1 byte - Segment length */
    uint8_t param_name_len;           /* 1 byte - Parameter name length */
    uint16_t _padding1;               /* 2 bytes - Padding to 32 bytes */
    uint32_t child_indices[12];       /* 48 bytes - Compact child storage */

    /* Cache line 2: Variable length data (64 bytes) */
    char segment_data[32];    /* 32 bytes - Path segment */
    char param_name_data[32]; /* 32 bytes - Parameter name */
} uvhttp_route_node_t;

// Array routing structure
typedef struct {
    char path[MAX_ROUTE_PATH_LEN];
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
} array_route_t;

// Router structure
struct uvhttp_router {
    /* Hot path fields (frequently accessed) - optimize memory locality */
    int use_trie;       /* 4 bytes - whether to use Trie */
    size_t route_count; /* 8 bytes - total route count */

    /* Trie routing related (8-byte aligned) - compact node pool */
    uvhttp_route_node_t* node_pool; /* 8 bytes - Compact node pool */
    uint32_t root_index;            /* 4 bytes - Root node index */
    uint32_t node_pool_size;        /* 4 bytes - Pool capacity */
    uint32_t node_pool_used;        /* 4 bytes - Pool usage */
    uint32_t _padding1;             /* 4 bytes - Padding */

    /* Array routing related (8-byte aligned) */
    array_route_t* array_routes; /* 8 bytes */
    size_t array_route_count;    /* 8 bytes */
    size_t array_capacity;       /* 8 bytes */

    /* Static file routing support (8-byte aligned) */
    char* static_prefix;                     /* 8 bytes */
    void* static_context;                    /* 8 bytes */
    uvhttp_request_handler_t static_handler; /* 8 bytes */

    /* Fallback routing support (8-byte aligned) */
    void* fallback_context;                    /* 8 bytes */
    uvhttp_request_handler_t fallback_handler; /* 8 bytes */
};

typedef struct uvhttp_router uvhttp_router_t;

/* ========== Memory Layout Verification Static Assertions ========== */

/* Verify pointer alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t, node_pool, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t, array_routes, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t, static_prefix,
                       UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t, static_context,
                       UVHTTP_POINTER_ALIGNMENT);

/* Verify size_t alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_router_t, route_count, UVHTTP_SIZE_T_ALIGNMENT);

/* Routing API functions */
/**
 * @brief Create new Router
 * @param router Output parameter, used to receive Router pointer
 * @return UVHTTP_OK on success, other values indicate failure
 * @note On success, *router is set to a valid Router object, must be freed
 * using uvhttp_router_free
 * @note On failure, *router is set to NULL
 */
uvhttp_error_t uvhttp_router_new(uvhttp_router_t** router);
void uvhttp_router_free(uvhttp_router_t* router);

/* Route addition (supports HTTP methods) */
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
uvhttp_error_t uvhttp_router_add_route_method(uvhttp_router_t* router,
                                              const char* path,
                                              uvhttp_method_t method,
                                              uvhttp_request_handler_t handler);

/* Route lookup */
uvhttp_request_handler_t uvhttp_router_find_handler(
    const uvhttp_router_t* router, const char* path, const char* method);

/* Route matching (get parameters) */
uvhttp_error_t uvhttp_router_match(const uvhttp_router_t* router,
                                   const char* path, const char* method,
                                   uvhttp_route_match_t* match);

/* Parameter parsing */
uvhttp_error_t uvhttp_parse_path_params(const char* path,
                                        uvhttp_param_t* params,
                                        size_t* param_count);

/* Method string conversion */
uvhttp_method_t uvhttp_method_from_string(const char* method);
const char* uvhttp_method_to_string(uvhttp_method_t method);

/* Static file routing support */
uvhttp_error_t uvhttp_router_add_static_route(uvhttp_router_t* router,
                                              const char* prefix_path,
                                              void* static_context);

/* Fallback routing support */
uvhttp_error_t uvhttp_router_add_fallback_route(uvhttp_router_t* router,
                                                void* static_context);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ROUTER_H */