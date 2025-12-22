/**
 * @file simple_server.c
 * @brief UVHTTP 简单服务器示例
 * 
 * 此示例展示了如何使用 UVHTTP 创建一个基本的 HTTP 服务器，
 * 包含路由处理、错误处理和日志记录功能。
 */

#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

/* 全局服务器实例，用于信号处理 */
static uvhttp_server_t* g_server = NULL;
static uv_loop_t* g_loop = NULL;
static size_t g_request_count = 0;

/* 信号处理函数 */
void signal_handler(int sig) {
    printf("\n接收到信号 %d，正在关闭服务器...\n", sig);
    if (g_server) {
        uvhttp_server_stop(g_server);
    }
    if (g_loop) {
        uv_stop(g_loop);
    }
}

/* 获取当前时间戳字符串 */
void get_timestamp(char* buffer, size_t size) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* 记录请求日志 */
void log_request(uvhttp_request_t* request) {
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    const char* method = uvhttp_request_get_method(request);
    const char* url = uvhttp_request_get_url(request);
    const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
    
    printf("[%s] %s %s", timestamp, method, url);
    if (user_agent) {
        printf(" (%s)", user_agent);
    }
    printf(" [%zu]\n", ++g_request_count);
}

/* 主页处理器 */
void home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    log_request(request);
    
    /* 构建动态 HTML 响应 */
    char html[1024];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    snprintf(html, sizeof(html),
        "<!DOCTYPE html>"
        "<html lang=\"zh-CN\">"
        "<head>"
        "    <meta charset=\"UTF-8\">"
        "    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">"
        "    <title>UVHTTP 服务器</title>"
        "    <style>"
        "        body { font-family: Arial, sans-serif; max-width: 800px; margin: 0 auto; padding: 20px; }"
        "        .header { background: #f4f4f4; padding: 20px; border-radius: 5px; margin-bottom: 20px; }"
        "        .stats { background: #e8f5e8; padding: 15px; border-radius: 5px; }"
        "        .footer { margin-top: 30px; padding-top: 20px; border-top: 1px solid #ddd; }"
        "    </style>"
        "</head>"
        "<body>"
        "    <div class=\"header\">"
        "        <h1>🚀 欢迎使用 UVHTTP 服务器</h1>"
        "        <p>一个基于 libuv 的高性能、轻量级 HTTP 服务器库</p>"
        "    </div>"
        "    <div class=\"stats\">"
        "        <h2>📊 服务器状态</h2>"
        "        <ul>"
        "            <li>当前时间: %s</li>"
        "            <li>请求计数: %zu</li>"
        "            <li>HTTP 方法: %s</li>"
        "            <li>请求路径: %s</li>"
        "        </ul>"
        "    </div>"
        "    <div>"
        "        <h2>🔗 可用的 API 端点</h2>"
        "        <ul>"
        "            <li><a href=\"/api\">GET /api</a> - JSON API 示例</li>"
        "            <li><a href=\"/info\">GET /info</a> - 服务器信息</li>"
        "            <li><a href=\"/health\">GET /health</a> - 健康检查</li>"
        "        </ul>"
        "    </div>"
        "    <div class=\"footer\">"
        "        <p>Powered by <strong>UVHTTP</strong> | 版本 1.0.0</p>"
        "    </div>"
        "</body>"
        "</html>",
        timestamp, g_request_count, uvhttp_request_get_method(request), 
        uvhttp_request_get_url(request));
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_header(response, "Server", "UVHTTP/1.0.0");
    
    if (uvhttp_response_set_body(response, html, strlen(html)) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        const char* error_msg = "内部服务器错误";
        uvhttp_response_set_body(response, error_msg, strlen(error_msg));
    }
    
    uvhttp_response_send(response);
}

/* API 处理器 */
void api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    log_request(request);
    
    /* 构建 JSON 响应 */
    char json[512];
    char timestamp[32];
    get_timestamp(timestamp, sizeof(timestamp));
    
    snprintf(json, sizeof(json),
        "{"
        "    \"message\": \"Hello from UVHTTP API\","
        "    \"status\": \"ok\","
        "    \"timestamp\": \"%s\","
        "    \"request_count\": %zu,"
        "    \"method\": \"%s\","
        "    \"path\": \"%s\","
        "    \"version\": \"1.0.0\""
        "}",
        timestamp, g_request_count, uvhttp_request_get_method(request),
        uvhttp_request_get_url(request));
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    
    if (uvhttp_response_set_body(response, json, strlen(json)) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        const char* error_msg = "API 内部错误";
        uvhttp_response_set_body(response, error_msg, strlen(error_msg));
    }
    
    uvhttp_response_send(response);
}

/* 服务器信息处理器 */
void info_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    log_request(request);
    
    const char* info_html =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>服务器信息 - UVHTTP</title></head>"
        "<body>"
        "<h1>📋 服务器信息</h1>"
        "<ul>"
        "    <li><strong>服务器:</strong> UVHTTP</li>"
        "    <li><strong>版本:</strong> 1.0.0</li>"
        "    <li><strong>协议:</strong> HTTP/1.1</li>"
        "    <li><strong>架构:</strong> 事件驱动 (libuv)</li>"
        "    <li><strong>解析器:</strong> llhttp</li>"
        "</ul>"
        "<p><a href=\"/\">返回主页</a></p>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, info_html, strlen(info_html));
    uvhttp_response_send(response);
}

/* 健康检查处理器 */
void health_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    log_request(request);
    
    const char* health_json = "{\"status\": \"healthy\", \"uptime\": \"ok\"}";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, health_json, strlen(health_json));
    uvhttp_response_send(response);
}

/* 404 错误处理器 */
void not_found_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    log_request(request);
    
    const char* not_found_html =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>404 - 页面未找到</title></head>"
        "<body>"
        "<h1>❌ 404 - 页面未找到</h1>"
        "<p>请求的页面 <code>%s</code> 不存在。</p>"
        "<p><a href=\"/\">返回主页</a></p>"
        "</body>"
        "</html>";
    
    char html[512];
    snprintf(html, sizeof(html), not_found_html, uvhttp_request_get_url(request));
    
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
}

int main(int argc, char* argv[]) {
    int port = 8080;
    const char* host = "0.0.0.0";
    
    /* 解析命令行参数 */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "错误: 无效的端口号 %s\n", argv[1]);
            return 1;
        }
    }
    
    printf("🚀 启动 UVHTTP 服务器...\n");
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建事件循环 */
    g_loop = uv_default_loop();
    if (!g_loop) {
        fprintf(stderr, "错误: 无法创建事件循环\n");
        return 1;
    }
    
    /* 创建服务器 */
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        fprintf(stderr, "错误: 无法创建服务器\n");
        return 1;
    }
    
    /* 创建路由 */
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "错误: 无法创建路由\n");
        uvhttp_server_free(g_server);
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    uvhttp_router_add_route(router, "/info", info_handler);
    uvhttp_router_add_route(router, "/health", health_handler);
    
    /* 设置默认处理器（404） */
    router->default_handler = not_found_handler;
    
    /* 配置服务器 */
    g_server->router = router;
    g_server->max_connections = 1000;
    
    /* 启动服务器 */
    if (uvhttp_server_listen(g_server, host, port) != 0) {
        fprintf(stderr, "错误: 无法启动服务器在 %s:%d\n", host, port);
        uvhttp_router_free(router);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("✅ 服务器已启动\n");
    printf("📍 监听地址: http://%s:%d\n", host, port);
    printf("\n📖 可用的端点:\n");
    printf("   http://localhost:%d/          - 主页\n", port);
    printf("   http://localhost:%d/api        - JSON API\n", port);
    printf("   http://localhost:%d/info       - 服务器信息\n", port);
    printf("   http://localhost:%d/health     - 健康检查\n", port);
    printf("\n按 Ctrl+C 停止服务器\n\n");
    
    /* 运行事件循环 */
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    printf("\n🧹 正在清理资源...\n");
    uvhttp_router_free(router);
    uvhttp_server_free(g_server);
    uv_loop_close(g_loop);
    
    printf("✅ 服务器已关闭，总共处理了 %zu 个请求\n", g_request_count);
    
    return 0;
}