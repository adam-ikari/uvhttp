/**
 * @file benchmark_router_minimal.c
 * @brief 最小化路由性能测试
 */

#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define TEST_ITERATIONS 10000

static uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int main(void) {
    printf("Minimal Router Performance Test\n");
    printf("===============================\n\n");
    
    uvhttp_router_t* router = NULL;
    if (uvhttp_router_new(&router) != UVHTTP_OK) {
        printf("Failed to create router\n");
        return 1;
    }
    
    printf("Router created successfully\n");
    
    /* 添加一个路由 */
    if (uvhttp_router_add_route(router, "/", (uvhttp_request_handler_t)1) != UVHTTP_OK) {
        printf("Failed to add route\n");
        uvhttp_router_free(router);
        return 1;
    }
    
    printf("Route added successfully\n");
    
    /* 测试查找 */
    printf("Testing %d lookups...\n", TEST_ITERATIONS);
    uint64_t total_time = 0;
    int found = 0;
    
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        uint64_t start = get_timestamp_ns();
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/", "GET");
        uint64_t end = get_timestamp_ns();
        
        if (handler != NULL) found++;
        total_time += (end - start);
    }
    
    printf("Found: %d/%d\n", found, TEST_ITERATIONS);
    printf("Avg time: %.2f ns\n", (double)total_time / TEST_ITERATIONS);
    printf("Ops/sec: %.0f\n", (TEST_ITERATIONS * 1e9) / total_time);
    
    uvhttp_router_free(router);
    printf("\nTest completed successfully!\n");
    
    return 0;
}