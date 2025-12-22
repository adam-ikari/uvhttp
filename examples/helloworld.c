#include "../include/uvhttp.h"
#include "../include/uvhttp_allocator.h"
#include <stdio.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    uvhttp_response_send(response);
    
    printf("Request handled: %s %s\n", 
           uvhttp_method_to_string(request->method), 
           request->url);
    
    return 0;
}

int main() {
    printf("Hello World Server starting...\n");
    
    // 获取默认循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to get default loop\n");
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)loop);
    
    // 创建服务器
    printf("Creating server...\n");
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    printf("Server created successfully: %p\n", (void*)server);
    
    // 创建路由器
    printf("Creating router...\n");
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "Failed to create router\n");
        return 1;
    }
    printf("Router created successfully: %p\n", (void*)router);
    
    // 添加路由
    printf("Adding route...\n");
    int route_result = uvhttp_router_add_route(router, "/", hello_handler);
    if (route_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to add route, error: %d\n", route_result);
        return 1;
    }
    printf("Route added successfully\n");
    
    // 设置路由器到服务器
    server->router = router;
    printf("Router set to server\n");
    
    // 启动服务器监听
    printf("Starting server listen on port 8080...\n");
    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server, error code: %d\n", result);
        return 1;
    }
    printf("Server listening on http://0.0.0.0:8080\n");
    printf("Server is running! Press Ctrl+C to stop.\n");
    
    // 启动事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");
    
    return 0;
}