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
 *    修复前全仓无任何测试构造同一父节点下 12+ 个子段；本文件即补上这一类，
 *    并覆盖更危险的一条：迁移**本身**也会撞这个上限（见文末
 *    RouterMigrationAtomicity）。
 *
 * 实测行为（probe 确认）：参数段 `:id` **也占用一个子节点额度**。
 * 注册 `/api/:id` 后再注册 `/api/r0`..`/api/r10`，父节点 "api" 下的
 * 子节点数为 1(参数) + 11(静态) = 12；`/api/r11` 起被拒。
 *
 * 这条上限是内存护栏：它生效时**上限内的路由必须仍能注册与查找**——
 * 若实现误在第 12 条就拒绝，或拒绝时损坏已有路由，护栏就成了功能缺陷。
 */

#include <gtest/gtest.h>

/* src/uvhttp_router.c 整体被 `#if !UVHTTP_FEATURE_ROUTER_CACHE` 包裹——启用
 * router cache 时改走另一套实现，本文件测的 12 子节点 trie 上限不存在。
 * 与其它 router 测试一致，本文件受同一条件控制。 */
#if !UVHTTP_FEATURE_ROUTER_CACHE

extern "C" {
#    include "uvhttp_router.h"
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

/* ==================================================================== */
/* ========== array → trie 迁移失败：必须不损坏已注册路由 ========== */
/* ==================================================================== */

/*
 * 上面所有用例都先用 EnterTrieMode() 进入 trie，迁移是在**空 array** 上发生
 * 的，因而从未测到真正的危险路径：array 里已有路由、迁移中途撞 12 子节点
 * 上限而失败。
 *
 * migrate_to_trie（src/uvhttp_router.c:321）把 array 的每条路由逐个插入 trie。
 * 插入用的是同一个 find_or_create_child，所以**迁移本身也会撞上限**。触发
 * 条件很普通：13 条不同顶层路由（都留在 array，因为 13 < HYBRID_THRESHOLD）
 * 之后再注册任意一条 `:参数` 路由 —— has_params 强制迁移，根节点需要 13 个
 * 子节点，第 13 个失败。
 *
 * 修复前这里会 free 掉整张 array 路由表（当年为躲 dangling pointer 而 detach
 * + free），而 use_trie 仍是 0，于是 find_handler 走 array 分支却已无表可查：
 * **所有已注册路由静默全丢，服务器整站 404**。
 *
 * 迁移因此必须是原子的：要么全部迁成，要么什么都不变。
 */

/* 触发迁移失败：13 条顶层静态路由 + 1 条参数路由 */
class RouterMigrationAtomicity : public ::testing::Test {
   protected:
    uvhttp_router_t* router_ = nullptr;
    static constexpr int kTopLevelRoutes = 13;

    void SetUp() override {
        ASSERT_EQ(uvhttp_router_new(&router_), UVHTTP_OK);
        ASSERT_NE(router_, nullptr);
        for (int i = 0; i < kTopLevelRoutes; i++) {
            char path[32];
            snprintf(path, sizeof(path), "/p%d", i);
            ASSERT_EQ(uvhttp_router_add_route(router_, path, DummyHandler),
                      UVHTTP_OK)
                << "前置条件：" << path << " 应注册成功";
        }
        ASSERT_EQ(router_->use_trie, 0) << "前置条件：应仍处于 array 模式";
    }

    void TearDown() override {
        if (router_) {
            /* 释放不崩、不二次释放——本用例同时是 ASan/UBSan 的探针 */
            uvhttp_router_free(router_);
            router_ = nullptr;
        }
    }

    /* 参数路由令 array 全量迁入 trie，根节点需 kTopLevelRoutes 个子节点 */
    uvhttp_error_t AddParamRouteForcingMigration() {
        return uvhttp_router_add_route(router_, "/p0/:id", DummyHandler);
    }
};

/* 迁移失败必须留在 array 模式：use_trie 与路由表同时才是有效状态 */
TEST_F(RouterMigrationAtomicity, FailedMigrationKeepsRouterInArrayMode) {
    EXPECT_EQ(AddParamRouteForcingMigration(), UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_EQ(router_->use_trie, 0);
}

/* 核心回归：迁移失败不得销毁已注册路由 */
TEST_F(RouterMigrationAtomicity,
       FailedMigrationPreservesEveryRegisteredRoute) {
    ASSERT_EQ(AddParamRouteForcingMigration(), UVHTTP_ERROR_OUT_OF_MEMORY);

    /* 表还在、计数还在——这正是修复前变成悬空指针的那三个字段 */
    EXPECT_EQ(router_->array_route_count,
              static_cast<size_t>(kTopLevelRoutes))
        << "失败的迁移不应清空 array 路由表";
    EXPECT_EQ(router_->route_count, static_cast<size_t>(kTopLevelRoutes))
        << "失败的迁移不应丢失路由计数";

    for (int i = 0; i < kTopLevelRoutes; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/p%d", i);
        EXPECT_NE(uvhttp_router_find_handler(router_, path, "GET"), nullptr)
            << path << " 在迁移失败后仍必须可解析";
    }
}

/* uvhttp_router_match 是请求路径实际走的接口，必须同样不受损 */
TEST_F(RouterMigrationAtomicity, FailedMigrationKeepsRoutesMatchable) {
    ASSERT_EQ(AddParamRouteForcingMigration(), UVHTTP_ERROR_OUT_OF_MEMORY);

    for (int i = 0; i < kTopLevelRoutes; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/p%d", i);
        uvhttp_route_match_t m;
        EXPECT_EQ(uvhttp_router_match(router_, path, "GET", &m), UVHTTP_OK)
            << path << " 应仍匹配成功，而非 404";
        EXPECT_NE(m.handler, nullptr) << path;
    }
}

/* 触发迁移的那条路由本身注册失败，不应留下半成品条目 */
TEST_F(RouterMigrationAtomicity, RouteThatTriggeredFailureIsNotAdded) {
    ASSERT_EQ(AddParamRouteForcingMigration(), UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_EQ(router_->route_count, static_cast<size_t>(kTopLevelRoutes))
        << "失败的注册不应计入 route_count";

    uvhttp_route_match_t m;
    EXPECT_NE(uvhttp_router_match(router_, "/p0/123", "GET", &m), UVHTTP_OK)
        << "未注册成功的参数路由不应匹配";
}

/* 迁移失败是可重复的：后续注册同样失败，但同样不损坏状态 */
TEST_F(RouterMigrationAtomicity, RepeatedFailedMigrationStaysStable) {
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(AddParamRouteForcingMigration(),
                  UVHTTP_ERROR_OUT_OF_MEMORY)
            << "第 " << i << " 次迁移应同样被拒";
        EXPECT_EQ(router_->use_trie, 0);
        EXPECT_EQ(router_->route_count, static_cast<size_t>(kTopLevelRoutes));
    }

    for (int i = 0; i < kTopLevelRoutes; i++) {
        char path[32];
        snprintf(path, sizeof(path), "/p%d", i);
        EXPECT_NE(uvhttp_router_find_handler(router_, path, "GET"), nullptr)
            << path << " 在多次失败迁移后仍必须可解析";
    }
}

/* 迁移失败后，array 侧新增路由仍应正常注册（上限只挡 trie，不该连带） */
TEST_F(RouterMigrationAtomicity, ArrayAddsStillWorkAfterFailedMigration) {
    ASSERT_EQ(AddParamRouteForcingMigration(), UVHTTP_ERROR_OUT_OF_MEMORY);

    EXPECT_EQ(uvhttp_router_add_route(router_, "/late", DummyHandler),
              UVHTTP_OK);
    EXPECT_NE(uvhttp_router_find_handler(router_, "/late", "GET"), nullptr);
    EXPECT_NE(uvhttp_router_find_handler(router_, "/p0", "GET"), nullptr)
        << "新增路由不应挤掉已有路由";
}

#endif /* !UVHTTP_FEATURE_ROUTER_CACHE */
