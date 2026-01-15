/*
 * WebSocket 集成测试服务器
 * 测试 WebSocket 握手后的连接创建和数据处理
 */

#include "uvhttp.h"
#include "uvhttp_websocket_native.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WebSocket 消息回调 */
static int on_ws_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    (void)opcode;

    printf("[WebSocket] 收到消息: %.*s\n", (int)len, data);

    /* 回显消息 */
    char response[256];
    snprintf(response, sizeof(response), "Echo: %.*s", (int)len, data);
    uvhttp_ws_send_text(ws_conn, response, strlen(response));

    return 0;
}

/* WebSocket 关闭回调 */
static int on_ws_close(uvhttp_ws_connection_t* ws_conn, int code, const char* reason) {
    printf("[WebSocket] 连接关闭: code=%d, reason=%s\n", code, reason);
    return 0;
}

/* WebSocket 错误回调 */
static int on_ws_error(uvhttp_ws_connection_t* ws_conn, int error_code, const char* error_msg) {
    printf("[WebSocket] 错误: code=%d, msg=%s\n", error_code, error_msg);
    return 0;
}

/* HTTP 请求处理器 */
static int http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* url = uvhttp_request_get_url(request);
    printf("[HTTP] 收到请求: %s\n", url);

    /* 返回简单的 HTML 页面 */
    const char* html = "<html><body><h1>WebSocket 测试服务器</h1>"
                       "<p>WebSocket 端点: /ws</p>"
                       "<script>var ws=new WebSocket('ws://'+location.host+'/ws');"
                       "ws.onmessage=function(e){console.log(e.data);};"
                       "ws.onopen=function(){ws.send('Hello WebSocket!');};</script>"
                       "</body></html>";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, html, strlen(html));
    uvhttp_response_send(response);

    return 0;
}

int main(int argc, char** argv) {
    const char* host = "0.0.0.0";
    int port = 8080;

    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }

    printf("启动 WebSocket 集成测试服务器...\n");
    printf("监听地址: http://%s:%d\n", host, port);
    printf("WebSocket 端点: ws://%s:%d/ws\n", host, port);
    printf("按 Ctrl+C 停止服务器\n");

    /* 创建服务器 */
    uvhttp_server_builder_t* server = uvhttp_server_create(host, port);
    if (!server) {
        fprintf(stderr, "无法创建服务器\n");
        return 1;
    }

    /* 添加 HTTP 路由 */
    uvhttp_get(server, "/", http_handler);

    /* 注册 WebSocket 处理器 */
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = NULL,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .user_data = NULL
    };

    uvhttp_error_t result = uvhttp_server_register_ws_handler(server->server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法注册 WebSocket 处理器: %d\n", result);
        uvhttp_server_simple_free(server);
        return 1;
    }

    /* 运行服务器 */
    int run_result = uvhttp_server_run(server);

    /* 清理 */
    uvhttp_server_simple_free(server);

    return run_result;
}
