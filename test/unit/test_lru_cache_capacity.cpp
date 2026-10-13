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

/*
 * 背景：uvhttp_lru_cache_create 拒绝 max_memory_usage == 0（"cannot be zero"）
 * 且要求 max_entries > 0，而淘汰路径把 "> 0" 读作"限额生效"。setter 此前
 * 直接赋值，于是两条写路径对同一个不变量给出相反答案：create 拒绝的值，
 * setter 照单全收，还顺手把运行中缓存的限额关掉了（0 → 视为无限额）。
 *
 * 同一个库内的正确范式是 uvhttp_lru_cache_set_batch_eviction_size：越界时
 * 告警并保持原值，而不是静默接受。
 */

TEST_F(LruCacheCapacity, SetMaxMemoryUsageRejectsZero) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_memory_usage(cache_, 0);

    EXPECT_EQ(cache_->max_memory_usage, kBudget)
        << "create 拒绝 0（cannot be zero），setter 不得把它写进去";
}

/* 越界值不生效后，限额仍是原来那个 —— 不是被静默放宽 */
TEST_F(LruCacheCapacity, ZeroMaxMemoryUsageDoesNotWidenTheBudget) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_memory_usage(cache_, 0);

    /* 预算仍是 16KB：8KB 内容条目（+约 10KB 开销）依旧放不进 */
    EXPECT_EQ(Put("/big.bin", 8 * 1024, 'x'), UVHTTP_ERROR_OUT_OF_MEMORY)
        << "写入 0 之后内存上限必须仍然生效";
}

TEST_F(LruCacheCapacity, SetMaxMemoryUsageAcceptsPositiveValue) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_memory_usage(cache_, 32 * 1024);

    EXPECT_EQ(cache_->max_memory_usage, 32 * 1024);
    EXPECT_EQ(Put("/big.bin", 8 * 1024, 'x'), UVHTTP_OK)
        << "合法的更大预算应立即生效";
}

TEST_F(LruCacheCapacity, SetMaxEntriesRejectsZeroAndNegative) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_entries(cache_, 0);
    EXPECT_EQ(cache_->max_entries, 10) << "create 要求 max_entries > 0";

    uvhttp_lru_cache_set_max_entries(cache_, -5);
    EXPECT_EQ(cache_->max_entries, 10) << "负值同样必须被拒";
}

/* 关闭条目限额会让有界缓存退化成无界缓存 —— 这正是校验要阻止的后果 */
TEST_F(LruCacheCapacity, ZeroMaxEntriesWouldDisableTheEntryCeiling) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_entries(cache_, 0);

    /* 限额仍是 10：放入第 11 条时应触发淘汰，entry_count 不越过上限 */
    for (int i = 0; i < 11; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/f%d.bin", i);
        Put(path, 64, 's');
    }
    EXPECT_LE(cache_->entry_count, cache_->max_entries)
        << "条目上限必须仍然生效";
}

TEST_F(LruCacheCapacity, SetMaxEntriesAcceptsPositiveValue) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_entries(cache_, 3);

    EXPECT_EQ(cache_->max_entries, 3);
}

/* ========== set_max_file_size：写点不变量 ========== */

/*
 * put() 在「check if file size exceeds limit」分支用
 *     if (content_length > cache->max_file_size) → INVALID_PARAM
 * 对内容长度做硬截断。而 create() 根本不接收 max_file_size（默认 1GB 写死在
 * lru_cache.c:82），所以 setter 是这个字段唯一的守卫。
 *
 * 评审探针实证：set_max_file_size(cache, 0) 之后，每次 put 都以 -1 失败 ——
 * 缓存被砖化，调用方只看到一串 "File size exceeds limit" 的 WARN。
 * 本次补上与另两个 setter 同范式的下限校验。
 */

TEST_F(LruCacheCapacity, SetMaxFileSizeRejectsZero) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_file_size(cache_, 0);

    EXPECT_EQ(cache_->max_file_size, 1024u * 1024 * 1024)
        << "create 把默认值写死为 1GB；setter 不得把 0 写进去";
}

/* 核心回归：0 不得砖化缓存。砖化的表现是 put 永久 INVALID_PARAM。 */
TEST_F(LruCacheCapacity, ZeroMaxFileSizeDoesNotBrickTheCache) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_file_size(cache_, 0);

    EXPECT_EQ(Put("/after.bin", 64, 's'), UVHTTP_OK)
        << "写入 0 之后 put 必须仍能成功（评审探针在此处实测返回 -1）";
    EXPECT_EQ(cache_->entry_count, 1);
}

/* 合法值仍然立即生效：小于新上限的进得去，大于的被拒 */
TEST_F(LruCacheCapacity, SetMaxFileSizeAcceptsPositiveValue) {
    Create(kBudget);

    uvhttp_lru_cache_set_max_file_size(cache_, 1024);
    EXPECT_EQ(cache_->max_file_size, 1024u);

    EXPECT_EQ(Put("/small.bin", 512, 's'), UVHTTP_OK)
        << "小于新上限的内容应能放入";
    EXPECT_EQ(Put("/big.bin", 4096, 'b'), UVHTTP_ERROR_INVALID_PARAM)
        << "大于新上限的内容应被拒（文件大小判定先于内存预算判定）";
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
