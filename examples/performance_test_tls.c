/*
 * UVHTTP TLS/HTTPS 性能测试服务器
 * 用于测试 TLS 加密通信的性能
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include "../include/uvhttp_tls.h"
#include <signal.h>
#include <stdlib.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_tls_context_t* g_tls_ctx = NULL;
static uvhttp_context_t* g_context = NULL;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;
    
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;
    }
    
    if (g_tls_ctx) {
        uvhttp_tls_context_free(g_tls_ctx);
        g_tls_ctx = NULL;
    }
    
    g_loop = NULL;
    exit(0);
}

// 最小化响应处理器（用于基准测试）
int minimal_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "OK";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, 2);
    uvhttp_response_send(response);
    
    return 0;
}

int main() {
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建配置
    uvhttp_config_t* config = uvhttp_config_new();
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
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 应用配置
    g_server->config = config;

    // 创建上下文
    g_context = uvhttp_context_create(g_loop);
    if (!g_context) {
        printf("错误：无法创建上下文\n");
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }

    uvhttp_config_set_current(g_context, config);

    // 创建TLS上下文
    g_tls_ctx = uvhttp_tls_context_new();
    if (!g_tls_ctx) {
        printf("错误：无法创建TLS上下文\n");
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }
    
    // 加载证书和私钥
    if (uvhttp_tls_context_load_cert_chain(g_tls_ctx, "./test/certs/server.crt") != UVHTTP_TLS_OK) {
        printf("错误：无法加载TLS证书\n");
        uvhttp_tls_context_free(g_tls_ctx);
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }
    
    if (uvhttp_tls_context_load_private_key(g_tls_ctx, "./test/certs/server.key") != UVHTTP_TLS_OK) {
        printf("错误：无法加载TLS私钥\n");
        uvhttp_tls_context_free(g_tls_ctx);
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }
    
    // 启用TLS
    if (uvhttp_server_enable_tls(g_server, g_tls_ctx) != UVHTTP_OK) {
        printf("错误：无法启用TLS\n");
        uvhttp_tls_context_free(g_tls_ctx);
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }
    
    // 创建路由器
    g_router = uvhttp_router_new();
    if (!g_router) {
        uvhttp_tls_context_free(g_tls_ctx);
        g_server = NULL;
        uvhttp_config_free(config);
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/", minimal_handler);
    
    // 设置路由器
    g_server->router = g_router;
    
    // 启动服务器（HTTPS 端口 8443）
    uvhttp_error_t result = uvhttp_server_listen(g_server, UVHTTP_DEFAULT_HOST, 8443);
    if (result != UVHTTP_OK) {
        printf("错误：无法启动 TLS 服务器 (错误码: %d)\n", result);
        uvhttp_tls_context_free(g_tls_ctx);
        uvhttp_server_free(g_server);
        g_server = NULL;
        uvhttp_config_free(config);
        return 1;
    }
    
    printf("TLS/HTTPS 性能测试服务器已启动\n");
    printf("服务地址: https://127.0.0.1:8443\n");
    printf("证书文件: ./test/certs/server.crt\n");
    printf("密钥文件: ./test/certs/server.key\n");
    printf("按 Ctrl+C 停止服务器\n");
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    // 清理
    if (g_tls_ctx) {
        uvhttp_tls_context_free(g_tls_ctx);
        g_tls_ctx = NULL;
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