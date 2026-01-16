#include <gtest/gtest.h>
#include <uvhttp_lru_cache.h>
#include <uvhttp_allocator.h>
#include <string.h>
#include <time.h>

/* 测试创建 LRU 缓存 NULL 参数 */
TEST(UvhttpLruCacheTest, CreateNullParams) {
    cache_manager_t* cache = uvhttp_lru_cache_create(0, 0, 0);
    ASSERT_NE(cache, nullptr);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试创建 LRU 缓存 */
TEST(UvhttpLruCacheTest, Create) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    EXPECT_EQ(cache->max_memory_usage, 1024 * 1024);
    EXPECT_EQ(cache->max_entries, 100);
    EXPECT_EQ(cache->cache_ttl, 3600);
    EXPECT_EQ(cache->entry_count, 0);
    EXPECT_EQ(cache->total_memory_usage, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试释放 LRU 缓存 NULL */
TEST(UvhttpLruCacheTest, FreeNull) {
    uvhttp_lru_cache_free(NULL);
    /* 不应该崩溃 */
}

/* 测试释放 LRU 缓存 */
TEST(UvhttpLruCacheTest, Free) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    uvhttp_lru_cache_free(cache);
    /* 不应该崩溃 */
}

/* 测试查找缓存 NULL 缓存 */
TEST(UvhttpLruCacheTest, FindNullCache) {
    cache_entry_t* entry = uvhttp_lru_cache_find(NULL, "/test/file.txt");
    EXPECT_EQ(entry, nullptr);
}

/* 测试查找缓存 NULL 路径 */
TEST(UvhttpLruCacheTest, FindNullPath) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, NULL);
    EXPECT_EQ(entry, nullptr);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试查找缓存未找到 */
TEST(UvhttpLruCacheTest, FindNotFound) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    EXPECT_EQ(entry, nullptr);
    EXPECT_EQ(cache->miss_count, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存 NULL 缓存 */
TEST(UvhttpLruCacheTest, PutNullCache) {
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(NULL, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    EXPECT_EQ(result, -1);
}

/* 测试添加缓存 NULL 路径 */
TEST(UvhttpLruCacheTest, PutNullPath) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, NULL, content, strlen(content), "text/plain", time(NULL), "etag");
    EXPECT_EQ(result, -1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存 NULL 内容 */
TEST(UvhttpLruCacheTest, PutNullContent) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file.txt", NULL, 0, "text/plain", time(NULL), "etag");
    EXPECT_EQ(result, -1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存 */
TEST(UvhttpLruCacheTest, Put) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(cache->entry_count, 1);
    EXPECT_GT(cache->total_memory_usage, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存更新已存在的条目 */
TEST(UvhttpLruCacheTest, PutUpdateExisting) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content1[] = "test content 1";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file.txt", content1, strlen(content1), "text/plain", time(NULL), "etag1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    char content2[] = "test content 2";
    result = uvhttp_lru_cache_put(cache, "/test/file.txt", content2, strlen(content2), "text/html", time(NULL), "etag2");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(cache->entry_count, 1);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    ASSERT_NE(entry, nullptr);
    EXPECT_STREQ(entry->mime_type, "text/html");
    
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存超过最大条目数 */
TEST(UvhttpLruCacheTest, PutExceedMaxEntries) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 2, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content1[] = "test content 1";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file1.txt", content1, strlen(content1), "text/plain", time(NULL), "etag1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    char content2[] = "test content 2";
    result = uvhttp_lru_cache_put(cache, "/test/file2.txt", content2, strlen(content2), "text/plain", time(NULL), "etag2");
    EXPECT_EQ(result, UVHTTP_OK);
    
    char content3[] = "test content 3";
    result = uvhttp_lru_cache_put(cache, "/test/file3.txt", content3, strlen(content3), "text/plain", time(NULL), "etag3");
    EXPECT_EQ(result, UVHTTP_OK);
    
    EXPECT_EQ(cache->entry_count, 2);
    EXPECT_EQ(cache->eviction_count, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试添加缓存超过最大内存 */
TEST(UvhttpLruCacheTest, PutExceedMaxMemory) {
    cache_manager_t* cache = uvhttp_lru_cache_create(100, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content1[] = "test content 1";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file1.txt", content1, strlen(content1), "text/plain", time(NULL), "etag1");
    EXPECT_EQ(result, UVHTTP_OK);
    
    char content2[] = "test content 2";
    result = uvhttp_lru_cache_put(cache, "/test/file2.txt", content2, strlen(content2), "text/plain", time(NULL), "etag2");
    EXPECT_EQ(result, UVHTTP_OK);
    
    EXPECT_EQ(cache->entry_count, 1);
    EXPECT_EQ(cache->eviction_count, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试查找缓存找到 */
TEST(UvhttpLruCacheTest, FindFound) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    EXPECT_EQ(result, UVHTTP_OK);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    ASSERT_NE(entry, nullptr);
    EXPECT_STREQ(entry->file_path, "/test/file.txt");
    EXPECT_STREQ(entry->content, content);
    EXPECT_EQ(entry->content_length, strlen(content));
    EXPECT_STREQ(entry->mime_type, "text/plain");
    EXPECT_EQ(cache->hit_count, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试查找缓存过期 */
TEST(UvhttpLruCacheTest, FindExpired) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 1);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    time_t now = time(NULL);
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", now, "etag");
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 等待缓存过期 (使用更短的时间) */
    usleep(1100000); /* 1.1秒，确保超过1秒TTL */
    
    /* 现在应该找不到缓存了 */
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    EXPECT_EQ(entry, nullptr);
    EXPECT_EQ(cache->miss_count, 1);
    EXPECT_EQ(cache->entry_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试删除缓存 NULL 缓存 */
TEST(UvhttpLruCacheTest, RemoveNullCache) {
    uvhttp_error_t result = uvhttp_lru_cache_remove(NULL, "/test/file.txt");
    EXPECT_EQ(result, -1);
}

/* 测试删除缓存 NULL 路径 */
TEST(UvhttpLruCacheTest, RemoveNullPath) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    uvhttp_error_t result = uvhttp_lru_cache_remove(cache, NULL);
    EXPECT_EQ(result, -1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试删除缓存未找到 */
TEST(UvhttpLruCacheTest, RemoveNotFound) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    uvhttp_error_t result = uvhttp_lru_cache_remove(cache, "/test/file.txt");
    EXPECT_EQ(result, UVHTTP_ERROR_NOT_FOUND);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试删除缓存 */
TEST(UvhttpLruCacheTest, Remove) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_error_t result = uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    EXPECT_EQ(result, UVHTTP_OK);
    
    result = uvhttp_lru_cache_remove(cache, "/test/file.txt");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(cache->entry_count, 0);
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    EXPECT_EQ(entry, nullptr);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试清空缓存 NULL */
TEST(UvhttpLruCacheTest, ClearNull) {
    uvhttp_lru_cache_clear(NULL);
    /* 不应该崩溃 */
}

/* 测试清空缓存 */
TEST(UvhttpLruCacheTest, Clear) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content1[] = "test content 1";
    uvhttp_lru_cache_put(cache, "/test/file1.txt", content1, strlen(content1), "text/plain", time(NULL), "etag1");
    
    char content2[] = "test content 2";
    uvhttp_lru_cache_put(cache, "/test/file2.txt", content2, strlen(content2), "text/plain", time(NULL), "etag2");
    
    EXPECT_EQ(cache->entry_count, 2);
    
    uvhttp_lru_cache_clear(cache);
    
    EXPECT_EQ(cache->entry_count, 0);
    EXPECT_EQ(cache->total_memory_usage, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试获取统计信息 NULL 缓存 */
TEST(UvhttpLruCacheTest, GetStatsNullCache) {
    size_t total_memory_usage;
    int entry_count;
    int hit_count;
    int miss_count;
    int eviction_count;
    
    uvhttp_lru_cache_get_stats(NULL, &total_memory_usage, &entry_count, &hit_count, &miss_count, &eviction_count);
    /* 不应该崩溃 */
}

/* 测试获取统计信息 */
TEST(UvhttpLruCacheTest, GetStats) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    
    uvhttp_lru_cache_find(cache, "/test/file.txt");
    uvhttp_lru_cache_find(cache, "/test/notfound.txt");
    
    size_t total_memory_usage;
    int entry_count;
    int hit_count;
    int miss_count;
    int eviction_count;
    
    uvhttp_lru_cache_get_stats(cache, &total_memory_usage, &entry_count, &hit_count, &miss_count, &eviction_count);
    
    EXPECT_EQ(entry_count, 1);
    EXPECT_EQ(hit_count, 1);
    EXPECT_EQ(miss_count, 1);
    EXPECT_GT(total_memory_usage, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试重置统计信息 NULL */
TEST(UvhttpLruCacheTest, ResetStatsNull) {
    uvhttp_lru_cache_reset_stats(NULL);
    /* 不应该崩溃 */
}

/* 测试重置统计信息 */
TEST(UvhttpLruCacheTest, ResetStats) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    
    uvhttp_lru_cache_find(cache, "/test/file.txt");
    uvhttp_lru_cache_find(cache, "/test/notfound.txt");
    
    EXPECT_GT(cache->hit_count, 0);
    EXPECT_GT(cache->miss_count, 0);
    
    uvhttp_lru_cache_reset_stats(cache);
    
    EXPECT_EQ(cache->hit_count, 0);
    EXPECT_EQ(cache->miss_count, 0);
    EXPECT_EQ(cache->eviction_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试清理过期条目 NULL */
TEST(UvhttpLruCacheTest, CleanupExpiredNull) {
    int count = uvhttp_lru_cache_cleanup_expired(NULL);
    EXPECT_EQ(count, 0);
}

/* 测试清理过期条目 */
TEST(UvhttpLruCacheTest, CleanupExpired) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 1);
    ASSERT_NE(cache, nullptr);
    
    char content1[] = "test content 1";
    time_t now = time(NULL);
    uvhttp_lru_cache_put(cache, "/test/file1.txt", content1, strlen(content1), "text/plain", now, "etag1");
    
    char content2[] = "test content 2";
    uvhttp_lru_cache_put(cache, "/test/file2.txt", content2, strlen(content2), "text/plain", now, "etag2");
    
    /* 手动修改缓存时间戳以模拟过期 */
    cache_entry_t* entry1 = uvhttp_lru_cache_find(cache, "/test/file1.txt");
    cache_entry_t* entry2 = uvhttp_lru_cache_find(cache, "/test/file2.txt");
    if (entry1) entry1->cache_time = now - 2;
    if (entry2) entry2->cache_time = now - 2;
    
    /* 清理过期条目 */
    int count = uvhttp_lru_cache_cleanup_expired(cache);
    EXPECT_EQ(count, 2);
    EXPECT_EQ(cache->entry_count, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试检查过期 NULL 条目 */
TEST(UvhttpLruCacheTest, IsExpiredNullEntry) {
    int result = uvhttp_lru_cache_is_expired(NULL, 3600);
    /* NULL 条目返回 0（未过期）而不是 1 */
    EXPECT_EQ(result, 0);
}

/* 测试检查过期永不过期 */
TEST(UvhttpLruCacheTest, IsExpiredNever) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 0);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    ASSERT_NE(entry, nullptr);
    
    int result = uvhttp_lru_cache_is_expired(entry, 0);
    EXPECT_EQ(result, 0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试检查过期已过期 */
TEST(UvhttpLruCacheTest, IsExpired) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 1);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    time_t now = time(NULL);
    uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", now, "etag");
    
    cache_entry_t* entry = uvhttp_lru_cache_find(cache, "/test/file.txt");
    ASSERT_NE(entry, nullptr);
    
    /* 手动修改缓存时间戳以模拟过期 */
    entry->cache_time = now - 2;
    
    int result = uvhttp_lru_cache_is_expired(entry, 1);
    EXPECT_EQ(result, 1);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试移动到头部 NULL */
TEST(UvhttpLruCacheTest, MoveToHeadNull) {
    uvhttp_lru_cache_move_to_head(NULL, NULL);
    /* 不应该崩溃 */
}

/* 测试移除尾部 NULL */
TEST(UvhttpLruCacheTest, RemoveTailNull) {
    cache_entry_t* entry = uvhttp_lru_cache_remove_tail(NULL);
    EXPECT_EQ(entry, nullptr);
}

/* 测试获取命中率 NULL */
TEST(UvhttpLruCacheTest, GetHitRateNull) {
    double rate = uvhttp_lru_cache_get_hit_rate(NULL);
    EXPECT_EQ(rate, 0.0);
}

/* 测试获取命中率 */
TEST(UvhttpLruCacheTest, GetHitRate) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    char content[] = "test content";
    uvhttp_lru_cache_put(cache, "/test/file.txt", content, strlen(content), "text/plain", time(NULL), "etag");
    
    uvhttp_lru_cache_find(cache, "/test/file.txt");
    uvhttp_lru_cache_find(cache, "/test/file.txt");
    uvhttp_lru_cache_find(cache, "/test/notfound.txt");
    
    double rate = uvhttp_lru_cache_get_hit_rate(cache);
    EXPECT_EQ(rate, 2.0 / 3.0);
    
    uvhttp_lru_cache_free(cache);
}

/* 测试获取命中率无访问 */
TEST(UvhttpLruCacheTest, GetHitRateNoAccess) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    ASSERT_NE(cache, nullptr);
    
    double rate = uvhttp_lru_cache_get_hit_rate(cache);
    EXPECT_EQ(rate, 0.0);
    
    uvhttp_lru_cache_free(cache);
}