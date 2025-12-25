/**
 * @file quick_api_demo.c
 * @brief UVHTTP 快速启动演示 - 使用核心API
 */

#include "../include/uvhttp.h"
#include <stdio.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

// 简单处理器
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* name = uvhttp_request_get_query_param(req);
    if (!name) name = "World";
    
    char content[256];
    snprintf(content, sizeof(content), "Hello, %s! 这是快速启动演示。", name);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(res, content, strlen(content));
    
    return uvhttp_response_send(res);
}

int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{"
        "\"message\": \"这是快速启动演示\"," 
        "\"status\": \"success\"," 
        "\"api\": \"core\""
    "}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int main() {
    printf("🚀 UVHTTP 快速启动演示\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_server_set_router(g_server, router);
    
    // 添加路由
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    
    printf("✅ 服务器配置完成!\n");
    printf("🌐 访问 http://localhost:8080 查看演示\n");
    printf("⏹️  按 Ctrl+C 停止服务器\n");
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "❌ 服务器启动失败: %d\n", result);
        return 1;
    }
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    if (g_server) {
        uvhttp_server_free(g_server);
    }
    
    return 0;
}