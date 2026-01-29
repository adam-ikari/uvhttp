/**
 * @file simple_api_demo.c
 * @brief UVHTTP 最简启动演示 - 使用核心API
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_constants.h"
#include <stdio.h>

// 简单的处理器
int simple_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>UVHTTP 简单演示</title></head>"
        "<body>"
        "<h1>🚀 UVHTTP 简单演示</h1>"
        "<p>这是使用核心API创建的最简单的HTTP服务器。</p>"
        "<p>请求路径: ";
    
    const char* path = uvhttp_request_get_path(req);
    if (!path) path = "unknown";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    
    // 构建响应
    char response[1024];
    snprintf(response, sizeof(response), 
        "%s%s</p>"
        "<p>方法: %s</p>"
        "</body>"
        "</html>", html, path, uvhttp_request_get_method(req));
    
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

int main() {
    printf("🚀 启动UVHTTP最简演示服务器...\n");
    printf("📡 服务器将运行在 http://localhost:%d\n", UVHTTP_DEFAULT_PORT);
    printf("⏹️  按 Ctrl+C 停止服务器\n");
    printf("\n✨ 这展示了核心API的最简用法\n");
    printf("💡 只需几行代码即可启动完整的HTTP服务器!\n\n");
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器
    uvhttp_server_t* server = NULL;
    uvhttp_error_t uvhttp_error_t server_result = uvhttp_server_new(loop, &server, &result);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        return 1;
    }
    if (!server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        return 1;
    }
    uvhttp_server_set_router(server, router);
    
    // 添加默认路由
    uvhttp_router_add_route(router, "/*", simple_handler);
    
    // 启动服务器
    int listen_result = uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "❌ 服务器启动失败: %d\n", result);
        return 1;
    }
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    uvhttp_server_free(server);
    
    return 0;
}