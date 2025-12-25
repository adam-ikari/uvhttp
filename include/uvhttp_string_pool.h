/* UVHTTP 字符串池 - 减少内存分配和复制开销 */

#ifndef UVHTTP_STRING_POOL_H
#define UVHTTP_STRING_POOL_H

#include "uvhttp_allocator.h"
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 字符串池配置 */
#define UVHTTP_STRING_POOL_SIZE      (1024 * 1024)  // 1MB初始大小
#define UVHTTP_STRING_POOL_MAX_SIZE  (16 * 1024 * 1024)  // 16MB最大大小
#define UVHTTP_MIN_STRING_LENGTH     8               // 最小缓存字符串长度
#define UVHTTP_MAX_STRING_LENGTH     1024            // 最大缓存字符串长度

/* 字符串池条目 */
typedef struct uvhttp_string_entry {
    struct uvhttp_string_entry* next;     // 哈希链表下一个条目
    uint32_t hash;                        // 字符串哈希值
    uint32_t length;                      // 字符串长度
    uint32_t ref_count;                   // 引用计数
    char data[];                          // 字符串数据
} uvhttp_string_entry_t;

/* 字符串池管理器 */
typedef struct {
    char* pool_memory;                    // 池内存基址
    size_t pool_size;                     // 当前池大小
    size_t pool_used;                     // 已使用大小
    size_t pool_capacity;                 // 池容量
    
    uvhttp_string_entry_t** hash_table;   // 哈希表
    size_t hash_size;                     // 哈希表大小
    size_t entry_count;                   // 条目数量
    
    /* 统计信息 */
    size_t total_strings;                 // 总字符串数
    size_t pool_hits;                     // 池命中次数
    size_t pool_misses;                   // 池未命中次数
    size_t memory_saved;                  // 节省的内存量
    
    int initialized;                      // 初始化标志
} uvhttp_string_pool_t;

/* 全局字符串池 */
extern uvhttp_string_pool_t g_string_pool;

/* 字符串池管理函数 */
uvhttp_error_t uvhttp_string_pool_init(void);
void uvhttp_string_pool_cleanup(void);
const char* uvhttp_string_pool_intern(const char* str, size_t length);
void uvhttp_string_pool_release(const char* str);
void uvhttp_string_pool_get_stats(size_t* total_strings, 
                                 size_t* pool_hits, 
                                 size_t* pool_misses, 
                                 size_t* memory_saved);

/* 便捷函数 */
static inline const char* uvhttp_string_pool_strdup(const char* str) {
    if (!str) return NULL;
    return uvhttp_string_pool_intern(str, strlen(str));
}

/* 哈希函数 - FNV-1a */
static inline uint32_t uvhttp_string_hash(const char* str, size_t length) {
    uint32_t hash = 2166136261U;
    for (size_t i = 0; i < length; i++) {
        hash ^= (uint8_t)str[i];
        hash *= 16777619U;
    }
    return hash;
}

/*
 * ============================================================================
 * 字符串池优化说明
 * ============================================================================
 * 
 * 性能优势：
 * 1. 零拷贝：相同字符串只存储一份，通过指针共享
 * 2. 减少分配：避免频繁的小内存分配
 * 3. 缓存友好：相似字符串存储在相近位置
 * 4. 引用计数：自动管理字符串生命周期
 * 
 * 使用场景：
 * - HTTP头部字段名（Content-Type, User-Agent等）
 * - URL路径片段
 * - MIME类型字符串
 * - 常用HTTP状态短语
 * 
 * 预期性能提升：
 * - 字符串比较速度提升2-3倍（指针比较vs字符串比较）
 * - 内存使用减少30-50%（去重）
 * - 减少内存分配次数70%+
 * ============================================================================
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_STRING_POOL_H */