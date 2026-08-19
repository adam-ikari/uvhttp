/**
 * @file test_gzip_cache.cpp
 * @brief uvhttp_gzip_cache module unit tests
 *
 * Validates the gzip compression cache implemented in src/uvhttp_gzip_cache.c:
 * - create/free lifecycle and NULL-safety
 * - find miss on empty cache, hit after put (value + length match)
 * - stats tracking (entries, memory, hits, misses)
 * - clear resets entries and memory
 * - LRU eviction when max_entries is exceeded
 * - set_max_entries shrink evicts
 * - memory budget: single entry larger than budget is not cached
 * - TTL lazy eviction
 * - same-key put replaces the cached value
 *
 * Build configuration: UVHTTP_FEATURE_COMPRESSION must be enabled.
 * Run with:
 *   ./build/dist/bin/uvhttp_test_gzip_cache
 */

#include <gtest/gtest.h>
#include <cstring>
#include <cstdint>

#include "uvhttp_gzip_cache.h"

#if UVHTTP_FEATURE_COMPRESSION

/* Deterministic FNV-1a hash for test keys (distinct from the real xxhash64,
 * only used to exercise cache key matching). */
static uint64_t test_hash(const char* s) {
    uint64_t h = 14695981039346656037ULL;
    while (*s) {
        h ^= (unsigned char)*s++;
        h *= 1099511628211ULL;
    }
    return h;
}

class GzipCacheTest : public ::testing::Test {
protected:
    void SetUp() override { cache = nullptr; }
    void TearDown() override { uvhttp_gzip_cache_free(cache); }

    uvhttp_gzip_cache_t* cache;
};

/* --- create / NULL safety --- */
TEST_F(GzipCacheTest, CreateNullOutput) {
    EXPECT_EQ(uvhttp_gzip_cache_create(0, 0, 0, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(GzipCacheTest, NullFunctionsAreNoops) {
    uvhttp_gzip_cache_free(nullptr);
    uvhttp_gzip_cache_clear(nullptr);
    uvhttp_gzip_cache_set_max_entries(nullptr, 8);
    uvhttp_gzip_cache_set_max_memory_usage(nullptr, 8);
    uvhttp_gzip_cache_set_cache_ttl(nullptr, 8);
    uvhttp_gzip_cache_get_stats(nullptr, nullptr, nullptr, nullptr, nullptr);
    EXPECT_EQ(uvhttp_gzip_cache_find(nullptr, 1, 2, nullptr), nullptr);
    EXPECT_EQ(uvhttp_gzip_cache_put(nullptr, 1, 2, "x", 1),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(GzipCacheTest, CreateDefaults) {
    ASSERT_EQ(uvhttp_gzip_cache_create(0, 0, 0, &cache), UVHTTP_OK);
    ASSERT_NE(cache, nullptr);
    size_t mem = 0;
    int entries = -1;
    uvhttp_gzip_cache_get_stats(cache, &mem, &entries, nullptr, nullptr);
    EXPECT_EQ(entries, 0);
    EXPECT_EQ(mem, 0u);
}

/* --- find / put semantics --- */
TEST_F(GzipCacheTest, MissOnEmptyCache) {
    ASSERT_EQ(uvhttp_gzip_cache_create(0, 0, 0, &cache), UVHTTP_OK);
    size_t out = 0;
    EXPECT_EQ(uvhttp_gzip_cache_find(cache, test_hash("a"), 1, &out), nullptr);
}

TEST_F(GzipCacheTest, PutThenFindHit) {
    ASSERT_EQ(uvhttp_gzip_cache_create(0, 0, 0, &cache), UVHTTP_OK);
    const char* body = "Hello, World!";
    const char* z = "compressed-gzip-stream-bytes";
    ASSERT_EQ(uvhttp_gzip_cache_put(cache, test_hash(body), strlen(body), z,
                                    strlen(z)),
              UVHTTP_OK);
    size_t out = 0;
    const char* got =
        uvhttp_gzip_cache_find(cache, test_hash(body), strlen(body), &out);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(out, strlen(z));
    EXPECT_EQ(std::memcmp(got, z, out), 0);
}

TEST_F(GzipCacheTest, MissOnDifferentLenAndHash) {
    ASSERT_EQ(uvhttp_gzip_cache_create(0, 0, 0, &cache), UVHTTP_OK);
    uvhttp_gzip_cache_put(cache, test_hash("same"), 4, "z", 1);
    size_t out = 0;
    EXPECT_EQ(uvhttp_gzip_cache_find(cache, test_hash("same"), 99, &out),
              nullptr);
    EXPECT_EQ(uvhttp_gzip_cache_find(cache, test_hash("other"), 4, &out),
              nullptr);
}

TEST_F(GzipCacheTest, StatsTracking) {
    ASSERT_EQ(uvhttp_gzip_cache_create(0, 0, 0, &cache), UVHTTP_OK);
    const char* body = "body";
    const char* z = "zzz";
    uvhttp_gzip_cache_put(cache, test_hash(body), strlen(body), z, strlen(z));
    size_t out = 0;
    uvhttp_gzip_cache_find(cache, test_hash(body), strlen(body), &out); /* hit */
    uvhttp_gzip_cache_find(cache, test_hash("nope"), 4, &out);          /* miss */
    uvhttp_gzip_cache_find(cache, test_hash("x"), 1, &out);             /* miss */
    size_t mem = 0;
    int entries = -1, hits = -1, misses = -1;
    uvhttp_gzip_cache_get_stats(cache, &mem, &entries, &hits, &misses);
    EXPECT_EQ(entries, 1);
    EXPECT_EQ(mem, strlen(z));
    EXPECT_EQ(hits, 1);
    EXPECT_EQ(misses, 2);
}

TEST_F(GzipCacheTest, ClearResetsEntriesAndMemory) {
    ASSERT_EQ(uvhttp_gzip_cache_create(0, 0, 0, &cache), UVHTTP_OK);
    uvhttp_gzip_cache_put(cache, test_hash("a"), 1, "zzz", 3);
    uvhttp_gzip_cache_clear(cache);
    size_t mem = 999;
    int entries = -1;
    uvhttp_gzip_cache_get_stats(cache, &mem, &entries, nullptr, nullptr);
    EXPECT_EQ(entries, 0);
    EXPECT_EQ(mem, 0u);
}

/* --- LRU eviction --- */
TEST_F(GzipCacheTest, LruEviction) {
    ASSERT_EQ(uvhttp_gzip_cache_create(1024 * 1024, 2, 0, &cache), UVHTTP_OK);
    uvhttp_gzip_cache_put(cache, test_hash("a"), 1, "z", 1);
    uvhttp_gzip_cache_put(cache, test_hash("b"), 1, "z", 1);
    uvhttp_gzip_cache_put(cache, test_hash("c"), 1, "z", 1); /* evicts "a" */
    size_t out = 0;
    EXPECT_EQ(uvhttp_gzip_cache_find(cache, test_hash("a"), 1, &out), nullptr);
    EXPECT_NE(uvhttp_gzip_cache_find(cache, test_hash("b"), 1, &out), nullptr);
    EXPECT_NE(uvhttp_gzip_cache_find(cache, test_hash("c"), 1, &out), nullptr);
}

TEST_F(GzipCacheTest, SetMaxEntriesShrinkEvicts) {
    ASSERT_EQ(uvhttp_gzip_cache_create(1024 * 1024, 2, 0, &cache), UVHTTP_OK);
    uvhttp_gzip_cache_put(cache, test_hash("a"), 1, "z", 1);
    uvhttp_gzip_cache_put(cache, test_hash("c"), 1, "z", 1); /* MRU */
    uvhttp_gzip_cache_set_max_entries(cache, 1);
    size_t out = 0;
    int entries = -1;
    uvhttp_gzip_cache_get_stats(cache, nullptr, &entries, nullptr, nullptr);
    EXPECT_LE(entries, 1);
    EXPECT_NE(uvhttp_gzip_cache_find(cache, test_hash("c"), 1, &out), nullptr);
}

/* --- memory budget --- */
TEST_F(GzipCacheTest, EntryLargerThanBudgetNotCached) {
    ASSERT_EQ(uvhttp_gzip_cache_create(10, 4, 0, &cache), UVHTTP_OK);
    EXPECT_EQ(uvhttp_gzip_cache_put(cache, test_hash("big"), 3,
                                    "0123456789ABCDEF", 16),
              UVHTTP_OK);
    size_t mem = 999;
    int entries = -1;
    uvhttp_gzip_cache_get_stats(cache, &mem, &entries, nullptr, nullptr);
    EXPECT_EQ(entries, 0);
    EXPECT_EQ(mem, 0u);
    size_t out = 0;
    EXPECT_EQ(uvhttp_gzip_cache_find(cache, test_hash("big"), 3, &out),
              nullptr);
}

/* --- TTL --- */
TEST_F(GzipCacheTest, TtlEviction) {
    ASSERT_EQ(uvhttp_gzip_cache_create(1024 * 1024, 4, 1, &cache), UVHTTP_OK);
    uvhttp_gzip_cache_put(cache, test_hash("t"), 1, "z", 1);
    /* negative TTL disables expiry -> entry survives */
    uvhttp_gzip_cache_set_cache_ttl(cache, -1);
    size_t out = 0;
    EXPECT_NE(uvhttp_gzip_cache_find(cache, test_hash("t"), 1, &out), nullptr);
}

/* --- same-key replace --- */
TEST_F(GzipCacheTest, PutSameKeyReplacesValue) {
    ASSERT_EQ(uvhttp_gzip_cache_create(1024 * 1024, 4, 0, &cache), UVHTTP_OK);
    uvhttp_gzip_cache_put(cache, test_hash("k"), 1, "old", 3);
    uvhttp_gzip_cache_put(cache, test_hash("k"), 1, "newval", 6);
    size_t out = 0;
    const char* got = uvhttp_gzip_cache_find(cache, test_hash("k"), 1, &out);
    ASSERT_NE(got, nullptr);
    EXPECT_EQ(out, 6u);
    EXPECT_EQ(std::memcmp(got, "newval", 6), 0);
}

#endif /* UVHTTP_FEATURE_COMPRESSION */