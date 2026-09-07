---
id: release-process-benchmark-gate
title: "发布流程两阶段化：pre-release 门禁 + 正式确认，main PR-only"
category: decision
status: active
tags: [release, ci, benchmark]
created: "2026-09-07T07:13:20"
updated: "2026-09-07T07:13:40"
---

<!-- compiled_truth -->
## 决定

1. **发布流程两阶段化**：
   - 阶段一（预发布）：全面测试（make test / ASan / UBSan）→ 更新 VERSION + CHANGELOG → 创建 tag → `gh release create vX.Y.Z --prerelease`，自动触发 ci-benchmark 回归门禁（10% RPS 阈值 vs 基线），绿才可转正式
   - 阶段二（正式发布）：确认 benchmark 门禁绿 → `gh release edit vX.Y.Z --latest`（移除 prerelease 标记）→ main 经 PR 合并触发文档部署

2. **benchmark 回归门禁绑定 release 事件**：ci-benchmark.yml 的 `push: branches: [pre-release]` 是死配置（项目从不创建 pre-release 分支），改为 `release: types: [published]`。正式 release（prerelease=false）重复跑 gate 无害——同一 commit 结果一致（幂等）；趋势数据只在 pre-release 落库（`github.event.release.prerelease == true`）。

3. **main PR-only**：所有 main 变更（含发布 commit）必须走 PR 合并，禁止直推。deploy-docs.yml 删除 `release/**` 死路径（项目从不推 release 分支），文档部署只依赖 main push。

## 替代方案（否决）

- deploy-docs.yml 保留 release 分支意图改用 `tags: ['v*']`：保守起见不采用，与现有实际流程（main push 部署文档）保持一致
- 维持 pre-release 分支触发：与真实发布流程（tag + GitHub Release）脱节，门禁从未在实际路径生效

## 影响范围

- `.github/workflows/ci-benchmark.yml`：触发器 + job if + gate if + 趋势落库 if
- `.github/workflows/deploy-docs.yml`：删除 `release/**` 分支触发
- `docs/release-strategy.md`：两阶段发布流程 + PR-only + 检查清单加 benchmark 门禁项
- 关联 [[perf-regression-gate]]（触发条件从 PR + pre-release push 变为 PR 标签 + release 事件）


## Timeline

- time: 2026-09-07T07:13:20
  kind: decision
  summary: "Created this page: 发布流程两阶段化：pre-release 门禁 + 正式确认，main PR-only"
  source: "CI/发布配置与真实流程脱节修复"
  affects: [release-process-benchmark-gate]

- time: 2026-09-07T07:13:40
  kind: decision
  summary: "发布流程改两阶段（pre-release 门禁 + 正式确认），benchmark 回归门禁绑定 release 事件，main PR-only"
  source: "CI/发布配置与真实流程脱节修复"
  affects: [release-process-benchmark-gate]
