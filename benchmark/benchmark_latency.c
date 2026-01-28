/**
 * @file benchmark_latency.c
 * @brief 延迟性能基准测试
 * 
 * 这个程序测试 UVHTTP 服务器的延迟性能，包括平均延迟、P50、P95、P99 等。
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <math.h>
#include <inttypes.h>

#define TEST_DURATION 10  /* 测试时长（秒） */
#define PORT 18081
#define MAX_SAMPLES 100000

/* 延迟统计 */
typedef struct {
    uint64_t samples[MAX_SAMPLES];
    int count;
    uint64_t min;
    uint64_t max;
    double sum;
} latency_stats_t;

static latency_stats_t g_stats = {0};

/* 获取当前时间（微秒） */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* 添加延迟样本 */
static void add_latency_sample(uint64_t latency_us) {
    if (g_stats.count < MAX_SAMPLES) {
        g_stats.samples[g_stats.count++] = latency_us;
        g_stats.sum += latency_us;
        
        if (g_stats.count == 1) {
            g_stats.min = latency_us;
            g_stats.max = latency_us;
        } else {
            if (latency_us < g_stats.min) g_stats.min = latency_us;
            if (latency_us > g_stats.max) g_stats.max = latency_us;
        }
    }
}

/* 比较函数用于 qsort */
static int compare_uint64(const void* a, const void* b) {
    uint64_t val_a = *(const uint64_t*)a;
    uint64_t val_b = *(const uint64_t*)b;
    return (val_a > val_b) ? 1 : ((val_a < val_b) ? -1 : 0);
}

/* 计算百分位数 */
static uint64_t calculate_percentile(double percentile) {
    if (g_stats.count == 0) return 0;
    
    /* 使用 qsort 进行高效排序 */
    qsort(g_stats.samples, g_stats.count, sizeof(uint64_t), compare_uint64);
    
    int index = (int)(percentile * g_stats.count);
    if (index >= g_stats.count) index = g_stats.count - 1;
    
    return g_stats.samples[index];
}

/* 打印延迟统计 */
static void print_latency_stats(void) {
    if (g_stats.count == 0) {
        printf("没有延迟样本\n");
        return;
    }
    
    double avg = g_stats.sum / g_stats.count;
    uint64_t p50 = calculate_percentile(0.50);
    uint64_t p95 = calculate_percentile(0.95);
    uint64_t p99 = calculate_percentile(0.99);
    uint64_t p999 = calculate_percentile(0.999);
    
    printf("=== 延迟统计 ===\n");
    printf("样本数量: %d\n", g_stats.count);
    printf("平均延迟: %.2f μs (%.3f ms)\n", avg, avg / 1000.0);
    printf("最小延迟: %" PRIu64 " μs (%.3f ms)\n", g_stats.min, g_stats.min / 1000.0);
    printf("最大延迟: %" PRIu64 " μs (%.3f ms)\n", g_stats.max, g_stats.max / 1000.0);
    printf("P50 延迟: %" PRIu64 " μs (%.3f ms)\n", p50, p50 / 1000.0);
    printf("P95 延迟: %" PRIu64 " μs (%.3f ms)\n", p95, p95 / 1000.0);
    printf("P99 延迟: %" PRIu64 " μs (%.3f ms)\n", p99, p99 / 1000.0);
    printf("P999 延迟: %" PRIu64 " μs (%.3f ms)\n", p999, p999 / 1000.0);
    printf("\n");
}

/* 简单的请求处理器 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    uint64_t start = get_timestamp_us();
    
    if (!response) {
        return -1;
    }
    
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);
    
    uint64_t end = get_timestamp_us();
    add_latency_sample(end - start);
    
    return 0;
}

/* 运行延迟基准测试 */
static void run_latency_benchmark(const char* test_name) {
    printf("=== %s ===\n", test_name);
    printf("测试时长: %d 秒\n", TEST_DURATION);
    printf("端口: %d\n", PORT);
    printf("\n");
    
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
    printf("\n");
    printf("等待 %d 秒...\n", TEST_DURATION);
    
    /* 运行事件循环 */
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 打印延迟统计 */
    print_latency_stats();
    
    /* 清理 */
    uvhttp_server_free(server);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP 延迟性能基准测试\n");
    printf("========================================\n\n");
    
    /* 延迟测试 */
    run_latency_benchmark("延迟基准测试");
    
    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");
    
    return 0;
}
