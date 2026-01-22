/*
 * 应用层自定义内存池示例
 * 
 * 演示如何在应用层实现和使用自定义内存池
 * 框架层只提供基础分配器接口，应用层可以根据需求自由扩展
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>

/* ========== 应用层内存池实现 ========== */

#define POOL_BLOCK_SIZE 4096
#define POOL_ALIGNMENT 8

typedef struct pool_block {
    struct pool_block* next;
    size_t used;
    char data[POOL_BLOCK_SIZE];
} pool_block_t;

typedef struct pool_header {
    size_t size;
    struct pool_header* next;
} pool_header_t;

typedef struct {
    pool_block_t* blocks;
    pool_header_t* allocations;
    size_t total_allocs;
    size_t total_frees;
    size_t current_bytes;
    size_t peak_bytes;
} app_pool_t;

/* 创建内存池 */
app_pool_t* app_pool_create(void) {
    app_pool_t* pool = (app_pool_t*)malloc(sizeof(app_pool_t));
    if (!pool) return NULL;
    
    memset(pool, 0, sizeof(app_pool_t));
    return pool;
}

/* 添加新的内存块 */
static pool_block_t* pool_add_block(app_pool_t* pool) {
    pool_block_t* block = (pool_block_t*)malloc(sizeof(pool_block_t));
    if (!block) return NULL;
    
    block->next = pool->blocks;
    block->used = 0;
    pool->blocks = block;
    
    return block;
}

/* 从内存池分配内存 */
void* app_pool_alloc(app_pool_t* pool, size_t size) {
    if (!pool) return NULL;
    
    /* 对齐到 POOL_ALIGNMENT */
    size_t aligned_size = (size + POOL_ALIGNMENT - 1) & ~(POOL_ALIGNMENT - 1);
    size_t total_size = aligned_size + sizeof(pool_header_t);
    
    /* 如果超过块大小，直接使用系统分配 */
    if (total_size > POOL_BLOCK_SIZE - sizeof(pool_block_t)) {
        pool_header_t* header = (pool_header_t*)malloc(total_size);
        if (!header) return NULL;
        
        header->size = aligned_size;
        header->next = pool->allocations;
        pool->allocations = header;
        
        pool->total_allocs++;
        pool->current_bytes += aligned_size;
        if (pool->current_bytes > pool->peak_bytes) {
            pool->peak_bytes = pool->current_bytes;
        }
        
        return header + 1;
    }
    
    /* 在内存池中查找可用空间 */
    pool_block_t* block = pool->blocks;
    while (block) {
        if (block->used + total_size <= POOL_BLOCK_SIZE) {
            pool_header_t* header = (pool_header_t*)(block->data + block->used);
            header->size = aligned_size;
            header->next = pool->allocations;
            pool->allocations = header;
            
            block->used += total_size;
            
            pool->total_allocs++;
            pool->current_bytes += aligned_size;
            if (pool->current_bytes > pool->peak_bytes) {
                pool->peak_bytes = pool->current_bytes;
            }
            
            return header + 1;
        }
        block = block->next;
    }
    
    /* 没有可用空间，添加新块 */
    block = pool_add_block(pool);
    if (!block) return NULL;
    
    pool_header_t* header = (pool_header_t*)(block->data);
    header->size = aligned_size;
    header->next = pool->allocations;
    pool->allocations = header;
    
    block->used = total_size;
    
    pool->total_allocs++;
    pool->current_bytes += aligned_size;
    if (pool->current_bytes > pool->peak_bytes) {
        pool->peak_bytes = pool->current_bytes;
    }
    
    return header + 1;
}

/* 释放内存到内存池 */
void app_pool_free(app_pool_t* pool, void* ptr) {
    if (!pool || !ptr) return;
    
    pool_header_t* header = (pool_header_t*)ptr - 1;
    
    /* 从分配列表中移除 */
    pool_header_t** p = &pool->allocations;
    while (*p) {
        if (*p == header) {
            *p = header->next;
            pool->total_frees++;
            pool->current_bytes -= header->size;
            return;
        }
        p = &((*p)->next);
    }
}

/* 获取内存池统计信息 */
void app_pool_get_stats(app_pool_t* pool, size_t* total_allocs, size_t* total_frees,
                        size_t* current_bytes, size_t* peak_bytes) {
    if (!pool) return;
    
    if (total_allocs) *total_allocs = pool->total_allocs;
    if (total_frees) *total_frees = pool->total_frees;
    if (current_bytes) *current_bytes = pool->current_bytes;
    if (peak_bytes) *peak_bytes = pool->peak_bytes;
}

/* 销毁内存池 */
void app_pool_destroy(app_pool_t* pool) {
    if (!pool) return;
    
    /* 释放所有大块分配 */
    pool_header_t* header = pool->allocations;
    while (header) {
        pool_header_t* next = header->next;
        
        /* 检查是否在内存池中 */
        pool_block_t* block = pool->blocks;
        int in_pool = 0;
        while (block) {
            if ((char*)header >= block->data && 
                (char*)header < block->data + POOL_BLOCK_SIZE) {
                in_pool = 1;
                break;
            }
            block = block->next;
        }
        
        if (!in_pool) {
            free(header);
        }
        header = next;
    }
    
    /* 释放所有块 */
    pool_block_t* block = pool->blocks;
    while (block) {
        pool_block_t* next = block->next;
        free(block);
        block = next;
    }
    
    free(pool);
}

/* ========== 应用层内存池测试 ========== */

void test_basic_allocation(void) {
    printf("测试: 基本分配和释放\n");
    
    app_pool_t* pool = app_pool_create();
    assert(pool != NULL);
    
    /* 分配一些内存 */
    void* ptr1 = app_pool_alloc(pool, 64);
    void* ptr2 = app_pool_alloc(pool, 128);
    void* ptr3 = app_pool_alloc(pool, 256);
    
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    assert(ptr3 != NULL);
    
    /* 使用内存 */
    strcpy((char*)ptr1, "Hello");
    strcpy((char*)ptr2, "World");
    strcpy((char*)ptr3, "Pool");
    
    printf("  ptr1: %s\n", (char*)ptr1);
    printf("  ptr2: %s\n", (char*)ptr2);
    printf("  ptr3: %s\n", (char*)ptr3);
    
    /* 释放内存 */
    app_pool_free(pool, ptr1);
    app_pool_free(pool, ptr2);
    app_pool_free(pool, ptr3);
    
    app_pool_destroy(pool);
    printf("  PASSED\n\n");
}

void test_statistics(void) {
    printf("测试: 统计信息\n");
    
    app_pool_t* pool = app_pool_create();
    assert(pool != NULL);
    
    size_t total_allocs, total_frees, current_bytes, peak_bytes;
    
    /* 分配前 */
    app_pool_get_stats(pool, &total_allocs, &total_frees, &current_bytes, &peak_bytes);
    printf("  分配前: allocs=%zu, frees=%zu, current=%zu, peak=%zu\n",
           total_allocs, total_frees, current_bytes, peak_bytes);
    
    /* 分配内存 */
    void* ptrs[100];
    for (int i = 0; i < 100; i++) {
        ptrs[i] = app_pool_alloc(pool, 64);
    }
    
    /* 分配后 */
    app_pool_get_stats(pool, &total_allocs, &total_frees, &current_bytes, &peak_bytes);
    printf("  分配后: allocs=%zu, frees=%zu, current=%zu, peak=%zu\n",
           total_allocs, total_frees, current_bytes, peak_bytes);
    
    /* 释放一半 */
    for (int i = 0; i < 50; i++) {
        app_pool_free(pool, ptrs[i]);
    }
    
    /* 释放后 */
    app_pool_get_stats(pool, &total_allocs, &total_frees, &current_bytes, &peak_bytes);
    printf("  释放后: allocs=%zu, frees=%zu, current=%zu, peak=%zu\n",
           total_allocs, total_frees, current_bytes, peak_bytes);
    
    /* 释放剩余 */
    for (int i = 50; i < 100; i++) {
        app_pool_free(pool, ptrs[i]);
    }
    
    app_pool_destroy(pool);
    printf("  PASSED\n\n");
}

void test_large_allocation(void) {
    printf("测试: 大块分配\n");
    
    app_pool_t* pool = app_pool_create();
    assert(pool != NULL);
    
    /* 分配超过块大小的内存 */
    void* ptr1 = app_pool_alloc(pool, 8192);  /* 8KB */
    void* ptr2 = app_pool_alloc(pool, 16384); /* 16KB */
    
    assert(ptr1 != NULL);
    assert(ptr2 != NULL);
    
    printf("  分配了 8KB 和 16KB 的大块内存\n");
    
    app_pool_free(pool, ptr1);
    app_pool_free(pool, ptr2);
    
    app_pool_destroy(pool);
    printf("  PASSED\n\n");
}

void test_mixed_allocation(void) {
    printf("测试: 混合大小分配\n");
    
    app_pool_t* pool = app_pool_create();
    assert(pool != NULL);
    
    void* ptrs[50];
    
    /* 分配不同大小的内存 */
    for (int i = 0; i < 50; i++) {
        size_t size = (i % 5 + 1) * 64;  /* 64, 128, 192, 256, 320 */
        ptrs[i] = app_pool_alloc(pool, size);
        assert(ptrs[i] != NULL);
    }
    
    printf("  分配了 50 个不同大小的内存块\n");
    
    /* 释放所有内存 */
    for (int i = 0; i < 50; i++) {
        app_pool_free(pool, ptrs[i]);
    }
    
    app_pool_destroy(pool);
    printf("  PASSED\n\n");
}

int main(void) {
    printf("========================================\n");
    printf("应用层自定义内存池示例\n");
    printf("========================================\n");
    printf("说明:\n");
    printf("  框架层只提供基础分配器接口\n");
    printf("  应用层可以根据需求自由实现内存池\n");
    printf("========================================\n\n");
    
    test_basic_allocation();
    test_statistics();
    test_large_allocation();
    test_mixed_allocation();
    
    printf("========================================\n");
    printf("所有测试通过\n");
    printf("========================================\n");
    printf("总结:\n");
    printf("  应用层可以自由实现自定义内存池\n");
    printf("  框架层保持简洁，只提供基础接口\n");
    printf("  这种设计更加灵活和可扩展\n");
    printf("========================================\n");
    
    return 0;
}