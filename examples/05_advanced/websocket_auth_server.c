/*
 * WebSocket 认证示例服务器
 * 演示如何在应用层实现 WebSocket 认证功能
 * 
 * 认证功能已从核心库中剥离，由应用层实现
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

/* 认证配置 */
static uvhttp_ws_auth_config_t* g_auth_config = NULL;

/* 应用层认证检查函数 */
int check_websocket_auth(const char* client_ip, const char* token) {
    if (!g_auth_config) {
        return 1;  /* 没有配置认证，允许连接 */
    }

    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(g_auth_config, client_ip, token);
    
    if (result != UVHTTP_WS_AUTH_SUCCESS) {
        printf("认证失败: %s (IP: %s)\n", 
               uvhttp_ws_auth_result_string(result), client_ip);
        return 0;  /* 认证失败 */
    }

    printf("认证成功: IP=%s\n", client_ip);
    return 1;  /* 认证成功 */
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

    /* 创建认证配置 */
    g_auth_config = uvhttp_ws_auth_config_create();
    if (!g_auth_config) {
        fprintf(stderr, "无法创建认证配置\n");
        return 1;
    }

    /* 启用 Token 认证 */
    g_auth_config->enable_token_auth = 1;
    uvhttp_ws_auth_set_token_validator(g_auth_config, validate_token, NULL);

    /* 添加 IP 白名单（可选） */
    /* uvhttp_ws_auth_add_ip_to_whitelist(g_auth_config, "127.0.0.1"); */
    /* uvhttp_ws_auth_add_ip_to_whitelist(g_auth_config, "192.168.1.0/24"); */

    /* 添加 IP 黑名单（可选） */
    /* uvhttp_ws_auth_add_ip_to_blacklist(g_auth_config, "192.168.1.100"); */

    /* 创建服务器 */
    uvhttp_server_builder_t* server = uvhttp_server_create(host, port);
    if (!server) {
        fprintf(stderr, "无法创建服务器\n");
        uvhttp_ws_auth_config_destroy(g_auth_config);
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
        uvhttp_ws_auth_config_destroy(g_auth_config);
        return 1;
    }

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
        uvhttp_ws_auth_config_destroy(g_auth_config);
        return 1;
    }

    /* 运行事件循环 */
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_ws_auth_config_destroy(g_auth_config);
    uvhttp_server_simple_free(server);

    return 0;
}
