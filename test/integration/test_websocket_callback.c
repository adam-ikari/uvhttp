/*
 * WebSocket 用户回调集成测试
 * 验证用户注册的回调函数能够正确接收和处理 WebSocket 事件
 */

#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* 测试状态 */
static int g_connect_called = 0;
static int g_message_count = 0;
static int g_close_called = 0;
static char g_last_message[256] = {0};

/* 用户回调函数 */
static int on_connect(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    g_connect_called++;
    printf("[测试] WebSocket 连接建立回调被调用\n");
    return 0;
}

static int on_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    (void)ws_conn;
    (void)opcode;

    g_message_count++;

    /* 保存最后一条消息 */
    size_t copy_len = len < sizeof(g_last_message) - 1 ? len : sizeof(g_last_message) - 1;
    memcpy(g_last_message, data, copy_len);
    g_last_message[copy_len] = '\0';

    printf("[测试] 收到 WebSocket 消息 #%d: %s\n", g_message_count, g_last_message);

    /* 回显消息 */
    uvhttp_server_ws_send(ws_conn, data, len);

    return 0;
}

static int on_close(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    g_close_called++;
    printf("[测试] WebSocket 连接关闭回调被调用\n");
    return 0;
}

/* HTTP 请求处理器（用于普通 HTTP 请求） */
static void http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* html_content =
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>WebSocket 回调测试</title>"
        "</head>"
        "<body>"
        "<h1>WebSocket 回调集成测试</h1>"
        "<h2>测试状态</h2>"
        "<ul>"
        "<li>连接回调调用次数: %d</li>"
        "<li>消息回调调用次数: %d</li>"
        "<li>关闭回调调用次数: %d</li>"
        "<li>最后一条消息: %s</li>"
        "</ul>"
        "<script>"
        "var ws = new WebSocket('ws://localhost:8080/ws');"
        "ws.onopen = function() { console.log('WebSocket 连接已建立'); ws.send('Hello from client!'); };"
        "ws.onmessage = function(e) { console.log('收到消息:', e.data); };"
        "ws.onclose = function() { console.log('WebSocket 连接已关闭'); };"
        "</script>"
        "</body>"
        "</html>";

    char html_body[1024];
    snprintf(html_body, sizeof(html_body), html_content,
             g_connect_called, g_message_count, g_close_called,
             g_last_message[0] ? g_last_message : "(无)");

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(response, html_body, strlen(html_body));
    uvhttp_response_send(response);
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("=== WebSocket 用户回调集成测试 ===\n\n");

    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 */
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "无法创建服务器\n");
        return 1;
    }

    /* 注册 WebSocket 处理器 */
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = on_connect,
        .on_message = on_message,
        .on_close = on_close,
        .user_data = NULL
    };

    uvhttp_error_t result = uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法注册 WebSocket 处理器: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("[测试] WebSocket 处理器已注册到路径: /ws\n");

    /* 设置 HTTP 处理器 */
    result = uvhttp_server_set_handler(server, http_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法设置 HTTP 处理器: %s\n", uvhttp_error_string(result));
        return 1;
    }

    /* 启动服务器 */
    result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法启动服务器: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("[测试] 服务器已启动，监听 0.0.0.0:8080\n");
    printf("[测试] 访问 http://localhost:8080 查看测试页面\n");
    printf("[测试] WebSocket 端点: ws://localhost:8080/ws\n");
    printf("[测试] 按 Ctrl+C 停止服务器\n\n");

    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_server_free(server);
    uv_loop_close(loop);

    printf("\n=== 测试结束 ===\n");
    printf("连接回调调用次数: %d\n", g_connect_called);
    printf("消息回调调用次数: %d\n", g_message_count);
    printf("关闭回调调用次数: %d\n", g_close_called);

    return 0;
}
