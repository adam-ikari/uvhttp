/*
 * WebSocket Echo Server Example
 * 演示如何使用uvhttp创建WebSocket服务器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp.h"

// WebSocket消息处理回调
static int ws_message_handler(uvhttp_ws_connection_t* ws_conn,
                             const char* data,
                             size_t len,
                             int opcode) {
    (void)opcode;  // 避免未使用参数警告
    printf("收到WebSocket消息: %.*s\n", (int)len, data);

    // 回显消息
    uvhttp_server_ws_send(ws_conn, data, len);

    return 0;
}

// WebSocket连接建立回调
static int ws_connect_handler(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;  // 避免未使用参数警告
    printf("WebSocket连接建立\n");
    return 0;
}

// WebSocket连接关闭回调
static int ws_close_handler(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;  // 避免未使用参数警告
    printf("WebSocket连接关闭\n");
    return 0;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("启动WebSocket Echo服务器，端口: %d\n", port);

    // 库提供原语：server_new + take_router + listen（v2.10 起 builder /
    // 一键启动已移除，事件循环归调用方所有）
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    if (uvhttp_server_new(loop, &server) != UVHTTP_OK || !server) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }

    // 注册WebSocket处理器
    uvhttp_ws_handler_t ws_handler;
    ws_handler.on_connect = ws_connect_handler;
    ws_handler.on_message = ws_message_handler;
    ws_handler.on_close = ws_close_handler;

    if (uvhttp_server_register_ws_handler(server, "/ws", &ws_handler) !=
        UVHTTP_OK) {
        fprintf(stderr, "WebSocket处理器注册失败\n");
        uvhttp_server_free(server);
        return 1;
    }

    if (uvhttp_server_listen(server, "0.0.0.0", port) != UVHTTP_OK) {
        fprintf(stderr, "监听 %d 失败\n", port);
        uvhttp_server_free(server);
        return 1;
    }
    
    printf("服务器运行中，按Ctrl+C停止...\n");
    printf("WebSocket URL: ws://localhost:%d/ws\n", port);
    
    // 运行事件循环（循环归调用方）
    uv_run(loop, UV_RUN_DEFAULT);

    // 清理
    uvhttp_server_free(server);

    return 0;
}