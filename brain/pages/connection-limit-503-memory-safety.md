---
id: connection-limit-503-memory-safety
title: "连接上限 503 临时客户端的生命周期与 max_connections 权威值"
category: decision
status: active
tags: [memory-safety, server, connection-limit]
created: "2026-08-17T14:17:54"
updated: "2026-08-17T14:18:06"
---

<!-- compiled_truth -->
# 结论

## 背景
代码审查发现 `on_connection`（src/uvhttp_server.c）中连接数达到上限（503 路径）时存在内存安全问题与误导性状态。

## 问题 1：uv_accept 失败路径直接 free 已初始化的 libuv 句柄
- 原代码：`uv_accept` 失败后直接 `uvhttp_free(temp_client)`。
- 风险：`temp_client` 已经过 `uv_tcp_init` 注册到 libuv 事件循环的 handle queue。直接释放内存会在队列里留下悬垂指针，下一次 `uv_run`/`uv_loop_close` 遍历句柄时触发 use-after-free。
- 修复：改用 `uv_close((uv_handle_t*)temp_client, (uv_close_cb)uvhttp_free)`，先让 libuv 把句柄移出队列，再由 close 回调释放。与同函数其他分支的清理方式保持一致。
- 注意：`write_503_response_cb` 中 `uv_is_closing` 为真时直接 `uvhttp_free` 是安全的（句柄已进入关闭流程）；`uv_tcp_init` 失败时直接 free 也是安全的（句柄未注册）。

## 问题 2：server->max_connections 字段是死的/误导性状态
- 原代码：`uvhttp_server_new` 把它初始化为 `UVHTTP_MAX_CONNECTIONS_MAX`(10000)，但 `on_connection` 从未读取该字段，实际限制来自 `config->max_connections`（默认 2048）或硬编码默认值。字段声称 10000 而实际执行 2048，属误导性死状态，违背“极简工程/零死状态”设计哲学。
- 修复：改为初始化为 `UVHTTP_MAX_CONNECTIONS_DEFAULT`(2048)，并让 `on_connection` 在无 config 时以 `server->max_connections` 作为权威值。行为与之前一致（默认 2048），但字段现在是真实的、可用的。

## 测试
- `test_connection_libuv_fail.cpp` 新增两个回归测试：
  - `ConnectionLimitAcceptFailClosesTempClient`：uv_accept 失败时 temp_client 必须经 uv_close 关闭（而非直接 free）。
  - `ServerMaxConnectionsFieldIsAuthoritative`：无 config 时 on_connection 必须遵从 server->max_connections 字段（置 0 应触发 503 路径）。
- 全量 100 个 ctest 全部通过，无回归。

## 设计原则（供后续审查参考）
- libuv 句柄生命周期：凡经 `uv_tcp_init`/`uv_*_init` 注册的句柄，释放前必须 `uv_close`，不能直接 free。
- 公共结构体字段要么是权威真源、要么不存在；不要初始化成与真实行为不一致的值。


## Timeline

- time: 2026-08-17T14:17:54
  kind: decision
  summary: "Created this page: 连接上限 503 临时客户端的生命周期与 max_connections 权威值"
  source: created via brain create-page
  affects: [connection-limit-503-memory-safety]

- time: 2026-08-17T14:18:06
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: brain update-truth
  affects: [connection-limit-503-memory-safety]
