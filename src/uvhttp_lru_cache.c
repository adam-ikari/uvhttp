/* UVHTTP LRU缓存模块实现 - 基于uthash，单线程版本 */

#if UVHTTP_FEATURE_STATIC_FILES

#include "uvhttp_lru_cache.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error.h"
#include <string.h>
#include <stdlib.h>
#include <time.h>

/* 包含uthash头文件 */
#include "uthash.h"

/**
 * 创建LRU缓存管理器
 */
cache_manager_t* uvhttp_lru_cache_create(size_t max_memory_usage, 
                                         int max_entries, 
                                         int cache_ttl) {
    UVHTTP_LOG_DEBUG("Creating LRU cache: max_memory=%zu, max_entries=%d, ttl=%d", 
                     max_memory_usage, max_entries, cache_ttl);
    
    cache_manager_t* cache = uvhttp_malloc(sizeof(cache_manager_t));
    if (!cache) {
        UVHTTP_LOG_ERROR("Failed to create LRU cache: memory allocation error");
        uvhttp_handle_memory_failure("cache_manager", NULL, NULL);
        return NULL;
    }
    
    memset(cache, 0, sizeof(cache_manager_t));
    cache->max_memory_usage = max_memory_usage;
    cache->max_entries = max_entries;
    cache->cache_ttl = cache_ttl;
    
    /* 单线程版本：不需要初始化锁 */
    
    UVHTTP_LOG_INFO("LRU cache created successfully");
    return cache;
}

/**
 * 释放缓存条目
 */
static void free_cache_entry(cache_entry_t* entry) {
    if (!entry) return;
    
    if (entry->content) {
        uvhttp_free(entry->content);
    }
    uvhttp_free(entry);
}

/**
 * 释放LRU缓存管理器
 */
void uvhttp_lru_cache_free(cache_manager_t* cache) {
    if (!cache) return;
    
    UVHTTP_LOG_DEBUG("Freeing LRU cache: current entries=%d, memory usage=%zu", 
                     cache->entry_count, cache->total_memory_usage);
    
    uvhttp_lru_cache_clear(cache);
    
    /* 单线程版本：不需要销毁锁 */
    
    uvhttp_free(cache);
    UVHTTP_LOG_DEBUG("LRU cache freed");
}

/**
 * 查找缓存条目 - 单线程版本
 */
cache_entry_t* uvhttp_lru_cache_find(cache_manager_t* cache, 
                                      const char* file_path) {
    if (!cache || !file_path) {
        UVHTTP_LOG_WARN("Invalid cache lookup parameters: cache=%p, file_path=%p", cache, file_path);
        return NULL;
    }
    
    UVHTTP_LOG_DEBUG("Looking up cache entry: %s", file_path);
    
    cache_entry_t* entry = NULL;
    
    /* 单线程版本：不需要加锁 */
    
    HASH_FIND_STR(cache->hash_table, file_path, entry);
    
    if (entry) {
        /* 检查是否过期 */
        if (uvhttp_lru_cache_is_expired(entry, cache->cache_ttl)) {
            UVHTTP_LOG_DEBUG("Cache entry expired: %s", file_path);
            /* 移除过期条目 */
            uvhttp_lru_cache_remove(cache, file_path);
            cache->miss_count++;
            return NULL;
        }
        
        /* 更新访问时间并移到LRU头部 */
        entry->access_time = time(NULL);
        uvhttp_lru_cache_move_to_head(cache, entry);
        cache->hit_count++;
        UVHTTP_LOG_DEBUG("Cache hit: %s (size: %zu)", file_path, entry->content_length);
        return entry;
    }
    
    cache->miss_count++;
    UVHTTP_LOG_DEBUG("Cache miss: %s", file_path);
    return NULL;
}

/**
 * 移动条目到LRU链表头部
 */
void uvhttp_lru_cache_move_to_head(cache_manager_t* cache, cache_entry_t* entry) {
    if (!cache || !entry) return;
    
    /* 如果已经是头部，无需移动 */
    if (entry == cache->lru_head) return;
    
    /* 检查entry是否在链表中（防止野指针） */
    if (entry->lru_prev == NULL && entry != cache->lru_head && cache->lru_head != NULL) {
        /* entry不在链表中，直接添加到头部 */
        entry->lru_prev = NULL;
        entry->lru_next = cache->lru_head;
        
        if (cache->lru_head) {
            cache->lru_head->lru_prev = entry;
        }
        
        cache->lru_head = entry;
        
        /* 如果链表为空，尾部也指向entry */
        if (!cache->lru_tail) {
            cache->lru_tail = entry;
        }
        return;
    }
    
    /* 从当前位置移除 */
    if (entry->lru_prev) {
        entry->lru_prev->lru_next = entry->lru_next;
    } else {
        /* entry是头部 */
        cache->lru_head = entry->lru_next;
    }
    
    if (entry->lru_next) {
        entry->lru_next->lru_prev = entry->lru_prev;
    } else {
        /* entry是尾部 */
        cache->lru_tail = entry->lru_prev;
    }
    
    /* 移动到头部 */
    entry->lru_prev = NULL;
    entry->lru_next = cache->lru_head;
    
    if (cache->lru_head) {
        cache->lru_head->lru_prev = entry;
    }
    
    cache->lru_head = entry;
    
    /* 如果链表为空，尾部也指向entry */
    if (!cache->lru_tail) {
        cache->lru_tail = entry;
    }
}

/**
 * 从LRU链表尾部移除条目
 */
cache_entry_t* uvhttp_lru_cache_remove_tail(cache_manager_t* cache) {
    if (!cache || !cache->lru_tail) return NULL;
    
    cache_entry_t* entry = cache->lru_tail;
    
    /* 更新尾部指针 */
    cache->lru_tail = entry->lru_prev;
    
    if (cache->lru_tail) {
        cache->lru_tail->lru_next = NULL;
    } else {
        /* 链表为空，头部也置空 */
        cache->lru_head = NULL;
    }
    
    /* 清理链表指针 */
    entry->lru_prev = NULL;
    entry->lru_next = NULL;
    
    return entry;
}

/**
 * 检查缓存条目是否过期
 */
int uvhttp_lru_cache_is_expired(cache_entry_t* entry, int cache_ttl) {
    if (!entry || cache_ttl <= 0) return 0;
    
    time_t now = time(NULL);
    return (now - entry->cache_time) > cache_ttl;
}

/**
 * 添加或更新缓存条目 - 单线程版本
 */
uvhttp_error_t uvhttp_lru_cache_put(cache_manager_t* cache,
                                     const char* file_path,
                                     char* content,
                                     size_t content_length,
                                     const char* mime_type,
                                     time_t last_modified,
                                     const char* etag) {
    if (!cache || !file_path || !content) {
        UVHTTP_LOG_WARN("Invalid cache add parameters: cache=%p, file_path=%p, content=%p", 
                        cache, file_path, content);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 检查文件大小是否超过限制 */
    if (content_length > UVHTTP_STATIC_MAX_FILE_SIZE) {
        UVHTTP_LOG_WARN("File size exceeds limit: %s (size: %zu, limit: %d)", 
                        file_path, content_length, UVHTTP_STATIC_MAX_FILE_SIZE);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 计算内存使用量，检查整数溢出 */
    if (content_length > SIZE_MAX - sizeof(cache_entry_t)) {
        UVHTTP_LOG_ERROR("Content length too large: %zu (causes integer overflow)", content_length);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    size_t memory_usage = sizeof(cache_entry_t) + content_length;
    
    UVHTTP_LOG_DEBUG("Adding cache entry: %s (size: %zu, type: %s)", 
                     file_path, content_length, mime_type ? mime_type : "unknown");
    
    /* 单线程版本：不需要加锁 */
    
    /* 检查是否需要驱逐条目 */
    int eviction_count = 0;
    while ((cache->max_memory_usage > 0 && 
            cache->total_memory_usage + memory_usage > cache->max_memory_usage) ||
           (cache->max_entries > 0 && 
            cache->entry_count >= cache->max_entries)) {
        
        cache_entry_t* evicted = uvhttp_lru_cache_remove_tail(cache);
        if (!evicted) break;
        
        UVHTTP_LOG_DEBUG("Evicting cache entry: %s (freeing memory: %zu)", 
                         evicted->file_path, evicted->memory_usage);
        
        /* 从哈希表中移除 */
        HASH_DEL(cache->hash_table, evicted);
        
        /* 更新统计 */
        cache->total_memory_usage -= evicted->memory_usage;
        cache->entry_count--;
        cache->eviction_count++;
        eviction_count++;
        
        /* 释放内存 */
        free_cache_entry(evicted);
    }
    
    if (eviction_count > 0) {
        UVHTTP_LOG_INFO("Evicted %d cache entries to make room for new entry", eviction_count);
    }
    
    /* 查找是否已存在 */
    cache_entry_t* entry = NULL;
    HASH_FIND_STR(cache->hash_table, file_path, entry);
    
    if (entry) {
        /* 更新现有条目 */
        UVHTTP_LOG_DEBUG("Updating existing cache entry: %s (old size: %zu, new size: %zu)", 
                         file_path, entry->content_length, content_length);
        uvhttp_free(entry->content);
        
        /* 更新内存使用量 */
        cache->total_memory_usage -= entry->memory_usage;
    } else {
        /* 创建新条目 */
        entry = uvhttp_malloc(sizeof(cache_entry_t));
        if (!entry) {
            UVHTTP_LOG_ERROR("Failed to create cache entry: memory allocation error");
            uvhttp_handle_memory_failure("cache_entry", NULL, NULL);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        memset(entry, 0, sizeof(cache_entry_t));
        
        /* 验证文件路径长度，防止缓冲区溢出 */
        if (strlen(file_path) >= sizeof(entry->file_path)) {
            UVHTTP_LOG_ERROR("File path too long: %s (max: %zu)", file_path, sizeof(entry->file_path) - 1);
            uvhttp_free(entry);
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        
        strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
        entry->file_path[sizeof(entry->file_path) - 1] = '\0';
        
        /* 初始化LRU链表指针 */
        entry->lru_prev = NULL;
        entry->lru_next = NULL;
        
        /* 添加到哈希表 */
        HASH_ADD_STR(cache->hash_table, file_path, entry);
        cache->entry_count++;
        
        UVHTTP_LOG_DEBUG("Created new cache entry: %s", file_path);
    }
    
    /* 设置条目内容 */
    entry->content = content;
    entry->content_length = content_length;
    entry->memory_usage = memory_usage;
    entry->last_modified = last_modified;
    entry->access_time = time(NULL);
    entry->cache_time = entry->access_time;
    entry->is_compressed = 0;
    
    /* 设置MIME类型 */
    if (mime_type) {
        strncpy(entry->mime_type, mime_type, sizeof(entry->mime_type) - 1);
        entry->mime_type[sizeof(entry->mime_type) - 1] = '\0';
    } else {
        strncpy(entry->mime_type, "application/octet-stream", sizeof(entry->mime_type) - 1);
        entry->mime_type[sizeof(entry->mime_type) - 1] = '\0';
    }
    
    /* 设置ETag */
    if (etag) {
        strncpy(entry->etag, etag, sizeof(entry->etag) - 1);
        entry->etag[sizeof(entry->etag) - 1] = '\0';
    } else {
        entry->etag[0] = '\0';
    }
    
    /* 更新内存使用量 */
    cache->total_memory_usage += memory_usage;
    
    /* 移动到LRU头部 */
    uvhttp_lru_cache_move_to_head(cache, entry);
    
    UVHTTP_LOG_DEBUG("Cache entry added successfully: %s (total memory: %zu, entries: %d)", 
                     file_path, cache->total_memory_usage, cache->entry_count);
    
    return UVHTTP_OK;
}

/**
 * 删除缓存条目 - 单线程版本
 */
uvhttp_error_t uvhttp_lru_cache_remove(cache_manager_t* cache, const char* file_path) {
    if (!cache || !file_path) {
        UVHTTP_LOG_WARN("Invalid cache remove parameters: cache=%p, file_path=%p", cache, file_path);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    UVHTTP_LOG_DEBUG("Removing cache entry: %s", file_path);
    
    /* 单线程版本：不需要加锁 */
    
    cache_entry_t* entry = NULL;
    HASH_FIND_STR(cache->hash_table, file_path, entry);
    
    if (!entry) {
        UVHTTP_LOG_WARN("Attempting to remove non-existent cache entry: %s", file_path);
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    /* 从哈希表中移除 */
    HASH_DEL(cache->hash_table, entry);
    
    /* 从LRU链表中移除 */
    if (entry->lru_prev) {
        entry->lru_prev->lru_next = entry->lru_next;
    } else {
        cache->lru_head = entry->lru_next;
    }
    
    if (entry->lru_next) {
        entry->lru_next->lru_prev = entry->lru_prev;
    } else {
        cache->lru_tail = entry->lru_prev;
    }
    
    /* 更新统计 */
    cache->total_memory_usage -= entry->memory_usage;
    cache->entry_count--;
    
    UVHTTP_LOG_DEBUG("Cache entry removed successfully: %s (freed memory: %zu)", 
                     file_path, entry->memory_usage);
    
    /* 释放内存 */
    free_cache_entry(entry);
    
    return UVHTTP_OK;
}

/**
 * 清空所有缓存 - 单线程版本
 */
void uvhttp_lru_cache_clear(cache_manager_t* cache) {
    if (!cache) return;
    
    UVHTTP_LOG_INFO("Clearing all cache entries: current entries=%d, total memory=%zu", 
                    cache->entry_count, cache->total_memory_usage);
    
    /* 单线程版本：不需要加锁 */
    
    int cleared_count = 0;
    size_t freed_memory = 0;
    
    cache_entry_t* entry, *tmp;
    HASH_ITER(hh, cache->hash_table, entry, tmp) {
        HASH_DEL(cache->hash_table, entry);
        freed_memory += entry->memory_usage;
        free_cache_entry(entry);
        cleared_count++;
    }
    
    cache->hash_table = NULL;
    cache->lru_head = NULL;
    cache->lru_tail = NULL;
    cache->total_memory_usage = 0;
    cache->entry_count = 0;
    
    UVHTTP_LOG_INFO("Cache cleared: removed %d entries, freed %zu bytes", 
                    cleared_count, freed_memory);
}

/**
 * 获取缓存统计信息 - 单线程版本
 */
void uvhttp_lru_cache_get_stats(cache_manager_t* cache,
                               size_t* total_memory_usage,
                               int* entry_count,
                               int* hit_count,
                               int* miss_count,
                               int* eviction_count) {
    if (!cache) return;
    
    /* 单线程版本：不需要加锁 */
    
    if (total_memory_usage) *total_memory_usage = cache->total_memory_usage;
    if (entry_count) *entry_count = cache->entry_count;
    if (hit_count) *hit_count = cache->hit_count;
    if (miss_count) *miss_count = cache->miss_count;
    if (eviction_count) *eviction_count = cache->eviction_count;
}

/**
 * 重置统计信息 - 单线程版本
 */
void uvhttp_lru_cache_reset_stats(cache_manager_t* cache) {
    if (!cache) return;
    
    /* 单线程版本：不需要加锁 */
    
    cache->hit_count = 0;
    cache->miss_count = 0;
    cache->eviction_count = 0;
}

/**
 * 清理过期条目 - 单线程版本
 */
int uvhttp_lru_cache_cleanup_expired(cache_manager_t* cache) {
    if (!cache || cache->cache_ttl <= 0) return 0;
    
    UVHTTP_LOG_DEBUG("Starting cleanup of expired cache entries (TTL: %d seconds)", cache->cache_ttl);
    
    /* 单线程版本：不需要加锁 */
    
    int cleaned_count = 0;
    size_t freed_memory = 0;
    time_t now = time(NULL);
    
    cache_entry_t* entry = cache->lru_tail;
    while (entry) {
        cache_entry_t* next = entry->lru_prev;
        
        if ((now - entry->cache_time) > cache->cache_ttl) {
            UVHTTP_LOG_DEBUG("Cleaning up expired entry: %s (expired %ld seconds ago)", 
                             entry->file_path, now - entry->cache_time);
            
            /* 从哈希表中移除 */
            HASH_DEL(cache->hash_table, entry);
            
            /* 从LRU链表中移除 */
            if (entry->lru_prev) {
                entry->lru_prev->lru_next = entry->lru_next;
            } else {
                cache->lru_tail = entry->lru_next;
            }
            
            if (entry->lru_next) {
                entry->lru_next->lru_prev = entry->lru_prev;
            } else {
                cache->lru_head = entry->lru_prev;
            }
            
            /* 更新统计 */
            cache->total_memory_usage -= entry->memory_usage;
            cache->entry_count--;
            cleaned_count++;
            freed_memory += entry->memory_usage;
            
            /* 释放内存 */
            free_cache_entry(entry);
        }
        
        entry = next;
    }
    
    if (cleaned_count > 0) {
        UVHTTP_LOG_INFO("Expired cache cleanup completed: removed %d entries, freed %zu bytes", 
                        cleaned_count, freed_memory);
    } else {
        UVHTTP_LOG_DEBUG("No expired cache entries found");
    }
    
    return cleaned_count;
}

/**
 * 计算缓存命中率 - 单线程版本
 */
double uvhttp_lru_cache_get_hit_rate(cache_manager_t* cache) {
    if (!cache || (cache->hit_count + cache->miss_count) == 0) {
        UVHTTP_LOG_DEBUG("Cache hit rate calculation: no statistics available");
        return 0.0;
    }
    
    /* 单线程版本：不需要加锁 */
    
    double hit_rate = (double)cache->hit_count / (cache->hit_count + cache->miss_count);
    
    UVHTTP_LOG_DEBUG("Cache hit rate: %.2f%% (hits: %d, misses: %d)", 
                     hit_rate * 100.0, cache->hit_count, cache->miss_count);
    
    return hit_rate;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */