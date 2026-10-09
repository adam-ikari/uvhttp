---
id: test-validity-always-green
title: "测试有效性：恒绿零验证的普查与 CI 门禁"
category: project
status: active
tags: [test, ci, mutation, quality]
created: "2026-10-03T01:19:51"
updated: "2026-10-03T01:19:51"
---

<!-- compiled_truth -->
# 测试有效性：恒绿零验证的普查与门禁

## 结论

2026-10-03 普查全部 3095 个测试（116 个文件）：

| 类别 | 数量 | 性质 |
|---|---|---|
| 断言被注释掉 | 4 | **恒绿零验证**，已全部修复 |
| 断言被条件静默跳过 | 101 | 前置条件不满足即跳过，多数需逐项核实 |
| 零断言测试 | 274 | 验证「不崩溃」，ASan/UBSan 下仍能捕获内存错误 |

**测试数量多 ≠ 测试有效。** 274 个零断言 + 101 个静默跳过意味着约 12% 的
测试在任何情况下都不会因被测行为错误而失败。

## 变异验证是唯一可信的判据

`test_server_error_coverage.cpp` 的 `check_rate_limit` 断言被注释后，把该
函数对 NULL 的返回值从 `UVHTTP_OK` 完全反转成 `UVHTTP_ERROR_INVALID_PARAM`
——**15 个测试全部仍然通过**。

同一文件相邻的 `get_rate_limit_status`（断言存在）做同样变异，测试立即
变红。对照组的存在让结论无可辩驳。

## 已知的陷阱模式（下次遇到直接查）

1. **断言被注释 + printf 警告代替**
   ```c
   // EXPECT_STREQ(query, "key=value");
   if (query) { EXPECT_STREQ(query, "key=value"); }
   else { printf("Warning: ..."); }
   ```
   作者因断言不可靠而注释掉它——但真实原因常是**测试前置条件没满足**。

2. **`max_file_size = 0` 意为「禁止所有文件」**（非「无限制」）
   `if (file_size > ctx->config.max_file_size) return UVHTTP_ERROR_INVALID_PARAM;`
   memset 后的 config 默认 0，任何文件都被拒。已在
   `test_static_enhanced_coverage.cpp:308` 记录此陷阱。

3. **`prewarm_cache` 接受单文件而非目录** —— 传目录时路径被二次拼接，
   stat 失败返回 NOT_FOUND。

## 门禁

`scripts/ci/check_test_validity.py`，接入 `ci-pr.yml` 的
`code-quality-check` job。三类违规零容忍新增，基线：

- [1] 断言被注释掉：基线 **0**
- [2] 静默跳过：基线 **101**（需逐项人工核实后下调）
- [3] 零断言测试：基线 **274**（不清零，ASan 下有价值）

基线下调即治理进度。

## 方法论：静态扫描只能识别模式

普查脚本能识别「断言在 if 内」「断言被注释」，但**不能证明该条件在真实
运行中是否成立**。101 个静默跳过里必然混有合规的（如用 `GTEST_SKIP`
显式标记跳过——那是对的），所以不能一次性批量改，必须逐个变异验证。

同理，274 个零断言测试不是缺陷——`uvhttp_free(nullptr)` 这类测试验证的
是「不崩溃」，在 ASan/UBSan 门禁下能捕获 use-after-free / 越界。它们的
问题是**在 Release 测试的通过率里贡献了分母**，让绿灯通胀。

## 已尝试但不可行的方法

**gtest listener 统计断言数**：`OnTestPartResult` 只对**失败**的断言触发，
无法统计通过的断言（实测 3 个测试只有 2 个通过 → 计数 2）。该方法无效，
不要再尝试。

静态计数（函数体内 EXPECT_/ASSERT_ 数量）可靠，可用于识别零断言。


## Timeline

- time: 2026-10-03T01:19:51
  kind: decision
  summary: "Created this page: 测试有效性：恒绿零验证的普查与 CI 门禁"
  source: "2026-10-03 普查 3095 个测试"
  affects: [test-validity-always-green]

- time: 2026-10-03T01:19:51
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-10-03 普查"
  affects: [test-validity-always-green]
