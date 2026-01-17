/* UVHTTP 统一内存分配器性能基准测试（简化版） */

#define UVHTTP_TEST_MODE 0  /* 生产模式，使用优化后的分配器 */
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

/* 测试小对象分配性能 */
static void benchmark_small_allocations(void) {
    printf("基准测试: 小对象分配 (%d 字节, %d 次迭代)\n", SMALL_SIZE, ITERATIONS);
    
    void* ptrs[100];
    memset(ptrs, 0, sizeof(ptrs));
    
    uint64_t start = get_timestamp_ms();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs[idx]) {
            uvhttp_free(ptrs[idx]);
        }
        ptrs[idx] = uvhttp_alloc(SMALL_SIZE);
    }
    
    /* 清理剩余指针 */
    for (int i = 0; i < 100; i++) {
        if (ptrs[i]) {
            uvhttp_free(ptrs[i]);
        }
    }
    
    uint64_t end = get_timestamp_ms();
    printf("  耗时: %lu ms\n", (unsigned long)(end - start));
    printf("  吞吐量: %.2f ops/ms\n", (double)ITERATIONS / (end - start));
    printf("\n");
}

/* 测试中等对象分配性能 */
static void benchmark_medium_allocations(void) {
    printf("基准测试: 中等对象分配 (%d 字节, %d 次迭代)\n", MEDIUM_SIZE, ITERATIONS);
    
    void* ptrs[100];
    memset(ptrs, 0, sizeof(ptrs));
    
    uint64_t start = get_timestamp_ms();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs[idx]) {
            uvhttp_free(ptrs[idx]);
        }
        ptrs[idx] = uvhttp_alloc(MEDIUM_SIZE);
    }
    
    /* 清理剩余指针 */
    for (int i = 0; i < 100; i++) {
        if (ptrs[i]) {
            uvhttp_free(ptrs[i]);
        }
    }
    
    uint64_t end = get_timestamp_ms();
    printf("  耗时: %lu ms\n", (unsigned long)(end - start));
    printf("  吞吐量: %.2f ops/ms\n", (double)ITERATIONS / (end - start));
    printf("\n");
}

/* 测试大对象分配性能 */
static void benchmark_large_allocations(void) {
    printf("基准测试: 大对象分配 (%d 字节, %d 次迭代)\n", LARGE_SIZE, ITERATIONS);
    
    void* ptrs[10];
    memset(ptrs, 0, sizeof(ptrs));
    
    uint64_t start = get_timestamp_ms();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 10;
        if (ptrs[idx]) {
            uvhttp_free(ptrs[idx]);
        }
        ptrs[idx] = uvhttp_alloc(LARGE_SIZE);
    }
    
    /* 清理剩余指针 */
    for (int i = 0; i < 10; i++) {
        if (ptrs[i]) {
            uvhttp_free(ptrs[i]);
        }
    }
    
    uint64_t end = get_timestamp_ms();
    printf("  耗时: %lu ms\n", (unsigned long)(end - start));
    printf("  吞吐量: %.2f ops/ms\n", (double)ITERATIONS / (end - start));
    printf("\n");
}

/* 测试混合大小分配性能 */
static void benchmark_mixed_allocations(void) {
    printf("基准测试: 混合大小分配 (%d 次迭代)\n", ITERATIONS);
    
    void* ptrs[100];
    memset(ptrs, 0, sizeof(ptrs));
    
    uint64_t start = get_timestamp_ms();
    
    for (int i = 0; i < ITERATIONS; i++) {
        int idx = i % 100;
        if (ptrs[idx]) {
            uvhttp_free(ptrs[idx]);
        }
        
        /* 随机选择大小 */
        size_t size;
        switch (i % 3) {
            case 0: size = SMALL_SIZE; break;
            case 1: size = MEDIUM_SIZE; break;
            case 2: size = LARGE_SIZE; break;
            default: size = SMALL_SIZE; break;
        }
        
        ptrs[idx] = uvhttp_alloc(size);
    }
    
    /* 清理剩余指针 */
    for (int i = 0; i < 100; i++) {
        if (ptrs[i]) {
            uvhttp_free(ptrs[i]);
        }
    }
    
    uint64_t end = get_timestamp_ms();
    printf("  耗时: %lu ms\n", (unsigned long)(end - start));
    printf("  吞吐量: %.2f ops/ms\n", (double)ITERATIONS / (end - start));
    printf("\n");
}

int main(void) {
    printf("========================================\n");
    printf("UVHTTP 统一内存分配器性能基准测试\n");
    printf("========================================\n");
    printf("分配器类型: %s\n", uvhttp_allocator_name());
    printf("迭代次数: %d\n", ITERATIONS);
    printf("========================================\n\n");
    
    benchmark_small_allocations();
    benchmark_medium_allocations();
    benchmark_large_allocations();
    benchmark_mixed_allocations();
    
    printf("========================================\n");
    printf("所有基准测试完成\n");
    printf("========================================\n");
    
    return 0;
}