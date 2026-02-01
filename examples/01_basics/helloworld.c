#include "../include/uvhttp.h"
#include "../include/uvhttp_allocator.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include "../include/uvhttp_constants.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

// 应用上下文结构 - 使用循环注入模式
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_config_t* config;
    uvhttp_context_t* uvhttp_ctx;
    int request_count;
} app_context_t;

// 创建应用上下文
app_context_t* app_context_new(uv_loop_t* loop) {
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
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
        // Router is freed by server_free
        if (ctx->config) {
            uvhttp_config_free(ctx->config);
            ctx->config = NULL;
        }
        if (ctx->uvhttp_ctx) {
            uvhttp_context_destroy(ctx->uvhttp_ctx);
            ctx->uvhttp_ctx = NULL;
        }
        uvhttp_free(ctx);
    }
}

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    
    uv_loop_t* loop = uv_default_loop();
    uv_stop(loop);
    
    printf("Cleanup completed. Exiting.\n");
    exit(0);
}

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    // 从 request 获取 server，然后从 server 获取 app_context
    if (!request->client || !request->client->data) {
        fprintf(stderr, "Error: Connection not found\n");
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    uvhttp_connection_t* conn = (uvhttp_connection_t*)request->client->data;
    if (!conn || !conn->server) {
        fprintf(stderr, "Error: Server not found\n");
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    app_context_t* ctx = (app_context_t*)conn->server->user_data;
    
    if (!ctx) {
        fprintf(stderr, "Error: Application context not found\n");
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return -1;
    }
    
    ctx->request_count++;
    
    // 获取配置信息
    const uvhttp_config_t* config = ctx->config;
    
    // 创建包含配置信息的响应
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
        "Hello, World!\n\n"
        "=== 服务器配置信息 ===\n"
        "最大连接数: %d\n"
        "每连接最大请求数: %d\n"
        "当前活动连接数: %zu\n"
        "最大请求体大小: %zuMB\n"
        "读取缓冲区大小: %zuKB\n"
        "========================\n",
        config ? config->max_connections : 0,
        config ? config->max_requests_per_connection : 0,
        ctx->server ? ctx->server->active_connections : 0,
        config ? config->max_body_size / (1024 * 1024) : 0,
        config ? config->read_buffer_size / 1024 : 0
    );
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    
    uvhttp_response_send(response);
    
    printf("Request handled: %s %s (Active connections: %zu/%d)\n", 
           uvhttp_method_to_string(request->method), 
           request->url,
           ctx->server ? ctx->server->active_connections : 0,
           config ? config->max_connections : 0);
    
    return 0;
}

int main() {
    printf("Hello World Server starting...\n");
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建和加载配置
    printf("Loading configuration...\n");
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n", uvhttp_error_string(result));
        return 1;
    }
    
    /* 确保在错误路径中清理配置 */
    int cleanup_needed = 1;
    
    // 直接设置默认值
    config->max_connections = 500;
    config->max_requests_per_connection = 100;
    config->backlog = 128;
    config->max_body_size = 1048576;  // 1MB
    config->read_buffer_size = 8192;
    config->keepalive_timeout = 30;
    config->request_timeout = 60;

    // 验证配置
    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        fprintf(stderr, "Configuration validation failed\n");
        uvhttp_config_free(config);
        return 1;
    }

    /* 配置验证成功，将由服务器负责清理 */
    cleanup_needed = 0;

    // 打印配置信息
    printf("Configuration loaded successfully:\n");
    printf("  Max connections: %d\n", config->max_connections);
    printf("  Max requests per connection: %d\n", config->max_requests_per_connection);
    printf("  Max body size: %zu bytes\n", config->max_body_size);
    printf("  Read buffer size: %d bytes\n", config->read_buffer_size);

    // 获取默认循环
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to get default loop\n");
        if (cleanup_needed) {
            uvhttp_config_free(config);
        }
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)loop);
    
    // 创建应用上下文
    printf("Creating application context...\n");
    app_context_t* ctx = app_context_new(loop);
    if (!ctx) {
        fprintf(stderr, "Failed to create application context\n");
        if (cleanup_needed) {
            uvhttp_config_free(config);
        }
        return 1;
    }
    ctx->config = config;
    printf("Application context created successfully: %p\n", (void*)ctx);
    
    // 创建服务器
    printf("Creating server...\n");
    ctx->uvhttp_error_t server_result = uvhttp_server_new(loop, &server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (!ctx->server) {
        fprintf(stderr, "Failed to create server\n");
        app_context_free(ctx);
        return 1;
    }
    
    // 应用配置到服务器
    ctx->server->config = config;
    // 将 app_context 设置到 server->user_data
    ctx->server->user_data = ctx;
    printf("Server created successfully: %p\n", (void*)ctx->server);
    printf("Applied configuration with max_connections=%d\n", config->max_connections);

    // 创建 uvhttp 上下文
    ctx->uvhttp_error_t result_uvhttp_ctx = uvhttp_context_create(loop, &uvhttp_ctx);
    if (!ctx->uvhttp_ctx) {
        fprintf(stderr, "Failed to create uvhttp context\n");
        app_context_free(ctx);
        return 1;
    }

    // 设置全局配置（重要：这会消除"Global configuration not initialized"警告）
    uvhttp_config_set_current(ctx->uvhttp_ctx, config);
    printf("Global configuration set\n");
    
    // 将 uvhttp_ctx 设置到 server->context
    uvhttp_server_set_context(ctx->server, ctx->uvhttp_ctx);
    printf("uvhttp context set to server\n");
    
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
    printf("Adding route...\n");
    int route_result = uvhttp_router_add_route(ctx->router, "/", hello_handler);
    if (route_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to add route, error: %d\n", route_result);
        app_context_free(ctx);
        return 1;
    }
    printf("Route added successfully\n");
    
    // 设置路由器到服务器
    ctx->server->router = ctx->router;
    printf("Router set to server\n");
    
    // 启动服务器监听
    printf("Starting server listen on port %d...\n", UVHTTP_DEFAULT_PORT);
    uvhttp_error_t result = uvhttp_server_listen(ctx->server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server, error code: %d\n", result);
        app_context_free(ctx);
        return 1;
    }
    printf("Server listening on http://%s:%d\n", UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    printf("Server is running! Press Ctrl+C to stop.\n");
    
    // 启动事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");
    
    // 正常退出时的清理
    printf("Performing final cleanup...\n");
    app_context_free(ctx);
    
    return 0;
}