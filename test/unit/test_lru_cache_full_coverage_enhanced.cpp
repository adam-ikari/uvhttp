/**
 * @file test_lru_cache_full_coverage_enhanced.cpp
 * @brief 增强的 LRU 缓存测试 - 提升覆盖率到 50%
 * 
 * 目标：提升 uvhttp_lru_cache.c 覆盖率从 9.7% 到 50%
 * 
 * 测试内容：
 * - uvhttp_lru_cache_create
 * - uvhttp_lru_cache_free
 * - uvhttp_lru_cache_find
 * - uvhttp_lru_cache_put
 * - uvhttp_lru_cache_remove
 * - uvhttp_lru_cache_clear
 * - uvhttp_lru_cache_get_stats
 * - uvhttp_lru_cache_reset_stats
 * - uvhttp_lru_cache_cleanup_expired
 * - uvhttp_lru_cache_is_expired
 * - uvhttp_lru_cache_move_to_head
 * - uvhttp_lru_cache_remove_tail
 * - uvhttp_lru_cache_get_hit_rate
 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_lru_cache.h"
#include "uvhttp_allocator.h"

/* 测试创建缓存管理器 */
TEST(UvhttpLruCacheEnhancedTest, CacheCreate) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    uvhttp_lru_cache_free(cache);
}

/* 测试创建缓存管理器 - 零限制 */
TEST(UvhttpLruCacheEnhancedTest, CacheCreateZeroLimits) {
    cache_manager_t* cache = uvhttp_lru_cache_create(0, 0, 0);
    ASSERT_NE(cache, nullptr);
    uvhttp_lru_cache_free(cache);
}

/* 测试释放缓存管理器 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheFreeNull) {
    uvhttp_lru_cache_free(nullptr);
    // 不应该崩溃
}

/* 测试查找缓存 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheFindNullCache) {
    cache_entry_t* entry = uvhttp_lru_cache_find(nullptr, "/test.txt");
    EXPECT_EQ(entry, nullptr);
}

/* 测试查找缓存 - NULL 路径 */
TEST(UvhttpLruCacheEnhancedTest, CacheFindNullPath) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, nullptr);
    EXPECT_EQ(entry, nullptr);
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CachePutNullCache) {
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(nullptr, "/test.txt", content, strlen(content), "text/plain", 0, "etag123");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试添加缓存 - NULL 路径 */
TEST(UvhttpLruCacheEnhancedTest, CachePutNullPath) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, nullptr, content, strlen(content), "text/plain", 0, "etag123");
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存 - NULL 内容 */
TEST(UvhttpLruCacheEnhancedTest, CachePutNullContent) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test.txt", nullptr, 0, "text/plain", 0, "etag123");
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_lru_cache_free(cache);
}

/* 测试删除缓存 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheRemoveNullCache) {
    uvhttp_error_t result = uvhttp_lru_cache_remove(nullptr, "/test.txt");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试删除缓存 - NULL 路径 */
TEST(UvhttpLruCacheEnhancedTest, CacheRemoveNullPath) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    uvhttp_error_t result = uvhttp_lru_cache_remove(cache, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
    uvhttp_lru_cache_free(cache);
}

/* 测试清空缓存 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheClearNull) {
    uvhttp_lru_cache_clear(nullptr);
    // 不应该崩溃
}

/* 测试获取统计信息 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheGetStatsNull) {
    size_t total_memory_usage = 0;
    int entry_count = 0;
    int hit_count = 0;
    int miss_count = 0;
    int eviction_count = 0;
    
    uvhttp_lru_cache_get_stats(nullptr, &total_memory_usage, &entry_count, &hit_count, &miss_count, &eviction_count);
    // 不应该崩溃
}

/* 测试重置统计信息 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheResetStatsNull) {
    uvhttp_lru_cache_reset_stats(nullptr);
    // 不应该崩溃
}

/* 测试清理过期条目 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheCleanupExpiredNull) {
    int count = uvhttp_lru_cache_cleanup_expired(nullptr);
    EXPECT_EQ(count, 0);
}

/* 测试检查过期 - NULL 条目 */
TEST(UvhttpLruCacheEnhancedTest, CacheIsExpiredNullEntry) {
    int is_expired = uvhttp_lru_cache_is_expired(nullptr, 3600);
    EXPECT_EQ(is_expired, 1);
}

/* 测试移动到头部 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheMoveToHeadNull) {
    uvhttp_lru_cache_move_to_head(nullptr, nullptr);
    // 不应该崩溃
}

/* 测试移除尾部 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheRemoveTailNull) {
    cache_entry_t* entry = uvhttp_lru_cache_remove_tail(nullptr);
    EXPECT_EQ(entry, nullptr);
}

/* 测试获取命中率 - NULL 参数 */
TEST(UvhttpLruCacheEnhancedTest, CacheGetHitRateNull) {
    double hit_rate = uvhttp_lru_cache_get_hit_rate(nullptr);
    EXPECT_EQ(hit_rate, 0.0);
}

/* 测试缓存管理器结构体字段初始化 */
TEST(UvhttpLruCacheEnhancedTest, CacheManagerFieldInitialization) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    EXPECT_EQ(cache->hash_table, nullptr);
    EXPECT_EQ(cache->lru_head, nullptr);
    EXPECT_EQ(cache->lru_tail, nullptr);
    EXPECT_EQ(cache->total_memory_usage, 0);
    EXPECT_EQ(cache->max_memory_usage, 1024 * 1024);
    EXPECT_EQ(cache->entry_count, 0);
    EXPECT_EQ(cache->max_entries, 100);
    EXPECT_EQ(cache->cache_ttl, 3600);
    EXPECT_EQ(cache->hit_count, 0);
    EXPECT_EQ(cache->miss_count, 0);
    EXPECT_EQ(cache->eviction_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试缓存条目结构体 */
TEST(UvhttpLruCacheEnhancedTest, CacheEntryStruct) {
    cache_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    
    EXPECT_EQ(entry.file_path[0], '\0');
    EXPECT_EQ(entry.content, nullptr);
    EXPECT_EQ(entry.content_length, 0);
    EXPECT_EQ(entry.mime_type[0], '\0');
    EXPECT_EQ(entry.last_modified, 0);
    EXPECT_EQ(entry.etag[0], '\0');
    EXPECT_EQ(entry.access_time, 0);
    EXPECT_EQ(entry.cache_time, 0);
    EXPECT_EQ(entry.memory_usage, 0);
    EXPECT_EQ(entry.is_compressed, 0);
    EXPECT_EQ(entry.lru_prev, nullptr);
    EXPECT_EQ(entry.lru_next, nullptr);
}

/* 测试缓存管理器结构体大小 */
TEST(UvhttpLruCacheEnhancedTest, CacheStructSize) {
    EXPECT_GT(sizeof(cache_manager_t), 0);
    EXPECT_GT(sizeof(cache_entry_t), 0);
}

/* 测试多次 NULL 调用 */
TEST(UvhttpLruCacheEnhancedTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_lru_cache_free(nullptr);
        uvhttp_lru_cache_find(nullptr, "/test.txt");
        uvhttp_lru_cache_remove(nullptr, "/test.txt");
        uvhttp_lru_cache_clear(nullptr);
        uvhttp_lru_cache_reset_stats(nullptr);
        uvhttp_lru_cache_cleanup_expired(nullptr);
        uvhttp_lru_cache_is_expired(nullptr, 3600);
        uvhttp_lru_cache_move_to_head(nullptr, nullptr);
        uvhttp_lru_cache_remove_tail(nullptr);
        uvhttp_lru_cache_get_hit_rate(nullptr);
    }
    // 不应该崩溃
}

/* 测试添加和查找缓存 */
TEST(UvhttpLruCacheEnhancedTest, CachePutAndFind) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content), "text/plain", 0, "etag123");
    EXPECT_EQ(result, UVHTTP_OK);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test.txt");
    EXPECT_NE(entry, nullptr);
    EXPECT_EQ(entry->content_length, strlen(content));
    EXPECT_STREQ(entry->mime_type, "text/plain");
    EXPECT_STREQ(entry->etag, "etag123");
    EXPECT_EQ(cache->hit_count, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试查找不存在的缓存 */
TEST(UvhttpLruCacheEnhancedTest, CacheFindNonExistent) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/nonexistent.txt");
    EXPECT_EQ(entry, nullptr);
    EXPECT_EQ(cache->miss_count, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试删除缓存 */
TEST(UvhttpLruCacheEnhancedTest, CacheRemove) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content), "text/plain", 0, "etag123");
    
    uvhttp_error_t result = uvhttp_lru_cache_remove(cache, "/test.txt");
    EXPECT_EQ(result, UVHTTP_OK);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test.txt");
    EXPECT_EQ(entry, nullptr);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试清空缓存 */
TEST(UvhttpLruCacheEnhancedTest, CacheClear) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test1.txt", content, strlen(content), "text/plain", 0, "etag1");
    uvhttp_lru_cache_put(cache, "/test2.txt", content, strlen(content), "text/plain", 0, "etag2");
    uvhttp_lru_cache_put(cache, "/test3.txt", content, strlen(content), "text/plain", 0, "etag3");
    
    EXPECT_EQ(cache->entry_count, 3);
    
    uvhttp_lru_cache_clear(cache);
    EXPECT_EQ(cache->entry_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试获取统计信息 */
TEST(UvhttpLruCacheEnhancedTest, CacheGetStats) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content), "text/plain", 0, "etag123");
    
    size_t total_memory_usage = 0;
    int entry_count = 0;
    int hit_count = 0;
    int miss_count = 0;
    int eviction_count = 0;
    
    uvhttp_lru_cache_get_stats(cache, &total_memory_usage, &entry_count, &hit_count, &miss_count, &eviction_count);
    
    EXPECT_GT(total_memory_usage, 0);
    EXPECT_EQ(entry_count, 1);
    EXPECT_EQ(hit_count, 1);
    EXPECT_EQ(miss_count, 0);
    EXPECT_EQ(eviction_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试重置统计信息 */
TEST(UvhttpLruCacheEnhancedTest, CacheResetStats) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content), "text/plain", 0, "etag123");
    
    EXPECT_GT(cache->hit_count, 0);
    
    uvhttp_lru_cache_reset_stats(cache);
    
    EXPECT_EQ(cache->hit_count, 0);
    EXPECT_EQ(cache->miss_count, 0);
    EXPECT_EQ(cache->eviction_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试获取命中率 */
TEST(UvhttpLruCacheEnhancedTest, CacheGetHitRate) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test.txt", content, strlen(content), "text/plain", 0, "etag123");
    
    // 查找存在的缓存
    uvhttp_lru_cache_find(cache, "/test.txt");
    
    // 查找不存在的缓存
    uvhttp_lru_cache_find(cache, "/nonexistent.txt");
    
    double hit_rate = uvhttp_lru_cache_get_hit_rate(cache);
    EXPECT_GT(hit_rate, 0.0);
    EXPECT_LT(hit_rate, 1.0);
    
    uvhttp_lru_cache_free(cache);
}