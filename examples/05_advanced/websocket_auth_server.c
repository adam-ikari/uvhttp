/*
 * WebSocket 认证示例服务器
 * 演示如何使用 Token 认证和 IP 白名单/黑名单
 */

#include "uvhttp.h"
#include "uvhttp_websocket_auth.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* 简单的 Token 验证函数 */
int validate_token(const char* token, void* user_data) {
    (void)user_data;

    /* 简单验证：Token 必须是 "secret123" 或 "admin456" */
    if (strcmp(token, "secret123") == 0 || strcmp(token, "admin456") == 0) {
        return 0;  /* 认证成功 */
    }

    return -1;  /* 认证失败 */
}

/* WebSocket 消息回调 */
int on_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    (void)opcode;

    printf("收到消息: %.*s\n", (int)len, data);

    /* 回显消息 */
    uvhttp_server_ws_send(ws_conn, data, len);

    return 0;
}

/* WebSocket 连接回调 */
int on_connect(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    printf("新的 WebSocket 连接已建立\n");
    return 0;
}

/* WebSocket 关闭回调 */
int on_close(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    printf("WebSocket 连接已关闭\n");
    return 0;
}

/* WebSocket 错误回调 */
int on_error(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg) {
    (void)ws_conn;
    printf("WebSocket 错误: %d - %s\n", error_code, error_msg);
    return 0;
}

int main(int argc, char* argv[]) {
    const char* host = "0.0.0.0";
    int port = 8080;

    /* 解析命令行参数 */
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    printf("启动 WebSocket 认证服务器...\n");
    printf("监听地址: %s:%d\n", host, port);

    /* 创建服务器 */
    uvhttp_server_builder_t* server = uvhttp_server_create(host, port);
    if (!server) {
        fprintf(stderr, "无法创建服务器\n");
        return 1;
    }

    /* 注册 WebSocket 处理器 */
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = on_connect,
        .on_message = on_message,
        .on_close = on_close,
        .on_error = on_error,
        .user_data = NULL
    };

    uvhttp_error_t result = uvhttp_server_register_ws_handler(server->server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "注册 WebSocket 处理器失败\n");
        return 1;
    }

    /* 启用 Token 认证 */
    result = uvhttp_server_ws_enable_token_auth(server->server, "/ws", validate_token, NULL);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启用 Token 认证失败\n");
        return 1;
    }

    /* 添加 IP 白名单（可选） */
    /* result = uvhttp_server_ws_add_ip_to_whitelist(server->server, "/ws", "127.0.0.1"); */
    /* if (result != UVHTTP_OK) { */
    /*     fprintf(stderr, "添加 IP 白名单失败\n"); */
    /*     return 1; */
    /* } */

    /* 添加 IP 黑名单（可选） */
    /* result = uvhttp_server_ws_add_ip_to_blacklist(server->server, "/ws", "192.168.1.100"); */
    /* if (result != UVHTTP_OK) { */
    /*     fprintf(stderr, "添加 IP 黑名单失败\n"); */
    /*     return 1; */
    /* } */

    printf("\n认证配置:\n");
    printf("  - Token 认证: 已启用\n");
    printf("  - 有效 Token: secret123, admin456\n");
    printf("  - Token 传递方式: 查询参数 (?token=xxx) 或 Authorization 头 (Bearer xxx)\n");
    printf("\n客户端连接示例:\n");
    printf("  ws://localhost:%d/ws?token=secret123\n", port);
    printf("  ws://localhost:%d/ws (带 Authorization: Bearer secret123 头)\n", port);
    printf("\n按 Ctrl+C 停止服务器\n");

    /* 启动服务器 */
    result = uvhttp_server_run(server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "启动服务器失败\n");
        return 1;
    }

    /* 运行事件循环 */
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_server_simple_free(server);

    return 0;
}