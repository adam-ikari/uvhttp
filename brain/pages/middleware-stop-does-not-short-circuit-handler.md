---
id: middleware-stop-does-not-short-circuit-handler
title: "中间件 STOP 不短路 handler 体：已知缺陷与本次不动代码的决定"
category: decision
status: active
tags: [middleware, api, defect, v2.10]
created: "2026-10-18T06:30:50"
updated: "2026-10-18T06:33:13"
---

<!-- compiled_truth -->
# 中间件 STOP 不短路 handler 体（已知缺陷，本次未修）

## 事实

`UVHTTP_EXECUTE_MIDDLEWARE` 是在 handler **函数体内**展开的宏。
`STOP` 只跳出中间件循环，handler 体照样从头跑到尾：

- `include/uvhttp_middleware.h:73-97` —— `do{...}while(0)` 里的
  `break`/标签只作用于中间件循环，宏展开点之后的业务代码无条件执行。
- 示例「看起来对」只是因为重复发送被拦住：`src/uvhttp_response.c:1087`
  的 `response->sent` 保护让 handler 里第二次 `send` 变成 no-op。
  **拦截效果是假的** —— 副作用（查库、写文件、调用下游）全部照常发生。

所以中间件不能作为鉴权 / 限流 / 计费这类「必须真的挡住」的策略点。

## 与 request filter 的关系

v2.10 路线 d3 删内置限流时，「让中间件承担这个接缝」这条路被核实并否决，
本缺陷是否决依据：分派路径上 `src/uvhttp_request.c:414`→`:558` 之间
除限流外没有任何用户代码，中间件又在 handler 体内，两者都不是分派前接缝。
分派前策略点现在由 `uvhttp_server_set_request_filter()` 承担
（见 [[request-filter-seam-replaces-rate-limiter]]）。

## 本次决定：只记录，不动代码（2026-10-18 用户拍板）

d3 的范围是删限流 + 给接缝。改宏展开形态（返回值检查提到 handler 调用点、
或把中间件执行移到分派层）是另一件事，触及所有现有 handler 的形状，
混进这个 PR 会让评审看不出「接缝」这一刀本身对不对。

## 后续 PR 的依据（未开工）

两条候选：
1. 中间件执行上移到分派层（`uvhttp_process_request` 里、router 查找之前），
   handler 只作为终点 —— 语义与 request filter 高度重叠，需先裁决两者关系
   （filter 是分派前、中间件是否也变分派前？若重叠则中间件该收敛）。
2. 保留宏形态，但让 `STOP` 真的短路：宏展开为
   `if (UVHTTP_EXECUTE_MIDDLEWARE(...) != UVHTTP_OK) return;` ——
   要求所有 handler 返回类型统一，破坏面比第 1 条更大。

判据：任何一条都必须能写出「STOP 之后 handler 体的副作用没发生」的测试
（现有示例的 sent 保护会掩盖这类断言，写测试时别依赖 send 次数做证据）。


## Timeline

- time: 2026-10-18T06:30:50
  kind: decision
  summary: "Created this page: 中间件 STOP 不短路 handler 体：已知缺陷与本次不动代码的决定"
  source: "2026-10-18 核实 d3 中间件方案时定位（include/uvhttp_middleware.h:73-97）"
  affects: [middleware-stop-does-not-short-circuit-handler]

- time: 2026-10-18T06:33:13
  kind: decision
  summary: "写入 STOP 不短路的证据与后续 PR 候选方案"
  source: "2026-10-18 核实 d3 中间件方案时定位"
  affects: [middleware-stop-does-not-short-circuit-handler]
