---
id: tls-session-cache
title: "TLS 会话缓存重新启用"
category: decision
status: active
tags: [tls, performance, session-cache]
created: "2026-08-21T03:34:43"
updated: "2026-09-29T01:22:29"
---

<!-- compiled_truth -->
## 决策（v2.7.0，commit aecb053，P0）
mbedtls 的 TLS 会话缓存此前被禁用（每次握手都做完整非对称握手）。重新启用：

- `uvhttp_tls_context_new()` 调 `mbedtls_ssl_conf_session_cache()`，默认 **2048 条目 / 86400s（24h）超时**
- `uvhttp_tls_context_enable_session_tickets()` 一并重新启用
- 线程安全前提：libuv 单线程事件循环，无需额外加锁（若将来引入多线程 loop，这里是要重看的点）

## 定位
会话缓存是"重复连接"路径的优化，对单次握手无收益。基准测试里体现为 HTTPS 短连接重连场景的握手开销下降，不属于 `/`、`/json` 这类小响应 RPS 的主导因素。

## 配置面
默认值可经 TLS 上下文 API 调整；对应文档见 `docs/spec/tls-api.md` 与性能文档的 TLS 小节。


## Timeline

- time: 2026-08-21T03:34:43
  kind: decision
  summary: "Created this page: TLS 会话缓存重新启用"
  source: TLS session cache re-enablement
  affects: [tls-session-cache]

- time: 2026-09-29T01:22:29
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）回填 — 依据 commit aecb053 / cf1d456 / 仓库现状"
  affects: [tls-session-cache]
