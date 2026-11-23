---
id: macos-cross-platform-build
title: "macOS 跨平台编译：4 个 GNU-only 障碍与 non-blocking 门禁"
category: decision
status: active
tags: [ci, portability, macos, build]
created: "2026-11-24T01:32:01"
updated: "2026-11-24T01:53:38"
---

<!-- compiled_truth -->
# macOS 跨平台编译：5 个 GNU-only 障碍与 non-blocking 门禁

PR #507 / #508（2026-11-23）给 `ci-pr.yml` 加 `macos-latest` 编译 job 时，
逐个暴露并修复了 5 个只在 macOS / AppleClang / Apple ld 下才触发的障碍：

| # | 障碍 | 修复 |
|---|---|---|
| 1 | vendored libuv 的 `uv__free(const char*)` 在我们的 `-Werror` 下报错 | `cmake/Dependencies.cmake` 按 `APPLE` 追加 `-Wno-error=incompatible-pointer-types-discards-qualifiers`，只影响依赖构建 |
| 2 | `src/uvhttp_server.c:602` 用 Linux 专有 `TCP_CORK` | 条件编译：Linux→`TCP_CORK`，Apple/BSD→`TCP_NOPUSH`，其他平台跳过（默认 disabled，跳过行为中性） |
| 3 | GNU ld 的 `--gc-sections` / `-z,relro,now` Apple ld 不认 | `CMakeLists.txt` 按平台选 `-dead_strip`；RELRO/BIND_NOW 仅非 Apple |
| 4 | mock 测试用 GNU ld `--wrap`，Apple ld 不支持 | `CMakeLists.txt` 把整个 mock 测试 target 块用 `if(NOT APPLE)` 包住；这些测试仍在 Linux CI 跑 |
| 5 | `examples/CMakeLists.txt` 的 `-Werror=discarded-qualifiers` 是 GCC 专有名，AppleClang 当作未知 warning 选项报错 | 仅 `CMAKE_C_COMPILER_ID STREQUAL "GNU"` 时追加该 flag |

## 决策：macOS job 设 `continue-on-error: true`

macOS runner 排队不可预测，多次出现 `queued` 后 `cancelled`（从未分配到
runner）。作为 PR 必需门禁会因外部容量阻塞合并。设为 non-blocking：有结果
时提供可见信号，排队取消不阻塞。Linux 的 9 个 job 仍是真实门禁。

## 验证边界

前 4 个障碍通过实际 macOS job 日志逐个验证（每次错误推进到下一个障碍）。
第 5 个在 PR #508 的 macOS 日志确认（examples/simple_server.c 编译失败）。
修复后尚未在 macOS 上观察到最终变绿——job 常在修复 push 后回到 `queued`。
每个修复都在 Linux 侧验证了非 Apple 路径无回归。


## Timeline

- time: 2026-11-24T01:32:01
  kind: decision
  summary: "Created this page: macOS 跨平台编译：4 个 GNU-only 障碍与 non-blocking 门禁"
  source: created via brain create-page
  affects: [macos-cross-platform-build]

- time: 2026-11-24T01:32:15
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #507 加 macOS CI job 时逐项暴露并修复"
  affects: [macos-cross-platform-build]

- time: 2026-11-24T01:53:38
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #508 追加第 5 个障碍：examples/CMakeLists.txt 的 GCC 专有 warning 名"
  affects: [macos-cross-platform-build]
