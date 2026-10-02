/*
 * UVHTTP gzip 缓存替换路径测试
 *
 * 为什么需要独立测试：uvhttp_gzip_cache_put 的「替换同键条目」分支
 * （src/uvhttp_gzip_cache.c:189-215）在原实现下从未被执行——
 *
 *   delta = new_len - old_len
 *   while (delta > 0 && total_memory + delta > max_memory_usage && count > 1)
 *       evict_one_excluding(cache, self);   // 淘汰其它 LRU，保留自己
 *
 * 现有 test_gzip_cache.cpp 的所有 put 都用等长值（如 "z" 长度 1），
 * delta 恒为 0，while 的 delta>0 条件立即为假——循环体从未进入。
 * 其余测试用不同 hash，走的是「新增条目」路径而非替换路径。
 *
 * 这条分支的正确性关键：替换时若增长超预算，要淘汰**其它**条目而保留
 * 正在替换的那个（它是最新值）。若误淘汰自己，新值就丢了；若完全不淘汰，
 * 内存预算保证被绕过。
 */

#include <gtest/gtest.h>
#include <string.h>

#if UVHTTP_FEATURE_COMPRESSION

extern "C" {
#    include "uvhttp_gzip_cache.h"
}

class GzipCacheReplace : public ::testing::Test {
   protected:
    uvhttp_gzip_cache_t* cache_ = nullptr;

    /* 预算按 8 倍条目开销设，使有限条目即可触发淘汰 */
    void SetUp() override {
        size_t budget = 64 * 1024;
        int entries = 64;
        ASSERT_EQ(uvhttp_gzip_cache_create(budget, entries, 3600, &cache_),
                  UVHTTP_OK);
        ASSERT_NE(cache_, nullptr);
    }

    void TearDown() override {
        if (cache_) {
            uvhttp_gzip_cache_free(cache_);
        }
    }

    void Stats(size_t* mem, int* count, int* hit, int* miss) {
        uvhttp_gzip_cache_get_stats(cache_, mem, count, hit, miss);
    }

    /* 用指定长度填充内容的缓冲 */
    static void Fill(char* buf, size_t n, char c) {
        memset(buf, c, n);
    }
};

/* ========== 等长替换（delta == 0） ========== */

/* 同 (hash, body_len) 等长替换：更新值而非新增槽位 */
TEST_F(GzipCacheReplace, EqualLengthReplacementKeepsSingleEntry) {
    char a[64], b[64];
    Fill(a, sizeof(a), 'a');
    Fill(b, sizeof(b), 'b');

    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x1111, 10, a, sizeof(a)),
              UVHTTP_OK);
    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x1111, 10, b, sizeof(b)),
              UVHTTP_OK);

    size_t mem;
    int count, hit, miss;
    Stats(&mem, &count, &hit, &miss);
    EXPECT_EQ(count, 1) << "等长替换不应新增条目";

    /* find 应返回新值 */
    size_t out_len = 0;
    const char* found = uvhttp_gzip_cache_find(cache_, 0x1111, 10, &out_len);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(out_len, sizeof(b));
    EXPECT_EQ(memcmp(found, b, sizeof(b)), 0) << "应返回替换后的新值";
}

/* ========== 变长替换 + 超预算 → 淘汰其它条目 ========== */

/* 精确预算场景：预算刚好装下 3 条 4KB 条目（约 12.4KB），把其中一条
 * 替换为 8KB 后净增 4KB 必然超预算，强制走替换路径的淘汰循环。
 * （若预算过大则替换后仍在限内，循环不会执行——这是该分支难测的原因。） */
TEST_F(GzipCacheReplace, LongerReplacementEvictsOtherEntriesWhenOverBudget) {
    uvhttp_gzip_cache_free(cache_);
    cache_ = nullptr;
    size_t tight_budget = 13 * 1024;
    ASSERT_EQ(uvhttp_gzip_cache_create(tight_budget, 64, 3600, &cache_),
              UVHTTP_OK);

    /* 3 条不同键的 4KB 条目 */
    char buf[4096];
    Fill(buf, sizeof(buf), 'x');
    for (int i = 0; i < 3; i++) {
        ASSERT_EQ(
            uvhttp_gzip_cache_put(cache_, 0x1000 + i, 100, buf, sizeof(buf)),
            UVHTTP_OK)
            << "第 " << i << " 条 put 失败";
    }

    size_t mem_before;
    int count_before, hit, miss;
    Stats(&mem_before, &count_before, &hit, &miss);
    ASSERT_EQ(count_before, 3) << "预算应恰好容纳 3 条";

    /* 同键 (0x1000, 100) 替换为 8KB：delta=+4KB，总量超预算 → 淘汰 1 条 */
    char bigger[8192];
    Fill(bigger, sizeof(bigger), 'y');
    ASSERT_EQ(
        uvhttp_gzip_cache_put(cache_, 0x1000, 100, bigger, sizeof(bigger)),
        UVHTTP_OK);

    size_t mem_after;
    int count_after;
    Stats(&mem_after, &count_after, &hit, &miss);

    /* 内存不得超预算——这是淘汰循环存在的根本理由 */
    EXPECT_LE(mem_after, tight_budget)
        << "替换后内存超预算：" << mem_after << " > " << tight_budget;

    /* 条目数必须减少 1：净增的 4KB 只能靠淘汰一条 4KB 条目腾出 */
    EXPECT_EQ(count_after, count_before - 1)
        << "应恰好淘汰 1 条以容纳增长（before=" << count_before
        << " after=" << count_after << "）";

    /* 被替换的条目仍在（它是最新值，evict_one_excluding 不该淘汰它） */
    size_t out_len = 0;
    const char* found = uvhttp_gzip_cache_find(cache_, 0x1000, 100, &out_len);
    ASSERT_NE(found, nullptr) << "正在替换的条目不应被淘汰";
    EXPECT_EQ(out_len, sizeof(bigger));
    EXPECT_EQ(memcmp(found, bigger, sizeof(bigger)), 0);
}

/* ========== 变长替换但不超预算 → 不应淘汰 ========== */

/* delta > 0 但仍有充足预算：条目数不变，只做替换 */
TEST_F(GzipCacheReplace, LongerReplacementWithoutOverBudgetKeepsEntries) {
    char small[256], bigger[512];
    Fill(small, sizeof(small), 's');
    Fill(bigger, sizeof(bigger), 'b');

    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x2222, 10, small, sizeof(small)),
              UVHTTP_OK);
    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x3333, 10, small, sizeof(small)),
              UVHTTP_OK);

    int count_before, hit, miss;
    size_t mem_before;
    Stats(&mem_before, &count_before, &hit, &miss);
    ASSERT_EQ(count_before, 2);

    /* 默认 64KB 预算，512 字节替换远未超限 */
    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x2222, 10, bigger, sizeof(bigger)),
              UVHTTP_OK);

    int count_after;
    size_t mem_after;
    Stats(&mem_after, &count_after, &hit, &miss);
    EXPECT_EQ(count_after, count_before) << "未超预算时不应淘汰任何条目";

    size_t out_len = 0;
    const char* found = uvhttp_gzip_cache_find(cache_, 0x2222, 10, &out_len);
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(out_len, sizeof(bigger));
}

/* ========== body_len 不同 → 不是替换路径 ========== */

/* 相同 hash 但 body_len 不同：应作为不同键处理（新增条目） */
TEST_F(GzipCacheReplace, SameHashDifferentBodyLenIsNewEntry) {
    char data[128];
    Fill(data, sizeof(data), 'd');

    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x4444, 100, data, sizeof(data)),
              UVHTTP_OK);
    ASSERT_EQ(uvhttp_gzip_cache_put(cache_, 0x4444, 200, data, sizeof(data)),
              UVHTTP_OK);

    size_t mem;
    int count, hit, miss;
    Stats(&mem, &count, &hit, &miss);
    EXPECT_EQ(count, 2) << "body_len 不同应视为不同条目";

    size_t l1 = 0, l2 = 0;
    EXPECT_NE(uvhttp_gzip_cache_find(cache_, 0x4444, 100, &l1), nullptr);
    EXPECT_NE(uvhttp_gzip_cache_find(cache_, 0x4444, 200, &l2), nullptr);
}

#endif /* UVHTTP_FEATURE_COMPRESSION */