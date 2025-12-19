#include "uvhttp_router_simple.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

uvhttp_router_t* uvhttp_router_new(void) {
    uvhttp_router_t* router = malloc(sizeof(uvhttp_router_t));
    memset(router, 0, sizeof(uvhttp_router_t));
    return router;
}

void uvhttp_router_free(uvhttp_router_t* router) {
    free(router);
}

int uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler) {
    if (router->route_count >= MAX_ROUTES) {
        fprintf(stderr, "Maximum number of routes reached\n");
        return -1;
    }
    
    uvhttp_route_t* route = &router->routes[router->route_count];
    strncpy(route->path, path, MAX_ROUTE_PATH_LEN - 1);
    route->path[MAX_ROUTE_PATH_LEN - 1] = '\0';
    route->handler = handler;
    
    router->route_count++;
    return 0;
}

uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path) {
    for (size_t i = 0; i < router->route_count; i++) {
        if (strcmp(router->routes[i].path, path) == 0) {
            return router->routes[i].handler;
        }
    }
    return NULL;
}