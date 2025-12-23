#include "../include/uvhttp.h"
#include "../include/uvhttp_allocator.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

// Global variables for cleanup
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    
    if (g_server) {
        printf("Stopping server...\n");
        uvhttp_server_stop(g_server);
        // Note: uvhttp_server_free will also free the router, so don't free it separately
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;  // Router is freed by server_free
    }
    
    // Note: Don't close the default loop - it's managed by libuv
    // Only close loops that we created ourselves
    g_loop = NULL;
    
    printf("Cleanup completed. Exiting.\n");
    exit(0);
}

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Connection", "close");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    
    /* The response send callback will handle connection closing 
     * based on the Connection: close header */
    uvhttp_response_send(response);
    
    // printf("Request handled: %s %s\n", 
    //        uvhttp_method_to_string(request->method), 
    //        request->url);
    
    return 0;
}

int main() {
    printf("Hello World Server starting...\n");
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 获取默认循环
    g_loop = uv_default_loop();
    if (!g_loop) {
        fprintf(stderr, "Failed to get default loop\n");
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)g_loop);
    
    // 创建服务器
    printf("Creating server...\n");
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    printf("Server created successfully: %p\n", (void*)g_server);
    
    // 创建路由器
    printf("Creating router...\n");
    g_router = uvhttp_router_new();
    if (!g_router) {
        fprintf(stderr, "Failed to create router\n");
        return 1;
    }
    printf("Router created successfully: %p\n", (void*)g_router);
    
    // 添加路由
    printf("Adding route...\n");
    int route_result = uvhttp_router_add_route(g_router, "/", hello_handler);
    if (route_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to add route, error: %d\n", route_result);
        return 1;
    }
    printf("Route added successfully\n");
    
    // 设置路由器到服务器
    g_server->router = g_router;
    printf("Router set to server\n");
    
    // 启动服务器监听
    printf("Starting server listen on port 8080...\n");
    uvhttp_error_t result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server, error code: %d\n", result);
        return 1;
    }
    printf("Server listening on http://0.0.0.0:8080\n");
    printf("Server is running! Press Ctrl+C to stop.\n");
    
    // 启动事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");
    
    // 正常退出时的清理
    signal_handler(0);
    
    return 0;
}