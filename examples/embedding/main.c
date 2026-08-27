/**
 * @file main.c
 * @brief 最小嵌入式服务器 —— 展示如何在自己的 CMake 项目中集成 uvhttp。
 *
 * 这是为「新嵌入者」准备的最小可运行示例，完整走一遍嵌入式接入流程：
 *   1. 创建 libuv 事件循环
 *   2. 创建 uvhttp 服务器与路由器
 *   3. 注册一个路由，返回 hello world
 *   4. 监听端口并运行事件循环
 *   5. 收到 SIGINT/SIGTERM 时优雅退出
 *
 * 构建与运行说明见本目录 README.md，
 * 完整接入指南见 docs/guide/EMBEDDING_GUIDE.md（中文：docs/zh/guide/EMBEDDING_GUIDE.md）。
 */

#include "uvhttp.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 应用上下文：通过 loop 关联，避免全局变量（uvhttp 设计原则） */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

/* 信号句柄，用于优雅退出 */
static uv_signal_t g_sigint;
static uv_signal_t g_sigterm;
static app_context_t g_app; /* 示例简化：单实例用静态上下文 */

static void on_signal(uv_signal_t* handle, int signum) {
    (void)signum;
    /* 停止事件循环；uv_signal 会唤醒正在阻塞的 loop */
    uv_stop(handle->loop);
    printf("\nReceived signal, shutting down...\n");
    fflush(stdout);
}

static int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    const char* body = "Hello from embedded uvhttp!\n";
    uvhttp_response_set_body(res, body, strlen(body));
    return (int)uvhttp_response_send(res);
}

int main(int argc, char** argv) {
    /* 端口可通过命令行参数指定，默认 8080：
     *   ./embedding_server              # 0.0.0.0:8080
     *   ./embedding_server 8090         # 0.0.0.0:8090
     */
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    uv_loop_t* loop = uv_default_loop();
    if (loop == NULL) {
        fprintf(stderr, "error: cannot create libuv event loop\n");
        return 1;
    }

    /* 创建服务器 */
    if (uvhttp_server_new(loop, &g_app.server) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot create uvhttp server\n");
        return 1;
    }

    /* 创建路由器并注册路由 */
    if (uvhttp_router_new(&g_app.router) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot create router\n");
        uvhttp_server_free(g_app.server);
        return 1;
    }
    if (uvhttp_router_add_route(g_app.router, "/", hello_handler) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot add route /\n");
        uvhttp_router_free(g_app.router);
        uvhttp_server_free(g_app.server);
        return 1;
    }
    uvhttp_server_set_router(g_app.server, g_app.router);

    /* 注册优雅退出信号 */
    uv_signal_init(loop, &g_sigint);
    uv_signal_start(&g_sigint, on_signal, SIGINT);
    uv_signal_init(loop, &g_sigterm);
    uv_signal_start(&g_sigterm, on_signal, SIGTERM);

    /* 监听并启动 */
    if (uvhttp_server_listen(g_app.server, "0.0.0.0", port) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot listen on 0.0.0.0:%d\n", port);
        uvhttp_router_free(g_app.router);
        uvhttp_server_free(g_app.server);
        return 1;
    }

    printf("Embedded uvhttp listening on http://0.0.0.0:%d\n", port);
    printf("Test: curl http://localhost:%d/\n", port);

    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理：
     * 1. 先关闭本示例自建的 uv_signal 句柄——uvhttp_server_free 内部会
     *    跑 UV_RUN_ONCE 清理循环，若 loop 中仍残留活跃句柄会使其阻塞。
     * 2. uvhttp_server_free 会一并释放其持有的 router 与 tcp_handle。
     */
    uv_close((uv_handle_t*)&g_sigint, NULL);
    uv_close((uv_handle_t*)&g_sigterm, NULL);
    uvhttp_server_free(g_app.server);
    printf("Shutdown complete.\n");
    fflush(stdout);
    return 0;
}
