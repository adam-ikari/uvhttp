/**
 * @file simple_config.c
 * @brief UVHTTP 简单配置示例
 *
 * 演示如何使用 UVHTTP 配置系统设置并发连接数限制
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

// 应用上下文结构
typedef struct {
    uvhttp_server_t* server;
    uvhttp_context_t* context;
} app_context_t;

static app_context_t* g_app_context = NULL;

void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    if (g_app_context && g_app_context->server) {
        uvhttp_server_stop(g_app_context->server);
        uvhttp_server_free(g_app_context->server);
        g_app_context->server = NULL;
    }
    if (g_app_context) {
        free(g_app_context);
        g_app_context = NULL;
    }
    exit(0);
}

int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // 从请求中获取服务器
    if (!request->client || !request->client->data) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal error", 14);
        uvhttp_response_send(response);
        return -1;
    }
    
    uvhttp_connection_t* conn = (uvhttp_connection_t*)request->client->data;
    uvhttp_server_t* server = conn->server;
    uvhttp_context_t* context = server->context;
    
    const uvhttp_config_t* config = uvhttp_config_get_current(context);
    
    char body[512];
    snprintf(body, sizeof(body),
        "UVHTTP 简单配置示例\n\n"
        "当前配置:\n"
        "- 最大连接数: %d\n"
        "- 每连接最大请求数: %d\n"
        "- 当前活动连接数: %zu\n"
        "- 最大请求体大小: %zuMB\n",
        config->max_connections,
        config->max_requests_per_connection,
        server ? server->active_connections : 0,
        config->max_body_size / (1024 * 1024)
    );
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

int main() {
    printf("UVHTTP 简单配置示例\n");
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    g_app_context = (app_context_t*)malloc(sizeof(app_context_t));
    if (!g_app_context) {
        fprintf(stderr, "无法分配应用上下文\n");
        return 1;
    }
    memset(g_app_context, 0, sizeof(app_context_t));
    
    // 创建服务器
    uvhttp_error_t server_result = uvhttp_server_new(loop, &g_app_context->server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        free(g_app_context);
        return 1;
    }
    if (!g_app_context->server) {
        fprintf(stderr, "服务器创建失败\n");
        free(g_app_context);
        return 1;
    }
    
    // 创建配置
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    if (!config) {
        fprintf(stderr, "配置创建失败\n");
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    
    // 设置并发连接数限制
    printf("设置配置参数...\n");
    config->max_connections = 1500;              // 最大1500个并发连接
    config->max_requests_per_connection = 100;   // 每个连接最多100个请求
    config->max_body_size = 2 * 1024 * 1024;     // 2MB最大请求体
    config->read_buffer_size = 8192;             // 8KB读取缓冲区
    
    // 验证配置
    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        fprintf(stderr, "配置验证失败\n");
        uvhttp_config_free(config);
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    
    // 应用配置
    g_app_context->server->config = config;

    // 创建上下文
    uvhttp_error_t result_context = uvhttp_context_create(loop, &g_app_context->context);
    if (result_context != UVHTTP_OK) {
        fprintf(stderr, "上下文创建失败\n");
        uvhttp_config_free(config);
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }

    // 设置全局配置（重要：这会消除"Global configuration not initialized"警告）
    uvhttp_config_set_current(g_app_context->context, config);

    // 创建路由
    uvhttp_router_t* router = NULL;
    uvhttp_error_t router_result = uvhttp_router_new(&router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(router_result));
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    uvhttp_router_add_route(router, "/", simple_handler);
    g_app_context->server->router = router;
    
    // 启动服务器
    printf("启动服务器 (端口 8080)...\n");
    printf("配置: 最大连接数=%d, 每连接最大请求数=%d\n", 
           config->max_connections, config->max_requests_per_connection);
    
    if (uvhttp_server_listen(g_app_context->server, "0.0.0.0", 8080) != UVHTTP_OK) {
        fprintf(stderr, "服务器启动失败\n");
        uvhttp_server_free(g_app_context->server);
        free(g_app_context);
        return 1;
    }
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("按 Ctrl+C 停止服务器\n");

    uv_run(loop, UV_RUN_DEFAULT);

    // 清理上下文
    if (g_app_context && g_app_context->context) {
        uvhttp_context_destroy(g_app_context->context);
    }
    if (g_app_context) {
        free(g_app_context);
    }

    return 0;
}