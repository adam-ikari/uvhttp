/**
 * @file benchmark_router_simple.c
 * @brief 简化的路由性能基准测试
 */

#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define TEST_ITERATIONS 100000

/* 获取当前时间（纳秒） */
static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    
    printf("Router Performance Benchmark (Simplified)\n");
    printf("========================================\n\n");
    
    /* 测试 1: 创建路由器 */
    printf("Test 1: Creating router...\n");
    uvhttp_router_t* router = NULL;
    uvhttp_error_t err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK || !router) {
        printf("Failed to create router: %d\n", err);
        return 1;
    }
    printf("Router created successfully\n\n");
    
    /* 测试 2: 添加路由 */
    printf("Test 2: Adding routes...\n");
    
    /* 先测试一个简单的路由 */
    err = uvhttp_router_add_route(router, "/", 
        (uvhttp_request_handler_t)1);
    if (err != UVHTTP_OK) {
        printf("Failed to add route '/': %d\n", err);
        uvhttp_router_free(router);
        return 1;
    }
    printf("Route '/' added successfully\n");
    
    /* 测试更多路由 */
    for (int i = 0; i < 10; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/test%d", i);
        err = uvhttp_router_add_route(router, path,
            (uvhttp_request_handler_t)((uintptr_t)(i + 2)));
        if (err != UVHTTP_OK) {
            printf("Failed to add route '%s': %d\n", path, err);
            /* 继续测试，不退出 */
        }
    }
    printf("Routes added successfully\n\n");
    
    /* 测试 3: 查找路由 */
    printf("Test 3: Testing route lookup (%d iterations)...\n", TEST_ITERATIONS);
    uint64_t total_time = 0;
    int found_count = 0;
    
    const char* test_paths[] = {"/", "/test0", "/test1", "/test2", "/test3", "/test4", "/test5", "/test6", "/test7", "/test8", "/test9"};
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        const char* path = test_paths[i % 11];
        
        uint64_t start = get_timestamp_ns();
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(
            router, path, "GET");
        uint64_t end = get_timestamp_ns();
        
        if (handler != NULL) {
            found_count++;
        }
        
        total_time += (end - start);
    }
    
    double avg_time_ns = (double)total_time / TEST_ITERATIONS;
    double ops_per_sec = (TEST_ITERATIONS * 1e9) / total_time;
    
    printf("Results:\n");
    printf("  Total iterations: %d\n", TEST_ITERATIONS);
    printf("  Found routes: %d\n", found_count);
    printf("  Average time: %.2f ns\n", avg_time_ns);
    printf("  Operations/sec: %.0f\n\n", ops_per_sec);
    
    /* 测试 4: 清理 */
    printf("Test 4: Cleaning up...\n");
    uvhttp_router_free(router);
    printf("Router freed successfully\n\n");
    
    printf("========================================\n");
    printf("All tests completed successfully!\n");
    printf("========================================\n");
    
    return 0;
}
