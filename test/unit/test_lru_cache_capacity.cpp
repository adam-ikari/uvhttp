/*
 * UVHTTP LRU 缓存容量边界测试
 *
 * 为什么需要独立测试：uvhttp_lru_cache_put 的「空缓存仍需空间」分支
 * （src/uvhttp_lru_cache.c:378-384）从未被执行——
 *
 *     while (total + memory_usage > max_memory * 0.9 || entry_count >=
 * max_entries) { if (!cache->lru_tail) {
 *             // cache 已空但仍需空间 → 无法满足
 *             return UVHTTP_ERROR_OUT_OF_MEMORY;
 *         }
 *         ... 淘汰 ...
 *     }
 *
 * 现有 test_lru_cache_full_coverage.cpp 的内存预算都远大于单条目
 * （sizeof(cache_entry_t) 约 10KB，因内含 etag[4096] + safe_path[1024] 等），
 * 因此淘汰循环总是有对象可淘汰，lru_tail==NULL 的早退分支永不触发。
 *
 * 该分支的价值：它是「单条目就超过内存预算」时的唯一防线。缺失则 put 会
 * 写入一个必然超预算的条目，破坏 max_memory_usage 的保证（后续所有 put
 * 都会立刻触发淘汰，缓存退化为不可用）。
 */

#include <gtest/gtest.h>
#include <string.h>
#include <time.h>

#if UVHTTP_FEATURE_STATIC_FILES

extern "C" {
#    include "uvhttp_error.h"
#    include "uvhttp_lru_cache.h"
}

class LruCacheCapacity : public ::testing::Test {
   protected:
    cache_manager_t* cache_ = nullptr;

    void TearDown() override {
        if (cache_) {
            uvhttp_lru_cache_free(cache_);
            cache_ = nullptr;
        }
    }

    /* 创建指定内存预算的缓存 */
    void Create(size_t budget) {
        ASSERT_EQ(uvhttp_lru_cache_create(budget, 10, 3600, &cache_),
                  UVHTTP_OK);
    }

    /* 放入指定长度的内容，返回 put 结果 */
    uvhttp_error_t Put(const char* path, size_t content_len, char fill) {
        std::vector<char> buf(content_len, fill);
        return uvhttp_lru_cache_put(cache_, path, buf.data(), buf.size(),
                                    "application/octet-stream", time(NULL),
                                    "\"etag-probe\"");
    }
};

/* sizeof(cache_entry_t) 约 10KB（内含 etag[4096] + safe_path[1024]
 * 等固定数组）， 故 16KB 预算能容纳一个小条目，但容不下 8KB 内容条目。 */
static constexpr size_t kBudget = 16 * 1024;

/* ========== 空缓存 + 单条目超 0.9×预算 → OOM ========== */

/* 内存预算 16KB，条目开销约 10KB + 内容 8KB = 约 18KB > 0.9×16KB(14.7KB)。
 * 缓存为空（lru_tail==NULL），淘汰无从下手 → 返回 OUT_OF_MEMORY。 */
TEST_F(LruCacheCapacity, SingleEntryExceedingBudgetOnEmptyCacheReturnsOom) {
    Create(kBudget);
    EXPECT_EQ(cache_->entry_count, 0);

    uvhttp_error_t r = Put("/big.bin", 8 * 1024, 'x');

    EXPECT_EQ(r, UVHTTP_ERROR_OUT_OF_MEMORY)
        << "单条目超过内存预算且缓存为空时应返回 OUT_OF_MEMORY";
    EXPECT_EQ(cache_->entry_count, 0) << "失败的 put 不应留下条目";
}

/* 关键不变量：失败的 put 不能写入超预算条目，否则 max_memory_usage
 * 保证被破坏（后续 put 会全部立即触发淘汰）。 */
TEST_F(LruCacheCapacity, FailedPutDoesNotBreakMemoryBudget) {
    Create(kBudget);
    ASSERT_EQ(Put("/big.bin", 8 * 1024, 'x'), UVHTTP_ERROR_OUT_OF_MEMORY);

    /* 预算内的小条目仍应能正常放入（缓存未被破坏） */
    EXPECT_EQ(Put("/small.bin", 64, 's'), UVHTTP_OK);
    EXPECT_EQ(cache_->entry_count, 1);
    EXPECT_LE(cache_->total_memory_usage, kBudget);
}

/* 同一缓存上重复触发 OOM 应稳定返回 OOM（非一次性偶发） */
TEST_F(LruCacheCapacity, RepeatedOversizedPutStablyReturnsOom) {
    Create(kBudget);
    for (int i = 0; i < 3; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/big%d.bin", i);
        EXPECT_EQ(Put(path, 8 * 1024, 'x'), UVHTTP_ERROR_OUT_OF_MEMORY)
            << "第 " << i << " 次超预算 put 应稳定返回 OOM";
    }
    EXPECT_EQ(cache_->entry_count, 0);
}

/* ========== 对照：预算充足时正常放入 ========== */

/* 同等内容但预算翻倍 → 不触发 OOM */
TEST_F(LruCacheCapacity, SameContentWithLargerBudgetSucceeds) {
    Create(32 * 1024);
    EXPECT_EQ(Put("/big.bin", 8 * 1024, 'x'), UVHTTP_OK);
    EXPECT_EQ(cache_->entry_count, 1);
    EXPECT_LE(cache_->total_memory_usage, 32 * 1024);
}

/* ========== 边界：刚好在 0.9×预算之内 ========== */

/* 0.9×16KB ≈ 14.7KB；条目开销约 10KB，内容 4KB → 约 14KB，略低于阈值，
 * 应能放入。这验证阈值边界不是「非 0 即 1」的粗判断。 */
TEST_F(LruCacheCapacity, EntryJustUnderThresholdIsAccepted) {
    Create(kBudget);
    uvhttp_error_t r = Put("/near.bin", 4 * 1024, 'n');
    EXPECT_EQ(r, UVHTTP_OK) << "接近但未超 0.9×预算的条目应可放入";
    EXPECT_EQ(cache_->entry_count, 1);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */