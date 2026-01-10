#ifndef UVHTTP_MEMPOOL_H
#define UVHTTP_MEMPOOL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 内存池块大小 */
#define UVHTTP_MEMPOOL_BLOCK_SIZE 4096

/* 内存池块结构 */
typedef struct uvhttp_mempool_block {
    struct uvhttp_mempool_block* next;
    uint8_t data[UVHTTP_MEMPOOL_BLOCK_SIZE];
} uvhttp_mempool_block_t;

/* 内存池结构 */
typedef struct {
    uvhttp_mempool_block_t* blocks;
    size_t block_count;
    size_t free_offset;
    uvhttp_mempool_block_t* current_block;
} uvhttp_mempool_t;

/* 创建内存池 */
uvhttp_mempool_t* uvhttp_mempool_create(void);

/* 销毁内存池 */
void uvhttp_mempool_destroy(uvhttp_mempool_t* pool);

/* 从内存池分配内存 */
void* uvhttp_mempool_alloc(uvhttp_mempool_t* pool, size_t size);

/* 重置内存池（释放所有分配的内存，但不销毁池） */
void uvhttp_mempool_reset(uvhttp_mempool_t* pool);

/* 获取内存池统计信息 */
void uvhttp_mempool_stats(uvhttp_mempool_t* pool, size_t* blocks, size_t* used);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MEMPOOL_H */