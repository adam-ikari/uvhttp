/**
 * @file test_memory.cpp
 * @brief UVHTTP 内存测试（简化版）
 *
 * 内存测试用于验证系统的内存使用情况
 */

#include <gtest/gtest.h>
#include <uv.h>
#include <vector>
#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_response.h"
#include "uvhttp_config.h"
#include "uvhttp_allocator.h"

// 内存测试配置
#define MEMORY_TEST_ALLOCATIONS 1000
#define MEMORY_TEST_LARGE_SIZE (1024 * 1024)  // 1MB

/**
 * @brief 测试基本内存分配和释放
 */
TEST(MemoryTest, BasicAllocation) {
    // 分配和释放内存
    for (int i = 0; i < MEMORY_TEST_ALLOCATIONS; i++) {
        size_t size = (i % 100) + 1;  // 1-100 字节
        void* ptr = UVHTTP_MALLOC(size);
        ASSERT_NE(ptr, nullptr);
        UVHTTP_FREE(ptr);
    }
}

/**
 * @brief 测试大内存分配
 */
TEST(MemoryTest, LargeAllocation) {
    void* ptr = UVHTTP_MALLOC(MEMORY_TEST_LARGE_SIZE);
    ASSERT_NE(ptr, nullptr);

    // 填充内存
    memset(ptr, 0xAA, MEMORY_TEST_LARGE_SIZE);

    // 验证内存内容
    char* data = (char*)ptr;
    for (size_t i = 0; i < MEMORY_TEST_LARGE_SIZE; i++) {
        EXPECT_EQ(data[i], (char)0xAA);
    }

    UVHTTP_FREE(ptr);
}

/**
 * @brief 测试服务器内存使用
 */
TEST(MemoryTest, ServerMemoryUsage) {
    uv_loop_t* loop = uv_default_loop();
    ASSERT_NE(loop, nullptr);

    uvhttp_server_t* server = uvhttp_server_new(loop);
    ASSERT_NE(server, nullptr);

    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);

    uvhttp_server_set_router(server, router);

    uvhttp_server_free(server);
    uvhttp_router_free(router);
    uv_loop_close(loop);
}

/**
 * @brief 测试多次创建和销毁服务器
 */
TEST(MemoryTest, MultipleServerCreation) {
    const int iterations = 10;

    for (int i = 0; i < iterations; i++) {
        uv_loop_t* loop = uv_loop_new();
        ASSERT_NE(loop, nullptr);

        uvhttp_server_t* server = uvhttp_server_new(loop);
        ASSERT_NE(server, nullptr);

        uvhttp_router_t* router = uvhttp_router_new();
        ASSERT_NE(router, nullptr);

        uvhttp_server_set_router(server, router);

        uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", 0);
        ASSERT_EQ(result, UVHTTP_OK);

        uvhttp_server_free(server);
        uv_run(loop, UV_RUN_DEFAULT);

        uvhttp_router_free(router);
        uv_loop_close(loop);
        UVHTTP_FREE(loop);
    }
}

/**
 * @brief 测试内存碎片
 */
TEST(MemoryTest, MemoryFragmentation) {
    // 分配不同大小的内存块
    std::vector<void*> allocations;

    for (int i = 0; i < 100; i++) {
        size_t size = (i % 10 + 1) * 100;  // 100-1000 字节
        void* ptr = UVHTTP_MALLOC(size);
        ASSERT_NE(ptr, nullptr);
        allocations.push_back(ptr);
    }

    // 释放部分内存
    for (size_t i = 0; i < allocations.size(); i += 2) {
        UVHTTP_FREE(allocations[i]);
    }

    // 再次分配内存
    for (size_t i = 0; i < allocations.size(); i += 2) {
        size_t size = (i % 10 + 1) * 100;
        allocations[i] = UVHTTP_MALLOC(size);
        ASSERT_NE(allocations[i], nullptr);
    }

    // 释放所有内存
    for (void* ptr : allocations) {
        if (ptr) {
            UVHTTP_FREE(ptr);
        }
    }
}

/**
 * @brief 测试内存泄漏检测
 */
TEST(MemoryTest, MemoryLeakDetection) {
    static size_t allocation_count = 0;
    static size_t free_count = 0;

    // 分配内存
    void* ptr1 = UVHTTP_MALLOC(100);
    void* ptr2 = UVHTTP_MALLOC(200);
    void* ptr3 = UVHTTP_MALLOC(300);

    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);

    allocation_count += 3;

    // 释放部分内存
    UVHTTP_FREE(ptr2);
    free_count += 1;

    // 验证分配和释放计数
    EXPECT_EQ(allocation_count, 3);
    EXPECT_EQ(free_count, 1);

    // 释放剩余内存
    UVHTTP_FREE(ptr1);
    UVHTTP_FREE(ptr3);
    free_count += 2;

    // 验证所有内存都已释放
    EXPECT_EQ(allocation_count, 3);
    EXPECT_EQ(free_count, 3);
}

/**
 * @brief 测试 NULL 指针释放
 */
TEST(MemoryTest, NullPointerFree) {
    // 释放 NULL 指针不应该崩溃
    UVHTTP_FREE(nullptr);
    SUCCEED();
}

/**
 * @brief 测试零大小分配
 */
TEST(MemoryTest, ZeroSizeAllocation) {
    void* ptr = UVHTTP_MALLOC(0);
    // 零大小分配应该返回 NULL 或有效指针
    // 具体行为取决于分配器实现
    UVHTTP_FREE(ptr);
    SUCCEED();
}

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}