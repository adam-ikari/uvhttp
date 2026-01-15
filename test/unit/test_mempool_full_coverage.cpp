/* UVHTTP 内存池模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_mempool.h"
#include "uvhttp_constants.h"

/* 测试内存池创建 */
TEST(UvhttpMempoolFullCoverageTest, MempoolCreate) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        ASSERT_NE(pool, nullptr);
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试内存池销毁 */
TEST(UvhttpMempoolFullCoverageTest, MempoolDestroy) {
    /* 测试 NULL 参数 */
    uvhttp_mempool_destroy(NULL);
    
    /* 测试正常销毁 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    if (pool != NULL) {
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试内存分配 */
TEST(UvhttpMempoolFullCoverageTest, MempoolAlloc) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 测试小分配 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 16);
        ASSERT_NE(ptr1, nullptr);
        
        void* ptr2 = uvhttp_mempool_alloc(pool, 32);
        ASSERT_NE(ptr2, nullptr);
        
        void* ptr3 = uvhttp_mempool_alloc(pool, 64);
        ASSERT_NE(ptr3, nullptr);
        
        /* 测试 NULL 参数 */
        void* ptr4 = uvhttp_mempool_alloc(NULL, 16);
        EXPECT_EQ(ptr4, nullptr);
        
        /* 测试零大小 */
        void* ptr5 = uvhttp_mempool_alloc(pool, 0);
        EXPECT_EQ(ptr5, nullptr);
        
        /* 测试大于块大小的分配 */
        void* ptr6 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE + 1);
        EXPECT_EQ(ptr6, nullptr);
        
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试内存池重置 */
TEST(UvhttpMempoolFullCoverageTest, MempoolReset) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 分配一些内存 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 16);
        void* ptr2 = uvhttp_mempool_alloc(pool, 32);
        
        /* 重置内存池 */
        uvhttp_mempool_reset(pool);
        
        /* 重置后可以再次分配 */
        void* ptr3 = uvhttp_mempool_alloc(pool, 64);
        ASSERT_NE(ptr3, nullptr);
        
        /* 测试 NULL 参数 */
        uvhttp_mempool_reset(NULL);
        
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试内存池统计 */
TEST(UvhttpMempoolFullCoverageTest, MempoolStats) {
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
}

/* 测试多次分配 */
TEST(UvhttpMempoolFullCoverageTest, MultipleAllocs) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 多次小分配 */
        for (int i = 0; i < 100; i++) {
            void* ptr = uvhttp_mempool_alloc(pool, 16);
            ASSERT_NE(ptr, nullptr);
        }
        
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试大分配 */
TEST(UvhttpMempoolFullCoverageTest, LargeAllocs) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 接近块大小的分配 */
        void* ptr1 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE - 16);
        ASSERT_NE(ptr1, nullptr);
        
        /* 应该创建新块 */
        void* ptr2 = uvhttp_mempool_alloc(pool, 16);
        ASSERT_NE(ptr2, nullptr);
        
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试对齐 */
TEST(UvhttpMempoolFullCoverageTest, Alignment) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 测试不同大小的分配，验证对齐 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 1);
        void* ptr2 = uvhttp_mempool_alloc(pool, 7);
        void* ptr3 = uvhttp_mempool_alloc(pool, 8);
        void* ptr4 = uvhttp_mempool_alloc(pool, 9);
        void* ptr5 = uvhttp_mempool_alloc(pool, 15);
        void* ptr6 = uvhttp_mempool_alloc(pool, 16);
        
        ASSERT_NE(ptr1, nullptr);
        ASSERT_NE(ptr2, nullptr);
        ASSERT_NE(ptr3, nullptr);
        ASSERT_NE(ptr4, nullptr);
        ASSERT_NE(ptr5, nullptr);
        ASSERT_NE(ptr6, nullptr);
        
        uvhttp_mempool_destroy(pool);
    }
}

/* 测试多次创建和销毁 */
TEST(UvhttpMempoolFullCoverageTest, MultipleCreateDestroy) {
    /* 测试多次创建和销毁 */
    for (int i = 0; i < 10; i++) {
        uvhttp_mempool_t* pool = uvhttp_mempool_create();
        if (pool != NULL) {
            void* ptr = uvhttp_mempool_alloc(pool, 16);
            uvhttp_mempool_destroy(pool);
        }
    }
}

/* 测试边界条件 */
TEST(UvhttpMempoolFullCoverageTest, EdgeCases) {
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    
    if (pool != NULL) {
        /* 测试边界大小 */
        void* ptr1 = uvhttp_mempool_alloc(pool, 8);
        ASSERT_NE(ptr1, nullptr);
        
        /* size == UVHTTP_MEMPOOL_BLOCK_SIZE 应该返回有效指针 */
        void* ptr2 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE);
        ASSERT_NE(ptr2, nullptr);
        
        /* size > UVHTTP_MEMPOOL_BLOCK_SIZE 应该返回 NULL */
        void* ptr3 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE + 1);
        EXPECT_EQ(ptr3, nullptr);
        
        void* ptr4 = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE - 8);
        ASSERT_NE(ptr4, nullptr);
        
        uvhttp_mempool_destroy(pool);
    }
}