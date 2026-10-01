---
id: release-v291-prep
title: "v2.9.1 发布准备：CI 门禁修复与死代码清理（patch）"
category: decision
status: active
tags: [release, v2.9.1, ci, quality, pre-release]
created: "2026-10-01T10:07:05"
updated: "2026-10-01T11:44:49"
---

<!-- compiled_truth -->
## 趋势数据落库：API 无法改设置，需手动走 web UI

尝试 `PATCH /repos/adam-ikari/uvhttp/actions/permissions/workflow` 设
`can_approve_pull_request_reviews=true`，**持续 404**：
- `gh api` 带与不带 `X-GitHub-Api-Version` header 都 404
- curl 直连（带 `repo` scope token）同样 404
- 同 endpoint 的 GET 正常返回（说明路径存在），但 PATCH 被拒写

**结论：该设置在 API 上对本仓库拒写，只能通过 web UI 改。**

web UI 路径：**Settings → Actions → General → Workflow permissions →
"Allow GitHub Actions to create and approve pull requests"**

### 已用手工方式验证 PR 路径可行

在设置改好之前，用维护者 token 手动开了 PR #423（`benchmark-trends` → main），
状态 `MERGEABLE`，已 squash 合并（main `a059bcd`）。

合并内容确认无误，是 v2.9.1 pre-release 的趋势数据：
```
docs/benchmark-trends/benchmark-2026-10-01.md                    (178 行)
docs/benchmark-trends/benchmark-2026-10-01.csv                   (60 行)
docs/benchmark-trends/benchmark-2026-10-01-head-paired.csv       (31 行)
docs/benchmark-trends/benchmark-2026-10-01-base-paired.csv       (31 行)
```

这证明「push 到分支 + 开 PR 合并」这条路径本身完全可行，缺的只是
Actions 自主开 PR 的权限。设置打开后，trend job 会自动完成这一步。

### 遗留

- `benchmark-trends` 分支仍存在（已与 main 内容相同）。下次 trend job 会
  force push 重建，无需手动清理
- v2.9.1 仍是 pre-release（v2.9.0 是 Latest）。benchmark 门禁已 ALL PASS，
  可 `gh release edit v2.9.1 --latest` 转正式


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

- time: 2026-10-01T11:19:46
  kind: decision
  summary: "趋势数据落库：API PATCH can_approve_pull_request_reviews 持续 404（curl 直连同样拒写，GET 正常），该设置只能走 web UI（Settings→Actions→General→Workflow permissions）；已用维护者 token 手动开 PR #423 验证路径可行并合并（main a059bcd，4 个趋势文件）。v2.9.1 仍 pre-release 待转正式"
  source: "趋势落库方案落地（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T11:44:49
  kind: reversal
  summary: "删除趋势数据落库功能（trend job + docs/benchmark-trends/ + benchmark-trends 分支）：用户决定不要趋势数据。两层限制（GH013 + Actions policy 不可 API 改）使修复链无尽头，且 trend 不参与门禁判定，删了对质量无影响"
  source: "删除趋势数据功能（2026-10-01）"
  affects: [release-v291-prep]
