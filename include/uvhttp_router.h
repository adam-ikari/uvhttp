#ifndef UVHTTP_ROUTER_H
#define UVHTTP_ROUTER_H

#include "uvhttp.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROUTES 64
#define MAX_ROUTE_PATH_LEN 256

typedef struct {
    char path[MAX_ROUTE_PATH_LEN];
    uvhttp_request_handler_t handler;
} uvhttp_route_t;

struct uvhttp_router {
    uvhttp_route_t routes[MAX_ROUTES];
    size_t route_count;
};

#ifdef __cplusplus
}
#endif

#endif