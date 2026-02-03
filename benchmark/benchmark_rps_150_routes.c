/**
 * @file benchmark_rps_150_routes.c
 * @brief HTTP 性能基准测试服务器 - 150 路由场景
 * 
 * 这个程序提供标准的 HTTP 服务器用于性能测试，支持 150 路由场景：
 * - 测试 Trie 模式下的路由性能
 * - 验证缓存优化对大量路由的影响
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define DEFAULT_PORT 18095

/* 应用上下文 - 使用 server->user_data 传递 */
typedef struct {
    volatile sig_atomic_t running;
} app_context_t;

/* 信号处理需要的静态变量（POSIX 允许） */
static uvhttp_server_t* g_signal_server = NULL;

/* 信号处理函数 */
static void signal_handler(int signum) {
    (void)signum;
    printf("\n收到停止信号，正在关闭服务器...\n");
    if (g_signal_server) {
        app_context_t* ctx = (app_context_t*)g_signal_server->user_data;
        if (ctx) {
            ctx->running = 0;
        }
        uvhttp_server_stop(g_signal_server);
    }
}

/* 通用的路由处理器 */
static int generic_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    const char* body = "Hello from UVHTTP!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "19");
    uvhttp_response_set_body(response, body, 19);
    uvhttp_response_send(response);
    
    return 0;
}

/* 打印使用说明 */
static void print_usage(int port) {
    printf("========================================\n");
    printf("  UVHTTP 性能基准测试服务器 (150 路由)\n");
    printf("========================================\n\n");
    printf("服务器已启动在 http://127.0.0.1:%d\n", port);
    printf("路由数量: 150\n\n");
    
    printf("性能测试示例:\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/api/resource1\n", port);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/api/resource50\n", port);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/api/resource100\n", port);
    printf("\n");
    printf("按 Ctrl+C 停止服务器\n\n");
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    
    /* 解析命令行参数 */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "无效的端口号: %s\n", argv[1]);
            return 1;
        }
    }
    
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "无法创建事件循环\n");
        return 1;
    }
    
    /* 创建应用上下文 */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "无法分配应用上下文\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->running = 1;
    
    /* 设置信号处理 */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    /* 创建服务器 */
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK || !server) {
        fprintf(stderr, "无法创建服务器: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* 设置服务器用户数据和信号处理需要的指针 */
    server->user_data = ctx;
    g_signal_server = server;
    
    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法创建路由: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_signal_server = NULL;
        return 1;
    }
    
    /* 添加 150 个路由（超过 HYBRID_THRESHOLD = 100） */
    printf("正在添加 150 个路由...\n");
    for (int i = 1; i <= 150; i++) {
        char path[256];
        snprintf(path, sizeof(path), "/api/resource%d", i);
        uvhttp_router_add_route(router, path, generic_handler);
        if (i % 50 == 0) {
            printf("已添加 %d 个路由\n", i);
        }
    }
    printf("路由添加完成\n");
    
    server->router = router;
    
    /* 启动服务器 */
    result = uvhttp_server_listen(server, "127.0.0.1", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法启动服务器: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_signal_server = NULL;
        return 1;
    }
    
    /* 打印使用说明 */
    print_usage(port);
    
    /* 运行事件循环 */
    while (ctx->running) {
        uv_run(loop, UV_RUN_ONCE);
    }
    
    /* 清理 */
    printf("\n正在关闭服务器...\n");
    uvhttp_server_free(server);
    uvhttp_free(ctx);
    g_signal_server = NULL;
    printf("服务器已关闭\n");
    
    return 0;
}