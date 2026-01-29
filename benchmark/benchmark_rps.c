/**
 * @file benchmark_rps.c
 * @brief HTTP 性能基准测试服务器
 * 
 * 这个程序提供标准的 HTTP 服务器用于性能测试，支持多种测试场景：
 * - GET 请求（简单文本响应）
 * - POST 请求（JSON 响应）
 * - 静态文件服务
 * - 不同大小的响应
 * 
 * 使用 wrk 或 ab 工具进行压力测试：
 *   wrk -t4 -c100 -d30s http://127.0.0.1:18081/
 *   ab -n 100000 -c 100 -k http://127.0.0.1:18081/
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

#define DEFAULT_PORT 18081

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

/* 简单的 GET 请求处理器 */
static int get_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);
    
    return 0;
}

/* JSON 响应处理器 */
static int json_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    const char* body = "{\"status\":\"ok\",\"message\":\"Hello from UVHTTP\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "50");
    uvhttp_response_set_body(response, body, 50);
    uvhttp_response_send(response);
    
    return 0;
}

/* POST 请求处理器 */
static int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    const char* body = "{\"status\":\"received\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "23");
    uvhttp_response_set_body(response, body, 23);
    uvhttp_response_send(response);
    
    return 0;
}

/* 小响应处理器（1KB） */
static int small_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    char body[1024];
    memset(body, 'A', sizeof(body) - 1);
    body[sizeof(body) - 1] = '\0';
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "1023");
    uvhttp_response_set_body(response, body, 1023);
    uvhttp_response_send(response);
    
    return 0;
}

/* 中等响应处理器（10KB） */
static int medium_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    char body[10240];
    memset(body, 'B', sizeof(body) - 1);
    body[sizeof(body) - 1] = '\0';
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "10239");
    uvhttp_response_set_body(response, body, 10239);
    uvhttp_response_send(response);
    
    return 0;
}

/* 大响应处理器（100KB） */
static int large_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    char body[102400];
    memset(body, 'C', sizeof(body) - 1);
    body[sizeof(body) - 1] = '\0';
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "102399");
    uvhttp_response_set_body(response, body, 102399);
    uvhttp_response_send(response);
    
    return 0;
}

/* 健康检查处理器 */
static int health_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        return -1;
    }
    
    const char* body = "{\"status\":\"healthy\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "22");
    uvhttp_response_set_body(response, body, 22);
    uvhttp_response_send(response);
    
    return 0;
}

/* 打印使用说明 */
static void print_usage(const char* program_name, int port) {
    (void)program_name;  /* 避免未使用参数警告 */
    printf("========================================\n");
    printf("  UVHTTP 性能基准测试服务器\n");
    printf("========================================\n\n");
    
    printf("服务器已启动在 http://127.0.0.1:%d\n\n", port);
    
    printf("可用的测试端点:\n");
    printf("  GET  /              - 简单文本响应 (13 bytes)\n");
    printf("  GET  /json          - JSON 响应 (50 bytes)\n");
    printf("  POST /api           - POST 请求处理 (23 bytes)\n");
    printf("  GET  /small         - 小响应 (1KB)\n");
    printf("  GET  /medium        - 中等响应 (10KB)\n");
    printf("  GET  /large         - 大响应 (100KB)\n");
    printf("  GET  /health        - 健康检查 (22 bytes)\n\n");
    
    printf("性能测试示例:\n");
    printf("  # wrk 测试（推荐）\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/\n", port);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/json\n", port);
    printf("  wrk -t4 -c100 -d30s -s post.lua http://127.0.0.1:%d/api\n", port);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/small\n", port);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/medium\n", port);
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:%d/large\n", port);
    printf("\n");
    printf("  # ab 测试\n");
    printf("  ab -n 100000 -c 100 -k http://127.0.0.1:%d/\n", port);
    printf("  ab -n 100000 -c 100 -k http://127.0.0.1:%d/json\n", port);
    printf("\n");
    
    printf("不同并发级别测试:\n");
    printf("  wrk -t2 -c10 -d30s  http://127.0.0.1:%d/   (低并发)\n", port);
    printf("  wrk -t4 -c50 -d30s  http://127.0.0.1:%d/   (中等并发)\n", port);
    printf("  wrk -t8 -c200 -d30s http://127.0.0.1:%d/   (高并发)\n", port);
    printf("  wrk -t16 -c500 -d30s http://127.0.0.1:%d/  (极高并发)\n", port);
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
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", get_handler);
    uvhttp_router_add_route(router, "/json", json_handler);
    uvhttp_router_add_route(router, "/api", post_handler);
    uvhttp_router_add_route(router, "/small", small_handler);
    uvhttp_router_add_route(router, "/medium", medium_handler);
    uvhttp_router_add_route(router, "/large", large_handler);
    uvhttp_router_add_route(router, "/health", health_handler);
    
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
    print_usage(argv[0], port);
    
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