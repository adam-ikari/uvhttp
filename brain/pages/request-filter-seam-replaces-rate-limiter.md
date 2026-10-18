---
id: request-filter-seam-replaces-rate-limiter
title: "库不实现限流：只留分派前 request filter 接缝，策略归调用方"
category: decision
status: active
tags: [api, design, v2.10, rate-limit, roadmap-d3]
created: "2026-10-18T06:30:34"
updated: "2026-10-18T06:33:13"
---

<!-- compiled_truth -->
# 库不实现限流：只给分派前接缝，策略归调用方

## 决定（2026-10-18）

UVHTTP **不再内置限流**。公共 API 只剩一个分派前接缝：

```c
uvhttp_server_set_request_filter(server, filter, user_data)
```

七个限流 API（`enable`/`disable`/`check`/`get_status`/`reset_client`/
`add_whitelist`/`clear_all`）、`uvhttp_server_t` 的八个 `rate_limit_*` 字段、
`struct whitelist_item`、`UVHTTP_FEATURE_RATE_LIMIT` 全线删除。
`UVHTTP_ERROR_RATE_LIMIT_EXCEEDED` (-550) 保留 —— 它从「库内部返回值」
变成「应用层 filter 可用的惯用码」。

## 为什么不选「做成真 per-client」

被删的实现确实是坏的（签名 per-client、实现是 server 上一个全局计数器，
A 打满连带 429 B；白名单只用 `sockaddr_in`，IPv6 客户端进不了键）。
但把它修成「真 per-client」需要：一张按 key 的表 + 淘汰策略 + 内存上限 +
信任哪层代理的判断。**这是新增运行时能力，不是原子化**，与 v2.10
「特性冻结、只还 API 形状的债」直接矛盾。

更根本的一层：限流策略（窗口长度、计数维度是 client IP 还是 tenant 还是
API key、要不要信 X-Forwarded-For）本质是应用层决定。本库定位是**嵌入库
而非框架** —— 框架才替使用者决定这些，嵌入库只提供位置。
这与 [[connection-limit-503-memory-safety]] 定的「公共字段要么是权威真源、
要么不存在」同源：`client_ip` 参数收了却不读，就是既不是真源也不存在的
第三种状态 —— 噪声。

## 接缝契约（改代码时必须保持）

1. **位置**：`on_message_complete` 内、协议升级探测**之前**、router 查找之前。
   即被删限流原来那一格。因此接缝也拦得住 WebSocket 升级。
2. **单槽，不是链**。与 `server->handler` 单指针先例一致；要链式组合，
   用户在自己的回调里串。固定容量列表要引入生命周期与释放语义，能力增量为零。
3. **返回值**：`UVHTTP_OK` 继续分派；任何其他错误码停止分派。
4. **停止时由谁发响应**：filter 自己发完是预期用法（429 + `Retry-After`）。
   没发则库兜底，且兜底前必须**保留 filter 已写的状态码**。
5. **兜底的哨兵是快照，不是 `status_code == 0`**。计划里原写的是查 0，
   实现证明那条不够：`uvhttp_response_create` 把 status 初始化成 200
   （`src/uvhttp_response.c:297`），只有连接复用路径才重置为 0
   （`src/uvhttp_connection.c:499`）。查 0 会让「设了 429 但没 send」
   被兜底覆盖成 200 + Forbidden 体。正确做法是**调用 filter 前记下
   status，返回后仍等于快照才判定 filter 没表态**，否则发 403。
6. `user_data` 借用语义，库不 free（同 `server->handler`）。
7. 双发安全靠 `uvhttp_response_send` 的 `sent` 保护
   （`src/uvhttp_response.c:1087`），因此兜底对已发送的 filter 是 no-op。
8. keep-alive 不受影响：拒绝走正常 send，回收路径与原 429 完全相同
   （有测试 `KeepAliveSurvivesDenial` 钉住）。

## 验证到的行为

`examples/03_middleware/rate_limit_demo.c` 重写为纯用户代码的 per-IP 固定窗口
（`uvhttp_server_set_request_filter` + `uvhttp_request_get_client_ip`，自发
429 + `Retry-After`）。手工端到端（窗口 5 req/s）：同一 IP 打满 → 429，
另一个 loopback 地址（127.0.0.2）同期请求全 200，窗口过一秒恢复 200。
「一个客户端打满不影响其他客户端」正是原实现做不到的那件事。

## 遗留限制（本次不修，见下页）

- 库**无法监听 IPv6**：`uvhttp_server_listen` 只走 `uv_ip4_addr`
  （`src/uvhttp_server.c:541`）。所以「删限流顺带修掉 IPv6 盲区」只在
  策略层成立（键可以是 IPv6 地址），传输层根本没到得了那一步。
- 429 的 reason phrase 打印为 `HTTP/1.1 429 Unknown` ——
  `src/uvhttp_response.c:158` 的状态短语表没有 429。原内置限流同样如此，
  非本次引入。


## Timeline

- time: 2026-10-18T06:30:34
  kind: decision
  summary: "Created this page: 库不实现限流：只留分派前 request filter 接缝，策略归调用方"
  source: "2026-10-18 v2.10 路线 d3 落地（PR refactor/drop-builtin-rate-limit）"
  affects: [request-filter-seam-replaces-rate-limiter]

- time: 2026-10-18T06:33:13
  kind: decision
  summary: "写入接缝契约与「库不实现限流」的裁决依据"
  source: "2026-10-18 路线 d3 落地"
  affects: [request-filter-seam-replaces-rate-limiter]
