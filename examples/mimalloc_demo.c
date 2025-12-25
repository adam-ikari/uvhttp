/* mimalloc 分配器演示程序 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

/* 性能测试配置 */
#define TEST_ITERATIONS 100000
#define SMALL_SIZE 32
#define MEDIUM_SIZE 512
#define LARGE_SIZE 4096

/* 获取当前时间（微秒） */
static long long get_time_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000000LL + tv.tv_usec;
}

/* 内存分配性能测试 */
static void benchmark_allocation(const char* name, size_t size, int iterations) {
    printf("\n=== %s (size: %zu, iterations: %d) ===\n", name, size, iterations);
    
    long long start_time = get_time_us();
    void* ptrs[iterations];
    
    /* 分配测试 */
    for (int i = 0; i < iterations; i++) {
        ptrs[i] = UVHTTP_MALLOC(size);
        if (!ptrs[i]) {
            printf("Allocation failed at iteration %d\n", i);
            return;
        }
        /* 写入一些数据确保内存真正分配 */
        memset(ptrs[i], 0xAA, size);
    }
    
    long long alloc_time = get_time_us();
    
    /* 释放测试 */
    for (int i = 0; i < iterations; i++) {
        UVHTTP_FREE(ptrs[i]);
    }
    
    long long free_time = get_time_us();
    
    /* 计算性能指标 */
    double alloc_us_per_op = (double)(alloc_time - start_time) / iterations;
    double free_us_per_op = (double)(free_time - alloc_time) / iterations;
    double total_us_per_op = (double)(free_time - start_time) / iterations;
    
    printf("Allocation: %.3f us/op (%.0f ops/sec)\n", 
           alloc_us_per_op, 1000000.0 / alloc_us_per_op);
    printf("Free:       %.3f us/op (%.0f ops/sec)\n", 
           free_us_per_op, 1000000.0 / free_us_per_op);
    printf("Total:      %.3f us/op (%.0f ops/sec)\n", 
           total_us_per_op, 1000000.0 / total_us_per_op);
}

#if UVHTTP_HAS_MIMALLOC
/* 显示mimalloc统计信息 */
static void show_mimalloc_stats(void) {
    mi_process_stats_t stats;
    mi_process_stats(&stats);
    
    printf("\n=== mimalloc 统计信息 ===\n");
    printf("已分配: %zu bytes\n", stats.allocated);
    printf("已提交: %zu bytes\n", stats.committed);
    printf("已保留: %zu bytes\n", stats.reserved);
    printf("峰值使用: %zu bytes\n", stats.peak);
    printf("页面错误: %zu\n", stats.page_faults);
}
#endif

/* 内存泄漏测试 */
static void test_memory_leak(void) {
    printf("\n========================================\n");
    printf("内存泄漏测试\n");
    printf("========================================\n");
    
    /* 分配但不释放一些内存，然后释放 */
    void* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        size_t size = (i % 3 == 0) ? SMALL_SIZE : 
                     (i % 3 == 1) ? MEDIUM_SIZE : LARGE_SIZE;
        ptrs[i] = UVHTTP_MALLOC(size);
        if (ptrs[i]) {
            memset(ptrs[i], i, size);
        }
    }
    
    /* 释放一半的内存 */
    for (int i = 0; i < 500; i++) {
        UVHTTP_FREE(ptrs[i]);
    }
    
    /* 释放剩余内存 */
    for (int i = 500; i < 1000; i++) {
        UVHTTP_FREE(ptrs[i]);
    }
    
    printf("内存泄漏测试完成\n");
}

int main(void) {
    printf("UVHTTP mimalloc 分配器演示\n");
    printf("============================\n");
    
    /* 检查mimalloc支持 */
#if UVHTTP_HAS_MIMALLOC
    printf("✓ mimalloc 支持已启用\n");
#else
    printf("✗ mimalloc 支持未启用\n");
#endif
    
    /* 检查当前分配器 */
    printf("当前分配器: %s\n", uvhttp_allocator_name());
    
    /* 运行性能测试 */
    printf("\n========================================\n");
    printf("内存分配性能测试\n");
    printf("========================================\n");
    
    benchmark_allocation("小对象", SMALL_SIZE, TEST_ITERATIONS);
    benchmark_allocation("中等对象", MEDIUM_SIZE, TEST_ITERATIONS / 2);
    benchmark_allocation("大对象", LARGE_SIZE, TEST_ITERATIONS / 10);
    
    /* 内存泄漏测试 */
    test_memory_leak();
    
#if UVHTTP_HAS_MIMALLOC
    /* 最终统计 */
    printf("\n========================================\n");
    printf("最终统计信息\n");
    printf("========================================\n");
    show_mimalloc_stats();
    
    /* 触发垃圾回收 */
    mi_collect(true);
    printf("\n垃圾回收完成\n");
#endif
    
    printf("\n演示完成！\n");
    return 0;
}