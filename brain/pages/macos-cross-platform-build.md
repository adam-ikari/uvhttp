---
id: macos-cross-platform-build
title: "macOS 跨平台编译：4 个 GNU-only 障碍与 non-blocking 门禁"
category: decision
status: active
tags: [ci, portability, macos, build]
created: "2026-11-24T01:32:01"
updated: "2026-11-24T01:32:15"
---

<!-- compiled_truth -->
# macOS 跨平台编译：4 个 GNU-only 障碍与 non-blocking 门禁

PR #507（2026-11-23）给 `ci-pr.yml` 加 `macos-latest` 编译 job 时，逐个暴露
并修复了 4 个只在 macOS/AppleClang/Apple ld 下才触发的障碍：

| # | 障碍 | 修复 |
|---|---|---|
| 1 | vendored libuv 的 `uv__free(const char*)` 在我们的 `-Werror` 下报错 | `cmake/Dependencies.cmake` 按 `APPLE` 追加 `-Wno-error=incompatible-pointer-types-discards-qualifiers`，只影响依赖构建 |
| 2 | `src/uvhttp_server.c:602` 用 Linux 专有 `TCP_CORK` | 条件编译：Linux→`TCP_CORK`，Apple/BSD→`TCP_NOPUSH`，其他平台跳过（默认 disabled，跳过行为中性） |
| 3 | GNU ld 的 `--gc-sections` / `-z,relro,now` Apple ld 不认 | `CMakeLists.txt` 按平台选 `-dead_strip`；RELRO/BIND_NOW 仅非 Apple |
| 4 | mock 测试用 GNU ld `--wrap`，Apple ld 不支持 | `CMakeLists.txt` 把整个 mock 测试 target 块用 `if(NOT APPLE)` 包住；这些测试仍在 Linux CI 跑 |

## 决策：macOS job 设 `continue-on-error: true`

macOS runner 排队不可预测，多次出现 `queued` 后 `cancelled`（从未分配到
runner）。作为 PR 必需门禁会因外部容量阻塞合并。设为 non-blocking：有结果
时提供可见信号，排队取消不阻塞。Linux 的 9 个 job 仍是真实门禁。

## 验证边界

最后一次改动（skip mock tests on Apple）后，macOS job 仍处于 `queued`，
未能观察到它真正跑完变绿。前 3 个障碍已通过实际 macOS job 日志逐个验证
修复（每次错误推进到一个新的障碍）。mock 跳过的 `if(NOT APPLE)` 逻辑只在
Linux 侧验证了非 Apple 路径不变。


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
