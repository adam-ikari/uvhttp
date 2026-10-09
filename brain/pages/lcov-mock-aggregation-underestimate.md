---
id: lcov-mock-aggregation-underestimate
title: "lcov 聚合低估 mock 测试覆盖（connection.c 45.3% 实为 ~79.6%）"
category: decision
status: active
tags: [coverage, lcov, test-infra, mock]
created: "2026-10-06T01:07:51"
updated: "2026-10-06T01:08:15"
---

<!-- compiled_truth -->
# lcov 聚合低估 mock 测试覆盖（connection.c 45.3% 实为 ~79.6%）

## 问题

`lcov --capture --directory .` 聚合本项目的覆盖率时，**严重低估含 mock 测试的文件**。
具体：connection.c 显示 45.3% 行覆盖，但跨全部 119 个测试 target 的 gcda 独立聚合后，
真缺口仅 332 行、真实覆盖约 **79.6%**。

## 根因

本项目每个测试 target 独立编译所有 src 源文件（CMakeLists 的 SOURCES 每个
target 都含），产生独立 .gcda。mock 测试（test_connection_libuv_fail、
test_server_accept_mock 等用 --wrap 链接）同样独立编译。lcov 聚合 119 份
同名记录块时，行覆盖取第一个块或错误合并——而第一块常是某个全 0 的 target，
把实际有命中的块覆盖掉。

**证据**：`connection_new_close_cb`（src/uvhttp_connection.c:560）gcov 直接测
显示执行 3 次，lcov 显示零覆盖。tls.c 的 add_extra_chain_cert 聚合后 DA669 命中
16 次（覆盖正常）——说明非 mock 文件聚合相对可靠。

## 正确测量方法

对单个文件用 `lcov --extract` 不行（同样聚合问题）。准确做法：
```python
# 从 lcov 数据解析，对每个 DA 行跨全部记录块求和
for block in records:
    for DA:行号,命中:
        hits[行号] += 命中
# 真缺口 = 所有块命中仍为 0 的行
```

## 对项目决策的影响

- **不要在 connection.c 上基于 45.3% 投入**——它实为 ~79.6%，剩余真缺口是
  TLS 回调（mbedtls_bio_recv/send、tls_decrypt_pending）与错误分支，
  需真实 TLS 连接或 mbedtls 桩，成本高、价值有限。
- 其他文件的 lcov 数据（tls 67.9%、server 84.9% 等）相对可靠（覆盖来自
  非 mock 测试）。
- context.c 22 行未覆盖全是 OOM/熵源失败（防御性错误分支，不可变异模拟）。

## 动作

- 记录在案，不据 connection.c 的 45.3% 规划测试。
- 若未来要做 mock 覆盖，优先修 lcov 聚合（或改用 gcov 按 target 合并）。


## Timeline

- time: 2026-10-06T01:07:51
  kind: decision
  summary: "Created this page: lcov 聚合低估 mock 测试覆盖（connection.c 45.3% 实为 ~79.6%）"
  source: "2026-10-06 调查发现"
  affects: [lcov-mock-aggregation-underestimate]

- time: 2026-10-06T01:08:15
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "lcov 聚合缺陷确认"
  affects: [lcov-mock-aggregation-underestimate]
