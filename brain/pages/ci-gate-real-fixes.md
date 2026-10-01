---
id: ci-gate-real-fixes
title: "CI 门禁修复：format-check 空转 + clang-format 版本漂移 + trend 直推 main"
category: decision
status: active
tags: [ci, gate, format, clang-format, cppcheck, dead-code, trend]
created: "2026-10-01T06:57:34"
updated: "2026-10-01T06:57:34"
---

<!-- compiled_truth -->
# CI 门禁修复（#416/#417/#418，2026-10-01）

「确保生产级质量」盘点出的三个真实质量缺口，全部已修。

## format-check 门禁此前完全不工作（#416）

`ci-pr.yml` 用 `${{ github.event.before }}` 作 git diff 的 base，但 pull_request 事件 payload 没有 before 字段 → 展开空串 → `git diff --name-only --diff-filter=ACMR "" <sha>` 报 `fatal: ambiguous argument ''` → files 为空 → 恒走 "No C/C++ files changed — skipping" exit 0。

**任何 C/C++ 变更都未被格式检查**。PR #380 引入该 job 时写错 base，此后所有 PR 的 format-check 都是空转绿灯。改用 `${{ github.event.pull_request.base.sha }}`。

## 更深一层：门禁用浮动 clang-format 版本（#416）

修好 base 后门禁**立刻在同一 PR 抓出 8 处违规**——暴露了真正的根因：门禁装 `apt` 的 clang-format（跟随 runner 镜像，ubuntu-24.04 是 18.x），格式化时用的是本地 **14**。

实测同一份代码：**v14 判 0 违规，v18 判 424 处**。跨大版本输出差异极大（宏续行空格对齐、`#    define` 缩进处理）。门禁用浮动版本 = 任何人本地过、CI 红，且无法复现。

- 门禁改 `pip install clang-format==18.1.8`
- 全量用 18.1.8 重格式化（424 处，30 文件）
- CONTRIBUTING 记录版本对齐要求

CONTRIBUTING 此前已记过这个坑（「CI 用 18，本地可能是 14 而无法复现」），但只记在 `.clang-format` 重复键那条，没推广到 format 门禁本身。

## cppcheck --std=c99 发现的死代码（#417）

`chunked_transfer_context_t`（uvhttp_static.c）**整个结构体从未被实例化**——不是字段死，是类型死。实际分块传输用同名局部变量/参数。删掉整个 typedef + 注释。

同批被 cppcheck 报的 gzip_cache 10 条、tls 2 条 `unusedStructMember` 是**误报**（字段通过 `cache->xxx` 指针访问，引用数 2-21 全部在用），不改。

非缺陷输出（记录避免重复排查）：
- `response.c:633 duplicateAssignExpression`：`body_length` 与 `original_body_length` 都赋 `response->body_length` 是故意的（original 保存原始值用于压缩对比），见 [[zerocopy-small-body-regression]]
- `server.c:146 unknownMacro UVHTTP_STRINGIFY`：cppcheck 配置解析产物，门禁用 `-D` 规避

## trend job 直推 main 被 GH013 拒绝（#418）

v2.9.0 pre-release 首次真正触发 trend job 即失败：`git push origin HEAD:main` 被 main 的 PR-only 分支保护拒（`GH013: Repository rule violations for refs/heads/main`）。#394 修过竞态重试但没解决「直推 main vs PR-only」的根本冲突。

修：改推专用分支 `benchmark-trends`（force，concurrency group 串行化保证安全）+ 自动开 PR，permissions 加 `pull-requests: write`。

## 教训

**门禁修复要连带验证「门禁本身是否可复现」。** 修 base 引用只做了一半——它立刻抓出违规，但根因是版本漂移，不修版本的话下次 CI 换个镜像版本又会漂。真正的生产级门禁要求：固定工具版本 + 本地能 100% 复现 CI 判定。


## Timeline

- time: 2026-10-01T06:57:34
  kind: decision
  summary: "Created this page: CI 门禁修复：format-check 空转 + clang-format 版本漂移 + trend 直推 main"
  source: "确保生产级质量（2026-10-01）"
  affects: [ci-gate-real-fixes]

- time: 2026-10-01T06:57:34
  kind: decision
  summary: "三个真实门禁缺口已修：format-check base 引用空转（原门禁完全不工作）、clang-format 版本漂移（v14 判0/v18判424，改为钉 18.1.8）、trend 直推 main 被 GH013 拒（改推分支+PR）；另删死代码 chunked_transfer_context_t"
  source: "确保生产级质量（2026-10-01）"
  affects: [ci-gate-real-fixes]
