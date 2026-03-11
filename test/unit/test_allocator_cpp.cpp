/*
 * UVHTTP Allocator C++ Compatibility Test
 *
 * This test verifies that uvhttp_allocator.h works correctly in C++ code
 * when using mimalloc. The forward declaration pattern should avoid
 * C++ linkage issues with mimalloc.h templates.
 */

#include <gtest/gtest.h>

/* Include uvhttp_allocator.h inside extern "C" block to verify it works */
extern "C" {
    #include "uvhttp_allocator.h"
}

/* Test that allocator functions are accessible from C++ */
TEST(UvhttpAllocatorCppTest, AllocatorFunctionsAccessible) {
    /* Test allocation and deallocation */
    void* ptr = uvhttp_alloc(100);
    ASSERT_NE(ptr, nullptr);
    
    /* Test reallocation */
    void* new_ptr = uvhttp_realloc(ptr, 200);
    ASSERT_NE(new_ptr, nullptr);
    
    /* Test free */
    uvhttp_free(new_ptr);
    
    /* Test calloc */
    void* calloc_ptr = uvhttp_calloc(10, 100);
    ASSERT_NE(calloc_ptr, nullptr);
    uvhttp_free(calloc_ptr);
}

/* Test allocator name */
TEST(UvhttpAllocatorCppTest, AllocatorName) {
    const char* name = uvhttp_allocator_name();
    ASSERT_NE(name, nullptr);
    ASSERT_STRNE(name, "");
}

/* Test zero-size allocation */
TEST(UvhttpAllocatorCppTest, ZeroSizeAllocation) {
    void* ptr = uvhttp_alloc(0);
    /* Zero-size allocation should return nullptr or a valid pointer */
    uvhttp_free(ptr);
}

/* Test large allocation */
TEST(UvhttpAllocatorCppTest, LargeAllocation) {
    void* ptr = uvhttp_alloc(1024 * 1024); /* 1 MB */
    ASSERT_NE(ptr, nullptr);
    uvhttp_free(ptr);
}

/* Test null free (should not crash) */
TEST(UvhttpAllocatorCppTest, NullFree) {
    /* This should not crash */
    uvhttp_free(nullptr);
}

/* Test null realloc */
TEST(UvhttpAllocatorCppTest, NullRealloc) {
    /* Realloc with nullptr should work like malloc */
    void* ptr = uvhttp_realloc(nullptr, 100);
    ASSERT_NE(ptr, nullptr);
    uvhttp_free(ptr);
}

/* Test shrink realloc */
TEST(UvhttpAllocatorCppTest, ShrinkRealloc) {
    void* ptr = uvhttp_alloc(1000);
    ASSERT_NE(ptr, nullptr);
    
    void* shrunk = uvhttp_realloc(ptr, 100);
    ASSERT_NE(shrunk, nullptr);
    uvhttp_free(shrunk);
}

/* Test multiple allocations */
TEST(UvhttpAllocatorCppTest, MultipleAllocations) {
    const int num_allocs = 100;
    void* ptrs[num_allocs];
    
    for (int i = 0; i < num_allocs; i++) {
        ptrs[i] = uvhttp_alloc((i + 1) * 100);
        ASSERT_NE(ptrs[i], nullptr);
    }
    
    for (int i = 0; i < num_allocs; i++) {
        uvhttp_free(ptrs[i]);
    }
}