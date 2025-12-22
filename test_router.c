#include "../include/uvhttp.h"
#include <stdio.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    printf("Handler called!\n");
    if (!request || !response) {
        return -1;
    }
    
    printf("Path: %s\n", uvhttp_request_get_path(request));
    printf("Method: %s\n", uvhttp_request_get_method(request));
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    
    uvhttp_response_send(response);
    return 0;
}

int main() {
    printf("Router test starting...\n");
    
    // 测试路由器
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        printf("Failed to create router\n");
        return 1;
    }
    
    printf("Router created\n");
    
    if (uvhttp_router_add_route(router, "/", hello_handler) != UVHTTP_OK) {
        printf("Failed to add route\n");
        return 1;
    }
    
    printf("Route added\n");
    
    // 测试路由查找
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/", "GET");
    if (handler) {
        printf("Handler found for /\n");
    } else {
        printf("Handler NOT found for /\n");
    }
    
    handler = uvhttp_router_find_handler(router, "/notfound", "GET");
    if (handler) {
        printf("Handler found for /notfound\n");
    } else {
        printf("Handler NOT found for /notfound (expected)\n");
    }
    
    uvhttp_router_free(router);
    printf("Router test completed\n");
    
    return 0;
}