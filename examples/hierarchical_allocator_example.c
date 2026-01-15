#include <time.h>
/*
 * UVHTTP 分层内存分配器使用示例
 * 
 * 本示例展示了如何使用分层内存分配器（mimalloc + 内存池混合模式）
 * 来优化内存分配性能。
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp_hierarchical_allocator.h"

/* 示例1：基本使用 */
void example1_basic_usage(void) {
    printf("\n=== 示例1：基本使用 ===\n");
    
    /* 创建分层分配器 */
    uvhttp_halloc_t* alloc = uvhttp_halloc_create(NULL);
    if (!alloc) {
        fprintf(stderr, "创建分配器失败\n");
        return;
    }
    
    /* 分配小对象（使用内存池） */
    void* small_ptr = uvhttp_halloc_alloc(alloc, 64);
    if (small_ptr) {
        printf("分配小对象成功: %p (64 字节)\n", small_ptr);
        strcpy((char*)small_ptr, "Hello, World!");
        printf("内容: %s\n", (char*)small_ptr);
        uvhttp_halloc_free(alloc, small_ptr);
    }
    
    /* 分配大对象（使用 mimalloc 或系统分配器） */
    void* large_ptr = uvhttp_halloc_alloc(alloc, 8192);
    if (large_ptr) {
        printf("分配大对象成功: %p (8192 字节)\n", large_ptr);
        memset(large_ptr, 0xAB, 8192);
        uvhttp_halloc_free(alloc, large_ptr);
    }
    
    /* 销毁分配器 */
    uvhttp_halloc_destroy(alloc);
    printf("分配器已销毁\n");
}

/* 示例2：自定义配置 */
void example2_custom_config(void) {
    printf("\n=== 示例2：自定义配置 ===\n");
    
    /* 配置分配器 */
    uvhttp_halloc_config_t config;
    uvhttp_halloc_get_default_config(&config);
    
    /* 修改配置 */
    config.allocator_type = UVHTTP_ALLOCATOR_TYPE_MIMALLOC;
    config.small_size_threshold = 1024;  /* 小对象阈值 1KB */
    config.pool_block_size = 8192;       /* 内存池块大小 8KB */
    config.max_pool_blocks = 500;        /* 最大内存池块数 */
    config.enable_stats = true;           /* 启用统计 */
    
    /* 创建分配器 */
    uvhttp_halloc_t* alloc = uvhttp_halloc_create(&config);
    if (!alloc) {
        fprintf(stderr, "创建分配器失败\n");
        return;
    }
    
    printf("配置:\n");
    printf("  小对象阈值: %zu 字节\n", config.small_size_threshold);
    printf("  内存池块大小: %zu 字节\n", config.pool_block_size);
    printf("  最大内存池块数: %zu\n", config.max_pool_blocks);
    printf("  统计: %s\n", config.enable_stats ? "启用" : "禁用");
    
    /* 分配一些内存 */
    for (int i = 0; i < 10; i++) {
        void* ptr = uvhttp_halloc_alloc(alloc, 256);
        if (ptr) {
            uvhttp_halloc_free(alloc, ptr);
        }
    }
    
    /* 获取统计信息 */
    uvhttp_halloc_stats_t stats;
    uvhttp_halloc_get_stats(alloc, &stats);
    
    printf("\n统计信息:\n");
    printf("  总分配次数: %zu\n", stats.total_allocations);
    printf("  总释放次数: %zu\n", stats.total_deallocations);
    printf("  内存池分配: %zu 次 (%zu 字节)\n", stats.pool_allocations, stats.pool_bytes_used);
    printf("  大对象分配: %zu 次 (%zu 字节)\n", stats.large_allocations, stats.large_bytes_used);
    
    uvhttp_halloc_destroy(alloc);
}

/* 示例3：使用全局分配器 */
void example3_global_allocator(void) {
    printf("\n=== 示例3：使用全局分配器 ===\n");
    
    /* 初始化全局分配器 */
    int result = uvhttp_halloc_global_init(NULL);
    if (result != 0) {
        fprintf(stderr, "初始化全局分配器失败\n");
        return;
    }
    
    printf("全局分配器已初始化\n");
    
    /* 使用全局分配器 */
    void* ptr = uvhttp_halloc_alloc(g_uvhttp_halloc, 512);
    if (ptr) {
        printf("使用全局分配器分配内存: %p\n", ptr);
        uvhttp_halloc_free(g_uvhttp_halloc, ptr);
    }
    
    /* 清理全局分配器 */
    uvhttp_halloc_global_cleanup();
    printf("全局分配器已清理\n");
}

/* 示例4：性能对比 */
void example4_performance_comparison(void) {
    printf("\n=== 示例4：性能对比 ===\n");
    
    const int iterations = 10000;
    const int small_size = 64;
    const int large_size = 4096;
    
    /* 测试分层分配器 */
    uvhttp_halloc_t* alloc = uvhttp_halloc_create(NULL);
    if (alloc) {
        printf("测试分层分配器...\n");
        
        /* 小对象分配 */
        clock_t start = clock();
        for (int i = 0; i < iterations; i++) {
            void* ptr = uvhttp_halloc_alloc(alloc, small_size);
            if (ptr) uvhttp_halloc_free(alloc, ptr);
        }
        clock_t end = clock();
        double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
        printf("  小对象分配 (%d 次): %.2f ms\n", iterations, time);
        
        /* 大对象分配 */
        start = clock();
        for (int i = 0; i < iterations; i++) {
            void* ptr = uvhttp_halloc_alloc(alloc, large_size);
            if (ptr) uvhttp_halloc_free(alloc, ptr);
        }
        end = clock();
        time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
        printf("  大对象分配 (%d 次): %.2f ms\n", iterations, time);
        
        uvhttp_halloc_destroy(alloc);
    }
    
    /* 测试系统分配器 */
    printf("\n测试系统分配器...\n");
    
    /* 小对象分配 */
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        void* ptr = malloc(small_size);
        if (ptr) free(ptr);
    }
    clock_t end = clock();
    double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  小对象分配 (%d 次): %.2f ms\n", iterations, time);
    
    /* 大对象分配 */
    start = clock();
    for (int i = 0; i < iterations; i++) {
        void* ptr = malloc(large_size);
        if (ptr) free(ptr);
    }
    end = clock();
    time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  大对象分配 (%d 次): %.2f ms\n", iterations, time);
}

/* 示例5：内存池优势 */
void example5_mempool_advantage(void) {
    printf("\n=== 示例5：内存池优势 ===\n");
    
    uvhttp_halloc_config_t config;
    uvhttp_halloc_get_default_config(&config);
    config.small_size_threshold = 256;
    config.enable_stats = true;
    
    uvhttp_halloc_t* alloc = uvhttp_halloc_create(&config);
    if (!alloc) {
        fprintf(stderr, "创建分配器失败\n");
        return;
    }
    
    /* 分配大量小对象 */
    const int count = 1000;
    void* ptrs[count];
    
    printf("分配 %d 个小对象...\n", count);
    for (int i = 0; i < count; i++) {
        ptrs[i] = uvhttp_halloc_alloc(alloc, 64);
    }
    
    /* 查看统计 */
    uvhttp_halloc_stats_t stats;
    uvhttp_halloc_get_stats(alloc, &stats);
    
    printf("\n内存池统计:\n");
    printf("  内存池分配: %zu 次\n", stats.pool_allocations);
    printf("  大对象分配: %zu 次\n", stats.large_allocations);
    printf("  内存池使用: %zu 字节\n", stats.pool_bytes_used);
    
    /* 释放所有对象 */
    for (int i = 0; i < count; i++) {
        if (ptrs[i]) {
            uvhttp_halloc_free(alloc, ptrs[i]);
        }
    }
    
    uvhttp_halloc_destroy(alloc);
}

int main(void) {
    printf("========================================\n");
    printf("UVHTTP 分层内存分配器使用示例\n");
    printf("========================================");
    
    example1_basic_usage();
    example2_custom_config();
    example3_global_allocator();
    example4_performance_comparison();
    example5_mempool_advantage();
    
    printf("\n========================================\n");
    printf("所有示例执行完成\n");
    printf("========================================\n");
    
    return 0;
}