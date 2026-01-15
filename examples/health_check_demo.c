/*
 * 健康检查示例
 * 演示如何使用 UVHTTP 的健康检查功能
 */

#include "uvhttp.h"
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    (void)sig;
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    exit(0);
}

/* 简单的请求处理器 */
int handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "Hello from UVHTTP with Health Check!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

/* 健康检查处理器声明（在 uvhttp_health.h 中） */
extern int uvhttp_health_handler(uvhttp_request_t* request, uvhttp_response_t* response);

int main() {
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();

    /* 创建服务器 */
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "无法创建服务器\n");
        return 1;
    }

    /* 创建路由器 */
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "无法创建路由器\n");
        uvhttp_server_free(g_server);
        return 1;
    }

    /* 添加路由 */
    uvhttp_router_add_route(router, "/", handler);

    /* 添加健康检查路由 */
    uvhttp_router_add_route(router, "/health", uvhttp_health_handler);

    /* 设置路由器 */
    uvhttp_server_set_router(g_server, router);

    /* 启用健康检查 */
    int result = uvhttp_server_enable_health_check(g_server, "/health");
    if (result != 0) {
        fprintf(stderr, "无法启用健康检查\n");
        uvhttp_server_free(g_server);
        uvhttp_router_free(router);
        return 1;
    }

    /* 启动服务器 */
    result = uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    if (result != 0) {
        fprintf(stderr, "无法启动服务器\n");
        uvhttp_server_free(g_server);
        uvhttp_router_free(router);
        return 1;
    }

    printf("服务器已启动，监听 0.0.0.0:8080\n");
    printf("访问 http://localhost:8080/health 查看健康状态\n");
    printf("访问 http://localhost:8080/ 查看主页\n");
    printf("按 Ctrl+C 停止服务器\n");

    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理 */
    uvhttp_server_free(g_server);
    uvhttp_router_free(router);
    uv_loop_close(loop);

    return 0;
}