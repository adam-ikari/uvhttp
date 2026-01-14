/**
 * @file health_check_demo.c
 * @brief 健康检查功能演示 - 应用层实现
 * 
 * 本示例展示如何在应用层实现健康检查功能，
 * 而不是依赖框架提供健康检查模块。
 * 这符合 UVHTTP 的设计理念：框架负责核心功能，
 * 业务逻辑由应用层控制。
 */

#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <time.h>

// 应用上下文结构 - 使用循环注入模式
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
} app_context_t;

// 创建应用上下文
app_context_t* app_context_new(uv_loop_t* loop) {
    if (!loop) {
        return NULL;
    }
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        return NULL;
    }
    memset(ctx, 0, sizeof(app_context_t));
    return ctx;
}

// 释放应用上下文
void app_context_free(app_context_t* ctx) {
    if (ctx) {
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
            ctx->server = NULL;
        }
        free(ctx);
    }
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);

    uv_loop_t* loop = uv_default_loop();
    if (loop && loop->data) {
        app_context_t* ctx = (app_context_t*)loop->data;
        if (ctx) {
            printf("Stopping server...\n");
            uvhttp_server_stop(ctx->server);
            // 不在这里释放 context，让主循环正常清理
        }
    }

    // 停止事件循环
    uv_stop(loop);
}

/**
 * @brief 健康检查处理器 - 应用层实现
 * 
 * 这个处理器完全由应用层实现，展示了如何：
 * 1. 从请求中获取服务器信息
 * 2. 检查服务器健康状态
 * 3. 返回 JSON 格式的健康状态
 * 
 * 应用层可以根据实际需求自定义健康检查逻辑，
 * 例如：检查数据库连接、外部服务可用性、内存使用等
 */
int health_check_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }

    // 从循环获取应用上下文
    uv_loop_t* loop = request->client->loop;
    app_context_t* ctx = (app_context_t*)loop->data;
    
    if (!ctx || !ctx->server) {
        uvhttp_response_set_status(response, 503);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        char body[256];
        snprintf(body, sizeof(body), "{\"status\":\"unhealthy\",\"error\":\"context_not_found\",\"timestamp\":%ld}", (long)time(NULL));
        uvhttp_response_set_body(response, body, strlen(body));
        return uvhttp_response_send(response);
    }
    
    // 获取服务器状态
    uvhttp_server_t* server = ctx->server;
    size_t max_connections = server->config ? server->config->max_connections : 100;
    size_t active_connections = server->active_connections;
    
    // 应用层自定义健康检查逻辑
    // 这里可以根据实际需求添加更多检查：
    // - 数据库连接状态
    // - Redis 可用性
    // - 磁盘空间
    // - 内存使用率
    // - 外部 API 可用性
    
    int status_code = 200;
    const char* status = "healthy";
    
    // 根据连接负载判断健康状态
    if (active_connections >= max_connections) {
        status_code = 503;
        status = "unhealthy";
    } else if (active_connections > max_connections * 0.8) {
        status_code = 200;
        status = "degraded";
    }
    
    // 构建 JSON 响应
    // 添加简单的引号转义检查
    char safe_status[64];
    const char* src = status;
    char* dst = safe_status;
    while (*src && dst < safe_status + sizeof(safe_status) - 1) {
        if (*src == '"' || *src == '\\') {
            *dst++ = '\\';
        }
        *dst++ = *src++;
    }
    *dst = '\0';

    char body[512];
    snprintf(body, sizeof(body),
        "{\"status\":\"%s\",\"timestamp\":%ld,\"connections\":{\"active\":%zu,\"max\":%zu},\"request_count\":%d}",
        safe_status, (long)time(NULL), active_connections, max_connections, ctx->request_count);
    
    uvhttp_response_set_status(response, status_code);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    
    return uvhttp_response_send(response);
}

// 主页处理器
int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }

    // 从循环获取应用上下文
    uv_loop_t* loop = request->client->loop;
    app_context_t* ctx = (app_context_t*)loop->data;
    
    if (!ctx) {
        fprintf(stderr, "Error: Application context not found\n");
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    ctx->request_count++;
    
    const char* body = 
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "    <title>UVHTTP Health Check Demo</title>\n"
        "    <style>\n"
        "        body { font-family: Arial, sans-serif; margin: 40px; }\n"
        "        .container { max-width: 800px; margin: 0 auto; }\n"
        "        .info { background: #e7f3ff; padding: 15px; border-radius: 5px; margin: 10px 0; }\n"
        "        .endpoint { background: #f0f0f0; padding: 10px; margin: 5px 0; border-radius: 3px; }\n"
        "    </style>\n"
        "</head>\n"
        "<body>\n"
        "    <div class=\"container\">\n"
        "        <h1>🏥 UVHTTP Health Check Demo</h1>\n"
        "        <div class=\"info\">\n"
        "            <h2>服务器状态</h2>\n"
        "            <p>请求计数: %d</p>\n"
        "        </div>\n"
        "        <div class=\"endpoint\">\n"
        "            <h3>健康检查端点（应用层实现）</h3>\n"
        "            <p><a href=\"/health\">GET /health</a> - 检查服务器健康状态</p>\n"
        "            <p><small>此端点完全由应用层实现，不依赖框架的健康检查模块</small></p>\n"
        "        </div>\n"
        "    </div>\n"
        "</body>\n"
        "</html>\n";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, body, strlen(body));
    return uvhttp_response_send(response);
}

int main() {
    printf("=== UVHTTP Health Check Demo (Application Layer) ===\n");
    printf("注意：健康检查功能完全由应用层实现，不依赖框架模块\n\n");
    
    // 获取默认循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to get default loop\n");
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)loop);
    
    // 创建应用上下文
    printf("Creating application context...\n");
    app_context_t* ctx = app_context_new(loop);
    if (!ctx) {
        fprintf(stderr, "Failed to create application context\n");
        return 1;
    }
    printf("Application context created successfully: %p\n", (void*)ctx);
    
    // 注入到循环
    loop->data = ctx;
    printf("Context injected to loop\n");
    
    // 创建服务器
    printf("Creating server...\n");
    ctx->server = uvhttp_server_new(loop);
    if (!ctx->server) {
        fprintf(stderr, "Failed to create server\n");
        app_context_free(ctx);
        return 1;
    }
    printf("Server created successfully: %p\n", (void*)ctx->server);
    
    // 创建路由器
    printf("Creating router...\n");
    ctx->router = uvhttp_router_new();
    if (!ctx->router) {
        fprintf(stderr, "Failed to create router\n");
        app_context_free(ctx);
        return 1;
    }
    printf("Router created successfully: %p\n", (void*)ctx->router);
    
    // 添加路由
    printf("Adding routes...\n");
    int route_result = uvhttp_router_add_route(ctx->router, "/", hello_handler);
    if (route_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to add route, error: %d\n", route_result);
        app_context_free(ctx);
        return 1;
    }
    printf("Route added successfully\n");
    
    // 添加健康检查路由（应用层实现）
    int health_result = uvhttp_router_add_route(ctx->router, "/health", health_check_handler);
    if (health_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to add health check route, error: %d\n", health_result);
        app_context_free(ctx);
        return 1;
    }
    printf("Health check route added successfully (application layer implementation)\n");
    
    // 设置路由器到服务器
    ctx->server->router = ctx->router;
    printf("Router set to server\n");
    
    // 启动服务器监听
    printf("Starting server listen on port 8080...\n");
    uvhttp_error_t result = uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server, error code: %d\n", result);
        app_context_free(ctx);
        return 1;
    }
    printf("Server listening on http://0.0.0.0:8080\n");
    printf("Server is running! Press Ctrl+C to stop.\n");
    printf("\n健康检查端点:\n");
    printf("  http://localhost:8080/health\n");
    printf("\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 启动事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");
    
    // 正常退出时的清理
    if (loop && loop->data) {
        printf("Performing final cleanup...\n");
        app_context_free((app_context_t*)loop->data);
        loop->data = NULL;
    }
    
    return 0;
}