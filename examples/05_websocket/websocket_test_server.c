/*
 * WebSocket Test Server
 * 简单的WebSocket回显服务器用于测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
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

// HTTP请求处理器（用于提供测试页面）
static int http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  // 避免未使用参数警告
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html; charset=utf-8");
    
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<meta charset='UTF-8'>"
        "<title>WebSocket Test</title>"
        "</head>"
        "<body>"
        "<h1>WebSocket Test Server</h1>"
        "<p>Use WebSocket client to connect to ws://localhost:8080/ws</p>"
        "<div>"
        "<input type='text' id='message' placeholder='输入消息'>"
        "<button onclick='sendMessage()'>发送</button>"
        "</div>"
        "<div id='output'></div>"
        "<script>"
        "var ws = new WebSocket('ws://localhost:8080/ws');"
        "ws.onopen = function() { "
        "  document.getElementById('output').innerHTML += '<p>已连接</p>';"
        "};"
        "ws.onmessage = function(e) { "
        "  document.getElementById('output').innerHTML += '<p>收到: ' + e.data + '</p>';"
        "};"
        "ws.onclose = function() { "
        "  document.getElementById('output').innerHTML += '<p>已断开</p>';"
        "};"
        "function sendMessage() {"
        "  var msg = document.getElementById('message').value;"
        "  if (msg) {"
        "    ws.send(msg);"
        "    document.getElementById('message').value = '';"
        "  }"
        "}"
        "</script>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);
    
    return 0;
}

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
    }
    
    printf("启动WebSocket测试服务器，端口: %d\n", port);

    // 库提供原语：server_new + 调用方自己的 router + take_router + listen
    // （v2.10 起 builder / 一键启动已移除，循环归调用方所有）
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    if (uvhttp_server_new(loop, &server) != UVHTTP_OK || !server) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }

    uvhttp_router_t* router = NULL;
    if (uvhttp_router_new(&router) != UVHTTP_OK) {
        fprintf(stderr, "路由器创建失败\n");
        uvhttp_server_free(server);
        return 1;
    }
    // 注册HTTP处理器（用于测试页面）
    uvhttp_router_add_route(router, "/", http_handler);
    // take_router 之后 router 归 server 所有
    if (uvhttp_server_take_router(server, router) != UVHTTP_OK) {
        fprintf(stderr, "路由器挂载失败\n");
        uvhttp_router_free(router);
        uvhttp_server_free(server);
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
    printf("HTTP URL: http://localhost:%d/\n", port);
    
    // 运行事件循环（循环归调用方）
    uv_run(loop, UV_RUN_DEFAULT);

    // 清理
    uvhttp_server_free(server);

    return 0;
}