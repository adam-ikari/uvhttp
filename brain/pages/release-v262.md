---
id: release-v262
title: "发布 v2.6.2"
category: decision
status: active
tags: [release, memory-safety, websocket]
created: "2026-08-17T16:10:13"
updated: "2026-08-17T16:10:52"
---

<!-- compiled_truth -->
# v2.6.2 发布记录

## 发布时间
2026-08-17（UTC 16:04 合并），VERSION_TYPE=patch

## 包含内容
- 连接上限 503 路径 use-after-free 修复：`on_connection` 中 `uv_accept` 失败时改用 `uv_close`（close 回调释放）而非直接 `uvhttp_free`
- `server->max_connections` 死状态修复：字段初始化为 `UVHTTP_MAX_CONNECTIONS_DEFAULT` 并作为无 config 时的权威值（此前初始化为 MAX 但从未被读取）
- WebSocket RFC 6455/内存安全修复（PR #336）：send-frame buffer 成功路径泄漏、build_frame double-free、fragmentation 状态机、message size 强制
- uv_strerror_r 一致性：server.c 中残留的 `uv_strerror` 改为 `uv_strerror_r`
- 测试泄漏修复：build_frame 错误路径测试释放 caller-owned buffer

## 门禁结果
- 100/100 ctest 通过
- verify-memory-safety（ASan + UBSan）PASS
- 文档构建通过（doxygen + api:generate + vitepress）
- CI：ubuntu-build / ubuntu-test-fast / asan-gate / format-check / code-quality-check / doc-sync-check 全部 pass

## 发布流程要点
- tag v2.6.2 已推送
- PR #347（release-2.6.2 → main）squash 合并，merge commit 43d3a80
- deploy-docs 工作流已成功部署
- 分支保护规则：main 只能通过 PR 合并（直接 push 被拒）
- 发布分支命名注意：不能用 `release/vX.Y.Z`（与已存在的 `release` 分支冲突），用 `release-X.Y.Z`

## 待处理
- PR #335（fix/ws-tls-large-frames）与 main 冲突（connection.c / websocket.c），需 rebase 解决
- 10 个 dependabot PR（#337-#346）CI 全绿但未合并


## Timeline

- time: 2026-08-17T16:10:13
  kind: decision
  summary: "Created this page: 发布 v2.6.2"
  source: created via brain create-page
  affects: [release-v262]

- time: 2026-08-17T16:10:52
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: brain update-truth
  affects: [release-v262]
