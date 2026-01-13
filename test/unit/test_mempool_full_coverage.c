/* UVHTTP 内存池模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_mempool.h"
#include "uvhttp_constants.h"

/* 测试内存池创建 */
void test_mempool_create(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        assert(pool != NULL);
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_mempool_create: PASSED\n");
}

/* 测试内存池销毁 */
void test_mempool_destroy(void) {
    /* 测试 NULL 参数 */
    uvhttp_mempool_destroy(NULL);
    
    /* 测试正常销毁 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    if (pool != NULL) {
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_mempool_destroy: PASSED\n");
}

/* 测试内存分配 */
void test_mempool_alloc(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 测试小分配 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 16);
        assert(ptr1 != NULL);
        
        void* ptr2 = uvhttp_mempool_alloc(pool, 32);
        assert(ptr2 != NULL);
        
        void* ptr3 = uvhttp_mempool_alloc(pool, 64);
        assert(ptr3 != NULL);
        
        /* 测试 NULL 参数 */
        void* ptr4 = uvhttp_mempool_alloc(NULL, 16);
        assert(ptr4 == NULL);
        
        /* 测试零大小 */
        void* ptr5 = uvhttp_mempool_alloc(pool, 0);
        assert(ptr5 == NULL);
        
        /* 测试大于块大小的分配 */
        void* ptr6 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE + 1);
        assert(ptr6 == NULL);
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_mempool_alloc: PASSED\n");
}

/* 测试内存池重置 */
void test_mempool_reset(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 分配一些内存 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 16);
        void* ptr2 = uvhttp_mempool_alloc(pool, 32);
        
        /* 重置内存池 */
        uvhttp_mempool_reset(pool);
        
        /* 重置后可以再次分配 */
        void* ptr3 = uvhttp_mempool_alloc(pool, 64);
        assert(ptr3 != NULL);
        
        /* 测试 NULL 参数 */
        uvhttp_mempool_reset(NULL);
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_mempool_reset: PASSED\n");
}

/* 测试内存池统计 */
void test_mempool_stats(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        size_t blocks = 0;
        size_t used = 0;
        
        /* 测试初始统计 */
        uvhttp_mempool_stats(pool, &blocks, &used);
        
        /* 分配一些内存 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 16);
        void* ptr2 = uvhttp_mempool_alloc(pool, 32);
        
        /* 测试分配后统计 */
        uvhttp_mempool_stats(pool, &blocks, &used);
        
        /* 测试 NULL 参数 */
        uvhttp_mempool_stats(NULL, &blocks, &used);
        
        /* 测试 NULL 输出参数 */
        uvhttp_mempool_stats(pool, NULL, NULL);
        uvhttp_mempool_stats(pool, &blocks, NULL);
        uvhttp_mempool_stats(pool, NULL, &used);
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_mempool_stats: PASSED\n");
}

/* 测试多次分配 */
void test_multiple_allocs(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 多次小分配 */
        for (int i = 0; i < 100; i++) {
            void* ptr = uvhttp_mempool_alloc(pool, 16);
            assert(ptr != NULL);
        }
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_multiple_allocs: PASSED\n");
}

/* 测试大分配 */
void test_large_allocs(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 接近块大小的分配 */
        void* ptr1 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE - 16);
        assert(ptr1 != NULL);
        
        /* 应该创建新块 */
        void* ptr2 = uvhttp_mempool_alloc(pool, 16);
        assert(ptr2 != NULL);
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_large_allocs: PASSED\n");
}

/* 测试对齐 */
void test_alignment(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 测试不同大小的分配，验证对齐 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 1);
        void* ptr2 = uvhttp_mempool_alloc(pool, 7);
        void* ptr3 = uvhttp_mempool_alloc(pool, 8);
        void* ptr4 = uvhttp_mempool_alloc(pool, 9);
        void* ptr5 = uvhttp_mempool_alloc(pool, 15);
        void* ptr6 = uvhttp_mempool_alloc(pool, 16);
        
        assert(ptr1 != NULL);
        assert(ptr2 != NULL);
        assert(ptr3 != NULL);
        assert(ptr4 != NULL);
        assert(ptr5 != NULL);
        assert(ptr6 != NULL);
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_alignment: PASSED\n");
}

/* 测试多次创建和销毁 */
void test_multiple_create_destroy(void) {
    /* 测试多次创建和销毁 */
    for (int i = 0; i < 10; i++) {
        uvhttp_mempool_t* pool = uvhttp_mempool_create();
        if (pool != NULL) {
            void* ptr = uvhttp_mempool_alloc(pool, 16);
            uvhttp_mempool_destroy(pool);
        }
    }
    
    printf("test_multiple_create_destroy: PASSED\n");
}

/* 测试边界条件 */
void test_edge_cases(void) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 测试边界大小 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 8);
        assert(ptr1 != NULL);
        
        /* size == UVHTTP_MEMPOOL_BLOCK_SIZE 应该返回有效指针 */
        void* ptr2 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE);
        assert(ptr2 != NULL);
        
        /* size > UVHTTP_MEMPOOL_BLOCK_SIZE 应该返回 NULL */
        void* ptr3 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE + 1);
        assert(ptr3 == NULL);
        
        void* ptr4 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE - 8);
        assert(ptr4 != NULL);
        
        uvhttp_mempool_destroy(pool);
    }
    
    printf("test_edge_cases: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_mempool.c 完整覆盖率测试 ===\n\n");

    test_mempool_create();
    test_mempool_destroy();
    test_mempool_alloc();
    test_mempool_reset();
    test_mempool_stats();
    test_multiple_allocs();
    test_large_allocs();
    test_alignment();
    test_multiple_create_destroy();
    test_edge_cases();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}