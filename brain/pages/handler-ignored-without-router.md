---
id: handler-ignored-without-router
title: "server->handler 只在挂了 router 时才跑，与头文件注释相反"
category: decision
status: active
tags: [api, defect, dispatch]
created: "2026-10-18T06:31:17"
updated: "2026-10-28T14:54:14"
---

<!-- compiled_truth -->
# `server->handler` 在无 router 时已真正执行（2026-10-28 修复）

## 当前事实

`uvhttp_server_set_handler` 的语义恢复为头文件注释所写：
「no router attached 时使用的请求处理器」。

分派路径现在有三层，且三层都可达：

1. 有 router 且命中 → router handler；
2. 有 router 但未命中（含 static miss 兜底）→ `server->handler`（若设置）；
3. **无 router → `server->handler`（若设置）**，否则库的默认 200 "OK"。

第 3 层是本次补上的（`src/uvhttp_request.c` 的 no-router `else` 分支）。
之前该分支从不读 `server->handler`，只发硬编码 200 "OK"，调用方装的回调
一次都不执行且没有任何信号。

## 回归测试

`test/unit/test_server_request_filter.cpp` 新增
`HandlerInvokedWithoutRouter`：只设 handler、不挂 router，发一个真实 HTTP
请求，断言 200、响应体来自 handler、且 `g_handler_calls == 1`。
断言计数而不是响应体 —— 默认 200 "OK" 无法蒙过计数。
该用例在修复前为红、修复后为绿（已做 mutation 验证）。

## 判据（保留）

- 文档与实现冲突时，以 API 的存在意义为准：这个 API 就是给「不挂 router」
  的用户用的，因此修实现而不是改注释。
- 分派接缝的测试必须断言「用户代码被调用了 N 次」，不能只断言响应状态/内容。

关联 [[request-filter-seam-replaces-rate-limiter]]、
[[middleware-stop-does-not-short-circuit-handler]]。


## Timeline

- time: 2026-10-18T06:31:17
  kind: decision
  summary: "Created this page: server->handler 只在挂了 router 时才跑，与头文件注释相反"
  source: "2026-10-18 d3 新增 test_server_request_filter.cpp 时 5 个用例转红定位"
  affects: [handler-ignored-without-router]

- time: 2026-10-18T06:33:13
  kind: decision
  summary: "写入分派死路证据、发现过程与修法判据"
  source: "2026-10-18 新增 test_server_request_filter.cpp 时定位"
  affects: [handler-ignored-without-router]

- time: 2026-10-28T14:54:14
  kind: decision
  summary: "no-router 分支已调用 server->handler 并加回归测试；缺陷修复"
  source: "2026-10-28 机制审计：src/uvhttp_request.c 分派 else 分支补 handler 调用，test_server_request_filter.cpp 新增 HandlerInvokedWithoutRouter"
  affects: [handler-ignored-without-router]
