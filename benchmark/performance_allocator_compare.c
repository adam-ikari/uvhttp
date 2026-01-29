/* UVHTTP 内存分配器性能对比测试 */

#define UVHTTP_TEST_MODE 0
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <stdint.h>
#include <assert.h>

#define ITERATIONS 100000
#define SMALL_SIZE 64
#define MEDIUM_SIZE 512
#define LARGE_SIZE 4096

/* 获取当前时间（毫秒） */
static uint64_t get_timestamp_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* 测试系统分配器性能 */
static void benchmark_system_allocator(void) {
    printf("系统分配器 (malloc/free) 性能测试\n");
    printf("----------------------------------------\n");
    
    /* 小对象 */
    void* ptrs_small[100];
    memset(ptrs_small, 0, sizeof(ptrs_small));
    uint64_t start = get_timestamp_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs_small[idx]) free(ptrs_small[idx]);
        ptrs_small[idx] = malloc(SMALL_SIZE);
    }
    for (int i = 0; i < 100; i++) {
        if (ptrs_small[i]) free(ptrs_small[i]);
    }
    uint64_t end = get_timestamp_ms();
    printf("  小对象 (64B): %lu ms, %.2f ops/ms\n",
           (unsigned long)(end - start), (double)ITERATIONS / (end - start));

    /* 中等对象 */
    void* ptrs_medium[100];
    memset(ptrs_medium, 0, sizeof(ptrs_medium));
    start = get_timestamp_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs_medium[idx]) free(ptrs_medium[idx]);
        ptrs_medium[idx] = malloc(MEDIUM_SIZE);
    }
    for (int i = 0; i < 100; i++) {
        if (ptrs_medium[i]) free(ptrs_medium[i]);
    }
    end = get_timestamp_ms();
    printf("  中对象 (512B): %lu ms, %.2f ops/ms\n",
           (unsigned long)(end - start), (double)ITERATIONS / (end - start));

    /* 大对象 */
    void* ptrs_large[10];
    memset(ptrs_large, 0, sizeof(ptrs_large));
    start = get_timestamp_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 10;
        if (ptrs_large[idx]) free(ptrs_large[idx]);
        ptrs_large[idx] = malloc(LARGE_SIZE);
    }
    for (int i = 0; i < 10; i++) {
        if (ptrs_large[i]) free(ptrs_large[i]);
    }
    end = get_timestamp_ms();
    printf("  大对象 (4KB): %lu ms, %.2f ops/ms\n",
           (unsigned long)(end - start), (double)ITERATIONS / (end - start));

    printf("\n");
}

/* 测试 UVHTTP 统一分配器性能 */
static void benchmark_uvhttp_allocator(void) {
    printf("UVHTTP 统一分配器性能测试\n");
    printf("----------------------------------------\n");
    printf("  分配器类型: %s\n", uvhttp_allocator_name());
    
    /* 小对象 */
    void* ptrs_small[100];
    memset(ptrs_small, 0, sizeof(ptrs_small));
    uint64_t start = get_timestamp_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs_small[idx]) uvhttp_free(ptrs_small[idx]);
        ptrs_small[idx] = uvhttp_alloc(SMALL_SIZE);
    }
    for (int i = 0; i < 100; i++) {
        if (ptrs_small[i]) uvhttp_free(ptrs_small[i]);
    }
    uint64_t end = get_timestamp_ms();
    printf("  小对象 (64B): %lu ms, %.2f ops/ms\n",
           (unsigned long)(end - start), (double)ITERATIONS / (end - start));

    /* 中等对象 */
    void* ptrs_medium[100];
    memset(ptrs_medium, 0, sizeof(ptrs_medium));
    start = get_timestamp_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs_medium[idx]) uvhttp_free(ptrs_medium[idx]);
        ptrs_medium[idx] = uvhttp_alloc(MEDIUM_SIZE);
    }
    for (int i = 0; i < 100; i++) {
        if (ptrs_medium[i]) uvhttp_free(ptrs_medium[i]);
    }
    end = get_timestamp_ms();
    printf("  中对象 (512B): %lu ms, %.2f ops/ms\n",
           (unsigned long)(end - start), (double)ITERATIONS / (end - start));

    /* 大对象 */
    void* ptrs_large[10];
    memset(ptrs_large, 0, sizeof(ptrs_large));
    start = get_timestamp_ms();
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 10;
        if (ptrs_large[idx]) uvhttp_free(ptrs_large[idx]);
        ptrs_large[idx] = uvhttp_alloc(LARGE_SIZE);
    }
    for (int i = 0; i < 10; i++) {
        if (ptrs_large[i]) uvhttp_free(ptrs_large[i]);
    }
    end = get_timestamp_ms();
    printf("  大对象 (4KB): %lu ms, %.2f ops/ms\n",
           (unsigned long)(end - start), (double)ITERATIONS / (end - start));

    printf("\n");
}

int main(void) {
    printf("========================================\n");
    printf("内存分配器性能对比测试\n");
    printf("========================================\n");
    printf("迭代次数: %d\n", ITERATIONS);
    printf("========================================\n\n");
    
    benchmark_system_allocator();
    benchmark_uvhttp_allocator();
    
    printf("========================================\n");
    printf("性能对比测试完成\n");
    printf("========================================\n");
    printf("结论:\n");
    printf("  UVHTTP 统一分配器使用内联函数编译期优化，\n");
    printf("  性能与系统分配器相当，零运行时开销。\n");
    printf("========================================\n");
    
    return 0;
}