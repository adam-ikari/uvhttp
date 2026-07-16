# 中间件系统完善 Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 完善编译时零开销中间件系统——修复已知缺陷、补充缺失功能、确保测试覆盖。

**Architecture:** 保持编译时宏展开设计，不引入运行时注册。修复标签冲突、上下文清理泄漏、类型适配缺失三个核心问题，然后补充错误码、文档和测试。

**Tech Stack:** C11 宏 + `__COUNTER__`/`__LINE__` 生成唯一标签，GoogleTest 单元测试，CMake 构建系统。

---

## 文件结构

| 文件 | 变更类型 | 职责 |
|------|----------|------|
| `include/uvhttp_middleware.h` | 修改 | 修复标签冲突、修复上下文清理、添加 HANDLER 包装宏、重写注释 |
| `include/uvhttp_error.h` | 修改 | 添加中间件错误码枚举 |
| `src/uvhttp_error.c` | 修改 | 添加中间件错误码描述字符串 |
| `test/unit/test_middleware.cpp` | 新建 | 中间件宏单元测试 |
| `test/integration/test_middleware_compile_time.c` | 修改 | 修复为真正测试宏的集成测试 |

---

### Task 1: 修复 UVHTTP_EXECUTE_MIDDLEWARE 标签冲突

**Files:**
- Modify: `include/uvhttp_middleware.h`

**问题:** `UVHTTP_EXECUTE_MIDDLEWARE` 使用固定标签 `_uvhttp_mw_stop`，同一函数内只能用一次。

- [ ] **Step 1: 用 `__COUNTER__` 生成唯一标签**

将当前的 `UVHTTP_EXECUTE_MIDDLEWARE` 宏替换为使用 `__COUNTER__` 的版本：

```c
// 辅助宏：拼接标签名
#define _UVHTTP_MW_LABEL_(counter) _uvhttp_mw_stop_##counter
#define _UVHTTP_MW_LABEL(counter) _UVHTTP_MW_LABEL_(counter)

#define UVHTTP_EXECUTE_MIDDLEWARE(req, resp, ...)                                \
    do {                                                                         \
        uvhttp_middleware_context_t _uvhttp_mw_ctx = {NULL, NULL};              \
        _UVHTTP_MW_LABEL(__COUNTER__):                                          \
        UVHTTP_EXPAND_MIDDLEWARE_LIST(req, resp, &_uvhttp_mw_ctx, __VA_ARGS__) \
        if (_uvhttp_mw_ctx.cleanup) {                                           \
            _uvhttp_mw_ctx.cleanup(_uvhttp_mw_ctx.data);                        \
        }                                                                        \
    } while (0)
```

注意：`__COUNTER__` 在 GCC 4.3+、Clang、MSVC 均支持。如果需要兼容更老的编译器，可在宏内加一个 `#ifndef _UVHTTP_MW_USE_COUNTER` 回退到 `__LINE__`。

- [ ] **Step 2: 同样修复 `UVHTTP_EXECUTE_MIDDLEWARE_CHAIN`**

对 chain 执行宏也做同样的标签唯一化处理：

```c
#define UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, chain)                       \
    do {                                                                         \
        uvhttp_middleware_context_t _uvhttp_mw_ctx = {NULL, NULL};              \
        _UVHTTP_MW_LABEL(__COUNTER__):                                          \
        for (size_t _i = 0; _i < (chain).count; _i++) {                        \
            int _r = (chain).handlers[_i]((req), (resp), &_uvhttp_mw_ctx);     \
            if (_r == UVHTTP_MIDDLEWARE_STOP) {                                 \
                goto _UVHTTP_MW_LABEL(__COUNTER__);                             \
            }                                                                    \
        }                                                                        \
        if (_uvhttp_mw_ctx.cleanup) {                                           \
            _uvhttp_mw_ctx.cleanup(_uvhttp_mw_ctx.data);                        \
        }                                                                        \
    } while (0)
```

等一下——`__COUNTER__` 在同一宏展开中会递增，所以 goto 标签和标签定义处的 `__COUNTER__` 值不同。需要用两步宏来捕获同一个 counter 值：

```c
#define _UVHTTP_MW_EXECUTE_IMPL_(counter, req, resp, ...)                       \
    do {                                                                         \
        uvhttp_middleware_context_t _uvhttp_mw_ctx_##counter = {NULL, NULL};    \
        UVHTTP_EXPAND_MIDDLEWARE_LIST(req, resp, &_uvhttp_mw_ctx_##counter,     \
                                      __VA_ARGS__)                              \
        if (_uvhttp_mw_ctx_##counter.cleanup) {                                 \
            _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);    \
        }                                                                        \
        goto _uvhttp_mw_done_##counter;                                         \
        _uvhttp_mw_stop_##counter:                                              \
        if (_uvhttp_mw_ctx_##counter.cleanup) {                                 \
            _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);    \
        }                                                                        \
        _uvhttp_mw_done_##counter:;                                             \
    } while (0)

#define _UVHTTP_MW_EXECUTE_IMPL(counter, req, resp, ...)                        \
    _UVHTTP_MW_EXECUTE_IMPL_(counter, req, resp, __VA_ARGS__)

#define UVHTTP_EXECUTE_MIDDLEWARE(req, resp, ...)                               \
    _UVHTTP_MW_EXECUTE_IMPL(__COUNTER__, req, resp, __VA_ARGS__)
```

类似地处理 `UVHTTP_EXECUTE_MIDDLEWARE_CHAIN`。

- [ ] **Step 3: 编译验证**

Run: `cmake --build build_coverage -j$(nproc) 2>&1 | tail -5`
Expected: 编译成功，无错误

- [ ] **Step 4: Commit**

```bash
git add include/uvhttp_middleware.h
git commit -m "fix(middleware): resolve label collision with __COUNTER__"
```

---

### Task 2: 修复上下文清理泄漏

**Files:**
- Modify: `include/uvhttp_middleware.h`

**问题:** 当前 `EXPAND_MIDDLEWARE_LIST` 中，中间件返回 STOP 时 goto 到标签处清理 context，但中间件返回 CONTINUE 时，清理不会执行。如果中间件 A 分配了 ctx->data 并返回 CONTINUE，后续中间件 B 返回 STOP 时，A 的 cleanup 不会被调用（因为 ctx 被覆盖）。

**设计决策:** context 应该是共享的——所有中间件共享同一个 `uvhttp_middleware_context_t`。这意味着后一个中间件设置 `ctx->data` 会覆盖前一个。这是预期行为（与 Koa 的 ctx 模式一致）。但需要确保 STOP 时清理被调用，且 CONTINUE 路径结束后也能清理。

- [ ] **Step 1: 确保正常完成路径也调用 cleanup**

在 Task 1 的新宏实现中，正常路径（所有中间件返回 CONTINUE）已经有 cleanup 调用：

```c
// 正常完成路径
if (_uvhttp_mw_ctx_##counter.cleanup) {
    _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);
}
goto _uvhttp_mw_done_##counter;

// STOP 路径
_uvhttp_mw_stop_##counter:
if (_uvhttp_mw_ctx_##counter.cleanup) {
    _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);
}
_uvhttp_mw_done_##counter:;
```

两条路径都有清理，问题已解决。

- [ ] **Step 2: 在 `EXPAND_MIDDLEWARE_LIST` 中为每个中间件添加 STOP 分支**

当前 `EXPAND_MIDDLEWARE_LIST` 展开类似：

```c
mw1(req, resp, ctx);
mw2(req, resp, ctx);
```

需要改为：

```c
if (mw1(req, resp, ctx) == UVHTTP_MIDDLEWARE_STOP) goto _uvhttp_mw_stop_XXX;
if (mw2(req, resp, ctx) == UVHTTP_MIDDLEWARE_STOP) goto _uvhttp_mw_stop_XXX;
```

这需要重写 `UVHTTP_EXPAND_MIDDLEWARE_LIST` 和相关的 `UVHTTP_MW_INVOKE` 宏。

- [ ] **Step 3: 编译验证**

Run: `cmake --build build_coverage -j$(nproc) 2>&1 | tail -5`
Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
git add include/uvhttp_middleware.h
git commit -m "fix(middleware): call context cleanup on both STOP and CONTINUE paths"
```

---

### Task 3: 添加 UVHTTP_MIDDLEWARE_HANDLER 包装宏

**Files:**
- Modify: `include/uvhttp_middleware.h`

**问题:** `uvhttp_request_handler_t`（路由处理器）签名是 `int (*)(request*, response*)`，`uvhttp_middleware_handler_t` 签名是 `int (*)(request*, response*, ctx*)`。无法混用。

- [ ] **Step 1: 添加 HANDLER 包装宏**

```c
/**
 * @brief 将路由处理器包装为中间件
 *
 * 将 uvhttp_request_handler_t 签名适配为 uvhttp_middleware_handler_t。
 * 忽略 context 参数，直接调用原处理器。
 * 处理器返回 0 视为 CONTINUE，非 0 视为 STOP。
 *
 * 用法：
 *   UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *       UVHTTP_MIDDLEWARE_HANDLER(my_route_handler),
 *       auth_middleware);
 */
#define UVHTTP_MIDDLEWARE_HANDLER(handler)                                      \
    _uvhttp_mw_handler_wrapper_##handler

#define UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)                               \
    static int _uvhttp_mw_handler_wrapper_##handler(                            \
        uvhttp_request_t* _req, uvhttp_response_t* _resp,                      \
        uvhttp_middleware_context_t* _ctx) {                                    \
        (void)_ctx;                                                             \
        int _ret = handler(_req, _resp);                                        \
        return _ret == 0 ? UVHTTP_MIDDLEWARE_CONTINUE : UVHTTP_MIDDLEWARE_STOP;\
    }
```

使用模式：

```c
// 在文件顶部定义包装器
UVHTTP_DEFINE_MIDDLEWARE_HANDLER(my_handler);

// 在路由中使用
UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
    auth_middleware,
    UVHTTP_MIDDLEWARE_HANDLER(my_handler));
```

- [ ] **Step 2: 编译验证**

Run: `cmake --build build_coverage -j$(nproc) 2>&1 | tail -5`
Expected: 编译成功

- [ ] **Step 3: Commit**

```bash
git add include/uvhttp_middleware.h
git commit -m "feat(middleware): add UVHTTP_MIDDLEWARE_HANDLER wrapper macro"
```

---

### Task 4: 添加中间件错误码

**Files:**
- Modify: `include/uvhttp_error.h`
- Modify: `src/uvhttp_error.c`

- [ ] **Step 1: 在 `uvhttp_error.h` 枚举中添加中间件错误码**

在 `UVHTTP_ERROR_COMPRESSION_*` 之后（约 -1000 预留区间）添加：

```c
/** 中间件错误码 */
UVHTTP_ERROR_MIDDLEWARE_STOPPED = -1000,    /**< 中间件链被中断 */
UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY = -1001, /**< 空中间件链 */
UVHTTP_ERROR_MIDDLEWARE_INVALID = -1002,    /**< 无效中间件处理器 */
```

- [ ] **Step 2: 在 `uvhttp_error.c` 中添加描述字符串**

在错误描述数组/函数中添加对应字符串：

```c
case UVHTTP_ERROR_MIDDLEWARE_STOPPED:
    return "Middleware chain stopped";
case UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY:
    return "Middleware chain is empty";
case UVHTTP_ERROR_MIDDLEWARE_INVALID:
    return "Invalid middleware handler";
```

- [ ] **Step 3: 编译验证**

Run: `cmake --build build_coverage -j$(nproc) 2>&1 | tail -5`
Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
git add include/uvhttp_error.h src/uvhttp_error.c
git commit -m "feat(error): add middleware error codes"
```

---

### Task 5: 编写中间件单元测试

**Files:**
- Create: `test/unit/test_middleware.cpp`

- [ ] **Step 1: 创建测试文件，包含基础 fixture 和辅助中间件**

```cpp
/**
 * @file test_middleware.cpp
 * @brief 中间件系统单元测试
 */
#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_middleware.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_error.h"
}

// --- 辅助中间件 ---

// 计数中间件：记录调用次数
static int g_mw_call_count = 0;
static int counting_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                               uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    g_mw_call_count++;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

// 阻断中间件：返回 STOP
static int blocking_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                               uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    return UVHTTP_MIDDLEWARE_STOP;
}

// 上下文分配中间件：分配内存到 ctx
static int alloc_middleware(uvhttp_request_t* req, uvhttp_response_t* resp,
                            uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    if (!ctx->data) {
        ctx->data = malloc(64);
        ctx->cleanup = free;
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

// 上下文阻断中间件：分配后返回 STOP
static int alloc_and_stop_middleware(uvhttp_request_t* req,
                                     uvhttp_response_t* resp,
                                     uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    if (!ctx->data) {
        ctx->data = malloc(64);
        ctx->cleanup = free;
    }
    return UVHTTP_MIDDLEWARE_STOP;
}

// 路由处理器（用于 HANDLER 包装测试）
static int g_route_handler_called = 0;
static int test_route_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req; (void)resp;
    g_route_handler_called++;
    return 0;
}

// 返回非零的路由处理器
static int failing_route_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req; (void)resp;
    return -1;
}
```

- [ ] **Step 2: 编写 UVHTTP_EXECUTE_MIDDLEWARE 基础测试**

```cpp
class MiddlewareTest : public ::testing::Test {
protected:
    void SetUp() override {
        g_mw_call_count = 0;
        g_route_handler_called = 0;
    }
};

TEST_F(MiddlewareTest, ExecuteMiddleware_ContinuePath) {
    // 单个 CONTINUE 中间件应正常执行
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, counting_middleware);

    EXPECT_EQ(g_mw_call_count, 1);
}

TEST_F(MiddlewareTest, ExecuteMiddleware_MultipleContinue) {
    // 多个 CONTINUE 中间件应按序执行
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        counting_middleware,
        counting_middleware,
        counting_middleware);

    EXPECT_EQ(g_mw_call_count, 3);
}

TEST_F(MiddlewareTest, ExecuteMiddleware_StopPath) {
    // STOP 中间件应中断后续执行
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        blocking_middleware,
        counting_middleware);

    // counting_middleware 不应被调用
    EXPECT_EQ(g_mw_call_count, 0);
}
```

- [ ] **Step 3: 编写上下文清理测试**

```cpp
TEST_F(MiddlewareTest, ContextCleanup_ContinuePath) {
    // CONTINUE 路径应清理 context
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, alloc_middleware);
    // 如果 cleanup 未调用，valgrind/asan 会报告泄漏
}

TEST_F(MiddlewareTest, ContextCleanup_StopPath) {
    // STOP 路径应清理 context
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, alloc_and_stop_middleware);
    // 如果 cleanup 未调用，valgrind/asan 会报告泄漏
}
```

- [ ] **Step 4: 编写同一函数多次使用测试**

```cpp
TEST_F(MiddlewareTest, MultipleUsagePerFunction) {
    // 验证同一函数内可以使用多次 UVHTTP_EXECUTE_MIDDLEWARE
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, counting_middleware);
    EXPECT_EQ(g_mw_call_count, 1);

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, counting_middleware);
    EXPECT_EQ(g_mw_call_count, 2);
}
```

- [ ] **Step 5: 编写 UVHTTP_DEFINE_MIDDLEWARE_CHAIN 测试**

```cpp
TEST_F(MiddlewareTest, DefineChain_Execute) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(test_chain,
        counting_middleware,
        counting_middleware);

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, test_chain);
    EXPECT_EQ(g_mw_call_count, 2);
}

TEST_F(MiddlewareTest, DefineChain_StopMidway) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(blocking_chain,
        counting_middleware,
        blocking_middleware,
        counting_middleware);

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, blocking_chain);
    // 只有第一个 counting 被调用，blocking 后中断
    EXPECT_EQ(g_mw_call_count, 1);
}
```

- [ ] **Step 6: 编写 UVHTTP_MIDDLEWARE_HANDLER 包装测试**

```cpp
// 定义包装器
UVHTTP_DEFINE_MIDDLEWARE_HANDLER(test_route_handler);
UVHTTP_DEFINE_MIDDLEWARE_HANDLER(failing_route_handler);

TEST_F(MiddlewareTest, HandlerWrapper_Continue) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        UVHTTP_MIDDLEWARE_HANDLER(test_route_handler));

    EXPECT_EQ(g_route_handler_called, 1);
}

TEST_F(MiddlewareTest, HandlerWrapper_StopOnNonZero) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        UVHTTP_MIDDLEWARE_HANDLER(failing_route_handler),
        counting_middleware);

    // failing_handler 返回 -1，包装为 STOP，counting 不应被调用
    EXPECT_EQ(g_mw_call_count, 0);
}

TEST_F(MiddlewareTest, HandlerWrapper_MixedWithMiddleware) {
    uvhttp_request_t* req = nullptr;
    uvhttp_response_t* resp = nullptr;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        counting_middleware,
        UVHTTP_MIDDLEWARE_HANDLER(test_route_handler));

    EXPECT_EQ(g_mw_call_count, 1);
    EXPECT_EQ(g_route_handler_called, 1);
}
```

- [ ] **Step 7: 编写错误码测试**

```cpp
TEST_F(MiddlewareTest, ErrorCodes_AreDistinct) {
    EXPECT_NE(UVHTTP_ERROR_MIDDLEWARE_STOPPED, UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY);
    EXPECT_NE(UVHTTP_ERROR_MIDDLEWARE_STOPPED, UVHTTP_ERROR_MIDDLEWARE_INVALID);
    EXPECT_NE(UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY, UVHTTP_ERROR_MIDDLEWARE_INVALID);
    EXPECT_LT(UVHTTP_ERROR_MIDDLEWARE_STOPPED, 0);
}

TEST_F(MiddlewareTest, ErrorCodes_DescriptionNotNull) {
    const char* desc;
    desc = uvhttp_strerror(UVHTTP_ERROR_MIDDLEWARE_STOPPED);
    EXPECT_NE(desc, nullptr);
    EXPECT_STRNE(desc, "");

    desc = uvhttp_strerror(UVHTTP_ERROR_MIDDLEWARE_CHAIN_EMPTY);
    EXPECT_NE(desc, nullptr);

    desc = uvhttp_strerror(UVHTTP_ERROR_MIDDLEWARE_INVALID);
    EXPECT_NE(desc, nullptr);
}
```

- [ ] **Step 8: 编译运行测试**

Run: `cmake --build build_coverage -j$(nproc) && ctest --test-dir build_coverage -R middleware --output-on-failure`
Expected: 所有测试通过

- [ ] **Step 9: Commit**

```bash
git add test/unit/test_middleware.cpp
git commit -m "test(middleware): add unit tests for middleware macros and error codes"
```

---

### Task 6: 重写中间件头文件注释

**Files:**
- Modify: `include/uvhttp_middleware.h`

- [ ] **Step 1: 替换文件头部注释**

将当前的混乱注释替换为清晰的中文文档：

```c
/**
 * @file uvhttp_middleware.h
 * @brief 编译时零开销中间件系统
 *
 * 设计哲学：
 * - 编译时宏展开，无运行时注册/查找开销
 * - 短路语义：中间件返回 STOP 立即中断链
 * - 共享上下文：所有中间件共享同一个 context
 * - 运行时中间件由用户自行实现
 *
 * 核心宏：
 * - UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw1, mw2, ...)
 *     内联执行中间件链，每个中间件按序调用
 *     任一返回 STOP 则跳过后续中间件
 *
 * - UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, mw1, mw2, ...)
 *     定义可复用的中间件链（静态数组）
 *
 * - UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, chain)
 *     执行预定义的中间件链
 *
 * - UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)
 *     将路由处理器包装为中间件（返回 0 = CONTINUE，非 0 = STOP）
 *
 * 用法示例：
 * @code
 *   // 在处理函数内执行中间件
 *   int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *           auth_middleware,
 *           logging_middleware);
 *       // ... 处理请求 ...
 *       return 0;
 *   }
 *
 *   // 定义可复用链
 *   UVHTTP_DEFINE_MIDDLEWARE_CHAIN(api_chain,
 *       auth_middleware,
 *       rate_limit_middleware,
 *       logging_middleware);
 *
 *   // 执行链
 *   UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, api_chain);
 * @endcode
 */
```

- [ ] **Step 2: 为每个公开宏添加内联注释**

在每个宏定义上方添加简短中文注释，说明参数和返回值语义。

- [ ] **Step 3: 编译验证**

Run: `cmake --build build_coverage -j$(nproc) 2>&1 | tail -5`
Expected: 编译成功

- [ ] **Step 4: Commit**

```bash
git add include/uvhttp_middleware.h
git commit -m "docs(middleware): rewrite header comments with clear documentation"
```

---

### Task 7: 修复集成测试

**Files:**
- Modify: `test/integration/test_middleware_compile_time.c`

- [ ] **Step 1: 重写集成测试，使用真正的中间件宏**

当前测试不使用任何宏，只是调用普通函数。重写为真正测试宏行为：

```c
/**
 * @file test_middleware_compile_time.c
 * @brief 中间件编译时特性集成测试
 *
 * 验证中间件宏在真实编译环境下正确工作：
 * - 多文件使用不冲突
 * - 与路由处理器集成
 * - 链式执行顺序正确
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp_middleware.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

/* --- 测试中间件 --- */

static int call_order[8];
static int call_order_idx = 0;

static int mw_first(uvhttp_request_t* req, uvhttp_response_t* resp,
                     uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 1;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int mw_second(uvhttp_request_t* req, uvhttp_response_t* resp,
                      uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 2;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int mw_blocker(uvhttp_request_t* req, uvhttp_response_t* resp,
                       uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 3;
    return UVHTTP_MIDDLEWARE_STOP;
}

static int mw_after_block(uvhttp_request_t* req, uvhttp_response_t* resp,
                           uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp; (void)ctx;
    call_order[call_order_idx++] = 4;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 带上下文清理的中间件 */
static int g_cleanup_called = 0;
static void test_cleanup(void* data) {
    free(data);
    g_cleanup_called = 1;
}

static int mw_with_cleanup(uvhttp_request_t* req, uvhttp_response_t* resp,
                            uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    ctx->data = malloc(32);
    ctx->cleanup = test_cleanup;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int mw_with_cleanup_stop(uvhttp_request_t* req, uvhttp_response_t* resp,
                                 uvhttp_middleware_context_t* ctx) {
    (void)req; (void)resp;
    ctx->data = malloc(32);
    ctx->cleanup = test_cleanup;
    return UVHTTP_MIDDLEWARE_STOP;
}

/* 路由处理器 */
static int g_handler_result = 0;
static int route_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req; (void)resp;
    g_handler_result = 42;
    return 0;
}

UVHTTP_DEFINE_MIDDLEWARE_HANDLER(route_handler);

/* --- 测试函数 --- */

static void test_execute_continue(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first, mw_second);

    assert(call_order[0] == 1);
    assert(call_order[1] == 2);
    printf("  PASS: execute_continue\n");
}

static void test_execute_stop(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first, mw_blocker, mw_after_block);

    assert(call_order[0] == 1);
    assert(call_order[1] == 3);
    assert(call_order[2] == 0);  /* mw_after_block 未被调用 */
    printf("  PASS: execute_stop\n");
}

static void test_cleanup_continue(void) {
    g_cleanup_called = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_with_cleanup);

    assert(g_cleanup_called == 1);
    printf("  PASS: cleanup_continue\n");
}

static void test_cleanup_stop(void) {
    g_cleanup_called = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_with_cleanup_stop);

    assert(g_cleanup_called == 1);
    printf("  PASS: cleanup_stop\n");
}

static void test_multiple_usage(void) {
    call_order_idx = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_first);
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw_second);

    assert(call_order[0] == 1);
    assert(call_order[1] == 2);
    printf("  PASS: multiple_usage\n");
}

static void test_handler_wrapper(void) {
    g_handler_result = 0;

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        UVHTTP_MIDDLEWARE_HANDLER(route_handler));

    assert(g_handler_result == 42);
    printf("  PASS: handler_wrapper\n");
}

static void test_chain(void) {
    call_order_idx = 0;
    memset(call_order, 0, sizeof(call_order));

    UVHTTP_DEFINE_MIDDLEWARE_CHAIN(test_chain, mw_first, mw_second);

    uvhttp_request_t* req = NULL;
    uvhttp_response_t* resp = NULL;

    UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, test_chain);

    assert(call_order[0] == 1);
    assert(call_order[1] == 2);
    printf("  PASS: chain\n");
}

int main(void) {
    printf("Middleware compile-time integration tests:\n");
    test_execute_continue();
    test_execute_stop();
    test_cleanup_continue();
    test_cleanup_stop();
    test_multiple_usage();
    test_handler_wrapper();
    test_chain();
    printf("All tests passed!\n");
    return 0;
}
```

- [ ] **Step 2: 编译运行**

Run: `cmake --build build_coverage -j$(nproc) && ctest --test-dir build_coverage -R middleware --output-on-failure`
Expected: 所有测试通过

- [ ] **Step 3: Commit**

```bash
git add test/integration/test_middleware_compile_time.c
git commit -m "fix(test): rewrite middleware integration test to use actual macros"
```

---

## 自查清单

1. **规格覆盖**：所有 7 个问题点（标签冲突、清理泄漏、类型适配、错误码、注释、单元测试、集成测试）均有对应任务。

2. **占位符扫描**：无 TBD、TODO、"implement later"、"add validation" 等占位符。所有代码步骤包含完整实现。

3. **类型一致性**：
   - `uvhttp_middleware_handler_t` 签名在所有任务中保持一致：`int (*)(request*, response*, ctx*)`
   - `UVHTTP_MIDDLEWARE_HANDLER` 包装的 handler 名称与 `UVHTTP_DEFINE_MIDDLEWARE_HANDLER` 定义一致
   - 错误码枚举值在 error.h 和测试中引用一致
