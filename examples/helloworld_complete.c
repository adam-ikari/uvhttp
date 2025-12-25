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
static uvhttp_config_t* g_config = NULL;

// Signal handler for graceful shutdown
void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);
    
    if (g_server) {
        printf("Stopping server...\n");
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;  // Router is freed by server_free
    }
    
    // 手动清理配置（如果服务器没有清理它）
    if (g_config) {
        uvhttp_config_free(g_config);
        g_config = NULL;
    }
    
    g_loop = NULL;
    
    printf("Cleanup completed. Exiting.\n");
    exit(0);
}

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    // 使用静态缓冲区避免动态分配
    static const char* response_template = 
        "Hello, World!\n\n"
        "=== 服务器配置信息 ===\n"
        "最大连接数: %d\n"
        "每连接最大请求数: %d\n"
        "当前活动连接数: %zu\n"
        "最大请求体大小: %zuMB\n"
        "读取缓冲区大小: %zuKB\n"
        "========================\n";
    
    // 使用栈上的缓冲区而不是动态分配
    char response_body[512];
    int max_connections = 500;  // 默认值
    int max_requests = 100;     // 默认值
    size_t active_connections = 0;
    size_t max_body_size = 1;   // MB
    size_t buffer_size = 8;     // KB
    
    // 安全地获取配置信息
    const uvhttp_config_t* config = NULL;
    if (g_server && g_server->config) {
        config = g_server->config;
        max_connections = config->max_connections;
        max_requests = config->max_requests_per_connection;
        max_body_size = config->max_body_size / (1024 * 1024);
        buffer_size = config->read_buffer_size / 1024;
    }
    
    // 安全地获取活动连接数
    if (g_server) {
        active_connections = g_server->active_connections;
    }
    
    // 使用snprintf的安全版本
    int written = snprintf(response_body, sizeof(response_body),
        response_template,
        max_connections,
        max_requests,
        active_connections,
        max_body_size,
        buffer_size
    );
    
    // 检查snprintf是否成功
    if (written < 0 || written >= sizeof(response_body)) {
        // 如果失败，使用简单的响应
        const char* simple_response = "Hello, World!\n";
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");
        uvhttp_response_set_body(response, simple_response, strlen(simple_response));
    } else {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");
        uvhttp_response_set_body(response, response_body, written);
    }
    
    uvhttp_response_send(response);
    
    printf("Request handled: %s %s (Active connections: %zu/%d)\n", 
           uvhttp_method_to_string(request->method), 
           request->url,
           active_connections,
           max_connections);
    
    return 0;
}

int main() {
    printf("Hello World Server starting...\n");
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建和加载配置
    printf("Loading configuration...\n");
    g_config = uvhttp_config_new();
    if (!g_config) {
        fprintf(stderr, "Failed to create configuration\n");
        return 1;
    }
    
    // 设置默认配置
    uvhttp_config_set_defaults(g_config);
    
    // 直接设置安全的默认值
    g_config->max_connections = 500;
    g_config->max_requests_per_connection = 100;
    g_config->backlog = 128;
    g_config->max_body_size = 1048576;  // 1MB
    g_config->read_buffer_size = 8192;
    g_config->keepalive_timeout = 30;
    g_config->request_timeout = 60;
    
    // 尝试从配置文件加载（会覆盖默认值）
    if (uvhttp_config_load_file(g_config, "helloworld.conf") != UVHTTP_OK) {
        printf("Config file not found, using default values...\n");
    } else {
        printf("Configuration loaded from file\n");
    }
    
    // 从环境变量加载配置（可选，会覆盖文件配置）
    uvhttp_config_load_env(g_config);
    
    // 验证配置
    if (uvhttp_config_validate(g_config) != UVHTTP_OK) {
        fprintf(stderr, "Configuration validation failed\n");
        uvhttp_config_free(g_config);
        g_config = NULL;
        return 1;
    }
    
    // 打印配置信息
    printf("Configuration loaded successfully:\n");
    printf("  Max connections: %d\n", g_config->max_connections);
    printf("  Max requests per connection: %d\n", g_config->max_requests_per_connection);
    printf("  Max body size: %zu bytes\n", g_config->max_body_size);
    printf("  Read buffer size: %d bytes\n", g_config->read_buffer_size);
    
    // 获取默认循环
    g_loop = uv_default_loop();
    if (!g_loop) {
        fprintf(stderr, "Failed to get default loop\n");
        uvhttp_config_free(g_config);
        g_config = NULL;
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)g_loop);
    
    // 创建服务器
    printf("Creating server...\n");
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        uvhttp_config_free(g_config);
        g_config = NULL;
        return 1;
    }
    
    // 应用配置到服务器
    g_server->config = g_config;
    printf("Server created successfully: %p\n", (void*)g_server);
    printf("Applied configuration with max_connections=%d\n", g_config->max_connections);
    
    // 创建路由器
    printf("Creating router...\n");
    g_router = uvhttp_router_new();
    if (!g_router) {
        fprintf(stderr, "Failed to create router\n");
        if (g_server) {
            uvhttp_server_free(g_server);
            g_server = NULL;
        }
        uvhttp_config_free(g_config);
        g_config = NULL;
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
    if (g_server) {
        printf("Performing final cleanup...\n");
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;
    }
    
    if (g_config) {
        uvhttp_config_free(g_config);
        g_config = NULL;
    }
    
    return 0;
}