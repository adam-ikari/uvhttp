#ifndef ALLOCATOR_CONFIG_H
#define ALLOCATOR_CONFIG_H

// 内存分配器配置示例

// 使用内存池分配器
#ifdef USE_POOL
#define UVHTTP_ALLOCATOR UVHTTP_ALLOCATOR_POOL
#define UVHTTP_POOL_SIZE (128 * 1024)  // 128KB
#endif

// 使用统计分配器
#ifdef USE_STATS
#define UVHTTP_ALLOCATOR UVHTTP_ALLOCATOR_STATS
#endif

// 使用mimalloc
#ifdef USE_MIMALLOC
#define UVHTTP_ALLOCATOR UVHTTP_ALLOCATOR_CUSTOM
#endif

// 自定义分配器示例
#ifdef USE_CUSTOM
#define UVHTTP_ALLOCATOR UVHTTP_ALLOCATOR_CUSTOM

extern void* my_malloc(size_t);
extern void my_free(void*);

UVHTTP_ALLOCATOR(my, my_malloc, my_free);
#endif

#endif /* ALLOCATOR_CONFIG_H */