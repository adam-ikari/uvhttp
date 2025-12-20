#ifndef UVHTTP_ROUTER_H
#define UVHTTP_ROUTER_H

#include <stddef.h>
#include "uvhttp_error.h"
#include "uvhttp_common.h"

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROUTES 64
#define MAX_ROUTE_PATH_LEN 256

typedef struct uvhttp_router uvhttp_router_t;


typedef struct {
    char path[MAX_ROUTE_PATH_LEN];
    uvhttp_request_handler_t handler;
} uvhttp_route_t;

struct uvhttp_router {
    uvhttp_route_t routes[MAX_ROUTES];
    size_t route_count;
};

// Router API functions
uvhttp_router_t* uvhttp_router_new(void);
void uvhttp_router_free(uvhttp_router_t* router);
uvhttp_error_t uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler);
uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path);

#ifdef __cplusplus
}
#endif

#endif