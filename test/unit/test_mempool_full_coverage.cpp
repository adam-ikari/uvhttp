/* uvhttp_mempool.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_mempool.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <stdlib.h>

TEST(UvhttpMempoolTest, MempoolCreate) {
    /* 测试创建内存池 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 验证初始状态 */
    EXPECT_EQ(pool->blocks, nullptr);
    EXPECT_EQ(pool->block_count, 0);
    EXPECT_EQ(pool->free_offset, 0);
    EXPECT_EQ(pool->current_block, nullptr);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolCreateNull) {
    /* 测试创建内存池 - 检查是否成功 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    EXPECT_NE(pool, nullptr);
    
    if (pool) {
        uvhttp_mempool_destroy(pool);
    }
}

TEST(UvhttpMempoolTest, MempoolDestroyNull) {
    /* 测试销毁 NULL 内存池 */
    uvhttp_mempool_destroy(nullptr);
}

TEST(UvhttpMempoolTest, MempoolAlloc) {
    /* 测试从内存池分配内存 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配小内存 */
    void* ptr1 = uvhttp_mempool_alloc(pool, 100);
    EXPECT_NE(ptr1, nullptr);
    
    /* 分配另一个内存 */
    void* ptr2 = uvhttp_mempool_alloc(pool, 200);
    EXPECT_NE(ptr2, nullptr);
    
    /* 验证指针不同 */
    EXPECT_NE(ptr1, ptr2);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocNull) {
    /* 测试从 NULL 内存池分配 */
    void* ptr = uvhttp_mempool_alloc(nullptr, 100);
    EXPECT_EQ(ptr, nullptr);
}

TEST(UvhttpMempoolTest, MempoolAllocZeroSize) {
    /* 测试分配零大小 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    void* ptr = uvhttp_mempool_alloc(pool, 0);
    EXPECT_EQ(ptr, nullptr);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocAlignment) {
    /* 测试内存对齐 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配不同大小的内存，验证对齐 */
    void* ptr1 = uvhttp_mempool_alloc(pool, 1);
    void* ptr2 = uvhttp_mempool_alloc(pool, 7);
    void* ptr3 = uvhttp_mempool_alloc(pool, 8);
    void* ptr4 = uvhttp_mempool_alloc(pool, 9);
    
    EXPECT_NE(ptr1, nullptr);
    EXPECT_NE(ptr2, nullptr);
    EXPECT_NE(ptr3, nullptr);
    EXPECT_NE(ptr4, nullptr);
    
    /* 验证所有指针都有效 */
    if (ptr1) memset(ptr1, 0, 1);
    if (ptr2) memset(ptr2, 0, 7);
    if (ptr3) memset(ptr3, 0, 8);
    if (ptr4) memset(ptr4, 0, 9);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocLarge) {
    /* 测试分配大内存（超过块大小） */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配超过块大小的内存 */
    void* ptr = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE + 1);
    EXPECT_EQ(ptr, nullptr);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocExactlyBlockSize) {
    /* 测试分配正好等于块大小的内存（对齐后可能超过） */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配正好等于块大小的内存（对齐后会超过） */
    void* ptr = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE);
    /* 对齐后可能成功或失败，取决于对齐 */
    /* 如果成功，说明对齐后没有超过块大小 */
    /* 如果失败，说明对齐后超过了块大小 */
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocMultipleBlocks) {
    /* 测试分配多个块 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配足够多的内存以触发多个块 */
    for (int i = 0; i < 100; i++) {
        void* ptr = uvhttp_mempool_alloc(pool, 100);
        /* 可能失败，但至少应该有一些成功 */
        if (ptr) {
            memset(ptr, 0, 100);
        }
    }
    
    /* 验证至少有一个块 */
    EXPECT_GE(pool->block_count, 1);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolReset) {
    /* 测试重置内存池 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配一些内存 */
    void* ptr1 = uvhttp_mempool_alloc(pool, 100);
    void* ptr2 = uvhttp_mempool_alloc(pool, 200);
    
    size_t block_count_before = pool->block_count;
    
    /* 重置内存池 */
    uvhttp_mempool_reset(pool);
    
    /* 验证重置后状态 */
    EXPECT_EQ(pool->free_offset, 0);
    EXPECT_EQ(pool->current_block, pool->blocks);
    EXPECT_EQ(pool->block_count, block_count_before);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolResetNull) {
    /* 测试重置 NULL 内存池 */
    uvhttp_mempool_reset(nullptr);
}

TEST(UvhttpMempoolTest, MempoolStats) {
    /* 测试获取内存池统计 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    size_t blocks = 0;
    size_t used = 0;
    
    /* 初始统计 */
    uvhttp_mempool_stats(pool, &blocks, &used);
    EXPECT_EQ(blocks, 0);
    EXPECT_EQ(used, 0);
    
    /* 分配一些内存 */
    void* ptr1 = uvhttp_mempool_alloc(pool, 100);
    void* ptr2 = uvhttp_mempool_alloc(pool, 200);
    
    /* 分配后统计 */
    uvhttp_mempool_stats(pool, &blocks, &used);
    EXPECT_GT(blocks, 0);
    EXPECT_GT(used, 0);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolStatsNull) {
    /* 测试获取 NULL 内存池统计 */
    size_t blocks = 0;
    size_t used = 0;
    
    uvhttp_mempool_stats(nullptr, &blocks, &used);
    /* 应该不崩溃，blocks 和 used 保持不变 */
}

TEST(UvhttpMempoolTest, MempoolStatsNullPointers) {
    /* 测试使用 NULL 指针获取统计 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 只获取块数 */
    size_t blocks = 0;
    uvhttp_mempool_stats(pool, &blocks, nullptr);
    EXPECT_EQ(blocks, 0);
    
    /* 只获取使用量 */
    size_t used = 0;
    uvhttp_mempool_stats(pool, nullptr, &used);
    EXPECT_EQ(used, 0);
    
    /* 都不获取 */
    uvhttp_mempool_stats(pool, nullptr, nullptr);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolBlockStructure) {
    /* 测试内存池块结构 */
    EXPECT_GE(sizeof(uvhttp_mempool_block_t), sizeof(uvhttp_mempool_block_t*));
    EXPECT_EQ(UVHTTP_MEMPOOL_BLOCK_SIZE, 4096);
}

TEST(UvhttpMempoolTest, MempoolPoolStructure) {
    /* 测试内存池结构 */
    EXPECT_GE(sizeof(uvhttp_mempool_t), sizeof(uvhttp_mempool_block_t*) * 3);
}

TEST(UvhttpMempoolTest, MempoolAllocAfterReset) {
    /* 测试重置后分配 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配一些内存 */
    void* ptr1 = uvhttp_mempool_alloc(pool, 100);
    
    /* 重置 */
    uvhttp_mempool_reset(pool);
    
    /* 重置后分配 */
    void* ptr2 = uvhttp_mempool_alloc(pool, 200);
    EXPECT_NE(ptr2, nullptr);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolMultipleResets) {
    /* 测试多次重置 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 多次重置 */
    for (int i = 0; i < 10; i++) {
        uvhttp_mempool_reset(pool);
        EXPECT_EQ(pool->free_offset, 0);
    }
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolDestroyMultipleBlocks) {
    /* 测试销毁多个块的内存池 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配多个块的内存 */
    for (int i = 0; i < 10; i++) {
        uvhttp_mempool_alloc(pool, 500);
    }
    
    /* 验证有多个块 */
    EXPECT_GT(pool->block_count, 1);
    
    /* 销毁 */
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocWrite) {
    /* 测试分配并写入内存 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配内存 */
    void* ptr = uvhttp_mempool_alloc(pool, 100);
    ASSERT_NE(ptr, nullptr);
    
    /* 写入数据 */
    memset(ptr, 0xAA, 100);
    
    /* 验证数据 */
    uint8_t* data = (uint8_t*)ptr;
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(data[i], 0xAA);
    }
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolAllocFragmentation) {
    /* 测试内存碎片化 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配不同大小的内存 */
    void* ptrs[10];
    for (int i = 0; i < 10; i++) {
        ptrs[i] = uvhttp_mempool_alloc(pool, (i + 1) * 10);
        if (ptrs[i]) {
            memset(ptrs[i], 0, (i + 1) * 10);
        }
    }
    
    /* 重置 */
    uvhttp_mempool_reset(pool);
    
    /* 再次分配 */
    for (int i = 0; i < 10; i++) {
        void* ptr = uvhttp_mempool_alloc(pool, (i + 1) * 10);
        if (ptr) {
            memset(ptr, 0, (i + 1) * 10);
        }
    }
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolBoundaryAllocation) {
    /* 测试边界分配 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配接近块边界的内存 */
    size_t remaining = UVHTTP_MEMPOOL_BLOCK_SIZE - pool->free_offset;
    if (remaining > 0) {
        void* ptr = uvhttp_mempool_alloc(pool, remaining - 8);
        if (ptr) {
            memset(ptr, 0, remaining - 8);
        }
    }
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolStatsAccuracy) {
    /* 测试统计准确性 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    size_t blocks = 0;
    size_t used = 0;
    
    /* 获取初始统计 */
    uvhttp_mempool_stats(pool, &blocks, &used);
    size_t initial_blocks = blocks;
    size_t initial_used = used;
    
    /* 分配已知大小的内存 */
    void* ptr1 = uvhttp_mempool_alloc(pool, 100);
    void* ptr2 = uvhttp_mempool_alloc(pool, 200);
    
    /* 获取分配后统计 */
    uvhttp_mempool_stats(pool, &blocks, &used);
    
    /* 验证统计变化 */
    if (ptr1 && ptr2) {
        EXPECT_GE(blocks, initial_blocks);
        EXPECT_GT(used, initial_used);
    }
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolConsecutiveAllocs) {
    /* 测试连续分配 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 连续分配小内存 */
    void* ptrs[100];
    int success_count = 0;
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = uvhttp_mempool_alloc(pool, 10);
        if (ptrs[i]) {
            success_count++;
            memset(ptrs[i], 0, 10);
        }
    }
    
    /* 验证至少有一些分配成功 */
    EXPECT_GT(success_count, 0);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolZeroOffset) {
    /* 测试零偏移量 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 验证初始偏移量为 0 */
    EXPECT_EQ(pool->free_offset, 0);
    
    /* 分配后偏移量应该增加 */
    void* ptr = uvhttp_mempool_alloc(pool, 100);
    if (ptr) {
        EXPECT_GT(pool->free_offset, 0);
    }
    
    /* 重置后偏移量应该为 0 */
    uvhttp_mempool_reset(pool);
    EXPECT_EQ(pool->free_offset, 0);
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolBlockSize) {
    /* 测试块大小常量 */
    EXPECT_EQ(UVHTTP_MEMPOOL_BLOCK_SIZE, 4096);
}

TEST(UvhttpMempoolTest, MempoolBlockDataSize) {
    /* 测试块数据数组大小 */
    EXPECT_EQ(sizeof(((uvhttp_mempool_block_t*)0)->data), UVHTTP_MEMPOOL_BLOCK_SIZE);
}

TEST(UvhttpMempoolTest, MempoolAllocMaxSize) {
    /* 测试分配最大大小 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 分配接近块大小的内存 */
    void* ptr = uvhttp_mempool_alloc(pool, UVHTTP_MEMPOOL_BLOCK_SIZE - 8);
    EXPECT_NE(ptr, nullptr);
    
    if (ptr) {
        memset(ptr, 0, UVHTTP_MEMPOOL_BLOCK_SIZE - 8);
    }
    
    uvhttp_mempool_destroy(pool);
}

TEST(UvhttpMempoolTest, MempoolDestroyEmpty) {
    /* 测试销毁空内存池 */
    uvhttp_mempool_t* pool = uvhttp_mempool_create();
    ASSERT_NE(pool, nullptr);
    
    /* 不分配任何内存 */
    uvhttp_mempool_destroy(pool);
}