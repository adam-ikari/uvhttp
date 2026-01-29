/* uvhttp_lru_cache.c 完整覆盖率测试 */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_lru_cache.h"
#include "uvhttp_error.h"
#include <string.h>
#include <time.h>

/* 测试LRU缓存创建和释放 */
TEST(UvhttpLruCacheFullCoverageTest, CacheCreateAndFree) {
    /* 创建缓存 */
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(cache, nullptr);
    
    /* 验证初始状态 */
    EXPECT_EQ(cache->total_memory_usage, 0);
    EXPECT_EQ(cache->entry_count, 0);
    EXPECT_EQ(cache->hit_count, 0);
    EXPECT_EQ(cache->miss_count, 0);
    EXPECT_EQ(cache->eviction_count, 0);
    EXPECT_EQ(cache->max_memory_usage, 1024 * 1024);
    EXPECT_EQ(cache->max_entries, 100);
    EXPECT_EQ(cache->cache_ttl, 3600);
    
    /* 释放缓存 */
    uvhttp_lru_cache_free(cache);
    
    /* 测试释放NULL */
    uvhttp_lru_cache_free(NULL);
}

/* 测试LRU缓存创建失败 */
TEST(UvhttpLruCacheFullCoverageTest, CacheCreateFailure) {
    /* 测试NULL输出参数 */
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零内存限制 */
    cache_manager_t* cache = NULL;
    result = uvhttp_lru_cache_create(0, 100, 3600, &cache);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试零条目数 */
    result = uvhttp_lru_cache_create(1024 * 1024, 0, 3600, &cache);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试LRU缓存添加和查找 */
TEST(UvhttpLruCacheFullCoverageTest, CachePutAndFind) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 准备测试数据 */
    char content[] = "Hello, World!";
    size_t content_length = strlen(content);
    const char* mime_type = "text/plain";
    time_t last_modified = time(NULL);
    const char* etag = "\"123456\"";
    
    /* 添加缓存条目 */
    result = uvhttp_lru_cache_put(cache, "/test.txt", content, content_length,
                                   mime_type, last_modified, etag);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证条目数增加 */
    EXPECT_EQ(cache->entry_count, 1);
    EXPECT_GT(cache->total_memory_usage, 0);
    
    /* 查找缓存条目 */
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test.txt");
    ASSERT_NE(entry, nullptr);
    EXPECT_STREQ(entry->file_path, "/test.txt");
    EXPECT_EQ(entry->content_length, content_length);
    EXPECT_STREQ(entry->mime_type, mime_type);
    
    /* 验证命中次数增加 */
    EXPECT_EQ(cache->hit_count, 1);
    EXPECT_EQ(cache->miss_count, 0);
    
    /* 查找不存在的条目 */
    entry = uvhttp_lru_cache_find(cache, "/notfound.txt");
    EXPECT_EQ(entry, nullptr);
    EXPECT_EQ(cache->miss_count, 1);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存删除 */
TEST(UvhttpLruCacheFullCoverageTest, CacheRemove) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 添加缓存条目 */
    char content[] = "Hello, World!";
    result = uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content),
                                   "text/plain", time(NULL), "\"123456\"");
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 删除缓存条目 */
    result = uvhttp_lru_cache_remove(cache, "/test.txt");
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证条目被删除 */
    EXPECT_EQ(cache->entry_count, 0);
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test.txt");
    EXPECT_EQ(entry, nullptr);
    
    /* 删除不存在的条目 */
    result = uvhttp_lru_cache_remove(cache, "/notfound.txt");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存清空 */
TEST(UvhttpLruCacheFullCoverageTest, CacheClear) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 添加多个缓存条目 */
    char content[] = "Hello, World!";
    for (int i = 0; i < 10; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/test%d.txt", i);
        result = uvhttp_lru_cache_put(cache, path, content, strlen(content),
                                       "text/plain", time(NULL), "\"123456\"");
        EXPECT_EQ(result, UVHTTP_OK);
    }
    
    EXPECT_EQ(cache->entry_count, 10);
    
    /* 清空缓存 */
    uvhttp_lru_cache_clear(cache);
    
    /* 验证缓存被清空 */
    EXPECT_EQ(cache->entry_count, 0);
    EXPECT_EQ(cache->total_memory_usage, 0);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存统计信息 */
TEST(UvhttpLruCacheFullCoverageTest, CacheStats) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 添加缓存条目 */
    char content[] = "Hello, World!";
    result = uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content),
                                   "text/plain", time(NULL), "\"123456\"");
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 查找缓存条目（命中） */
    uvhttp_lru_cache_find(cache, "/test.txt");
    
    /* 查找不存在的条目（未命中） */
    uvhttp_lru_cache_find(cache, "/notfound.txt");
    
    /* 获取统计信息 */
    size_t total_memory_usage;
    int entry_count, hit_count, miss_count, eviction_count;
    uvhttp_lru_cache_get_stats(cache, &total_memory_usage, &entry_count,
                               &hit_count, &miss_count, &eviction_count);
    
    EXPECT_EQ(entry_count, 1);
    EXPECT_EQ(hit_count, 1);
    EXPECT_EQ(miss_count, 1);
    EXPECT_EQ(eviction_count, 0);
    EXPECT_GT(total_memory_usage, 0);
    
    /* 重置统计信息 */
    uvhttp_lru_cache_reset_stats(cache);
    
    /* 验证统计信息被重置 */
    uvhttp_lru_cache_get_stats(cache, &total_memory_usage, &entry_count,
                               &hit_count, &miss_count, &eviction_count);
    EXPECT_EQ(hit_count, 0);
    EXPECT_EQ(miss_count, 0);
    EXPECT_EQ(eviction_count, 0);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存过期清理 */
TEST(UvhttpLruCacheFullCoverageTest, CacheCleanupExpired) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 1, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 添加缓存条目 */
    char content[] = "Hello, World!";
    result = uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content),
                                   "text/plain", time(NULL), "\"123456\"");
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 等待过期 */
    sleep(2);
    
    /* 清理过期条目 */
    int cleaned = uvhttp_lru_cache_cleanup_expired(cache);
    EXPECT_EQ(cleaned, 1);
    EXPECT_EQ(cache->entry_count, 0);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存内存限制 */
TEST(UvhttpLruCacheFullCoverageTest, CacheMemoryLimit) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024, 10, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 添加大文件，触发驱逐 */
    char content[512];
    memset(content, 'A', sizeof(content));
    
    for (int i = 0; i < 5; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/test%d.txt", i);
        result = uvhttp_lru_cache_put(cache, path, content, sizeof(content),
                                       "text/plain", time(NULL), "\"123456\"");
        if (result != UVHTTP_OK) {
            /* 可能因为内存限制而失败 */
            break;
        }
    }
    
    /* 验证驱逐次数 */
    EXPECT_GT(cache->eviction_count, 0);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存条目数限制 */
TEST(UvhttpLruCacheFullCoverageTest, CacheEntryLimit) {
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 3, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    /* 添加超过限制的条目数 */
    char content[] = "Hello, World!";
    for (int i = 0; i < 5; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/test%d.txt", i);
        result = uvhttp_lru_cache_put(cache, path, content, strlen(content),
                                       "text/plain", time(NULL), "\"123456\"");
        EXPECT_EQ(result, UVHTTP_OK);
    }
    
    /* 验证条目数不超过限制 */
    EXPECT_LE(cache->entry_count, 3);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheNullParameters) {
    /* 测试NULL缓存管理器 */
    cache_entry_t* entry = uvhttp_lru_cache_find(NULL, "/test.txt");
    EXPECT_EQ(entry, nullptr);
    
    /* 测试NULL文件路径 */
    cache_manager_t* cache = NULL;
    uvhttp_error_t result = uvhttp_lru_cache_create(1024 * 1024, 100, 3600, &cache);
    ASSERT_EQ(result, UVHTTP_OK);
    
    entry = uvhttp_lru_cache_find(cache, NULL);
    EXPECT_EQ(entry, nullptr);
    
    /* 测试NULL缓存管理器 - put */
    char content[] = "Hello, World!";
    result = uvhttp_lru_cache_put(NULL, "/test.txt", content, strlen(content),
                                   "text/plain", time(NULL), "\"123456\"");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL文件路径 - put */
    result = uvhttp_lru_cache_put(cache, NULL, content, strlen(content),
                                   "text/plain", time(NULL), "\"123456\"");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL缓存管理器 - remove */
    result = uvhttp_lru_cache_remove(NULL, "/test.txt");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试NULL文件路径 - remove */
    result = uvhttp_lru_cache_remove(cache, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 清理 */
    uvhttp_lru_cache_free(cache);
}

/* 测试LRU缓存清空NULL */
TEST(UvhttpLruCacheFullCoverageTest, CacheClearNull) {
    /* 测试NULL缓存管理器 */
    uvhttp_lru_cache_clear(NULL);
    
    /* 测试NULL缓存管理器 - reset stats */
    uvhttp_lru_cache_reset_stats(NULL);
    
    /* 测试NULL缓存管理器 - cleanup expired */
    int cleaned = uvhttp_lru_cache_cleanup_expired(NULL);
    EXPECT_EQ(cleaned, 0);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
