/*
 * UVHTTP 静态文件服务性能测试服务器
 * 用于测试静态文件服务的性能
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_static_context_t* g_static_ctx = NULL;
static uvhttp_context_t* g_context = NULL;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;
    
    if (g_static_ctx) {
        uvhttp_static_free(g_static_ctx);
        g_static_ctx = NULL;
    }
    
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;
    }
    
    g_loop = NULL;
    exit(0);
}

// 静态文件请求处理器
int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!g_static_ctx || !request || !response) {
        return -1;
    }
    
    int static_result = uvhttp_static_handle_request(g_static_ctx, request, response);
    if (static_result != 0) {
        // 错误处理：设置错误响应
        const char* error_body = "Error processing static file request";
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
        uvhttp_response_send(response);
    }
    // 成功情况下，uvhttp_static_handle_request 已经发送了响应
    
    return 0;
}

// 创建测试文件
void create_test_files() {
    // 创建测试目录
    (void)system("mkdir -p ./test_static/small");
    (void)system("mkdir -p ./test_static/medium");
    (void)system("mkdir -p ./test_static/large");
    
    // 创建小文件（1KB）
    FILE* small_file = fopen("./test_static/small/test.txt", "w");
    if (small_file) {
        for (int i = 0; i < 1024; i++) {
            fputc('A' + (i % 26), small_file);
        }
        fclose(small_file);
    }
    
    // 创建中等文件（10KB）
    FILE* medium_file = fopen("./test_static/medium/test.txt", "w");
    if (medium_file) {
        for (int i = 0; i < 10240; i++) {
            fputc('B' + (i % 26), medium_file);
        }
        fclose(medium_file);
    }
    
    // 创建大文件（100KB）
    FILE* large_file = fopen("./test_static/large/test.txt", "w");
    if (large_file) {
        for (int i = 0; i < 102400; i++) {
            fputc('C' + (i % 26), large_file);
        }
        fclose(large_file);
    }
    
    // 创建 HTML 文件
    FILE* html_file = fopen("./test_static/index.html", "w");
    if (html_file) {
        fprintf(html_file, 
            "<!DOCTYPE html>\n"
            "<html>\n"
            "<head>\n"
            "    <title>性能测试页面</title>\n"
            "</head>\n"
            "<body>\n"
            "    <h1>性能测试页面</h1>\n"
            "    <p>这是一个用于性能测试的HTML文件。</p>\n"
            "</body>\n"
            "</html>");
        fclose(html_file);
    }
    
    printf("测试文件已创建在 ./test_static/ 目录下\n");
}

int main() {
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建测试文件
    create_test_files();
    
    // 创建配置
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n", uvhttp_error_string(result));
        return 1;
    }
    if (!config) {
        return 1;
    }
    
    // 设置默认值（优化高并发）
    config->max_connections = 10000;
    config->max_requests_per_connection = 1000;
    config->backlog = 8192;
    config->max_body_size = 10485760;  // 10MB
    config->read_buffer_size = 32768;
    config->keepalive_timeout = 60;
    config->request_timeout = 120;
    
    // 验证配置
    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 获取默认循环
    g_loop = uv_default_loop();
    if (!g_loop) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 创建服务器
    uvhttp_error_t server_result = uvhttp_server_new(g_loop, &g_server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (!g_server) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 应用配置
    g_server->config = config;

    // 创建上下文
    uvhttp_error_t result_g_context = uvhttp_context_create(g_loop, &g_context);
    if (result_g_context != UVHTTP_OK) {
        printf("错误：无法创建上下文\n");
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }

    uvhttp_config_set_current(g_context, config);

    // 配置静态文件服务
    uvhttp_static_config_t static_config = {
        .root_directory = "./test_static",
        .index_file = "index.html",
        .enable_directory_listing = 0,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,  // 10MB缓存
        .cache_ttl = 3600,
        .custom_headers = ""
    };
    
    // 创建静态文件服务上下文
    uvhttp_error_t static_result = uvhttp_static_create(&static_config, &g_static_ctx);
    if (static_result != UVHTTP_OK || !g_static_ctx) {
        printf("错误：无法创建静态文件服务上下文\n");
        uvhttp_server_free(g_server);
        g_server = NULL;
        uvhttp_config_free(config);
        return 1;
    }
    
    // 创建路由器
    uvhttp_error_t router_result = uvhttp_router_new(&g_router);
    if (router_result != UVHTTP_OK) {
        printf("错误：无法创建路由器: %s\n", uvhttp_error_string(router_result));
        uvhttp_static_free(g_static_ctx);
        g_static_ctx = NULL;
        uvhttp_server_free(g_server);
        g_server = NULL;
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/static/small/test.txt", static_file_handler);
    uvhttp_router_add_route(g_router, "/static/medium/test.txt", static_file_handler);
    uvhttp_router_add_route(g_router, "/static/large/test.txt", static_file_handler);
    uvhttp_router_add_route(g_router, "/static/index.html", static_file_handler);
    uvhttp_router_add_route(g_router, "/", static_file_handler);
    
    // 设置路由器
    g_server->router = g_router;
    
    // 启动服务器
    uvhttp_error_t listen_result = uvhttp_server_listen(g_server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    if (listen_result != UVHTTP_OK) {
        printf("错误：无法启动服务器 (错误码: %d)\n", listen_result);
        uvhttp_static_free(g_static_ctx);
        g_static_ctx = NULL;
        uvhttp_server_free(g_server);
        g_server = NULL;
        return 1;
    }
    
    printf("静态文件性能测试服务器已启动\n");
    printf("服务地址: http://127.0.0.1:8080\n");
    printf("静态文件目录: %s\n", static_config.root_directory);
    printf("按 Ctrl+C 停止服务器\n");
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);

    // 清理
    if (g_static_ctx) {
        uvhttp_static_free(g_static_ctx);
        g_static_ctx = NULL;
    }

    if (g_context) {
        uvhttp_context_destroy(g_context);
        g_context = NULL;
    }

    if (g_server) {
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;
    }

    return 0;
}