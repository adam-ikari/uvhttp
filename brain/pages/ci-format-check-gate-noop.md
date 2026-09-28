---
id: ci-format-check-gate-noop
title: "format-check 门禁在 pull_request 下空转：event.before 为空导致恒跳过"
category: decision
status: active
tags: [ci, gates, format]
created: "2026-09-23T04:16:34"
updated: "2026-09-23T04:16:34"
---

<!-- compiled_truth -->
## 结论

`ci-pr.yml` 的 `format-check` job 当前**不构成任何约束**：它用 `${{ github.event.before }}` 作为 `git diff` 的 base，而 `pull_request` 事件 payload 里没有 `before` 字段 → 展开为空串 → `git diff --name-only --diff-filter=ACMR "" <sha>` 直接 `fatal: ambiguous argument ''` → `files` 为空 → 恒定走 "No C/C++ files changed — skipping" 分支 exit 0。

实证：PR #380（fix/src-round2）改动了多个 `src/*.c`，format-check 仍 48s pass；而 `origin/main` 上 `src/uvhttp_version.c` 单文件就有 25 处 clang-format 违规（`--dry-run --Werror`）。

## 影响

- 任何 C/C++ 变更都不会被格式门禁检查——门禁是绿色的假象
- 现存代码有大量格式漂移（`ColumnLimit: 80`、include 重排等），**若直接修好该 job，CI 会立刻被存量漂移卡红**

## 修复路径（未排期）

1. 先全量 `clang-format -i` 清理存量（独立 PR，diff 大但纯机械）
2. 再把 job 的 base 改为 `github.event.pull_request.base.sha`（pull_request 事件的正确 base）
3. 或改用 `git clang-format --diff` 只检查变更行，避免存量阻塞

## 时序

- 2026-09-12 PR #376 把 format-check 从占位改成"真实实现"（只查变更文件），但 base 引用写错，实际仍为空转
- 2026-09-23 v2.8.0 发布会话核实 format-check 行为时发现（当时正评估改动 `src/uvhttp_version.c` 是否会被卡）

关联 [[release-process-benchmark-gate]]


## Timeline

- time: 2026-09-23T04:16:34
  kind: decision
  summary: "Created this page: format-check 门禁在 pull_request 下空转：event.before 为空导致恒跳过"
  source: "v2.8.0 发布会话"
  affects: [ci-format-check-gate-noop]

- time: 2026-09-23T04:16:34
  kind: decision
  summary: "format-check 空转根因、存量格式漂移证据与三步修复路径"
  source: "v2.8.0 发布会话"
  affects: [ci-format-check-gate-noop]
