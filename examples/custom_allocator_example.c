/**
 * @file custom_allocator_example.c
 * @brief 自定义内存分配器示例实现
 * 
 * 本文件展示了如何实现自定义内存分配器，并通过外部链接的方式与uvhttp集成。
 * 编译时使用 -DUVHTTP_ALLOCATOR_TYPE=2 来启用自定义分配器。
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* 简单的内存池实现示例 */
#define POOL_SIZE (1024 * 1024)  /* 1MB内存池 */
#define ALIGNMENT 8              /* 8字节对齐 */

typedef struct {
    void* pool;
    size_t offset;
    size_t total_size;
    size_t allocated_count;
    size_t freed_count;
} memory_pool_t;

static memory_pool_t g_pool = {0};

/* 初始化内存池 */
static void init_pool(void) {
    if (!g_pool.pool) {
        g_pool.pool = malloc(POOL_SIZE);
        if (g_pool.pool) {
            g_pool.offset = 0;
            g_pool.total_size = POOL_SIZE;
            g_pool.allocated_count = 0;
            g_pool.freed_count = 0;
        }
    }
}

/* 对齐内存分配 */
static void* align_ptr(void* ptr, size_t alignment) {
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    return (void*)aligned;
}

/* 自定义内存分配函数 */
void* uvhttp_custom_malloc(size_t size) {
    init_pool();
    
    if (!g_pool.pool) {
        /* 内存池初始化失败，回退到标准分配 */
        return malloc(size);
    }
    
    /* 对齐分配 */
    void* ptr = (char*)g_pool.pool + g_pool.offset;
    ptr = align_ptr(ptr, ALIGNMENT);
    
    /* 计算实际需要的空间 */
    size_t actual_size = ((char*)ptr - ((char*)g_pool.pool + g_pool.offset)) + size;
    
    /* 检查是否有足够空间 */
    if (g_pool.offset + actual_size > g_pool.total_size) {
        /* 内存池空间不足，回退到标准分配 */
        printf("Pool exhausted, falling back to malloc for %zu bytes\n", size);
        return malloc(size);
    }
    
    g_pool.offset += actual_size;
    g_pool.allocated_count++;
    
    return ptr;
}

/* 自定义内存重新分配函数 */
void* uvhttp_custom_realloc(void* ptr, size_t size) {
    if (!ptr) {
        return uvhttp_custom_malloc(size);
    }
    
    if (size == 0) {
        uvhttp_custom_free(ptr);
        return NULL;
    }
    
    /* 简单实现：分配新内存，复制数据，释放旧内存 */
    void* new_ptr = uvhttp_custom_malloc(size);
    if (new_ptr) {
        /* 这里简化处理，实际应该知道原始大小 */
        memcpy(new_ptr, ptr, size < 1024 ? size : 1024); /* 假设最大1KB */
        uvhttp_custom_free(ptr);
    }
    
    return new_ptr;
}

/* 自定义内存释放函数 */
void uvhttp_custom_free(void* ptr) {
    if (!ptr) {
        return;
    }
    
    /* 检查是否在内存池范围内 */
    if (ptr >= g_pool.pool && ptr < (char*)g_pool.pool + g_pool.total_size) {
        /* 内存池中的内存，简单计数 */
        g_pool.freed_count++;
        /* 注意：简单内存池不实际释放内存，实际应用中可以实现更复杂的策略 */
    } else {
        /* 非内存池内存，使用标准free */
        free(ptr);
    }
}

/* 自定义内存清零分配函数 */
void* uvhttp_custom_calloc(size_t nmemb, size_t size) {
    size_t total_size = nmemb * size;
    void* ptr = uvhttp_custom_malloc(total_size);
    
    if (ptr) {
        memset(ptr, 0, total_size);
    }
    
    return ptr;
}

/* 获取内存池统计信息 */
void uvhttp_custom_allocator_stats(size_t* pool_size, size_t* used_size, 
                                  size_t* allocated_count, size_t* freed_count) {
    if (pool_size) *pool_size = g_pool.total_size;
    if (used_size) *used_size = g_pool.offset;
    if (allocated_count) *allocated_count = g_pool.allocated_count;
    if (freed_count) *freed_count = g_pool.freed_count;
}

/* 清理内存池 */
void uvhttp_custom_allocator_cleanup(void) {
    if (g_pool.pool) {
        printf("Cleaning up memory pool: allocated=%zu, freed=%zu\n", 
               g_pool.allocated_count, g_pool.freed_count);
        free(g_pool.pool);
        g_pool.pool = NULL;
        g_pool.offset = 0;
        g_pool.total_size = 0;
        g_pool.allocated_count = 0;
        g_pool.freed_count = 0;
    }
}