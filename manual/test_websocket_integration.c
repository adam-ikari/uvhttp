/*
 * WebSocket 集成测试服务器
 * 测试 WebSocket 握手后的连接创建和数据处理
 */

#include "uvhttp.h"
#include "uvhttp_websocket.h"
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
    
        /* 库提供原语，组合权在调用方：先建 server 与 router，装载路由后
         * 一次性 listening（v2.10 起 builder / 一键启动已移除）。 */
        uv_loop_t* loop = uv_default_loop();
        uvhttp_server_t* server = NULL;
        if (uvhttp_server_new(loop, &server) != UVHTTP_OK || !server) {
            fprintf(stderr, "无法创建服务器\n");
            return 1;
        }

        uvhttp_router_t* router = NULL;
        if (uvhttp_router_new(&router) != UVHTTP_OK) {
            fprintf(stderr, "无法创建路由器\n");
            uvhttp_server_free(server);
            return 1;
        }
        uvhttp_router_add_route(router, "/", http_handler);
        /* take_router 之后 router 归 server 所有，server_free 会释放它 */
        if (uvhttp_server_take_router(server, router) != UVHTTP_OK) {
            fprintf(stderr, "无法挂载路由器\n");
            uvhttp_router_free(router);
            uvhttp_server_free(server);
            return 1;
        }

    /* 注册 WebSocket 处理器 */
    uvhttp_ws_handler_t ws_handler = {
        .on_connect = NULL,
        .on_message = on_ws_message,
        .on_close = on_ws_close,
        .user_data = NULL
    };

    uvhttp_error_t result =
        uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法注册 WebSocket 处理器: %d\n", result);
        uvhttp_server_free(server);
        return 1;
    }

    /* 监听是调用方显式发起的动作，不是构造的副产品 */
    if (uvhttp_server_listen(server, host, port) != UVHTTP_OK) {
        fprintf(stderr, "无法监听 %s:%d\n", host, port);
        uvhttp_server_free(server);
        return 1;
    }

    /* 事件循环归调用方所有：库只注册句柄，不替你跑 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_server_free(server);

    return 0;
}
