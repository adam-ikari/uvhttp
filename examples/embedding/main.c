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

    /* 创建服务器 + 注册路由 + 监听：一次调用完成（原子构造）
     *
     * v2.10 之前这里需要 5 步（server_new / router_new / add_route /
     * take_router / listen），并且每个失败分支都要手写回滚。那段回滚代码
     * 在 listen 失败时会先 router_free 再 server_free —— 而 server 已经
     * 通过 take_router 接管了 router，于是二次释放（ASan 可复现的
     * double-free）。
     *
     * 现在只有一次调用：要么返回一个完全就绪的监听中的 server，
     * 要么什么都没创建，不需要任何清理。 */
    const uvhttp_route_t routes[] = {
        {"/", UVHTTP_ANY, hello_handler},
    };
    if (uvhttp_server_listen_routes(loop, routes, 1, "0.0.0.0", port,
                                    &g_app.server) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot start server on 0.0.0.0:%d\n", port);
        return 1;
    }
    /* router 由 server 内部创建并持有，不要（也不能）自己释放 */

    /* 注册优雅退出信号 */
    uv_signal_init(loop, &g_sigint);
    uv_signal_start(&g_sigint, on_signal, SIGINT);
    uv_signal_init(loop, &g_sigterm);
    uv_signal_start(&g_sigterm, on_signal, SIGTERM);

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
