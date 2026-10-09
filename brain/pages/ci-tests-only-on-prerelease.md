---
id: ci-tests-only-on-prerelease
title: "CI 测试只在 pre-release 跑（PR 只编译+质量）"
category: decision
status: active
tags: [ci, workflow, testing, release]
created: "2026-10-06T02:39:14"
updated: "2026-10-06T02:39:24"
---

<!-- compiled_truth -->
# CI 测试只在 pre-release 跑

## 变更

按用户要求（避免 CI 频繁跑测试浪费资源），把完整测试从 PR 阶段移到 pre-release。

**改前**：每个 PR 跑 ubuntu-build + code-quality + ubuntu-test-fast +
asan-gate + build-matrix（含 4 配置 fast 测试）——约 5-10 分钟/次。

**改后**：
- `ci-pr.yml`（PR 触发，只编译+质量，**不跑测试**）：
  ubuntu-build、code-quality-check（cppcheck + 恒绿测试门禁）、
  doc-sync-check、build-matrix（**只编译各配置，不跑测试**）、generate-summary
- `ci-release-gate.yml`（`release: [prerelease]` + 手动触发，**完整测试**）：
  ubuntu-build + upload artifact、ubuntu-test-fast（fast 测试）、
  build-matrix（各配置编译 + 测试）、asan-gate（ASan 内存安全）、generate-summary

发布流程（docs/release-strategy.md）同步更新：`gh release create --prerelease`
自动触发测试门禁 + benchmark 门禁，两条全绿才可转正式。

## 为什么 PR 保留 build-matrix 编译（而非完全跳过）

compile-time feature 裁剪是高危面——各 feature 有独立 `#if` 路径，裁剪后才发现
编译错已太晚。实例：PR #464 的 unused-function 只在 `COMPRESSION=OFF` 暴露
（helper 定义在 `#if UVHTTP_FEATURE_COMPRESSION` 外）。**编译验证 ≠ 测试**：
裁剪配置只编译（快），测试集中在 pre-release。

## 取舍

代价：PR 阶段无测试保护，回归要到 pre-release 才暴露。这换取 CI 资源与速度。
发布前本地仍需 `make test` + `make verify-memory-safety`（检查清单已注明）。


## Timeline

- time: 2026-10-06T02:39:14
  kind: decision
  summary: "Created this page: CI 测试只在 pre-release 跑（PR 只编译+质量）"
  source: "2026-10-06 调整"
  affects: [ci-tests-only-on-prerelease]

- time: 2026-10-06T02:39:24
  kind: decision
  summary: "PR 只跑编译+质量+裁剪编译；完整测试矩阵（fast/各配置/ASan）移到 ci-release-gate.yml，仅 pre-release 触发"
  source: "2026-10-06 调整"
  affects: [ci-tests-only-on-prerelease]
