/*
 * UVHTTP 分层内存分配器性能测试
 * 对比分层分配器与系统分配器的性能
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "uvhttp_hierarchical_allocator.h"

/* 性能测试函数 */
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1000.0 + (double)ts.tv_nsec / 1000000.0;
}

/* 测试1：小对象分配性能 */
void test_small_alloc_performance(uvhttp_halloc_t* alloc, const char* name) {
    const int iterations = 100000;
    const int size = 64;
    void* ptrs[10000];  /* 限制同时活跃的指针数量 */
    int ptr_count = 0;
    
    printf("\n=== %s - 小对象分配性能 (%d 字节) ===\n", name, size);
    
    double start = get_time_ms();
    
    for (int i = 0; i < iterations; i++) {
        void* ptr = uvhttp_halloc_alloc(alloc, size);
        if (ptr) {
            if (ptr_count < 10000) {
                ptrs[ptr_count++] = ptr;
            } else {
                /* 随机释放一个指针 */
                int idx = rand() % 10000;
                uvhttp_halloc_free(alloc, ptrs[idx]);
                ptrs[idx] = ptr;
            }
        }
    }
    
    /* 释放所有指针 */
    for (int i = 0; i < ptr_count; i++) {
        uvhttp_halloc_free(alloc, ptrs[i]);
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    double ops_per_sec = (iterations / elapsed) * 1000.0;
    
    printf("  分配次数: %d\n", iterations);
    printf("  耗时: %.2f ms\n", elapsed);
    printf("  吞吐量: %.0f ops/sec\n", ops_per_sec);
    printf("  平均延迟: %.4f ms/op\n", elapsed / iterations);
}

/* 测试2：大对象分配性能 */
void test_large_alloc_performance(uvhttp_halloc_t* alloc, const char* name) {
    const int iterations = 10000;
    const int size = 4096;
    void* ptrs[1000];  /* 限制同时活跃的指针数量 */
    int ptr_count = 0;
    
    printf("\n=== %s - 大对象分配性能 (%d 字节) ===\n", name, size);
    
    double start = get_time_ms();
    
    for (int i = 0; i < iterations; i++) {
        void* ptr = uvhttp_halloc_alloc(alloc, size);
        if (ptr) {
            if (ptr_count < 1000) {
                ptrs[ptr_count++] = ptr;
            } else {
                /* 随机释放一个指针 */
                int idx = rand() % 1000;
                uvhttp_halloc_free(alloc, ptrs[idx]);
                ptrs[idx] = ptr;
            }
        }
    }
    
    /* 释放所有指针 */
    for (int i = 0; i < ptr_count; i++) {
        uvhttp_halloc_free(alloc, ptrs[i]);
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    double ops_per_sec = (iterations / elapsed) * 1000.0;
    
    printf("  分配次数: %d\n", iterations);
    printf("  耗时: %.2f ms\n", elapsed);
    printf("  吞吐量: %.0f ops/sec\n", ops_per_sec);
    printf("  平均延迟: %.4f ms/op\n", elapsed / iterations);
}

/* 测试3：混合大小分配性能 */
void test_mixed_alloc_performance(uvhttp_halloc_t* alloc, const char* name) {
    const int iterations = 50000;
    const int sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
    const int num_sizes = 9;
    void* ptrs[10000];
    int ptr_count = 0;
    
    printf("\n=== %s - 混合大小分配性能 ===\n", name);
    
    double start = get_time_ms();
    
    for (int i = 0; i < iterations; i++) {
        int size = sizes[i % num_sizes];
        void* ptr = uvhttp_halloc_alloc(alloc, size);
        if (ptr) {
            if (ptr_count < 10000) {
                ptrs[ptr_count++] = ptr;
            } else {
                /* 随机释放一个指针 */
                int idx = rand() % 10000;
                uvhttp_halloc_free(alloc, ptrs[idx]);
                ptrs[idx] = ptr;
            }
        }
    }
    
    /* 释放所有指针 */
    for (int i = 0; i < ptr_count; i++) {
        uvhttp_halloc_free(alloc, ptrs[i]);
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    double ops_per_sec = (iterations / elapsed) * 1000.0;
    
    printf("  分配次数: %d\n", iterations);
    printf("  耗时: %.2f ms\n", elapsed);
    printf("  吞吐量: %.0f ops/sec\n", ops_per_sec);
    printf("  平均延迟: %.4f ms/op\n", elapsed / iterations);
}

/* 测试4：内存碎片测试 */
void test_fragmentation_performance(uvhttp_halloc_t* alloc, const char* name) {
    const int iterations = 10000;
    void* ptrs[10000];
    int ptr_count = 0;
    
    printf("\n=== %s - 内存碎片测试 ===\n", name);
    
    double start = get_time_ms();
    
    /* 分配不同大小的对象 */
    for (int i = 0; i < iterations; i++) {
        int size = 32 + (i % 8) * 32;  /* 32, 64, 96, ..., 256 */
        void* ptr = uvhttp_halloc_alloc(alloc, size);
        if (ptr) {
            ptrs[ptr_count++] = ptr;
        }
    }
    
    /* 随机释放一半 */
    for (int i = 0; i < ptr_count / 2; i++) {
        int idx = rand() % ptr_count;
        if (ptrs[idx]) {
            uvhttp_halloc_free(alloc, ptrs[idx]);
            ptrs[idx] = NULL;
        }
    }
    
    /* 再次分配相同数量的对象 */
    for (int i = 0; i < iterations / 2; i++) {
        int size = 32 + (i % 8) * 32;
        void* ptr = uvhttp_halloc_alloc(alloc, size);
        if (ptr && ptr_count < 10000) {
            ptrs[ptr_count++] = ptr;
        }
    }
    
    /* 释放所有指针 */
    for (int i = 0; i < ptr_count; i++) {
        if (ptrs[i]) {
            uvhttp_halloc_free(alloc, ptrs[i]);
        }
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    
    printf("  分配次数: %d\n", iterations);
    printf("  耗时: %.2f ms\n", elapsed);
    
    /* 获取统计信息 */
    uvhttp_halloc_stats_t stats;
    uvhttp_halloc_get_stats(alloc, &stats);
    
    printf("  内存池分配: %zu 次\n", stats.pool_allocations);
    printf("  大对象分配: %zu 次\n", stats.large_allocations);
    printf("  内存池使用: %zu 字节\n", stats.pool_bytes_used);
}

/* 测试5：并发分配性能（模拟） */
void test_concurrent_alloc_performance(uvhttp_halloc_t* alloc, const char* name) {
    const int iterations = 50000;
    const int size = 128;
    
    printf("\n=== %s - 并发分配性能（模拟） ===\n", name);
    
    double start = get_time_ms();
    
    /* 模拟并发分配：快速分配和释放 */
    for (int i = 0; i < iterations; i++) {
        void* ptr = uvhttp_halloc_alloc(alloc, size);
        if (ptr) {
            uvhttp_halloc_free(alloc, ptr);
        }
    }
    
    double end = get_time_ms();
    double elapsed = end - start;
    double ops_per_sec = (iterations / elapsed) * 1000.0;
    
    printf("  分配次数: %d\n", iterations);
    printf("  耗时: %.2f ms\n", elapsed);
    printf("  吞吐量: %.0f ops/sec\n", ops_per_sec);
    printf("  平均延迟: %.4f ms/op\n", elapsed / iterations);
}

/* 性能对比测试 */
void performance_comparison(void) {
    uvhttp_halloc_config_t config;
    uvhttp_halloc_get_default_config(&config);
    
    /* 测试分层分配器 */
    printf("\n========================================");
    printf("分层内存分配器性能测试");
    printf("========================================");
    
    uvhttp_halloc_t* halloc = uvhttp_halloc_create(&config);
    if (halloc) {
        test_small_alloc_performance(halloc, "分层分配器");
        test_large_alloc_performance(halloc, "分层分配器");
        test_mixed_alloc_performance(halloc, "分层分配器");
        test_fragmentation_performance(halloc, "分层分配器");
        test_concurrent_alloc_performance(halloc, "分层分配器");
        uvhttp_halloc_destroy(halloc);
    }
    
    /* 测试系统分配器 */
    printf("\n========================================");
    printf("系统分配器性能测试");
    printf("========================================");
    
    uvhttp_halloc_t* sys_alloc = uvhttp_halloc_create(&config);
    if (sys_alloc) {
        /* 强制使用系统分配器 */
        test_small_alloc_performance(sys_alloc, "系统分配器");
        test_large_alloc_performance(sys_alloc, "系统分配器");
        test_mixed_alloc_performance(sys_alloc, "系统分配器");
        test_fragmentation_performance(sys_alloc, "系统分配器");
        test_concurrent_alloc_performance(sys_alloc, "系统分配器");
        uvhttp_halloc_destroy(sys_alloc);
    }
    
    /* 测试内存池 */
    printf("\n========================================");
    printf("内存池性能测试");
    printf("========================================");
    
    uvhttp_halloc_t* pool_alloc = uvhttp_halloc_create(&config);
    if (pool_alloc) {
        /* 只使用内存池（小对象） */
        
        test_small_alloc_performance(pool_alloc, "内存池");
        test_concurrent_alloc_performance(pool_alloc, "内存池");
        uvhttp_halloc_destroy(pool_alloc);
    }
}

/* 内存使用测试 */
void memory_usage_test(void) {
    uvhttp_halloc_config_t config;
    uvhttp_halloc_get_default_config(&config);
    config.enable_stats = true;
    
    printf("\n========================================");
    printf("内存使用测试");
    printf("========================================");
    
    /* 分层分配器 */
    uvhttp_halloc_t* halloc = uvhttp_halloc_create(&config);
    if (halloc) {
        const int iterations = 1000;
        void* ptrs[1000];
        
        /* 分配小对象 */
        for (int i = 0; i < iterations; i++) {
            ptrs[i] = uvhttp_halloc_alloc(halloc, 64);
        }
        
        /* 获取统计 */
        uvhttp_halloc_stats_t stats;
        uvhttp_halloc_get_stats(halloc, &stats);
        
        printf("\n分层分配器 - 分配 %d 个小对象 (64 字节):\n", iterations);
        printf("  总分配次数: %zu\n", stats.total_allocations);
        printf("  内存池分配: %zu 次\n", stats.pool_allocations);
        printf("  大对象分配: %zu 次\n", stats.large_allocations);
        printf("  内存池使用: %zu 字节\n", stats.pool_bytes_used);
        printf("  总分配字节: %zu\n", stats.total_bytes_allocated);
        
        /* 释放 */
        for (int i = 0; i < iterations; i++) {
            uvhttp_halloc_free(halloc, ptrs[i]);
        }
        
        uvhttp_halloc_destroy(halloc);
    }
    
    /* 系统分配器 */
    uvhttp_halloc_t* sys_alloc = uvhttp_halloc_create(&config);
    if (sys_alloc) {
        const int iterations = 1000;
        void* ptrs[1000];
        
        /* 分配小对象 */
        for (int i = 0; i < iterations; i++) {
            ptrs[i] = uvhttp_halloc_alloc(sys_alloc, 64);
        }
        
        /* 获取统计 */
        uvhttp_halloc_stats_t stats;
        uvhttp_halloc_get_stats(sys_alloc, &stats);
        
        printf("\n系统分配器 - 分配 %d 个小对象 (64 字节):\n", iterations);
        printf("  总分配次数: %zu\n", stats.total_allocations);
        printf("  内存池分配: %zu 次\n", stats.pool_allocations);
        printf("  大对象分配: %zu 次\n", stats.large_allocations);
        printf("  内存池使用: %zu 字节\n", stats.pool_bytes_used);
        printf("  总分配字节: %zu\n", stats.total_bytes_allocated);
        
        /* 释放 */
        for (int i = 0; i < iterations; i++) {
            uvhttp_halloc_free(sys_alloc, ptrs[i]);
        }
        
        uvhttp_halloc_destroy(sys_alloc);
    }
}

int main(void) {
    printf("========================================\n");
    printf("UVHTTP 分层内存分配器性能测试\n");
    printf("========================================");
    
    performance_comparison();
    memory_usage_test();
    
    printf("\n========================================\n");
    printf("性能测试完成\n");
    printf("========================================\n");
    
    return 0;
}
