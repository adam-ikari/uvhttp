---
id: docs-vitepress-ssr-event-build-fail
title: "VitePress 文档构建失败：Vue SSR reading 'event'（CI 连续 10 次）"
category: project
status: active
tags: [docs, ci, vitepress]
created: "2026-10-02T10:31:34"
updated: "2026-10-02T10:31:59"
---

<!-- compiled_truth -->
# VitePress 文档构建失败

## 结论

`npm run docs:build` 在 main 上持续失败，`deploy-docs.yml` 最近 12 次运行
失败 10 次。**与 v2.9.2 发布内容无关**——最早失败发生在 #433（ROADMAP 重写），
早于本轮全部测试 PR。

## 现象

```
TypeError: Cannot read properties of undefined (reading 'event')
    at _sfc_ssrRender (.vitepress/.temp/guide_CHANGELOG.md.js:7:7496)
    at renderComponentSubTree (@vue/server-renderer)
```

CI（Node 18）与本地（Node 22 / Node 24）报同一个错。

## 已排除的原因

1. **不是 CHANGELOG 内容**：把 `docs/guide/CHANGELOG.md` 换成 3 行极简内容、
   清空 `.vitepress/cache` `.vitepress/dist` `.vitepress/.temp` 后仍失败。
2. **不是 Node 版本**：Node 18（CI）/ 22 / 24 均失败。
3. **不是本次发布改动**：`git stash push docs/` 移除全部 docs 改动后仍失败。
4. **报错栈指向 CHANGELOG 具误导性**：该页内容换成极简后仍报同一栈。

## 可疑点（未验证）

`docs/.vitepress/components/VersionSelect.vue` 的
`handleVersionChange(event: Event)` 里访问 `event.target`。SSR 阶段事件对象
为 undefined。需确认该组件是否被 CHANGELOG 页面的 layout 引入。

## 排查困难点

VitePress 构建结束会删除 `.vitepress/.temp/`，无法在构建后读编译产物定位
`7:7496` 对应的模板片段。可行做法：构建过程中并发抓取该文件，或临时改
VitePress 配置保留 temp 目录。

## 影响

文档站点无法自动部署。不阻塞 GitHub Release 本身。


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
