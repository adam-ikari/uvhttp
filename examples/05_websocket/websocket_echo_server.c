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

    // 使用统一API创建服务器
    uvhttp_server_builder_t* server = NULL;
    if (!uvhttp_server_create("0.0.0.0", port, &server)) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }
    
    // 注册WebSocket处理器
    uvhttp_ws_handler_t ws_handler;
    ws_handler.on_connect = ws_connect_handler;
    ws_handler.on_message = ws_message_handler;
    ws_handler.on_close = ws_close_handler;
    
    uvhttp_server_register_ws_handler(server->server, "/ws", &ws_handler);
    
    printf("服务器运行中，按Ctrl+C停止...\n");
    printf("WebSocket URL: ws://localhost:%d/ws\n", port);
    
    // 运行服务器
    int run_result = uvhttp_server_run(server);
    
    // 清理
    uvhttp_server_simple_free(server);
    
    return run_result;
}