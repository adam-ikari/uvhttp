/*
 * WebSocket 集成测试服务器
 * 测试 WebSocket 握手后的连接创建和数据处理
 */

#include "uvhttp.h"
#include "uvhttp_websocket_native.h"
#include "uvhttp_connection.h"
#include "uvhttp_context.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WebSocket 消息回调 */
static int on_ws_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    (void)opcode;

    /* 回显消息（测试中可以使用 NULL context） */
    char response[256];
    snprintf(response, sizeof(response), "Echo: %.*s", (int)len, data);
    uvhttp_ws_send_text(NULL, ws_conn, response, strlen(response));

    return 0;
}

/* WebSocket 关闭回调 */
static int on_ws_close(uvhttp_ws_connection_t* ws_conn) {
    (void)ws_conn;
    return 0;
}

/* HTTP 请求处理器 */
static int http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;

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
