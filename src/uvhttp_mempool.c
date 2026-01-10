#include "uvhttp_mempool.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 创建内存池 */
uvhttp_mempool_t* uvhttp_mempool_create(void) {
    uvhttp_mempool_t* pool = uvhttp_malloc(sizeof(uvhttp_mempool_t));
    if (!pool) {
        return NULL;
    }
    
    memset(pool, 0, sizeof(uvhttp_mempool_t));
    return pool;
}

/* 销毁内存池 */
void uvhttp_mempool_destroy(uvhttp_mempool_t* pool) {
    if (!pool) {
        return;
    }
    
    /* 释放所有块 */
    uvhttp_mempool_block_t* block = pool->blocks;
    while (block) {
        uvhttp_mempool_block_t* next = block->next;
        uvhttp_free(block);
        block = next;
    }
    
    /* 释放池结构 */
    uvhttp_free(pool);
}

/* 从内存池分配内存 */
void* uvhttp_mempool_alloc(uvhttp_mempool_t* pool, size_t size) {
    if (!pool || size == 0) {
        return NULL;
    }
    
    /* 对齐到 8 字节边界 */
    size = (size + 7) & ~7;
    
    /* 检查是否超过块大小 */
    if (size > UVHTTP_MEMPOOL_BLOCK_SIZE) {
        /* 大于块大小的分配应该使用普通分配器，内存池不跟踪这些分配 */
        /* 返回 NULL 强制调用者使用 uvhttp_malloc/uvhttp_free */
        return NULL;
    }
    
    /* 检查当前块是否有足够空间 */
    if (!pool->current_block || pool->free_offset + size > UVHTTP_MEMPOOL_BLOCK_SIZE) {
        /* 需要新块 */
        uvhttp_mempool_block_t* new_block = uvhttp_malloc(sizeof(uvhttp_mempool_block_t));
        if (!new_block) {
            return NULL;
        }
        
        new_block->next = pool->blocks;
        pool->blocks = new_block;
        pool->current_block = new_block;
        pool->free_offset = 0;
        pool->block_count++;
    }
    
    /* 从当前块分配 */
    void* ptr = &pool->current_block->data[pool->free_offset];
    pool->free_offset += size;
    
    return ptr;
}

/* 重置内存池 */
void uvhttp_mempool_reset(uvhttp_mempool_t* pool) {
    if (!pool) {
        return;
    }
    
    /* 重置到第一个块 */
    pool->current_block = pool->blocks;
    pool->free_offset = 0;
}

/* 获取内存池统计信息 */
void uvhttp_mempool_stats(uvhttp_mempool_t* pool, size_t* blocks, size_t* used) {
    if (!pool) {
        return;
    }
    
    if (blocks) {
        *blocks = pool->block_count;
    }
    
    if (used) {
        *used = pool->block_count * UVHTTP_MEMPOOL_BLOCK_SIZE - 
                (pool->current_block ? (UVHTTP_MEMPOOL_BLOCK_SIZE - pool->free_offset) : 0);
    }
}