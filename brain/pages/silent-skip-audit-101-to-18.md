---
id: silent-skip-audit-101-to-18
title: "静默跳过测试治理：101→18（#453/#454/#455/#456）"
category: project
status: active
tags: [test, ci, quality, false-positive]
created: "2026-10-05T01:30:20"
updated: "2026-10-05T04:20:27"
---

<!-- compiled_truth -->
# 静默跳过测试治理：101→18（4 个 PR）

## 成果

2026-10-04 治理 `if (X) { 断言 }` 静默跳过测试：**101 → 18**（消除 83 项）。

| PR | 处理 | 数量 |
|---|---|---|
| #453 | static_file_operations（MIME/ETag）+ api_coverage | 51 |
| #454 | static_enhanced + extended + comprehensive + more | 23 |
| #455 | request_full + server_simple_handlers | 7 |
| #456 | server_error | 2 |

## 核心模式：create 前置恒真守卫

最常见假阳性：
```c
result = create(...);
if (result == UVHTTP_OK) {     // create 对合法输入恒 OK → 守卫恒真
    inner = do_x(...);
    EXPECT_NE(inner, OK);      // 若 create 回归失败，此断言被静默跳过
}
```
修复：`if (result == UVHTTP_OK) {` → `ASSERT_EQ(result, UVHTTP_OK);` +
删配对 `}`。create 失败时测试**显式终止**而非静默跳过。

被验证是恒绿的：变异 create 恒失败 → 测试显式变红（改造前全绿）。

## 三类不同语义（不是所有 if 守卫都是假阳性）

1. **create 前置恒真**（static/server/...）—— 真假阳性，可修（本轮主体）
2. **`if (ctx)` 防御 OOM**（tls_api_coverage 12 项）—— 合法，不可变异
3. **feature 条件守卫**（version 3 项）—— 合法，feature 字段只在对应配置非零

## 剩余 18 项分档（不在本轮改）

- **tls_api_coverage 12 项**：`if (ctx)`，`tls_context_new` 只在 OOM 失败
  （`src/uvhttp_tls.c:140` calloc），恒定非 OOM 时 ctx 非空。改成
  ASSERT_NE(ctx, nullptr) 有防御价值但 OOM 不可模拟，非恒绿假阳性。
- **version 3 项**：`if (info.feature_router_cache)` 等，feature 字段只在
  对应构建配置非零——合法条件守卫。
- **request_extended AddHeaderEmptyName**：探针确认 `add_header("", v)`
  返回 OK 且 count=1，恒走 else 分支断言执行（非假阳性）。但暴露产品
  问题：**空 header 名被接受**（HTTP 规范应拒）。
- **router_enhanced ParsePathParamsNoParams** / **server_api
  ServerNewWithLoopSuccess**：前置恒真，弱断言（非紧急）。

## 遗留产品问题（本次顺带发现，未修）

`uvhttp_request_add_header("", "value")` 返回 OK（0）且成功添加一个
空名 header、header_count=1。空 header 名按 RFC 7230 应被拒。这是产品
行为问题，不影响本测试（断言走 else 分支有效执行）。

## 记忆

- batch 处理 if 守卫用大括号配平脚本，但遇内层重新声明同名变量（如
  cleanup 返回 int vs create 返回 uvhttp_error_t）会作用域冲突，需改名。
- 变异点注入 `return NULL; /* MUTATION */` 会触发 -Werror=unused-parameter，
  需补 `(void)param;`。


## Timeline

- time: 2026-10-05T01:30:20
  kind: decision
  summary: "Created this page: 静默跳过测试治理：101→18（#453/#454/#455/#456）"
  source: "2026-10-04 治理完成"
  affects: [silent-skip-audit-101-to-18]

- time: 2026-10-05T01:30:20
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "治理完成"
  affects: [silent-skip-audit-101-to-18]

- time: 2026-10-05T04:20:27
  kind: decision
  summary: "静跳过治理收官：101→15。#453-#457 消除 86 项真假阳性。剩余 15 项经逐项核实为合规守卫（tls 12 项 if(ctx) 是 OOM 防御、version 3 项是 feature 条件），不应改。#457 顺带修了 add_header 空名产品缺口（RFC 7230）——修复刻意只拒空名不连带改长名截断，因为长名截断是独立决策。"
  affects: [silent-skip-audit-101-to-18]
