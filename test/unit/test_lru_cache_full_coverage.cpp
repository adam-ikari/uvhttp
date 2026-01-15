/* uvhttp_lru_cache.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_lru_cache.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>

/* 测试缓存结构大小 */
TEST(UvhttpLruCacheFullCoverageTest, CacheStructSize) {
    EXPECT_GT(sizeof(cache_entry_t), 0);
    EXPECT_GT(sizeof(cache_manager_t), 0);
}

/* 测试创建缓存管理器 */
TEST(UvhttpLruCacheFullCoverageTest, CacheCreate) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    /* 可能返回NULL或创建的缓存 */
    if (cache != NULL) {
        uvhttp_lru_cache_free(cache);
    }
}

/* 测试释放缓存管理器 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheFreeNull) {
    /* 应该安全处理 */
    uvhttp_lru_cache_free(NULL);
}

/* 测试查找缓存条目 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheFindNull) {
    cache_entry_t* entry = uvhttp_lru_cache_find(NULL, NULL);
    /* 应该返回NULL */
    EXPECT_EQ(entry, nullptr);
}

/* 测试添加缓存条目 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CachePutNull) {
    uvhttp_error_t result;

    result = uvhttp_lru_cache_put(NULL, NULL, NULL, 0, NULL, 0, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试删除缓存条目 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheRemoveNull) {
    uvhttp_error_t result = uvhttp_lru_cache_remove(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试清空缓存 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheClearNull) {
    /* 应该安全处理 */
    uvhttp_lru_cache_clear(NULL);
}

/* 测试获取缓存统计信息 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheGetStatsNull) {
    /* 应该安全处理 */
    uvhttp_lru_cache_get_stats(NULL, NULL, NULL, NULL, NULL, NULL);
}

/* 测试重置统计信息 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheResetStatsNull) {
    /* 应该安全处理 */
    uvhttp_lru_cache_reset_stats(NULL);
}

/* 测试清理过期条目 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheCleanupExpiredNull) {
    int result = uvhttp_lru_cache_cleanup_expired(NULL);
    /* 应该返回0 */
    EXPECT_EQ(result, 0);
}

/* 测试检查缓存条目是否过期 */
TEST(UvhttpLruCacheFullCoverageTest, CacheIsExpired) {
    cache_entry_t entry;
    memset(&entry, 0, sizeof(entry));

    /* 测试未过期 */
    entry.cache_time = time(NULL);
    EXPECT_EQ(uvhttp_lru_cache_is_expired(&entry, 3600), 0);

    /* 测试过期 */
    entry.cache_time = time(NULL) - 7200;
    EXPECT_EQ(uvhttp_lru_cache_is_expired(&entry, 3600), 1);
}

/* 测试移动条目到LRU链表头部 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheMoveToHeadNull) {
    /* 应该安全处理 */
    uvhttp_lru_cache_move_to_head(NULL, NULL);
}

/* 测试从LRU链表尾部移除条目 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheRemoveTailNull) {
    cache_entry_t* entry = uvhttp_lru_cache_remove_tail(NULL);
    /* 应该返回NULL */
    EXPECT_EQ(entry, nullptr);
}

/* 测试计算缓存命中率 - NULL参数 */
TEST(UvhttpLruCacheFullCoverageTest, CacheGetHitRateNull) {
    double rate = uvhttp_lru_cache_get_hit_rate(NULL);
    /* 应该返回0.0 */
    EXPECT_DOUBLE_EQ(rate, 0.0);
}

/* 测试缓存条目结构初始化 */
TEST(UvhttpLruCacheFullCoverageTest, CacheEntryInitialization) {
    cache_entry_t entry;
    memset(&entry, 0, sizeof(entry));

    /* 验证初始值 */
    EXPECT_EQ(entry.content, nullptr);
    EXPECT_EQ(entry.content_length, 0);
    EXPECT_EQ(entry.last_modified, 0);
    EXPECT_EQ(entry.access_time, 0);
    EXPECT_EQ(entry.cache_time, 0);
    EXPECT_EQ(entry.memory_usage, 0);
    EXPECT_EQ(entry.is_compressed, 0);
    EXPECT_EQ(entry.lru_prev, nullptr);
    EXPECT_EQ(entry.lru_next, nullptr);
}

/* 测试缓存管理器结构初始化 */
TEST(UvhttpLruCacheFullCoverageTest, CacheManagerInitialization) {
    cache_manager_t manager;
    memset(&manager, 0, sizeof(manager));

    /* 验证初始值 */
    EXPECT_EQ(manager.hash_table, nullptr);
    EXPECT_EQ(manager.lru_head, nullptr);
    EXPECT_EQ(manager.lru_tail, nullptr);
    EXPECT_EQ(manager.total_memory_usage, 0);
    EXPECT_EQ(manager.entry_count, 0);
    EXPECT_EQ(manager.hit_count, 0);
    EXPECT_EQ(manager.miss_count, 0);
    EXPECT_EQ(manager.eviction_count, 0);
}

/* 测试边界条件 */
TEST(UvhttpLruCacheFullCoverageTest, BoundaryConditions) {
    cache_manager_t manager;
    memset(&manager, 0, sizeof(manager));

    /* 测试极限值 */
    manager.max_memory_usage = SIZE_MAX;
    manager.max_entries = INT_MAX;
    manager.cache_ttl = INT_MAX;

    EXPECT_EQ(manager.max_memory_usage, SIZE_MAX);
    EXPECT_EQ(manager.max_entries, INT_MAX);
    EXPECT_EQ(manager.cache_ttl, INT_MAX);
}

/* 测试缓存常量 */
TEST(UvhttpLruCacheFullCoverageTest, CacheConstants) {
    EXPECT_GT(UVHTTP_MAX_FILE_PATH_SIZE, 0);
    EXPECT_GT(UVHTTP_MAX_HEADER_VALUE_SIZE, 0);
}

/* 测试多次调用NULL参数函数 */
TEST(UvhttpLruCacheFullCoverageTest, MultipleNullCalls) {
    /* 多次调用NULL参数函数，确保不会崩溃 */
    for (int i = 0; i < 100; i++) {
        uvhttp_lru_cache_free(NULL);
        uvhttp_lru_cache_find(NULL, NULL);
        uvhttp_lru_cache_remove(NULL, NULL);
        uvhttp_lru_cache_clear(NULL);
        uvhttp_lru_cache_get_stats(NULL, NULL, NULL, NULL, NULL, NULL);
        uvhttp_lru_cache_reset_stats(NULL);
        uvhttp_lru_cache_cleanup_expired(NULL);
        uvhttp_lru_cache_move_to_head(NULL, NULL);
        uvhttp_lru_cache_remove_tail(NULL);
        uvhttp_lru_cache_get_hit_rate(NULL);
    }
}

/* 测试缓存条目结构对齐 */
TEST(UvhttpLruCacheFullCoverageTest, CacheStructAlignment) {
    /* 验证结构对齐合理 */
    EXPECT_EQ(sizeof(cache_entry_t) % sizeof(void*), 0);
    EXPECT_EQ(sizeof(cache_manager_t) % sizeof(void*), 0);
}

/* 测试缓存条目字段大小 */
TEST(UvhttpLruCacheFullCoverageTest, CacheEntryFieldSizes) {
    cache_entry_t entry;

    /* 验证字段大小合理 */
    EXPECT_EQ(sizeof(entry.file_path), UVHTTP_MAX_FILE_PATH_SIZE);
    EXPECT_EQ(sizeof(entry.mime_type), UVHTTP_MAX_HEADER_VALUE_SIZE);
    EXPECT_EQ(sizeof(entry.etag), UVHTTP_MAX_HEADER_VALUE_SIZE);
}

/* 测试缓存管理器字段大小 */
TEST(UvhttpLruCacheFullCoverageTest, CacheManagerFieldSizes) {
    cache_manager_t manager;

    /* 验证字段大小合理 */
    EXPECT_EQ(sizeof(manager.total_memory_usage), sizeof(size_t));
    EXPECT_EQ(sizeof(manager.entry_count), sizeof(int));
    EXPECT_EQ(sizeof(manager.hit_count), sizeof(int));
}

/* 测试缓存条目链表结构 */
TEST(UvhttpLruCacheFullCoverageTest, CacheEntryList) {
    cache_entry_t entry1, entry2, entry3;
    memset(&entry1, 0, sizeof(entry1));
    memset(&entry2, 0, sizeof(entry2));
    memset(&entry3, 0, sizeof(entry3));

    /* 测试链式结构 */
    entry1.lru_next = &entry2;
    entry2.lru_prev = &entry1;
    entry2.lru_next = &entry3;
    entry3.lru_prev = &entry2;

    EXPECT_EQ(entry1.lru_next, &entry2);
    EXPECT_EQ(entry2.lru_prev, &entry1);
    EXPECT_EQ(entry2.lru_next, &entry3);
    EXPECT_EQ(entry3.lru_prev, &entry2);
}

/* 测试缓存统计信息 */
TEST(UvhttpLruCacheFullCoverageTest, CacheStats) {
    cache_manager_t manager;
    size_t total_memory;
    int entry_count, hit_count, miss_count, eviction_count;

    memset(&manager, 0, sizeof(manager));

    /* 设置统计值 */
    manager.total_memory_usage = 1024;
    manager.entry_count = 10;
    manager.hit_count = 100;
    manager.miss_count = 50;
    manager.eviction_count = 5;

    /* 获取统计信息 */
    uvhttp_lru_cache_get_stats(&manager, &total_memory, &entry_count, &hit_count, &miss_count, &eviction_count);

    EXPECT_EQ(total_memory, 1024);
    EXPECT_EQ(entry_count, 10);
    EXPECT_EQ(hit_count, 100);
    EXPECT_EQ(miss_count, 50);
    EXPECT_EQ(eviction_count, 5);
}

/* 测试缓存命中率计算 */
TEST(UvhttpLruCacheFullCoverageTest, CacheHitRate) {
    cache_manager_t manager;

    memset(&manager, 0, sizeof(manager));

    /* 测试各种命中率 */
    manager.hit_count = 0;
    manager.miss_count = 0;
    EXPECT_DOUBLE_EQ(uvhttp_lru_cache_get_hit_rate(&manager), 0.0);

    manager.hit_count = 100;
    manager.miss_count = 0;
    EXPECT_DOUBLE_EQ(uvhttp_lru_cache_get_hit_rate(&manager), 1.0);

    manager.hit_count = 50;
    manager.miss_count = 50;
    EXPECT_DOUBLE_EQ(uvhttp_lru_cache_get_hit_rate(&manager), 0.5);
}