---
id: runner-toolchain-drift
title: "CI runner 工具链漂移：CMake 4 与新版 cppcheck"
category: project
status: active
tags: [ci, cmake, cppcheck, runner]
created: "2026-10-20T16:12:46"
updated: "2026-10-20T16:47:59"
---

<!-- compiled_truth -->
## 事实
CI runner（`ubuntu-latest`）的工具版本不固定。2026-10-20 前后出现两起漂移，都在没有改动相关代码的 PR #485 上暴露：

- **CMake 4.x**：vendored cJSON 声明 `cmake_minimum_required(VERSION 3.0)`，CMake 4 直接拒绝（"Compatibility with CMake < 3.5 has been removed"），`build-matrix` 五个配置全部倒在 configure。源码侧修复是给 cJSON 子配置传 `-DCMAKE_POLICY_VERSION_MINIMUM=3.5`（`cmake/Dependencies.cmake`）；旧版 CMake 把它当未使用变量忽略，因此无条件传入是安全的。
- **cppcheck**：runner 上的版本高于本地 2.7，首次报出 `src/uvhttp_websocket.c` 空 CLOSE 帧回包缓冲区的 `uninitvar`（缓冲区未写入即以 `len=0` 传入发送层）。工作流注释里「本地验证 18/18 文件通过」只对当时的 cppcheck 版本成立。

## 已发布 tag 的特殊性
`release` 事件使用的是 **tag 提交上的 workflow 文件**，不是默认分支：官方事件列表里 release 不在「workflow 文件必须位于默认分支」的名单中；实测 `published` 事件已经发出，而 tag 里的触发器是无效的 `types: [prerelease]` 时零运行，换成包含修复的探针 tag 后两条门禁立即触发。

因此 tag 提交里的 workflow 决定门禁能否启动。v3.0.0 首发 tag（045a160）既带无效触发器、又早于 cJSON 修复；2026-10-20 将 v3.0.0 重打到 8376f1b（含 cJSON 修复与顶层 `env: CMAKE_POLICY_VERSION_MINIMUM=3.5`）后，两条门禁才真实运行。该环境变量会初始化**每个新 build tree** 的 cache，包括 `Dependencies.cmake` 里 `execute_process` 拉起的子配置，以及 benchmark 对 base v2.9.2 的 checkout；CMake < 4 忽略它。

## 判据
- 门禁结论以 CI 实际运行的工具版本为准，本地静态分析只是预检；不要用本地通过去推断 CI。
- 修 vendored 依赖的 CMake 兼容性时，源码侧用子配置级 flag。需要 release 事件门禁的 tag，其提交必须包含可用的 workflow 与环境补偿 —— 否则门禁根本不会启动，只能重打 tag（2026-10-20 v3.0.0 经用户授权重打）。
- 「本 PR 没碰相关代码却红了」先按环境漂移排查，再判断是否真回归。

## 边界
本项目不 pin runner 的 CMake/cppcheck 版本：上升期维护成本高于收益。若漂移变成常态（连续多次因同一工具版本红），再考虑在 `setup-build` 中钉版本。


## Timeline

- time: 2026-10-20T16:12:46
  kind: decision
  summary: "Created this page: CI runner 工具链漂移：CMake 4 与新版 cppcheck"
  source: "PR #485 CI 诊断（2026-10-20）"
  affects: [runner-toolchain-drift]

- time: 2026-10-20T16:12:52
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #485 CI 诊断（2026-10-20）"
  affects: [runner-toolchain-drift]

- time: 2026-10-20T16:12:52
  kind: decision
  summary: "定位并修复 PR #485 的两起 runner 漂移：CMake 4 拒绝 cJSON 的 policy 3.0，改用子配置级 CMAKE_POLICY_VERSION_MINIMUM=3.5；新版 cppcheck 报 close 回包缓冲区 uninitvar，改为零初始化"
  source: "commits 82042b0 / 310c373；PR #485"
  affects: [runner-toolchain-drift]

- time: 2026-10-20T16:21:22
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #485 CI 诊断与发布门禁复核（2026-10-20）"
  affects: [runner-toolchain-drift]

- time: 2026-10-20T16:21:22
  kind: decision
  summary: "发布门禁补丁：v3.0.0 tag 源码早于 cJSON 修复，两条 release workflow 顶层注入 CMAKE_POLICY_VERSION_MINIMUM=3.5（env 会初始化嵌套 build tree；不改 tag）"
  source: "PR #485"
  affects: [runner-toolchain-drift]

- time: 2026-10-20T16:47:59
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "v3.0.0 tag 重打诊断（2026-10-20）"
  affects: [runner-toolchain-drift]

- time: 2026-10-20T16:47:59
  kind: reversal
  summary: "推翻「release 事件 workflow 取自默认分支」：实际取 tag 提交上的 workflow。实证：published 事件已发出但 tag 内触发器无效时零运行；探针 tag 含修复后立即触发。v3.0.0 因此重打到 8376f1b（含 cJSON/env 修复），两条门禁首次真实运行"
  source: "GitHub 事件文档 + 探针 tag gate-probe-20261020"
  affects: [runner-toolchain-drift, release-checklist-enforced, release-v300]
