#ifndef UVHTTP_ROUTER_SIMPLE_H
#define UVHTTP_ROUTER_SIMPLE_H

#include <stdlib.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_ROUTES 64
#define MAX_ROUTE_PATH_LEN 256

// 前向声明
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 请求处理函数类型
typedef void (*uvhttp_request_handler_t)(uvhttp_request_t* request, uvhttp_response_t* response);

typedef struct {
    char path[MAX_ROUTE_PATH_LEN];
    uvhttp_request_handler_t handler;
} uvhttp_route_t;

struct uvhttp_router {
    uvhttp_route_t routes[MAX_ROUTES];
    size_t route_count;
};

typedef struct uvhttp_router uvhttp_router_t;

// 路由函数
uvhttp_router_t* uvhttp_router_new();
void uvhttp_router_free(uvhttp_router_t* router);
int uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler);
uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path);

#ifdef __cplusplus
}
#endif

#endif