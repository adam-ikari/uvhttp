---
id: docs-vitepress-ssr-event-build-fail
title: "VitePress 文档构建失败：Vue SSR reading 'event'（CI 连续 10 次）"
category: project
status: active
tags: [docs, ci, vitepress]
created: "2026-10-02T10:31:34"
updated: "2026-10-02T15:02:34"
---

<!-- compiled_truth -->
# VitePress 文档构建失败（已修复）

## 状态

**已修复** — PR #446（2026-10-02）。`npm run docs:build` 成功。

## 根因

v2.9.1 发布准备（#421）在 CHANGELOG 里写了 GitHub Actions 表达式
`${{ github.event.before }}`。VitePress 把 Markdown 编译成 Vue 模板，
**行内代码中的 `{{ ... }}` 不被转义**，被 Vue 当模板插值求值——
`github` 未定义 → 读取 `.event` 报错：

```
TypeError: Cannot read properties of undefined (reading 'event')
    at _sfc_ssrRender (.vitepress/.temp/guide_CHANGELOG.md.js)
```

`deploy-docs.yml` 自 v2.9.1 起连续失败 10 次。

## 关键区分

**围栏代码块内的 `{{ }}` 会被 markdown-it 转义，Vue 不插值** ——
`dev/CI_CD_DESIGN.md`、`dev/DEVELOPMENT_PLAN.md` 等文件里大量
`${{ github.* }}` 写法是安全的。

**只有行内代码（单反引号）会触发。**

## 排查方法（下次遇到同类问题照此做）

1. `gh run list --workflow=deploy-docs.yml` 找最后一次成功与第一次
   失败的 headSha——比直接猜模板片段可靠得多
2. 在主仓库 `git checkout <sha>` 逐提交构建，**每个提交前必须清空**
   `.vitepress/cache` `.vitepress/dist` `.vitepress/.temp`
3. 二分结果定位到 `970146b`（该区间唯一改动 `docs/` 的提交）

之前卡住的「报错栈指向 CHANGELOG 但内容换成极简仍失败」是**缓存未清**
造成的假象，不是线索误导。

## 修复方案

✅ `<span v-pre>`\`\`${{ ... }}\`\`\`</span> —— `v-pre` 让 Vue 跳过该元素的
模板编译，markdown-it 仍正常把反引号解析为 `<code>`。渲染输出：

    <span><code>${{ github.event.before }}</code></span>

❌ `&#123;&#123;` 实体 —— markdown-it 会二次转义成 `&amp;#123;`，
页面显示字面量而非 `{{`。

## 教训

文档里写 GitHub Actions 表达式时，行内代码中的 `${{ }}` 必须用
`v-pre` 包裹；围栏代码块内则安全。


## Timeline

- time: 2026-10-02T10:31:34
  kind: decision
  summary: "Created this page: VitePress 文档构建失败：Vue SSR reading 'event'（CI 连续 10 次）"
  source: "v2.9.2 发布前排查 2026-10-02"
  affects: [docs-vitepress-ssr-event-build-fail]

- time: 2026-10-02T10:31:59
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "v2.9.2 发布前排查 2026-10-02"
  affects: [docs-vitepress-ssr-event-build-fail]

- time: 2026-10-02T15:02:00
  kind: reversal
  summary: "文档构建失败已修复（#446）：根因是 CHANGELOG 行内代码中的 ${{ }} 被 Vue 当模板插值求值，github 未定义致读取 .event 报错；用 <span v-pre> 包裹修复。二分定位：逐提交构建，970146b（v2.9.1 发布准备）是引入点。围栏代码块内的 {{ }} 安全，只有行内代码会触发。"
  source: "v2.9.2 发布后修复，PR #446"
  affects: [docs-vitepress-ssr-event-build-fail]

- time: 2026-10-02T15:02:34
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "修复后更新 2026-10-02，PR #446"
  affects: [docs-vitepress-ssr-event-build-fail]
