/* UVHTTP LRU缓存模块 - 基于uthash实现 */

#ifndef UVHTTP_LRU_CACHE_H
#define UVHTTP_LRU_CACHE_H

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_constants.h"
#    include "uvhttp_error.h"

#    include <stddef.h>
#    include <time.h>

/* 包含uthash头文件 */
#    include "uthash.h"

#    ifdef __cplusplus
extern "C" {
#    endif

/* 前向声明 */
typedef struct cache_entry cache_entry_t;
typedef struct cache_manager cache_manager_t;

/* LRU缓存条目结构 */
struct cache_entry {
    char file_path[UVHTTP_MAX_FILE_PATH_SIZE];    /* 文件路径 */
    char* content;                                /* 文件内容 */
    size_t content_length;                        /* 内容长度 */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE]; /* MIME类型 */
    time_t last_modified;                         /* 最后修改时间 */
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];      /* ETag值 */
    time_t access_time;                           /* 最后访问时间 */
    time_t cache_time;                            /* 缓存时间 */
    size_t memory_usage;                          /* 内存使用量 */
    int is_compressed;                            /* 是否压缩 */

    /* uthash哈希句柄 */
    UT_hash_handle hh;

    /* LRU链表句柄 */
    struct cache_entry* lru_prev;
    struct cache_entry* lru_next;
};

/* LRU缓存管理器 */
struct cache_manager {
    cache_entry_t* hash_table; /* 哈希表头 */
    cache_entry_t* lru_head;   /* LRU链表头 */
    cache_entry_t* lru_tail;   /* LRU链表尾 */
    size_t total_memory_usage; /* 总内存使用 */
    size_t max_memory_usage;   /* 最大内存限制 */
    int entry_count;           /* 缓存条目数 */
    int max_entries;           /* 最大条目数 */
    int cache_ttl;             /* 缓存TTL（秒） */

    /* 统计信息 */
    int hit_count;      /* 命中次数 */
    int miss_count;     /* 未命中次数 */
    int eviction_count; /* 驱逐次数 */
};

/**
 * 创建LRU缓存管理器
 *
 * @param max_memory_usage 最大内存使用量（字节）
 * @param max_entries 最大缓存条目数
 * @param cache_ttl 缓存TTL（秒），0表示永不过期
 * @param cache 输出参数，返回创建的缓存管理器指针
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_lru_cache_create(size_t max_memory_usage, int max_entries,
                                       int cache_ttl, cache_manager_t** cache);

/**
 * 释放LRU缓存管理器
 *
 * @param cache 缓存管理器
 */
void uvhttp_lru_cache_free(cache_manager_t* cache);

/**
 * 查找缓存条目
 *
 * @param cache 缓存管理器
 * @param file_path 文件路径
 * @return 缓存条目指针，未找到返回NULL
 */
cache_entry_t* uvhttp_lru_cache_find(cache_manager_t* cache,
                                     const char* file_path);

/**
 * 添加或更新缓存条目
 *
 * @param cache 缓存管理器
 * @param file_path 文件路径
 * @param content 文件内容
 * @param content_length 内容长度
 * @param mime_type MIME类型
 * @param last_modified 最后修改时间
 * @param etag ETag值
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_error_t uvhttp_lru_cache_put(cache_manager_t* cache,
                                    const char* file_path, char* content,
                                    size_t content_length,
                                    const char* mime_type, time_t last_modified,
                                    const char* etag);

/**
 * 删除缓存条目
 *
 * @param cache 缓存管理器
 * @param file_path 文件路径
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_error_t uvhttp_lru_cache_remove(cache_manager_t* cache,
                                       const char* file_path);

/**
 * 清空所有缓存
 *
 * @param cache 缓存管理器
 */
void uvhttp_lru_cache_clear(cache_manager_t* cache);

/**
 * 获取缓存统计信息
 *
 * @param cache 缓存管理器
 * @param total_memory_usage 输出总内存使用量
 * @param entry_count 输出条目数量
 * @param hit_count 输出命中次数
 * @param miss_count 输出未命中次数
 * @param eviction_count 输出驱逐次数
 */
void uvhttp_lru_cache_get_stats(cache_manager_t* cache,
                                size_t* total_memory_usage, int* entry_count,
                                int* hit_count, int* miss_count,
                                int* eviction_count);

/**
 * 重置统计信息
 *
 * @param cache 缓存管理器
 */
void uvhttp_lru_cache_reset_stats(cache_manager_t* cache);

/**
 * 清理过期条目
 *
 * @param cache 缓存管理器
 * @return 清理的条目数量
 */
int uvhttp_lru_cache_cleanup_expired(cache_manager_t* cache);

/**
 * 检查缓存条目是否过期
 *
 * @param entry 缓存条目
 * @param cache_ttl 缓存TTL
 * @return 1过期，0未过期
 */
int uvhttp_lru_cache_is_expired(cache_entry_t* entry, int cache_ttl);

/**
 * 移动条目到LRU链表头部
 *
 * @param cache 缓存管理器
 * @param entry 缓存条目
 */
void uvhttp_lru_cache_move_to_head(cache_manager_t* cache,
                                   cache_entry_t* entry);

/**
 * 从LRU链表尾部移除条目
 *
 * @param cache 缓存管理器
 * @return 被移除的条目指针
 */
cache_entry_t* uvhttp_lru_cache_remove_tail(cache_manager_t* cache);

/**
 * 计算缓存命中率
 *
 * @param cache 缓存管理器
 * @return 命中率（0.0-1.0）
 */
double uvhttp_lru_cache_get_hit_rate(cache_manager_t* cache);

#    ifdef __cplusplus
}
#    endif

#endif /* UVHTTP_FEATURE_STATIC_FILES */

#endif /* UVHTTP_LRU_CACHE_H */