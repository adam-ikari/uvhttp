/*
 * UVHTTP 统一内存分配器使用示例
 *
 * 本示例展示了如何使用 UVHTTP 的统一内存分配接口
 * （编译时可选 system / mimalloc / custom），以及与系统分配器的性能对比。
 *
 * 分配器类型在编译时通过 UVHTTP_ALLOCATOR_TYPE 选择：
 *   0 = system (默认)
 *   1 = mimalloc
 *   2 = custom (应用层实现 uvhttp_custom_alloc / uvhttp_custom_free 等)
 *
 * 对应头文件：include/uvhttp_allocator.h
 */

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp_allocator.h"

/* 示例1：基本使用 */
void example1_basic_usage(void) {
    printf("\n=== 示例1：基本使用 ===\n");

    printf("当前分配器: %s\n", uvhttp_allocator_name());

    /* 分配小对象 */
    void* small_ptr = uvhttp_alloc(64);
    if (small_ptr) {
        printf("分配小对象成功: %p (64 字节)\n", small_ptr);
        strcpy((char*)small_ptr, "Hello, World!");
        printf("内容: %s\n", (char*)small_ptr);
        uvhttp_free(small_ptr);
    }

    /* 分配大对象 */
    void* large_ptr = uvhttp_alloc(8192);
    if (large_ptr) {
        printf("分配大对象成功: %p (8192 字节)\n", large_ptr);
        memset(large_ptr, 0xAB, 8192);
        uvhttp_free(large_ptr);
    }

    printf("分配器为内联实现，无需显式销毁\n");
}

/* 示例2：编译时配置说明 */
void example2_compile_time_config(void) {
    printf("\n=== 示例2：编译时配置 ===\n");

    printf("当前分配器类型: UVHTTP_ALLOCATOR_TYPE = %d\n", UVHTTP_ALLOCATOR_TYPE);
    printf("  0 = system, 1 = mimalloc, 2 = custom\n");
    printf("当前分配器名称: %s\n", uvhttp_allocator_name());

    /* 分配一些内存并立即释放，演示统一接口 */
    for (int i = 0; i < 10; i++) {
        void* ptr = uvhttp_alloc(256);
        if (ptr) {
            uvhttp_free(ptr);
        }
    }

    printf("统一分配接口 (uvhttp_alloc/uvhttp_free) 零运行时开销\n");
}

/* 示例3：使用 calloc 与 realloc */
void example3_companion_allocators(void) {
    printf("\n=== 示例3：calloc 与 realloc ===\n");

    /* uvhttp_calloc 分配并清零 */
    void* ptr = uvhttp_calloc(16, 32);  /* 16 * 32 = 512 字节，内容清零 */
    if (ptr) {
        printf("使用 uvhttp_calloc 分配内存: %p (512 字节，已清零)\n", ptr);
        /* 验证内存已清零 */
        int zeroed = 1;
        for (int i = 0; i < 512; i++) {
            if (((unsigned char*)ptr)[i] != 0) { zeroed = 0; break; }
        }
        printf("  内存已清零: %s\n", zeroed ? "是" : "否");

        /* 扩展内存 */
        void* bigger = uvhttp_realloc(ptr, 4096);
        if (bigger) {
            printf("使用 uvhttp_realloc 扩展至 4096 字节: %p\n", bigger);
            uvhttp_free(bigger);
        } else {
            printf("uvhttp_realloc 失败，释放原内存\n");
            uvhttp_free(ptr);
        }
    }
}

/* 示例4：性能对比（统一分配器 vs 系统分配器） */
void example4_performance_comparison(void) {
    printf("\n=== 示例4：性能对比 ===\n");

    const int iterations = 10000;
    const int small_size = 64;
    const int large_size = 4096;

    /* 测试 UVHTTP 统一分配器 */
    printf("测试 UVHTTP 统一分配器 (%s)...\n", uvhttp_allocator_name());

    /* 小对象分配 */
    clock_t start = clock();
    for (int i = 0; i < iterations; i++) {
        void* ptr = uvhttp_alloc(small_size);
        if (ptr) uvhttp_free(ptr);
    }
    clock_t end = clock();
    double time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  小对象分配 (%d 次): %.2f ms\n", iterations, time);

    /* 大对象分配 */
    start = clock();
    for (int i = 0; i < iterations; i++) {
        void* ptr = uvhttp_alloc(large_size);
        if (ptr) uvhttp_free(ptr);
    }
    end = clock();
    time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    printf("  大对象分配 (%d 次): %.2f ms\n", iterations, time);

    /* 测试系统分配器 (malloc/free) */
    printf("\n测试系统分配器 (malloc/free)...\n");

    /* 小对象分配 */
    start = clock();
    for (int i = 0; i < iterations; i++) {
        void* ptr = malloc(small_size);
        if (ptr) free(ptr);
    }
    end = clock();
    time = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
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

/* 示例5：批量分配与释放 */
void example5_batch_alloc_free(void) {
    printf("\n=== 示例5：批量分配与释放 ===\n");

    const int count = 1000;
    void* ptrs[count];

    printf("分配 %d 个小对象...\n", count);
    for (int i = 0; i < count; i++) {
        ptrs[i] = uvhttp_alloc(64);
    }

    /* 释放所有对象 */
    int freed = 0;
    for (int i = 0; i < count; i++) {
        if (ptrs[i]) {
            uvhttp_free(ptrs[i]);
            freed++;
        }
    }
    printf("已释放 %d/%d 个对象\n", freed, count);
}

int main(void) {
    printf("========================================\n");
    printf("UVHTTP 统一内存分配器使用示例\n");
    printf("========================================\n");

    example1_basic_usage();
    example2_compile_time_config();
    example3_companion_allocators();
    example4_performance_comparison();
    example5_batch_alloc_free();

    printf("\n========================================\n");
    printf("所有示例执行完成\n");
    printf("========================================\n");

    return 0;
}
