---
id: release-v291-prep
title: "v2.9.1 发布准备：CI 门禁修复与死代码清理（patch）"
category: decision
status: active
tags: [release, v2.9.1, ci, quality, pre-release]
created: "2026-10-01T10:07:05"
updated: "2026-10-01T10:07:06"
---

<!-- compiled_truth -->
# v2.9.1 发布准备进展（2026-10-01）

## 状态：VERSION + CHANGELOG（EN/ZH）已更新，tag 与 prerelease 未创建

### v2.9.0 已发布（Latest）
tag `v2.9.0` → `e7764af`，发布于 2026-10-01。详见 [[release-v290]]。

### v2.9.1 内容（v2.9.0 后 6 个提交）

全部是质量修复，无新功能——所以是 **patch** 而非 minor：

| PR | 内容 |
|---|---|
| #416 | format-check 门禁 base 引用修复（原门禁空转）+ clang-format 钉 18.1.8 + 全量重格式化 424 处 |
| #417 | 删除死代码 `chunked_transfer_context_t` + 清理 websocket 冗余 NULL 检查 |
| #418 | trend job 改推专用分支 + 开 PR（修 GH013） |
| #419 | brain 记录 CI 门禁修复 |
| #420 | ci-pr / ci-daily GITHUB_TOKEN 权限最小化 |

改动统计：38 文件、694 插入 / 464 删除（其中 30 文件、483 行是 clang-format 机械重排）。

### 已完成的发布准备
- `VERSION`：2.9.0 → **2.9.1**，`VERSION_TYPE=patch`，`PREVIOUS_VERSION=2.9.0`，`NEXT_VERSION=2.9.2`，`VERSION_NAME="CI 门禁修复与死代码清理"`
- `docs/guide/CHANGELOG.md` + `docs/zh/guide/CHANGELOG.md`：新增 `[2.9.1]` 条目（Fixed 3 项 / Security 1 项 / Changed 1 项）+ compare 链接
- `docs/release-strategy.md`：版本历史表加 v2.9.1 行
- `check-doc-sync.sh --check` → 31/31 一致
- `check-links.sh` → All internal links valid

### 未完成（下一步）
- PR 未创建、未合并
- tag `v2.9.1` 未打
- `gh release create v2.9.1 --prerelease` 未执行
- **trend job 端到端验证未做** — #418 改推分支+开 PR 的实际行为，要等这个 prerelease 触发 ci-benchmark 才验证

### 质量门禁状态（main `67e7d94` 之前）
ctest 102/102、ASan、UBSan、build-matrix 全配置、cppcheck（c99）零告警、clang-format 18.1.8 零违规、ci-fuzz 四 harness success、benchmark vs v2.8.1 ALL PASS。

### trend job 验证是这次 prerelease 的主要目的
#418 修了 GH013 但**未做过端到端验证**（只在本地验证了 YAML/shell 语法和 push 到非 main 分支可行）。发布 prerelease 会触发 ci-benchmark 的 `trend` job，届时可确认：
1. push 到 `benchmark-trends` 成功
2. `gh pr create` 自动开 PR 成功（或至少不致命失败）


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
