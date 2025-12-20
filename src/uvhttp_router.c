#include "uvhttp_router.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uvhttp_router_t* uvhttp_router_new(void) {
    uvhttp_router_t* router = uvhttp_malloc(sizeof(uvhttp_router_t));
    memset(router, 0, sizeof(uvhttp_router_t));
    return router;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    uvhttp_free(router);
}

int uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler) {
    if (!router || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (router->route_count >= MAX_ROUTES) {
        return UVHTTP_ERROR_ALREADY_EXISTS;
    }
    
    if (strlen(path) >= MAX_ROUTE_PATH_LEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    uvhttp_route_t* route = &router->routes[router->route_count];
    if (uvhttp_safe_strcpy(route->path, sizeof(route->path), path) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    route->handler = handler;
    
    router->route_count++;
    return UVHTTP_OK;
}

uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path) {
    if (!router || !path) {
        return NULL;
    }
    
    for (size_t i = 0; i < router->route_count; i++) {
        if (strcmp(router->routes[i].path, path) == 0) {
            return router->routes[i].handler;
        }
    }
    return NULL;
}