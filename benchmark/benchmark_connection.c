/**
 * @file benchmark_connection.c
 * @brief 连接管理性能基准测试
 * 
 * 这个程序测试 UVHTTP 服务器的连接管理性能，包括连接建立、保持和关闭。
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define TEST_DURATION 10  /* 测试时长（秒） */
#define PORT 18082

/* 连接统计 */
typedef struct {
    int total_connections;
    int active_connections;
    int successful_requests;
    int failed_requests;
} connection_stats_t;

static connection_stats_t g_conn_stats = {0};

/* 打印连接统计 */
static void print_connection_stats(void) {
    printf("=== 连接统计 ===\n");
    printf("总连接数: %d\n", g_conn_stats.total_connections);
    printf("活跃连接数: %d\n", g_conn_stats.active_connections);
    printf("成功请求数: %d\n", g_conn_stats.successful_requests);
    printf("失败请求数: %d\n", g_conn_stats.failed_requests);
    
    if (g_conn_stats.total_connections > 0) {
        double success_rate = (double)g_conn_stats.successful_requests / g_conn_stats.total_connections * 100.0;
        printf("成功率: %.2f%%\n", success_rate);
    }
    printf("\n");
}

/* 简单的请求处理器 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        g_conn_stats.failed_requests++;
        return -1;
    }
    
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_header(response, "Connection", "keep-alive");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);
    
    g_conn_stats.successful_requests++;
    return 0;
}

/* 运行连接管理基准测试 */
static void run_connection_benchmark(const char* test_name) {
    printf("=== %s ===\n", test_name);
    printf("测试时长: %d 秒\n", TEST_DURATION);
    printf("端口: %d\n", PORT);
    printf("\n");
    
    /* 重置统计 */
    memset(&g_conn_stats, 0, sizeof(g_conn_stats));
    
    /* 创建事件循环 */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "无法创建事件循环\n");
        return;
    }
    
    /* 创建服务器 */
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    if (result != UVHTTP_OK || !server) {
        fprintf(stderr, "无法创建服务器\n");
        return;
    }
    
    /* 创建路由 */
    uvhttp_router_t* router = NULL;
    result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法创建路由\n");
        uvhttp_server_free(server);
        return;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/", simple_handler);
    server->router = router;
    
    /* 启动服务器 */
    result = uvhttp_server_listen(server, "127.0.0.1", PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "无法启动服务器\n");
        uvhttp_server_free(server);
        return;
    }
    
    printf("服务器已启动在 http://127.0.0.1:%d\n", PORT);
    printf("请使用 wrk 或 ab 进行性能测试:\n");
    printf("  wrk -t4 -c100 -d%ds http://127.0.0.1:%d/\n", TEST_DURATION, PORT);
    printf("  ab -n 100000 -c 100 -k http://127.0.0.1:%d/\n", PORT);
    printf("\n");
    printf("等待 %d 秒...\n", TEST_DURATION);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 打印连接统计 */
    print_connection_stats();
    
    /* 清理 */
    uvhttp_server_free(server);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP 连接管理性能基准测试\n");
    printf("========================================\n\n");
    
    /* 连接管理测试 */
    run_connection_benchmark("连接管理基准测试");
    
    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");
    
    return 0;
}