/* UVHTTP 统一内存分配器测试 */

#define UVHTTP_TEST_MODE 1  /* 启用测试模式 */
#define UVHTTP_MEMORY_TRACKING_ENABLED 1  /* 启用内存跟踪 */

#include "uvhttp_allocator.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/* 测试模式下的内存跟踪函数声明（在 test_memory_helpers.c 中实现） */
extern void* uvhttp_test_malloc(size_t size, const char* file, int line);
extern void uvhttp_test_free(void* ptr, const char* file, int line);
extern void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line);

void test_allocator_basic(void) {
    printf("test_allocator_basic: ");
    
    void* ptr = uvhttp_alloc(100);
    assert(ptr != NULL);
    
    strcpy((char*)ptr, "hello");
    assert(strcmp((char*)ptr, "hello") == 0);
    
    uvhttp_free(ptr);
    
    printf("PASSED\n");
}

void test_allocator_calloc(void) {
    printf("test_allocator_calloc: ");
    
    int* ptr = (int*)uvhttp_calloc(10, sizeof(int));
    assert(ptr != NULL);
    
    for (int i = 0; i < 10; i++) {
        assert(ptr[i] == 0);
    }
    
    uvhttp_free(ptr);
    
    printf("PASSED\n");
}

void test_allocator_realloc(void) {
    printf("test_allocator_realloc: ");
    
    void* ptr = uvhttp_alloc(100);
    assert(ptr != NULL);
    
    strcpy((char*)ptr, "hello");
    
    ptr = uvhttp_realloc(ptr, 200);
    assert(ptr != NULL);
    assert(strcmp((char*)ptr, "hello") == 0);
    
    uvhttp_free(ptr);
    
    printf("PASSED\n");
}

void test_allocator_null_free(void) {
    printf("test_allocator_null_free: ");
    
    /* 应该安全处理 NULL 指针 */
    uvhttp_free(NULL);
    
    printf("PASSED\n");
}

void test_allocator_size_zero(void) {
    printf("test_allocator_size_zero: ");
    
    void* ptr = uvhttp_alloc(0);
    /* 返回值可能是 NULL 或有效指针 */
    if (ptr != NULL) {
        uvhttp_free(ptr);
    }
    
    printf("PASSED\n");
}

void test_allocator_large_allocation(void) {
    printf("test_allocator_large_allocation: ");
    
    void* ptr = uvhttp_alloc(1024 * 1024);  /* 1MB */
    assert(ptr != NULL);
    
    memset(ptr, 0xAA, 1024 * 1024);
    
    uvhttp_free(ptr);
    
    printf("PASSED\n");
}

void test_allocator_many_allocations(void) {
    printf("test_allocator_many_allocations: ");
    
    void* ptrs[100];
    
    for (int i = 0; i < 100; i++) {
        ptrs[i] = uvhttp_alloc(64 + i);
        assert(ptrs[i] != NULL);
    }
    
    for (int i = 0; i < 100; i++) {
        uvhttp_free(ptrs[i]);
    }
    
    printf("PASSED\n");
}

void test_allocator_info(void) {
    printf("test_allocator_info: ");

    const char* name = uvhttp_allocator_name();
    assert(name != NULL);
    printf("(%s) ", name);

    printf("PASSED\n");
}

int main(void) {
    printf("=== UVHTTP 统一内存分配器测试 ===\n");
    printf("分配器类型: %s\n\n", uvhttp_allocator_name());

    test_allocator_basic();
    test_allocator_calloc();
    test_allocator_realloc();
    test_allocator_null_free();
    test_allocator_size_zero();
    test_allocator_large_allocation();
    test_allocator_many_allocations();
    test_allocator_info();

    printf("\n=== 所有测试通过 ===\n");

    return 0;
}