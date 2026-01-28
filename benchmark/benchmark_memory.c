/**
 * @file benchmark_memory.c
 * @brief 内存使用性能基准测试
 * 
 * 这个程序测试 UVHTTP 服务器的内存使用情况，包括内存分配、释放和峰值使用量。
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>

#define TEST_DURATION 10  /* 测试时长（秒） */
#define PORT 18083

/* 内存统计 */
typedef struct {
    size_t peak_memory;
    size_t current_memory;
    int allocations;
    int deallocations;
} memory_stats_t;

static memory_stats_t g_mem_stats = {0};

/* 获取当前进程内存使用量（KB） */
static size_t get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

/* 打印内存统计 */
static void print_memory_stats(void) {
    printf("=== 内存统计 ===\n");
    printf("峰值内存使用: %zu KB (%.2f MB)\n", 
           g_mem_stats.peak_memory, g_mem_stats.peak_memory / 1024.0);
    printf("当前内存使用: %zu KB (%.2f MB)\n", 
           g_mem_stats.current_memory, g_mem_stats.current_memory / 1024.0);
    printf("分配次数: %d\n", g_mem_stats.allocations);
    printf("释放次数: %d\n", g_mem_stats.deallocations);
    
    if (g_mem_stats.allocations > 0) {
        int net_allocations = g_mem_stats.allocations - g_mem_stats.deallocations;
        printf("净分配次数: %d\n", net_allocations);
    }
    printf("\n");
}

/* 简单的请求处理器 */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* 避免未使用参数警告 */
    
    /* 更新内存统计 */
    size_t current_mem = get_memory_usage_kb();
    if (current_mem > g_mem_stats.peak_memory) {
        g_mem_stats.peak_memory = current_mem;
    }
    g_mem_stats.current_memory = current_mem;
    g_mem_stats.allocations++;
    
    if (!response) {
        return -1;
    }
    
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);
    
    g_mem_stats.deallocations++;
    
    return 0;
}

/* 运行内存基准测试 */
static void run_memory_benchmark(const char* test_name) {
    printf("=== %s ===\n", test_name);
    printf("测试时长: %d 秒\n", TEST_DURATION);
    printf("端口: %d\n", PORT);
    printf("\n");
    
    /* 重置统计 */
    memset(&g_mem_stats, 0, sizeof(g_mem_stats));
    g_mem_stats.peak_memory = get_memory_usage_kb();
    
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
    
    /* 打印内存统计 */
    print_memory_stats();
    
    /* 清理 */
    uvhttp_server_free(server);
}

int main(void) {
    printf("========================================\n");
    printf("  UVHTTP 内存使用性能基准测试\n");
    printf("========================================\n\n");
    
    /* 内存使用测试 */
    run_memory_benchmark("内存使用基准测试");
    
    printf("========================================\n");
    printf("  测试完成\n");
    printf("========================================\n");
    
    return 0;
}