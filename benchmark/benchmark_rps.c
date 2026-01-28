/**
 * @file benchmark_rps.c
 * @brief RPS 性能基准测试
 * 
 * 这个程序测试 UVHTTP 服务器的 RPS（每秒请求数）性能。
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#define TEST_DURATION 10  /* 测试时长（秒） */
#define PORT 18081

/* RPS 统计 */
typedef struct {
    int total_requests;
    int successful_requests;
    int failed_requests;
    uint64_t start_time;
    uint64_t end_time;
} rps_stats_t;

static rps_stats_t g_rps_stats = {0};

/* 获取当前时间（毫秒） */
static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* 打印 RPS 统计 */
static void print_rps_stats(void) {
    printf("=== RPS 统计 ===\n");
    printf("总请求数: %d\n", g_rps_stats.total_requests);
    printf("成功请求数: %d\n", g_rps_stats.successful_requests);
    printf("失败请求数: %d\n", g_rps_stats.failed_requests);
    
    if (g_rps_stats.total_requests > 0) {
        double success_rate = (double)g_rps_stats.successful_requests / g_rps_stats.total_requests * 100.0;
        printf("成功率: %.2f%%\n", success_rate);
    }
    
    uint64_t duration_ms = g_rps_stats.end_time - g_rps_stats.start_time;
    if (duration_ms > 0) {
        double rps = (double)g_rps_stats.successful_requests / (duration_ms / 1000.0);
        printf("RPS: %.2f\n", rps);
    }
    printf("\n");
}

/* 简单的请求处理器 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    if (!response) {
        g_rps_stats.failed_requests++;
        return -1;
    }
    
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);
    
    g_rps_stats.successful_requests++;
    
    return 0;
}

/* 运行 RPS 基准测试 */
static void run_rps_benchmark(const char* test_name) {
    printf("=== %s ===\n", test_name);
    printf("测试时长: %d 秒\n", TEST_DURATION);
    printf("端口: %d\n", PORT);
    printf("\n");
    
    /* 重置统计 */
    memset(&g_rps_stats, 0, sizeof(g_rps_stats));
    g_rps_stats.start_time = get_timestamp_ms();
    
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
    
    g_rps_stats.end_time = get_timestamp_ms();
    
    /* 打印 RPS 统计 */
    print_rps_stats();
    
    /* 清理 */
    uvhttp_server_free(server);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP RPS 性能基准测试\n");
    printf("========================================\n\n");
    
    /* RPS 测试 */
    run_rps_benchmark("RPS 基准测试");
    
    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");
    
    return 0;
}