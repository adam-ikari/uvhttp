#include "../include/uvhttp.h"
#include "../include/uvhttp_allocator.h"
#include "../include/uvhttp_config.h"
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
        // Note: uvhttp_server_free will also free the router and config, so don't free them separately
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
    
    // 获取服务器特定的配置信息
    const uvhttp_config_t* config = NULL;
    if (g_server && g_server->config) {
        config = g_server->config;
    } else {
        // 回退到全局配置
        config = uvhttp_config_get_current();
    }
    
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
        g_server ? g_server->active_connections : 0,
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
           g_server ? g_server->active_connections : 0,
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
    uvhttp_config_t* config = uvhttp_config_new();
    if (!config) {
        fprintf(stderr, "Failed to create configuration\n");
        return 1;
    }
    
    /* 确保在错误路径中清理配置 */
    int cleanup_needed = 1;
    
    // 设置默认配置
    uvhttp_config_set_defaults(config);
    
    // 直接设置默认值
    config->max_connections = 500;
    config->max_requests_per_connection = 100;
    config->backlog = 128;
    config->max_body_size = 1048576;  // 1MB
    config->read_buffer_size = 8192;
    config->keepalive_timeout = 30;
    config->request_timeout = 60;
    
    // 尝试从配置文件加载（会覆盖默认值）
    if (uvhttp_config_load_file(config, "helloworld.conf") != UVHTTP_OK) {
        printf("Config file not found, using default values...\n");
        
        // 创建默认配置文件供参考
        FILE* conf_file = fopen("helloworld.conf", "w");
        if (conf_file) {
            fprintf(conf_file, "# Hello World Server Configuration\n");
            fprintf(conf_file, "# Connection limits\n");
            fprintf(conf_file, "max_connections=500\n");
            fprintf(conf_file, "max_requests_per_connection=100\n");
            fprintf(conf_file, "backlog=128\n\n");
            fprintf(conf_file, "# Performance settings\n");
            fprintf(conf_file, "max_body_size=1048576\n");
            fprintf(conf_file, "read_buffer_size=8192\n");
            fprintf(conf_file, "keepalive_timeout=30\n");
            fprintf(conf_file, "request_timeout=60\n");
            fclose(conf_file);
            printf("Default config file created: helloworld.conf\n");
        }
    } else {
        printf("Configuration loaded from file\n");
    }
    
    // 从环境变量加载配置（可选，会覆盖文件配置）
    uvhttp_config_load_env(config);
    
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
    g_loop = uv_default_loop();
    if (!g_loop) {
        fprintf(stderr, "Failed to get default loop\n");
        uvhttp_config_free(config);
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)g_loop);
    
    // 创建服务器
    printf("Creating server...\n");
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        if (cleanup_needed) {
            uvhttp_config_free(config);
        }
        return 1;
    }
    
    // 应用配置到服务器
    g_server->config = config;
    printf("Server created successfully: %p\n", (void*)g_server);
    printf("Applied configuration with max_connections=%d\n", config->max_connections);
    
    // 创建路由器
    printf("Creating router...\n");
    g_router = uvhttp_router_new();
    if (!g_router) {
        fprintf(stderr, "Failed to create router\n");
        if (g_server) {
            uvhttp_server_free(g_server);
            g_server = NULL;
        }
        if (cleanup_needed) {
            uvhttp_config_free(config);
        }
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
    // Use UV_RUN_ONCE to have more control, or just let it run until stopped
    uv_run(g_loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");
    
    // 正常退出时的清理 - only if not already cleaned up by signal
    if (g_server) {
        printf("Performing final cleanup...\n");
        // Don't call signal_handler again if it already ran (it calls exit)
        // Instead, just ensure server is NULL to prevent double cleanup
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;  // Router is freed by server_free
    }
    
    return 0;
}