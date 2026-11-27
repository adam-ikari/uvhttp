---
id: middleware-stop-does-not-short-circuit-handler
title: "中间件 STOP 不短路 handler 体：已知缺陷与本次不动代码的决定"
category: decision
status: active
tags: [middleware, api, defect, v2.10]
created: "2026-10-18T06:30:50"
updated: "2026-11-24T14:10:31"
---

<!-- compiled_truth -->
# 中间件 STOP 现已可真正短路 handler 体（2026-11-24 修复）

## 修复形态

分两层，调用方按用途选：

- `UVHTTP_EXECUTE_MIDDLEWARE` / `UVHTTP_EXECUTE_MIDDLEWARE_CHAIN`：改为表达式，
  返回 `UVHTTP_MIDDLEWARE_CONTINUE`(0) / `STOP`(1)。只断链，不断 handler 体 ——
  日志、计时这类无副作用场景继续用它，语句式旧调用点源码兼容。
- `UVHTTP_MIDDLEWARE_GUARD` / `UVHTTP_MIDDLEWARE_CHAIN_GUARD`（新增）：STOP 时
  发送中间件已设置的响应（`uvhttp_response_send` 有 `sent` 幂等保护），然后
  `return UVHTTP_OK`，handler 体副作用真的不发生。要求 handler 返回 `int`
  （库的标准 `uvhttp_request_handler_t` 签名）。

`include/uvhttp_middleware.h` 的实现从宏内 `goto` 标签改为
`static inline uvhttp_middleware_run(handlers, count, req, resp)`，链内共享一个
context，正常/中断两条路径都恰好清理一次；`__COUNTER__` 标签碰撞问题消失。

## 证据（观测量，不是 send 次数）

`test/unit/test_middleware.cpp` 新增 4 个用例：
`Guard_StopPreventsHandlerBodySideEffect` 断言 `g_handler_body_side_effect == 0`，
用全局副作用计数器而非响应状态 —— 旧形态下 `sent` 保护会掩盖重复 send，计数不会。

变异验证（已执行）：
- 把 `UVHTTP_MIDDLEWARE_GUARD` 的 `return` 去掉 → 该用例红（计数=1）
- 把 `UVHTTP_MIDDLEWARE_CHAIN_GUARD` 的 `return` 去掉 → 链版本用例红
- 恢复后两者全绿；`test_middleware` 39/39 通过

## 文档与示例

`examples/03_middleware/middleware_compile_time_demo.c` 的 `protected_handler`
与 `middleware_chain_demo.c` 的 `user_handler`/`admin_handler`（含鉴权链）改用
GUARD；`docs/spec/middleware-api.md` 写清两层语义与前置条件。

## 与 request filter 的关系（保留）

分派前强制接缝仍是 `uvhttp_server_set_request_filter()`；中间件守卫是 handler
内部的强制点，两者语义不同但都可拦请求。旧「d3 不动代码」的决定已被本次
用户明确授权的四方向推进推翻，timeline 保留原条目作为依据。


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

- time: 2026-11-24T14:10:31
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 生产级推进 slice 1：守卫宏落地 + 变异验证"
  affects: [middleware-stop-does-not-short-circuit-handler]
