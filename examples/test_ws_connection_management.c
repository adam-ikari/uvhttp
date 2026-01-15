/*
 * WebSocket 连接管理功能测试示例
 *
 * 测试功能：
 * 1. 连接池管理
 * 2. 超时检测
 * 3. 心跳检测
 * 4. 连接统计
 * 5. 广播消息
 */

#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static uvhttp_server_t* g_server = NULL;
static int g_message_count = 0;

/* WebSocket 连接回调 */
static int on_ws_connect(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;  // 未使用的参数
    printf("WebSocket client connected\n");
    return 0;
}

/* WebSocket 消息回调 */
static int on_ws_message(uvhttp_ws_connection_t* ws_conn,
                         const char* data,
                         size_t len,
                         int opcode) {
    (void)opcode;

    printf("Received message: %.*s\n", (int)len, data);

    /* 回显消息 */
    char response[256];
    snprintf(response, sizeof(response), "Echo: %.*s", (int)len, data);
    uvhttp_ws_send_text(ws_conn, response, strlen(response));

    g_message_count++;

    return 0;
}

/* WebSocket 关闭回调 */
static int on_ws_close(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    printf("WebSocket client disconnected\n");

    /* 显示当前连接数 */
    if (g_server) {
        int count = uvhttp_server_ws_get_connection_count(g_server);
        printf("Current connections: %d\n", count);
    }

    return 0;
}

/* HTTP 处理器（用于 WebSocket 升级） */
static int http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* url = uvhttp_request_get_url(request);
    const char* method = uvhttp_request_get_method(request);

    printf("HTTP %s %s\n", method, url);

    /* 返回简单的 HTML 页面 */
    const char* html =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>WebSocket Test</title>"
        "</head>"
        "<body>"
        "<h1>WebSocket Connection Management Test</h1>"
        "<p>Open browser console and connect to WebSocket:</p>"
        "<script>"
        "var ws = new WebSocket('ws://localhost:8080/ws');"
        "ws.onopen = function() { console.log('Connected'); };"
        "ws.onmessage = function(e) { console.log('Received:', e.data); };"
        "ws.send('Hello from browser!');"
        "</script>"
        "</body>"
        "</html>";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);

    return 0;
}

/* 信号处理函数 */
static void signal_handler(int signum) {
    (void)signum;
    printf("\nShutting down server...\n");

    if (g_server) {
        /* 关闭所有 WebSocket 连接 */
        uvhttp_server_ws_close_all(g_server, NULL);

        /* 停止服务器 */
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
    }

    exit(0);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    printf("=== WebSocket Connection Management Test ===\n\n");

    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to create event loop\n");
        return 1;
    }

    /* 创建服务器 */
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    /* 设置 HTTP 处理器 */
    uvhttp_server_set_handler(g_server, http_handler);

    /* 注册 WebSocket 处理器 */
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = on_ws_connect,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .user_data = NULL
    };

    uvhttp_error_t result = uvhttp_server_register_ws_handler(g_server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to register WebSocket handler: %s\n",
                uvhttp_error_string(result));
        return 1;
    }

    /* 启用连接管理 */
    result = uvhttp_server_ws_enable_connection_management(
        g_server,
        60,   /* 超时时间：60秒 */
        30    /* 心跳间隔：30秒 */
    );

    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to enable connection management: %s\n",
                uvhttp_error_string(result));
        return 1;
    }

    printf("Connection management enabled:\n");
    printf("  - Timeout: 60 seconds\n");
    printf("  - Heartbeat interval: 30 seconds\n\n");

    /* 启动服务器 */
    result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server started on http://0.0.0.0:8080\n");
    printf("WebSocket endpoint: ws://0.0.0.0:8080/ws\n");
    printf("Press Ctrl+C to stop\n\n");

    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_server_ws_disable_connection_management(g_server);
    uvhttp_server_free(g_server);

    printf("Server stopped. Total messages processed: %d\n", g_message_count);

    return 0;
}