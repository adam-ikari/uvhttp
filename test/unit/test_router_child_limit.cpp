/*
 * UVHTTP 路由 trie 子节点上限测试
 *
 * 为什么需要独立测试：find_or_create_child（src/uvhttp_router.c:149）限制
 * 每个 trie 节点最多 12 个子节点：
 *
 *     // create new child node
 *     if (parent->child_count >= 12) {
 *         return UINT32_MAX;
 *     }
 *
 * 上层 add_route_method（src/uvhttp_router.c:447）把 UINT32_MAX 转成
 * UVHTTP_ERROR_OUT_OF_MEMORY，即超额的同级路由**静默注册失败**。
 *
 * 两个前置事实决定了这个分支为何从未被测到：
 *
 * 1. **短路径根本不走 trie**。add_route_method 只在「路径含 ':' 参数」
 *    「array_route_count >= HYBRID_THRESHOLD(100)」或「已 use_trie」时
 *    才迁移到 trie（src/uvhttp_router.c:408-411）。注册 13 条 `/pN` 这类
 *    短路径全部落在 array 路由里，trie 上限无从触发。
 * 2. **现有测试刻意避开上限**。注释明写：
 *      test_router_boost_coverage.cpp:90「well under the 12-child limit per
 * node」 test_router_boost_coverage.cpp:180/213「spread across ... to avoid
 *                                       exceeding the 12-child limit」
 *    循环上界全是 i < 10，全仓无任何测试构造同一父节点下 12+ 个子段。
 *
 * 实测行为（probe 确认）：参数段 `:id` **也占用一个子节点额度**。
 * 注册 `/api/:id` 后再注册 `/api/r0`..`/api/r10`，父节点 "api" 下的
 * 子节点数为 1(参数) + 11(静态) = 12；`/api/r11` 起被拒。
 *
 * 这条上限是内存护栏：它生效时**上限内的路由必须仍能注册与查找**——
 * 若实现误在第 12 条就拒绝，或拒绝时损坏已有路由，护栏就成了功能缺陷。
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_router.h"
}

namespace {
/* 各级路由共用同一 handler——trie 测试关注注册与查找，不关心 handler 身份 */
int DummyHandler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    (void)res;
    return 0;
}
}  // namespace

class RouterChildLimit : public ::testing::Test {
   protected:
    uvhttp_router_t* router_ = nullptr;

    void SetUp() override {
        ASSERT_EQ(uvhttp_router_new(&router_), UVHTTP_OK);
        ASSERT_NE(router_, nullptr);
    }

    void TearDown() override {
        if (router_) {
            uvhttp_router_free(router_);
            router_ = nullptr;
        }
    }

    /* 切换到 trie 模式：含 ':' 参数的路径直接走 trie（不经 HYBRID_THRESHOLD）。
     * 该路由同时占掉父节点 "api" 下的一个子节点额度。 */
    void EnterTrieMode() {
        ASSERT_EQ(uvhttp_router_add_route(router_, "/api/:id", DummyHandler),
                  UVHTTP_OK);
        ASSERT_EQ(router_->use_trie, 1) << "参数路由应使 router 进入 trie 模式";
    }

    /* 在已存在的 "api" 子节点下注册静态路由 /api/rN */
    uvhttp_error_t AddUnderApi(int n) {
        char path[32];
        snprintf(path, sizeof(path), "/api/r%d", n);
        return uvhttp_router_add_route(router_, path, DummyHandler);
    }

    bool Matches(const char* path) {
        uvhttp_route_match_t m;
        return uvhttp_router_match(router_, path, "GET", &m) == UVHTTP_OK &&
               m.handler != nullptr;
    }
};

/* 上限内可注册的静态兄弟数：1 个参数段占 1 个额度，故静态侧最多 11 条 */
static constexpr int kMaxStaticSiblings = 11;

/* ========== 上限边界 ========== */

/* 恰好填满 12 个子节点：1 参数 + 11 静态，全部注册成功 */
TEST_F(RouterChildLimit, FillingQuotaExactlySucceeds) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        EXPECT_EQ(AddUnderApi(i), UVHTTP_OK)
            << "第 " << i << " 条静态兄弟应注册成功（上限内）";
    }
    EXPECT_EQ(router_->route_count, 1u + kMaxStaticSiblings);
}

/* 第 13 个子节点应被拒绝 */
TEST_F(RouterChildLimit, ExceedingChildQuotaIsRejected) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    EXPECT_EQ(AddUnderApi(kMaxStaticSiblings), UVHTTP_ERROR_OUT_OF_MEMORY)
        << "超过 12 子节点上限的路由应被拒绝";
}

/* 拒绝后 route_count 不应增长——注册失败不应留下半成品条目 */
TEST_F(RouterChildLimit, RejectedRouteDoesNotIncrementCount) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    size_t before = router_->route_count;
    EXPECT_EQ(AddUnderApi(kMaxStaticSiblings), UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_EQ(router_->route_count, before) << "失败的注册不应增加 route_count";
}

/* 拒绝后上限内的路由仍可正常查找——护栏只挡新路由，不损坏已有路由 */
TEST_F(RouterChildLimit, ExistingRoutesStillResolveAfterRejection) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    ASSERT_EQ(AddUnderApi(kMaxStaticSiblings), UVHTTP_ERROR_OUT_OF_MEMORY);

    /* 参数路由仍可匹配（含参数提取） */
    uvhttp_route_match_t pm;
    ASSERT_EQ(uvhttp_router_match(router_, "/api/123", "GET", &pm), UVHTTP_OK);
    EXPECT_NE(pm.handler, nullptr);

    /* 上限内的静态路由全部仍可匹配 */
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/api/r%d", i);
        EXPECT_TRUE(Matches(path)) << path << " 在超额路由被拒后仍应可匹配";
    }
}

/* 超额注册的静态路由虽注册失败，但该路径仍由参数路由 `/api/:id` 兜住——
 * 这是正确行为（注册失败 ≠ 请求不可服务），且值得钉死：
 * 若参数路由也匹配不上，超额路径才会变成 404。 */
TEST_F(RouterChildLimit, RejectedStaticRouteStillFallsBackToParamRoute) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    char path[32];
    snprintf(path, sizeof(path), "/api/r%d", kMaxStaticSiblings);
    ASSERT_EQ(AddUnderApi(kMaxStaticSiblings), UVHTTP_ERROR_OUT_OF_MEMORY);

    /* 该路径匹配到参数路由而非 404 */
    uvhttp_route_match_t m;
    EXPECT_EQ(uvhttp_router_match(router_, path, "GET", &m), UVHTTP_OK)
        << "超额静态路由的路径应回落到参数路由，而非 404";
    EXPECT_NE(m.handler, nullptr);
}

/* 连续多次超额都稳定返回同一错误（非一次性偶发） */
TEST_F(RouterChildLimit, RepeatedOverflowStablyRejected) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    for (int i = kMaxStaticSiblings; i < kMaxStaticSiblings + 8; i++) {
        EXPECT_EQ(AddUnderApi(i), UVHTTP_ERROR_OUT_OF_MEMORY)
            << "第 " << i << " 条应稳定被拒";
    }
    EXPECT_EQ(router_->route_count, 1u + kMaxStaticSiblings);
}

/* ========== 上限不误伤 ========== */

/* 上限已满时，重新注册一条**已存在**的路由应成功——find_or_create_child
 * 先查现有子节点再检查上限，该路径不需新建节点故不受上限影响。 */
TEST_F(RouterChildLimit, RegisteringExistingRouteStillWorksAtCapacity) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    /* 上限已满时重新注册已有路由：find_or_create_child 先查现有子节点
     * 再检查上限，该路径不需新建节点，故不受上限影响。
     * （route_count 会再次 +1——add_route_method 在 trie 分支末尾无条件
     * 递增，不去重；这是既有计数语义，不在本次测试范围内。） */
    EXPECT_EQ(AddUnderApi(0), UVHTTP_OK)
        << "容量已满时重新注册已有路由应成功（不需新建子节点）";
    /* 关键：注册未被拒，且该路由仍可匹配 */
    EXPECT_TRUE(Matches("/api/r0")) << "/api/r0 应仍可匹配";
}

/* ========== 上限按节点独立计算 ========== */

/* 上限是**每节点** 12 个子节点，不同父节点各有独立额度 */
TEST_F(RouterChildLimit, LimitIsPerNodeNotGlobal) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    /* 另一个父节点下仍有完整额度，不受 "api" 节点已满影响 */
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/other/r%d", i);
        EXPECT_EQ(uvhttp_router_add_route(router_, path, DummyHandler),
                  UVHTTP_OK)
            << path << " 位于另一个父节点，应有独立额度";
    }
}

/* 更深层的节点同样有独立额度 */
TEST_F(RouterChildLimit, DeeperNodeHasIndependentQuota) {
    EnterTrieMode();
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        ASSERT_EQ(AddUnderApi(i), UVHTTP_OK);
    }
    ASSERT_EQ(AddUnderApi(kMaxStaticSiblings), UVHTTP_ERROR_OUT_OF_MEMORY);

    /* 在某个已存在的静态子节点下再挂 11 条孙路由 */
    for (int i = 0; i < kMaxStaticSiblings; i++) {
        char path[40];
        snprintf(path, sizeof(path), "/api/r0/deep%d", i);
        EXPECT_EQ(uvhttp_router_add_route(router_, path, DummyHandler),
                  UVHTTP_OK)
            << path << " 位于 r0 节点下，应有独立额度";
    }
}

/* ========== 对照：array 模式不受此上限约束 ========== */

/* 未进入 trie 模式时，13 条短静态路由全部注册成功——说明 12 子节点上限
 * 只作用于 trie，不影响 array 路由路径（避免误以为上限是全局的）。 */
TEST_F(RouterChildLimit, ArrayModeIsNotSubjectToChildLimit) {
    for (int i = 0; i < 13; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/p%d", i);
        EXPECT_EQ(uvhttp_router_add_route(router_, path, DummyHandler),
                  UVHTTP_OK)
            << "/p" << i << " 在 array 模式下不应受 trie 子节点上限约束";
    }
    EXPECT_EQ(router_->route_count, 13u);
}
