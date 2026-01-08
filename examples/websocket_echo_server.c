/*
 * WebSocket Echo Server Example
 * 演示如何使用uvhttp创建WebSocket服务器
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_websocket_native.h"

static int on_ws_message(uvhttp_ws_connection_t* ws_conn, 
                        const char* data, 
                        size_t len, 
                        int opcode) {
    printf("收到WebSocket消息: %.*s (opcode: %d)\n", (int)len, data, opcode);
    
    // 回显消息
    uvhttp_ws_send(ws_conn, data, len, opcode);
    
    return 0;
}

static int on_ws_connect(uvhttp_ws_connection_t* ws_conn) {
    printf("WebSocket连接建立\n");
    return 0;
}

static int on_ws_close(uvhttp_ws_connection_t* ws_conn) {
    printf("WebSocket连接关闭\n");
    return 0;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("启动WebSocket Echo服务器，端口: %d\n", port);
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器配置
    uvhttp_server_config_t config;
    uvhttp_server_config_init(&config);
    config.port = port;
    config.host = "0.0.0.0";
    
    // 创建服务器
    uvhttp_server_t* server = uvhttp_server_create(&config);
    if (!server) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }
    
    // 注册WebSocket处理器
    uvhttp_ws_handler_t ws_handler;
    ws_handler.on_connect = on_ws_connect;
    ws_handler.on_message = on_ws_message;
    ws_handler.on_close = on_ws_close;
    
    uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
    
    printf("服务器运行中，按Ctrl+C停止...\n");
    
    // 运行服务器
    uvhttp_server_run(server);
    
    // 清理
    uvhttp_server_free(server);
    
    return 0;
}