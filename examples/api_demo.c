/**
 * @file api_demo.c
 * @brief UVHTTP 核心API演示
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

// 处理器函数 - 使用核心API签名
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* name = uvhttp_request_get_query_param(req, "name");
    if (!name) name = "World";
    
    char content[1024];
    snprintf(content, sizeof(content), "Hello, %s! 欢迎使用UVHTTP核心API", name);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(res, content, strlen(content));
    
    return uvhttp_response_send(res);
}

int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    const char* body = uvhttp_request_get_body(req);
    
    char json[2048];
    snprintf(json, sizeof(json), "{"
        "\"status\": \"success\","
        "\"method\": \"%s\","
        "\"url\": \"%s\","
        "\"body\": \"%s\","
        "\"message\": \"这是使用核心API创建的响应\""
    "}", method, url, body ? body : "");
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>UVHTTP 核心API演示</title></head>"
        "<body>"
        "<h1>🚀 UVHTTP 核心API演示</h1>"
        "<p>这是一个使用核心API创建的HTTP服务器。</p>"
        "<h2>可用的API端点：</h2>"
        "<ul>"
        "<li><a href='/hello?name=UVHTTP'>/hello?name=UVHTTP</a> - 问候API</li>"
        "<li><a href='/api'>/api</a> - JSON API</li>"
        "</ul>"
        "<h2>核心API特性：</h2>"
        "<ul>"
        "<li>轻量级设计，最小依赖</li>"
        "<li>高性能，基于libuv</li>"
        "<li>灵活的路由系统</li>"
        "<li>完整的HTTP/1.1支持</li>"
        "</ul>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

int main() {
    printf("🚀 UVHTTP 核心API演示\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "❌ 创建事件循环失败\n");
        return 1;
    }
    
    // 创建服务器
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "❌ 服务器创建失败\n");
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "❌ 路由器创建失败\n");
        uvhttp_server_free(g_server);
        return 1;
    }
    
    // 设置路由器到服务器
    g_server->router = router;
    
    // 添加路由
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/hello", hello_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    
    printf("✅ 服务器配置完成!\n");
    printf("🌐 访问 http://localhost:8080 查看演示\n");
    printf("⏹️  按 Ctrl+C 停止服务器\n");
    
    // 启动服务器
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "❌ 服务器启动失败: %d\n", result);
        uvhttp_server_free(g_server);
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