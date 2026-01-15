/* UVHTTP 统一内存分配器测试 */

#define UVHTTP_TEST_MODE 1  /* 启用测试模式 */
#define UVHTTP_MEMORY_TRACKING_ENABLED 1  /* 启用内存跟踪 */

#include <gtest/gtest.h>
#include <string.h>
#include "../include/uvhttp_allocator.h"

/* 测试模式下的内存跟踪函数声明（在 test_memory_helpers.c 中实现） */
extern void* uvhttp_test_malloc(size_t size, const char* file, int line);
extern void uvhttp_test_free(void* ptr, const char* file, int line);
extern void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line);

// ============================================================================
// 基本分配测试
// ============================================================================

TEST(UvhttpAllocatorTest, BasicAllocation) {
    void* ptr = uvhttp_alloc(100);
    ASSERT_NE(ptr, nullptr);
    
    strcpy((char*)ptr, "hello");
    EXPECT_STREQ((char*)ptr, "hello");
    
    uvhttp_free(ptr);
}

TEST(UvhttpAllocatorTest, CallocAllocation) {
    int* ptr = (int*)uvhttp_calloc(10, sizeof(int));
    ASSERT_NE(ptr, nullptr);
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(ptr[i], 0);
    }
    
    uvhttp_free(ptr);
}

TEST(UvhttpAllocatorTest, ReallocAllocation) {
    void* ptr = uvhttp_alloc(100);
    ASSERT_NE(ptr, nullptr);
    
    strcpy((char*)ptr, "hello");
    
    ptr = uvhttp_realloc(ptr, 200);
    ASSERT_NE(ptr, nullptr);
    EXPECT_STREQ((char*)ptr, "hello");
    
    uvhttp_free(ptr);
}

// ============================================================================
// 边界条件测试
// ============================================================================

TEST(UvhttpAllocatorTest, NullFree) {
    // 应该安全处理 NULL 指针
    uvhttp_free(nullptr);
}

TEST(UvhttpAllocatorTest, ZeroSizeAllocation) {
    void* ptr = uvhttp_alloc(0);
    // 返回值可能是 NULL 或有效指针
    if (ptr != nullptr) {
        uvhttp_free(ptr);
    }
}

TEST(UvhttpAllocatorTest, LargeAllocation) {
    void* ptr = uvhttp_alloc(1024 * 1024);  /* 1MB */
    ASSERT_NE(ptr, nullptr);
    
    memset(ptr, 0xAA, 1024 * 1024);
    
    uvhttp_free(ptr);
}

TEST(UvhttpAllocatorTest, ManyAllocations) {
    void* ptrs[100];
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = uvhttp_alloc(64 + i);
        ASSERT_NE(ptrs[i], nullptr);
    }
    
    for (int i = 0; i < 100; i++) {
        uvhttp_free(ptrs[i]);
    }
}

// ============================================================================
// 分配器信息测试
// ============================================================================

TEST(UvhttpAllocatorTest, AllocatorInfo) {
    const char* name = uvhttp_allocator_name();
    ASSERT_NE(name, nullptr);
    EXPECT_GT(strlen(name), 0);
}