---
id: handler-ignored-without-router
title: "server->handler 只在挂了 router 时才跑，与头文件注释相反"
category: decision
status: active
tags: [api, defect, dispatch]
created: "2026-10-18T06:31:17"
updated: "2026-10-18T06:33:13"
---

<!-- compiled_truth -->
# `server->handler` 只在挂了 router 时才跑：与文档正好相反

## 事实（2026-10-18 定位，非本次引入）

`uvhttp_server_set_handler` 的头文件注释写的是
「Set the request handler used when **no router is attached**」
（`include/uvhttp_server.h:299`）。实现是反的：

- `src/uvhttp_request.c:566` —— `else` 分支「no router」直接发硬编码
  `200 OK` + body `"OK"`，**从不读 `server->handler`**。
- `server->handler` 唯一的两个调用点（`:539` static miss 兜底、`:553` router
  未命中兜底）都在 `if (conn->server->router)` 块内部。

所以「不挂 router、只设一个 catch-all handler」这种装配方式下，
handler 恒不被调用，每个请求得到库自己的 200 "OK"。
调用方拿到 200，没有任何信号说明自己装的回调一次都没执行。

## 怎么发现的

d3 新增 `test/unit/test_server_request_filter.cpp` 时，5 个「filter 放行 →
handler 应执行」的用例全红：断言的是 handler 计数，计数恒为 0。
修法不是改测试 —— 测试夹具改成挂一个空 router（`uvhttp_server_take_router`），
用例转绿，同时把这条缺陷写进测试注释。
**测试的价值就在这里**：这类形状缺陷单靠读代码会在 `if (server->router)`
的嵌套里看漏。

## 本次决定：不在 d3 范围内修

d3 那一刀是「删限流 + 给分派前接缝」，动 `else` 分支属于分派语义变更，
与本 PR 的评审焦点无关，且需要先裁决文档与实现哪个是权威。

## 修的时候（未开工）

两处都改才自洽，只改一处会留下新的说谎：
1. `src/uvhttp_request.c:566` 的 no-router 分支补上 `if (server->handler)`，
   否则 404/默认响应；
2. 或者把 `include/uvhttp_server.h:299` 的注释改成实话（「仅在有 router
   且未命中/未命中 static 时作为兜底」）—— 但这条 API 的存在意义就是
   给不挂 router 的用户用，改注释等于承认 API 无用。

倾向第 1 条。判据：测试必须断言「handler 计数 > 0」而不是「响应体等于某串」，
后者用库的默认 200 "OK" 也能蒙过一部分写法。
关联 [[request-filter-seam-replaces-rate-limiter]]、
[[middleware-stop-does-not-short-circuit-handler]]（同属「分派路径上的
用户代码接缝」这一族）。


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
