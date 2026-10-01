---
id: release-v291-prep
title: "v2.9.1 发布准备：CI 门禁修复与死代码清理（patch）"
category: decision
status: active
tags: [release, v2.9.1, ci, quality, pre-release]
created: "2026-10-01T10:07:05"
updated: "2026-10-01T10:55:33"
---

<!-- compiled_truth -->
# v2.9.1 预发布已创建 + trend job 端到端验证结果

## 发布状态

- tag `v2.9.1` → `55a79ac`（main `970146b`）
- GitHub Release v2.9.1 = **pre-release**（未转 latest，v2.9.0 仍是 Latest）
- release 事件自动触发 ci-benchmark `36848815265`（head v2.9.1，base 自动解析为 v2.9.0）

### benchmark 门禁：ALL PASS
| 端点 | head | base | ratio | pairs<lim | MAD | verdict |
|---|---|---|---|---|---|---|
| / | 78155 | 79023 | 96.3% | 1/10 | 6.1% | PASS |
| /json | 77264 | 79525 | 98.4% | 3/10 | 8.8% | PASS |
| /large | 10072 | 10192 | 98.8% | 0/10 | 3.3% | PASS |

全远高于 90% 阈值，差异在 MAD 内（1–3 个 MAD），无回归。v2.9.0..v2.9.1 唯一的 src 改动是删死代码与冗余 NULL 检查（零性能影响），结果符合预期。

## trend job 端到端验证：GH013 已修复，但暴露第二层限制

#418 的核心修复**验证通过**：
- `git push --force origin HEAD:benchmark-trends` 成功（分支 `benchmark-trends` 存在，ahead_by 1，commit `docs(benchmark): trend data for 2026-10-01`）
- 旧的 `GH013 Repository rule violations for refs/heads/main` 不再出现

但 `gh pr create` 失败：
```
pull request create failed: GraphQL: GitHub Actions is not permitted to
create or approve pull requests (createPullRequest)
```

**这是仓库级 Actions policy 限制**，不是 workflow 权限问题：
`gh api repos/adam-ikari/uvhttp/actions/permissions/workflow` 返回
`can_approve_pull_request_reviews: False`。我给 trend job 加的
`pull-requests: write` 只控制 GITHUB_TOKEN 的 scope，而 GitHub 仓库设置里
「Allow GitHub Actions to create and approve pull requests」是独立的开关。

## 待决策：趋势数据如何落库

当前状态：趋势数据堆在 `benchmark-trends` 分支（1 个 commit），永不合并到 main。

三个选项：
1. **启用仓库设置**允许 Actions 创建 PR（admin 权限可改）——标准做法，但扩大 Actions 权限
2. **保持现状**：趋势数据只在分支，需手动定期合并
3. **改用 artifact / gh-pages 落趋势数据**，不经过 main

选项 1 最符合「PR-only + 自动落库」的设计意图（见 [[release-process-benchmark-gate]]）。

## v2.9.1 是否转正式？
两阶段流程要求 benchmark 门禁绿 → `gh release edit v2.9.1 --latest`。门禁已 ALL PASS，可转。但转正式前建议先定 trend job 方案，否则每次 pre-release 都会留一个未合并分支。


## Timeline

- time: 2026-10-01T10:07:05
  kind: decision
  summary: "Created this page: v2.9.1 发布准备：CI 门禁修复与死代码清理（patch）"
  source: "v2.9.1 发布准备（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T10:07:06
  kind: decision
  summary: "v2.9.1 发布准备：VERSION 2.9.0→2.9.1 patch + CHANGELOG EN/ZH + release-strategy 已更新，PR/tag/prerelease 未创建。内容全是质量修复（#416-#420）。trend job 端到端验证是主要目的"
  source: "v2.9.1 发布准备（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T10:55:33
  kind: decision
  summary: "v2.9.1 prerelease 已发布（tag 55a79ac，pre-release 未转 latest）；benchmark 门禁 ALL PASS（96.3/98.4/98.8%，差异在 MAD 内无回归）；trend job 端到端验证：push 到 benchmark-trends 成功（GH013 已修），但 gh pr create 被仓库 Actions policy 拒（can_approve_pull_request_reviews: False），趋势数据堆在分支待决策"
  source: "v2.9.1 pre-release + trend job 验证（2026-10-01）"
  affects: [release-v291-prep]
