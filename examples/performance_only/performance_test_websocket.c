/*
 * UVHTTP WebSocket 性能测试服务器
 * 用于测试 WebSocket 通信的性能
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include "../include/uvhttp_websocket_middleware.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_ws_middleware_t* g_ws_middleware = NULL;
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
    
    if (g_ws_middleware) {
        uvhttp_ws_middleware_destroy(g_ws_middleware);
        g_ws_middleware = NULL;
    }
    
    g_loop = NULL;
    exit(0);
}

// WebSocket 连接回调
int on_ws_connect(uvhttp_ws_connection_t* conn, void* user_data) {
    (void)conn;
    (void)user_data;
    // 连接建立
    return 0;
}

// WebSocket 消息回调
int on_ws_message(uvhttp_ws_connection_t* conn, const char* message, size_t len, int opcode, void* user_data) {
    (void)opcode;
    (void)user_data;
    
    // 回显消息
    uvhttp_ws_middleware_send(g_ws_middleware, conn, message, len);
    return 0;
}

// WebSocket 关闭回调
int on_ws_close(uvhttp_ws_connection_t* conn, void* user_data) {
    (void)conn;
    (void)user_data;
    // 连接关闭
    return 0;
}

// WebSocket 错误回调
int on_ws_error(uvhttp_ws_connection_t* conn, int error_code, void* user_data) {
    (void)conn;
    (void)error_code;
    (void)user_data;
    // 错误处理
    return 0;
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
        uvhttp_server_free(g_server);
        uvhttp_config_free(config);
        return 1;
    }

    uvhttp_config_set_current(g_context, config);

    // 创建 WebSocket 中间件
    uvhttp_ws_middleware_config_t ws_config = UVHTTP_WS_MIDDLEWARE_DEFAULT_CONFIG;
    g_ws_middleware = uvhttp_ws_middleware_create("/ws", &ws_config);
    if (!g_ws_middleware) {
        uvhttp_server_free(g_server);
        g_server = NULL;
        uvhttp_config_free(config);
        return 1;
    }
    
    // 设置 WebSocket 回调
    uvhttp_ws_middleware_callbacks_t ws_callbacks = {
        .on_connect = on_ws_connect,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .on_error = on_ws_error,
        .user_data = NULL
    };
    uvhttp_ws_middleware_set_callbacks(g_ws_middleware, &ws_callbacks);
    
    // 创建路由器
    g_router = uvhttp_router_new();
    if (!g_router) {
        uvhttp_ws_middleware_destroy(g_ws_middleware);
        g_ws_middleware = NULL;
        uvhttp_server_free(g_server);
        g_server = NULL;
        uvhttp_config_free(config);
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/", minimal_handler);
    
    // 设置路由器
    g_server->router = g_router;
    
    // 添加 WebSocket 中间件到服务器
    uvhttp_server_add_middleware(g_server, uvhttp_ws_middleware_get_http_middleware(g_ws_middleware));
    
    // 启动服务器
    uvhttp_error_t result = uvhttp_server_listen(g_server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    if (result != UVHTTP_OK) {
        printf("错误：无法启动服务器 (错误码: %d)\n", result);
        uvhttp_ws_middleware_destroy(g_ws_middleware);
        g_ws_middleware = NULL;
        uvhttp_server_free(g_server);
        g_server = NULL;
        uvhttp_config_free(config);
        return 1;
    }
    
    printf("WebSocket 性能测试服务器已启动\n");
    printf("HTTP 服务地址: http://127.0.0.1:8080\n");
    printf("WebSocket 服务地址: ws://127.0.0.1:8080/ws\n");
    printf("按 Ctrl+C 停止服务器\n");
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);

    // 清理
    if (g_ws_middleware) {
        uvhttp_ws_middleware_destroy(g_ws_middleware);
        g_ws_middleware = NULL;
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