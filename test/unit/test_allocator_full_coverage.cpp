/* uvhttp_allocator.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_allocator.h"
#include <string.h>

// ============================================================================
// 测试用例
// ============================================================================

// 声明测试函数（来自 uvhttp_allocator.c）
extern "C" {
    void* uvhttp_test_malloc(size_t size, const char* file, int line);
    void uvhttp_test_free(void* ptr, const char* file, int line);
    void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line);
}

TEST(UvhttpAllocatorFullCoverageTest, TestMallocBasic) {
    void* ptr = uvhttp_test_malloc(100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // 写入数据验证内存可用
    memset(ptr, 0xAA, 100);
    EXPECT_EQ(((uint8_t*)ptr)[0], 0xAA);
    
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestMallocZero) {
    void* ptr = uvhttp_test_malloc(0, "test_file.c", 10);
    // malloc(0) 可能返回 NULL 或一个有效的指针
    // 两种行为都是符合标准的
    if (ptr != nullptr) {
        uvhttp_test_free(ptr, "test_file.c", 10);
    }
}

TEST(UvhttpAllocatorFullCoverageTest, TestMallocLarge) {
    const size_t large_size = 1024 * 1024;  // 1MB
    void* ptr = uvhttp_test_malloc(large_size, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // 写入数据验证内存可用
    memset(ptr, 0xBB, large_size);
    EXPECT_EQ(((uint8_t*)ptr)[0], 0xBB);
    EXPECT_EQ(((uint8_t*)ptr)[large_size - 1], 0xBB);
    
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestFreeNull) {
    // 释放 NULL 指针应该是安全的
    uvhttp_test_free(nullptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestFreeBasic) {
    void* ptr = uvhttp_test_malloc(100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_test_free(ptr, "test_file.c", 10);
    
    // 释放后再释放应该是安全的（虽然不应该这样做）
    // 但测试一下系统的行为
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestReallocBasic) {
    void* ptr = uvhttp_test_malloc(100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // 写入初始数据
    memset(ptr, 0xCC, 100);
    
    // 重新分配更大的内存
    ptr = uvhttp_test_realloc(ptr, 200, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // 验证旧数据仍然存在
    EXPECT_EQ(((uint8_t*)ptr)[0], 0xCC);
    EXPECT_EQ(((uint8_t*)ptr)[99], 0xCC);
    
    // 写入新数据
    memset(ptr, 0xDD, 200);
    EXPECT_EQ(((uint8_t*)ptr)[0], 0xDD);
    EXPECT_EQ(((uint8_t*)ptr)[199], 0xDD);
    
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestReallocNull) {
    // realloc(NULL, size) 等同于 malloc(size)
    void* ptr = uvhttp_test_realloc(nullptr, 100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    memset(ptr, 0xEE, 100);
    EXPECT_EQ(((uint8_t*)ptr)[0], 0xEE);
    
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestReallocZero) {
    void* ptr = uvhttp_test_malloc(100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // realloc(ptr, 0) 等同于 free(ptr)
    ptr = uvhttp_test_realloc(ptr, 0, "test_file.c", 10);
    // 返回值可能是 NULL 或一个有效的最小指针
    // 无论哪种情况，都不应该调用 free
}

TEST(UvhttpAllocatorFullCoverageTest, TestReallocShrink) {
    void* ptr = uvhttp_test_malloc(200, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // 写入初始数据
    memset(ptr, 0xFF, 200);
    
    // 重新分配更小的内存
    ptr = uvhttp_test_realloc(ptr, 100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    
    // 验证旧数据仍然存在（前 100 字节）
    EXPECT_EQ(((uint8_t*)ptr)[0], 0xFF);
    EXPECT_EQ(((uint8_t*)ptr)[99], 0xFF);
    
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestMultipleAllocations) {
    void* ptrs[10];
    
    // 分配多个内存块
    for (int i = 0; i < 10; i++) {
        ptrs[i] = uvhttp_test_malloc((i + 1) * 100, "test_file.c", 10);
        ASSERT_NE(ptrs[i], nullptr);
        memset(ptrs[i], i, (i + 1) * 100);
    }
    
    // 验证所有内存块都有不同的地址
    for (int i = 0; i < 10; i++) {
        for (int j = i + 1; j < 10; j++) {
            EXPECT_NE(ptrs[i], ptrs[j]);
        }
    }
    
    // 释放所有内存块
    for (int i = 0; i < 10; i++) {
        uvhttp_test_free(ptrs[i], "test_file.c", 10);
    }
}

TEST(UvhttpAllocatorFullCoverageTest, TestMemoryLeak) {
    // 分配内存但不释放（测试内存泄漏）
    void* ptr = uvhttp_test_malloc(100, "test_file.c", 10);
    ASSERT_NE(ptr, nullptr);
    // 故意不释放，模拟内存泄漏
    // 在实际运行中，这会被内存泄漏检测工具捕获
}

TEST(UvhttpAllocatorFullCoverageTest, TestAllocatorName) {
    const char* name = uvhttp_allocator_name();
    ASSERT_NE(name, nullptr);
    EXPECT_GT(strlen(name), 0);
    
    // 验证返回的是已知的分配器名称
    if (strcmp(name, "system") == 0) {
        EXPECT_STREQ(name, "system");
    } else if (strcmp(name, "mimalloc") == 0) {
        EXPECT_STREQ(name, "mimalloc");
    } else {
        // 可能是 "system (mimalloc not available)"
        EXPECT_TRUE(strstr(name, "system") != nullptr);
    }
}

TEST(UvhttpAllocatorFullCoverageTest, TestAllocFreeCycle) {
    // 多次分配和释放循环
    for (int i = 0; i < 100; i++) {
        void* ptr = uvhttp_test_malloc(100, "test_file.c", 10);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, i % 256, 100);
        uvhttp_test_free(ptr, "test_file.c", 10);
    }
}

TEST(UvhttpAllocatorFullCoverageTest, TestReallocCycle) {
    void* ptr = nullptr;
    
    // 多次重新分配循环
    for (int i = 1; i <= 100; i++) {
        ptr = uvhttp_test_realloc(ptr, i * 100, "test_file.c", 10);
        ASSERT_NE(ptr, nullptr);
        memset(ptr, i % 256, i * 100);
    }
    
    uvhttp_test_free(ptr, "test_file.c", 10);
}

TEST(UvhttpAllocatorFullCoverageTest, TestDifferentFileAndLine) {
    void* ptr1 = uvhttp_test_malloc(100, "file1.c", 10);
    void* ptr2 = uvhttp_test_malloc(100, "file2.c", 20);
    void* ptr3 = uvhttp_test_malloc(100, "file3.c", 30);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    
    // 验证所有指针都不同
    EXPECT_NE(ptr1, ptr2);
    EXPECT_NE(ptr2, ptr3);
    EXPECT_NE(ptr1, ptr3);
    
    uvhttp_test_free(ptr1, "file1.c", 10);
    uvhttp_test_free(ptr2, "file2.c", 20);
    uvhttp_test_free(ptr3, "file3.c", 30);
}

TEST(UvhttpAllocatorFullCoverageTest, TestNullFileAndLine) {
    void* ptr = uvhttp_test_malloc(100, nullptr, 0);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_test_free(ptr, nullptr, 0);
}

TEST(UvhttpAllocatorFullCoverageTest, TestEmptyFileAndLine) {
    void* ptr = uvhttp_test_malloc(100, "", 0);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_test_free(ptr, "", 0);
}

TEST(UvhttpAllocatorFullCoverageTest, TestNegativeLine) {
    void* ptr = uvhttp_test_malloc(100, "test_file.c", -1);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_test_free(ptr, "test_file.c", -1);
}

TEST(UvhttpAllocatorFullCoverageTest, TestLargeLine) {
    void* ptr = uvhttp_test_malloc(100, "test_file.c", 999999);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_test_free(ptr, "test_file.c", 999999);
}

TEST(UvhttpAllocatorFullCoverageTest, TestMemoryAlignment) {
    // 测试内存对齐
    void* ptr1 = uvhttp_test_malloc(1, "test_file.c", 10);
    void* ptr2 = uvhttp_test_malloc(2, "test_file.c", 10);
    void* ptr3 = uvhttp_test_malloc(4, "test_file.c", 10);
    void* ptr4 = uvhttp_test_malloc(8, "test_file.c", 10);
    void* ptr5 = uvhttp_test_malloc(16, "test_file.c", 10);
    
    ASSERT_NE(ptr1, nullptr);
    ASSERT_NE(ptr2, nullptr);
    ASSERT_NE(ptr3, nullptr);
    ASSERT_NE(ptr4, nullptr);
    ASSERT_NE(ptr5, nullptr);
    
    // 验证指针对齐（通常是对齐到 8 或 16 字节）
    EXPECT_EQ((uintptr_t)ptr1 % sizeof(void*), 0);
    EXPECT_EQ((uintptr_t)ptr2 % sizeof(void*), 0);
    EXPECT_EQ((uintptr_t)ptr3 % sizeof(void*), 0);
    EXPECT_EQ((uintptr_t)ptr4 % sizeof(void*), 0);
    EXPECT_EQ((uintptr_t)ptr5 % sizeof(void*), 0);
    
    uvhttp_test_free(ptr1, "test_file.c", 10);
    uvhttp_test_free(ptr2, "test_file.c", 10);
    uvhttp_test_free(ptr3, "test_file.c", 10);
    uvhttp_test_free(ptr4, "test_file.c", 10);
    uvhttp_test_free(ptr5, "test_file.c", 10);
}