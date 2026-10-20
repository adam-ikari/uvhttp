---
id: release-checklist-enforced
title: "发布检查清单：从靠人读到脚本门禁（scripts/ci/release_checklist.sh）"
category: decision
status: active
tags: [release, ci, quality]
created: "2026-10-04T14:24:33"
updated: "2026-10-19T14:07:50"
---

<!-- compiled_truth -->
# 发布检查清单：从靠人读到脚本门禁

## 结论

`docs/release-strategy.md` 的检查清单原是纯 Markdown，发布靠人记忆。
现已由 `scripts/ci/release_checklist.sh` 两阶段强制，退出码 1 即卡住。

## 它能抓什么（实证）

**变异验证**：把 VERSION 改成 2.9.3（未发布版本）→ `--post` 立刻抓到
4 项未过（EN/ZH CHANGELOG 缺条目、release-strategy 缺行、远端无 tag），
退出码 1。

## 现实中卡过什么

- v2.9.1 / v2.9.2 发布时 `deploy-docs` 连续失败 10 次，**没人拦**
- v2.9.2 发布后才发现线上版本徽章停在 2.0.0（见 #451）——VERSION 与
  线上 `versions.json` 漂了数月，无人察觉

## 两阶段划分

|阶段|何时|检查项|
|---|---|---|
|`--pre`|合并版本 PR 前|VERSION/EN/ZH CHANGELOG/release-strategy 一致、ctest、docs:build、本次改动 clang-format、test_version_consistency|
|`--post`|转正式 latest 后|同上 + 远端 tag、Release isLatest、线上 versions.json 的 current 与 VERSION 一致|

分两阶段是因为 tag / release / 线上站点只能在发布动作完成后查。

## 设计取舍（为什么不都放进去）

- **静态检查只查本次改动**（`git diff main...HEAD`）——CI 的
  `code-quality-check` 已做全量。main 上仍残留 #416 修复前空转期进入的
  既有格式违规（如 `test_websocket_api_coverage.cpp`），全量查会报红但
  与本次发布无关，会让人学会忽略门禁。
- **ASan/UBSan 不入脚本**——`make verify-memory-safety` 单跑约 9 分钟，
  放脚本里等于逼人跳过；它已是 PR 的 `asan-gate` 门禁。
- **benchmark 门禁不重复**——由 release 事件触发的 CI 自动判定。

## 踩过的两个坑

1. **`gh release view --json isLatest` 报 Unknown JSON field**——本机 gh
   版本 `view` 不支持 `isLatest`，但 `gh release list --json isLatest` 支
   持。tag 检查同理：改用 `gh release list` 统一处理，不要用
   `gh api repos/:owner/::repo/...`（`:owner:` 占位符语法不成立）。
2. **`$?` 在管道后取到的是 tail 的退出码**——`script | tail` 后的
   `echo $?` 不是脚本的。要用 `${PIPESTATUS[0]}`。


## Timeline

- time: 2026-10-04T14:24:33
  kind: decision
  summary: "Created this page: 发布检查清单：从靠人读到脚本门禁（scripts/ci/release_checklist.sh）"
  source: "2026-10-04 PR #452"
  affects: [release-checklist-enforced]

- time: 2026-10-04T14:24:33
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #452"
  affects: [release-checklist-enforced]

- time: 2026-10-19T14:07:50
  kind: decision
  summary: "门禁的覆盖面缺口（v3.0.0 准备时暴露）：--pre 只查 CHANGELOG 里有没有 ## [版本号] 段落、release-strategy 里有没有对应历史行，不查这一段是否覆盖了整个未发布窗口。本次 [Unreleased] 只剩最后一个 PR（#483）的条目，照它成文就会漏掉 2.9.2 之后 34 个 first-parent 合并里带 src/ 改动的 #449（prewarm 每文件漏一次缓冲）、#457（空 header 名开始被拒，行为收紧）、#464（手工 Content-Length + 压缩分帧错位，数据损坏级）。打 tag 前的固定动作：git log --first-parent <上个tag>..main 与 changelog 段落逐条对，尤其筛 git diff --name-only c^1 c 命中 src/ 或 include/ 的合并——merge commit 要用 c^1 对比，git show --name-only 对合并提交输出为空，据此判断会漏。"
  source: "v3.0.0 发布评审（2026-10-19）"
  affects: [release-checklist-enforced]
