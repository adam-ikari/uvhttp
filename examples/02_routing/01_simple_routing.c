/**
 * @file 01_simple_routing.c
 * @brief 简单路由示例
 * 
 * 展示如何设置多个路由和不同的处理器
 * 
 * 学习目标：
 * 1. 如何添加多个路由
 * 2. 如何为不同的路径设置不同的处理器
 * 3. 如何返回不同类型的响应（HTML、JSON、文本）
 * 
 * 运行方法：
 * gcc -o simple_routing 01_simple_routing.c -I../../include -L../../build -luvhttp -luv -lpthread
 * ./simple_routing
 * 
 * 测试：
 * curl http://localhost:8080/
 * curl http://localhost:8080/about
 * curl http://localhost:8080/api
 * curl http://localhost:8080/status
 */

#include "../../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/**
 * @brief 应用上下文结构
 */
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    int is_running;
} app_context_t;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    exit(0);
}

/**
 * @brief 主页处理器
 */
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <meta charset='utf-8'>\n"
        "    <title>UVHTTP 示例</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n"
        "        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }\n"
        "        h1 { color: #333; }\n"
        "        .route { background: #e7f3ff; padding: 15px; margin: 10px 0; border-radius: 5px; }\n"
        "        code { background: #f4f4f4; padding: 2px 6px; border-radius: 3px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='container'>\n"
        "        <h1>🚀 UVHTTP 路由示例</h1>\n"
        "        <p>欢迎使用 UVHTTP！这是一个简单的路由示例。</p>\n"
        "        \n"
        "        <h2>可用的路由：</h2>\n"
        "        <div class='route'>\n"
        "            <strong>/</strong> - 主页（HTML）<br>\n"
        "            <code>curl http://localhost:8080/</code>\n"
        "        </div>\n"
        "        <div class='route'>\n"
        "            <strong>/about</strong> - 关于页面（HTML）<br>\n"
        "            <code>curl http://localhost:8080/about</code>\n"
        "        </div>\n"
        "        <div class='route'>\n"
        "            <strong>/api</strong> - API 接口（JSON）<br>\n"
        "            <code>curl http://localhost:8080/api</code>\n"
        "        </div>\n"
        "        <div class='route'>\n"
        "            <strong>/status</strong> - 服务器状态（JSON）<br>\n"
        "            <code>curl http://localhost:8080/status</code>\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

/**
 * @brief 关于页面处理器
 */
int about_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <meta charset='utf-8'>\n"
        "    <title>关于 UVHTTP</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; background: #f5f5f5; }\n"
        "        .container { max-width: 800px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; }\n"
        "        h1 { color: #333; }\n"
        "        .feature { padding: 10px; margin: 5px 0; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class='container'>\n"
        "        <h1>关于 UVHTTP</h1>\n"
        "        <p>UVHTTP 是一个基于 libuv 的高性能 HTTP 服务器库。</p>\n"
        "        \n"
        "        <h2>主要特性：</h2>\n"
        "        <div class='feature'>✓ 高性能事件驱动架构</div>\n"
        "        <div class='feature'>✓ 轻量级设计，最小依赖</div>\n"
        "        <div class='feature'>✓ 灵活的路由系统</div>\n"
        "        <div class='feature'>✓ 完整的 HTTP/1.1 支持</div>\n"
        "        <div class='feature'>✓ WebSocket 支持</div>\n"
        "        <div class='feature'>✓ TLS/SSL 支持</div>\n"
        "        \n"
        "        <p><a href='/'>返回主页</a></p>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

/**
 * @brief API 接口处理器
 */
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\n"
        "  \"name\": \"UVHTTP\",\n"
        "  \"version\": \"1.0.0\",\n"
        "  \"description\": \"高性能 HTTP 服务器库\",\n"
        "  \"features\": [\n"
        "    \"事件驱动\",\n"
        "    \"轻量级\",\n"
        "    \"高性能\",\n"
        "    \"易于使用\"\n"
        "  ],\n"
        "  \"status\": \"running\"\n"
        "}\n";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

/**
 * @brief 服务器状态处理器
 */
int status_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取服务器信息
    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    
    char json[512];
    snprintf(json, sizeof(json),
        "{\n"
        "  \"status\": \"healthy\",\n"
        "  \"uptime\": 3600,\n"
        "  \"active_connections\": %zu,\n"
        "  \"request\": {\n"
        "    \"method\": \"%s\",\n"
        "    \"url\": \"%s\"\n"
        "  }\n"
        "}\n",
        g_server ? g_server->active_connections : 0,
        method ? method : "unknown",
        url ? url : "unknown");
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int main() {
    printf("========================================\n");
    printf("  UVHTTP 简单路由示例\n");
    printf("========================================\n\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建应用上下文
    app_context_t ctx = {0};
    
    // 创建服务器
    ctx.loop = uv_default_loop();
    ctx.server = uvhttp_server_new(ctx.loop);
    
    if (!ctx.server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加多个路由
    printf("添加路由...\n");
    uvhttp_router_add_route(router, "/", home_handler);
    printf("  ✓ /\n");
    
    uvhttp_router_add_route(router, "/about", about_handler);
    printf("  ✓ /about\n");
    
    uvhttp_router_add_route(router, "/api", api_handler);
    printf("  ✓ /api\n");
    
    uvhttp_router_add_route(router, "/status", status_handler);
    printf("  ✓ /status\n");
    
    printf("\n");
    
    // 设置路由器
    uvhttp_server_set_router(ctx.server, router);
    
    // 启动服务器
    int result = uvhttp_server_listen(ctx.server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(ctx.server);
        return 1;
    }
    
    printf("========================================\n");
    printf("  服务器运行在 http://localhost:8080\n");
    printf("========================================\n\n");
    
    printf("可用的路由：\n");
    printf("  curl http://localhost:8080/          # 主页\n");
    printf("  curl http://localhost:8080/about     # 关于页面\n");
    printf("  curl http://localhost:8080/api       # API 接口\n");
    printf("  curl http://localhost:8080/status    # 服务器状态\n\n");
    
    printf("按 Ctrl+C 停止服务器\n\n");
    
    // 运行事件循环
    uv_run(ctx.loop, UV_RUN_DEFAULT);
    
    // 清理
    if (ctx.server) {
        uvhttp_server_free(ctx.server);
    }
    
    return 0;
}
