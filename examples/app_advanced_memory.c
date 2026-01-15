/*
 * 应用层高级内存管理示例
 *
 * 演示如何在应用层集成框架分配器和自定义内存池
 * 展示灵活的内存管理策略
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/time.h>
#include <assert.h>

/* ========== 应用层内存管理器 ========== */

typedef enum {
    MEM_ALLOCATOR_SYSTEM,    /* 系统分配器 */
    MEM_ALLOCATOR_MIMALLOC,   /* mimalloc 分配器 */
    MEM_ALLOCATOR_CUSTOM_POOL /* 自定义内存池 */
} mem_allocator_type_t;

typedef struct {
    mem_allocator_type_t type;
    void* custom_pool;  /* 指向自定义内存池 */
} mem_allocator_t;

/* 简单的自定义内存池实现 */
typedef struct {
    void* blocks;
    size_t block_size;
    size_t total_allocs;
} simple_pool_t;

simple_pool_t* simple_pool_create(size_t block_size) {
    simple_pool_t* pool = (simple_pool_t*)malloc(sizeof(simple_pool_t));
    if (!pool) return NULL;
    
    pool->blocks = malloc(block_size);
    pool->block_size = block_size;
    pool->total_allocs = 0;
    
    return pool;
}

void* simple_pool_alloc(simple_pool_t* pool, size_t size) {
    if (!pool) return malloc(size);
    
    /* 简化实现：直接使用 malloc */
    pool->total_allocs++;
    return malloc(size);
}

void simple_pool_free(simple_pool_t* pool, void* ptr) {
    if (!pool) {
        free(ptr);
        return;
    }
    free(ptr);
}

void simple_pool_destroy(simple_pool_t* pool) {
    if (!pool) return;
    if (pool->blocks) free(pool->blocks);
    free(pool);
}

/* ========== 应用层内存管理器接口 ========== */

mem_allocator_t* mem_allocator_create(mem_allocator_type_t type) {
    mem_allocator_t* allocator = (mem_allocator_t*)malloc(sizeof(mem_allocator_t));
    if (!allocator) return NULL;
    
    allocator->type = type;
    allocator->custom_pool = NULL;
    
    if (type == MEM_ALLOCATOR_CUSTOM_POOL) {
        allocator->custom_pool = simple_pool_create(4096);
    }
    
    return allocator;
}

void* mem_alloc(mem_allocator_t* allocator, size_t size) {
    if (!allocator) return malloc(size);
    
    switch (allocator->type) {
        case MEM_ALLOCATOR_SYSTEM:
            return malloc(size);
            
        case MEM_ALLOCATOR_MIMALLOC:
            /* 如果有 mimalloc，使用 mi_malloc */
            return malloc(size);  /* 回退到系统分配器 */
            
        case MEM_ALLOCATOR_CUSTOM_POOL:
            if (allocator->custom_pool) {
                return simple_pool_alloc((simple_pool_t*)allocator->custom_pool, size);
            }
            return malloc(size);
            
        default:
            return malloc(size);
    }
}

void mem_free(mem_allocator_t* allocator, void* ptr) {
    if (!ptr) return;
    
    if (!allocator) {
        free(ptr);
        return;
    }
    
    switch (allocator->type) {
        case MEM_ALLOCATOR_SYSTEM:
            free(ptr);
            break;
            
        case MEM_ALLOCATOR_MIMALLOC:
            /* 如果有 mimalloc，使用 mi_free */
            free(ptr);  /* 回退到系统分配器 */
            break;
            
        case MEM_ALLOCATOR_CUSTOM_POOL:
            if (allocator->custom_pool) {
                simple_pool_free((simple_pool_t*)allocator->custom_pool, ptr);
            } else {
                free(ptr);
            }
            break;
            
        default:
            free(ptr);
            break;
    }
}

void mem_allocator_destroy(mem_allocator_t* allocator) {
    if (!allocator) return;
    
    if (allocator->type == MEM_ALLOCATOR_CUSTOM_POOL && allocator->custom_pool) {
        simple_pool_destroy((simple_pool_t*)allocator->custom_pool);
    }
    
    free(allocator);
}

/* ========== 应用层使用示例 ========== */

void example_http_request_handling(void) {
    printf("示例: HTTP 请求处理\n");
    
    /* 为 HTTP 请求处理创建内存分配器 */
    mem_allocator_t* allocator = mem_allocator_create(MEM_ALLOCATOR_CUSTOM_POOL);
    assert(allocator != NULL);
    
    /* 模拟 HTTP 请求处理 */
    char* request_buffer = (char*)mem_alloc(allocator, 1024);
    char* response_buffer = (char*)mem_alloc(allocator, 2048);
    char* headers = (char*)mem_alloc(allocator, 512);
    
    strcpy(request_buffer, "GET /api/users HTTP/1.1");
    strcpy(response_buffer, "HTTP/1.1 200 OK");
    strcpy(headers, "Content-Type: application/json");
    
    printf("  请求: %s\n", request_buffer);
    printf("  响应: %s\n", response_buffer);
    printf("  头部: %s\n", headers);
    
    /* 处理完成后释放内存 */
    mem_free(allocator, request_buffer);
    mem_free(allocator, response_buffer);
    mem_free(allocator, headers);
    
    mem_allocator_destroy(allocator);
    printf("  完成\n\n");
}

void example_database_connection_pool(void) {
    printf("示例: 数据库连接池\n");
    
    /* 为数据库连接池创建内存分配器 */
    mem_allocator_t* allocator = mem_allocator_create(MEM_ALLOCATOR_SYSTEM);
    assert(allocator != NULL);
    
    /* 模拟数据库连接 */
    typedef struct {
        int id;
        char host[64];
        int port;
        void* connection;
    } db_connection_t;
    
    /* 创建连接池 */
    db_connection_t* connections[10];
    for (int i = 0; i < 10; i++) {
        connections[i] = (db_connection_t*)mem_alloc(allocator, sizeof(db_connection_t));
        connections[i]->id = i;
        snprintf(connections[i]->host, sizeof(connections[i]->host), "db-server-%d", i);
        connections[i]->port = 3306 + i;
        connections[i]->connection = mem_alloc(allocator, 1024);
    }
    
    printf("  创建了 10 个数据库连接\n");
    
    /* 使用连接 */
    for (int i = 0; i < 10; i++) {
        printf("  连接 %d: %s:%d\n", connections[i]->id, connections[i]->host, connections[i]->port);
        mem_free(allocator, connections[i]->connection);
        mem_free(allocator, connections[i]);
    }
    
    mem_allocator_destroy(allocator);
    printf("  完成\n\n");
}

void example_cache_with_different_allocators(void) {
    printf("示例: 使用不同分配器的缓存\n");
    
    /* 为小对象缓存创建内存池分配器 */
    mem_allocator_t* pool_allocator = mem_allocator_create(MEM_ALLOCATOR_CUSTOM_POOL);
    
    /* 为大对象缓存使用系统分配器 */
    mem_allocator_t* system_allocator = mem_allocator_create(MEM_ALLOCATOR_SYSTEM);
    
    /* 小对象缓存 */
    char* cache_keys[100];
    for (int i = 0; i < 100; i++) {
        cache_keys[i] = (char*)mem_alloc(pool_allocator, 32);
        snprintf(cache_keys[i], 32, "key-%d", i);
    }
    
    /* 大对象缓存 */
    char* large_objects[10];
    for (int i = 0; i < 10; i++) {
        large_objects[i] = (char*)mem_alloc(system_allocator, 4096);
        snprintf(large_objects[i], 4096, "large-object-%d-data", i);
    }
    
    printf("  小对象缓存: 100 个键\n");
    printf("  大对象缓存: 10 个对象\n");
    
    /* 释放小对象 */
    for (int i = 0; i < 100; i++) {
        mem_free(pool_allocator, cache_keys[i]);
    }
    
    /* 释放大对象 */
    for (int i = 0; i < 10; i++) {
        mem_free(system_allocator, large_objects[i]);
    }
    
    mem_allocator_destroy(pool_allocator);
    mem_allocator_destroy(system_allocator);
    printf("  完成\n\n");
}

/* ========== 性能对比测试 ========== */

void performance_comparison(void) {
    printf("示例: 性能对比\n");

    const int iterations = 10000;
    const int size = 128;

    /* 测试系统分配器 */
    mem_allocator_t* system_alloc = mem_allocator_create(MEM_ALLOCATOR_SYSTEM);

    struct timeval start, end;
    gettimeofday(&start, NULL);

    void* ptrs1[100];
    memset(ptrs1, 0, sizeof(ptrs1));
    for (int i = 0; i < iterations; i++) {
        int idx = i % 100;
        if (ptrs1[idx]) mem_free(system_alloc, ptrs1[idx]);
        ptrs1[idx] = mem_alloc(system_alloc, size);
    }

    gettimeofday(&end, NULL);
    long system_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    /* 清理 */
    for (int i = 0; i < 100; i++) {
        if (ptrs1[i]) mem_free(system_alloc, ptrs1[i]);
    }

    mem_allocator_destroy(system_alloc);

    /* 测试自定义内存池 */
    mem_allocator_t* pool_alloc = mem_allocator_create(MEM_ALLOCATOR_CUSTOM_POOL);

    gettimeofday(&start, NULL);

    void* ptrs2[100];
    memset(ptrs2, 0, sizeof(ptrs2));
    for (int i = 0; i < iterations; i++) {
        int idx = i % 100;
        if (ptrs2[idx]) mem_free(pool_alloc, ptrs2[idx]);
        ptrs2[idx] = mem_alloc(pool_alloc, size);
    }

    gettimeofday(&end, NULL);
    long pool_ms = (end.tv_sec - start.tv_sec) * 1000 + (end.tv_usec - start.tv_usec) / 1000;

    /* 清理 */
    for (int i = 0; i < 100; i++) {
        if (ptrs2[i]) mem_free(pool_alloc, ptrs2[i]);
    }

    mem_allocator_destroy(pool_alloc);

    printf("  系统分配器: %ld ms (%.2f ops/ms)\n", system_ms, (double)iterations / system_ms);
    printf("  自定义内存池: %ld ms (%.2f ops/ms)\n", pool_ms, (double)iterations / pool_ms);
    printf("  完成\n\n");
}

int main(void) {
    printf("========================================\n");
    printf("应用层高级内存管理示例\n");
    printf("========================================\n");
    printf("说明:\n");
    printf("  应用层可以自由选择和组合不同的内存分配策略\n");
    printf("  框架层只提供基础接口，不限制应用层的实现\n");
    printf("========================================\n\n");
    
    example_http_request_handling();
    example_database_connection_pool();
    example_cache_with_different_allocators();
    performance_comparison();
    
    printf("========================================\n");
    printf("所有示例运行完成\n");
    printf("========================================\n");
    printf("总结:\n");
    printf("  应用层拥有完全的内存管理自由\n");
    printf("  可以根据场景选择最合适的分配策略\n");
    printf("  框架层保持简洁，不限制应用层的灵活性\n");
    printf("========================================\n");
    
    return 0;
}