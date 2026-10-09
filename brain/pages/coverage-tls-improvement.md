---
id: coverage-tls-improvement
title: "TLS 覆盖率提升三 PR（#461-#463）：tls.c 60.3%→67.9%"
category: decision
status: active
tags: [tls, coverage, test, quality]
created: "2026-10-06T00:44:03"
updated: "2026-10-06T00:44:28"
---

<!-- compiled_truth -->
# TLS 覆盖率提升三 PR（#461-#463）

## 成果

2026-10-06 完成三个 TLS 行为测试 PR，**tls.c 行覆盖 60.3% → 67.9%**，
总覆盖 74.9% → 75.8%（118 测试通过）。

| PR | 内容 | 测试数 |
|---|---|---|
| #461 | add_extra_chain_cert（证书链扩展） | 10 |
| #462 | load_cert_chain/load_private_key/load_ca_file 成功路径 | 6 |
| #463 | get_cert_subject/issuer/serial 证书信息提取 | 7 |

## 共性：成功路径零覆盖

三组函数的**成功路径**从未执行——现有测试只测 NULL/空串/不存在文件，
走函数第一行参数校验或 mbedtls parse 失败分支就 return。成功路径需真实
X.509 证书（test/certs/）。

## 复用模式

- **证书路径**：#434 首创的 `__FILE__` 上溯源码根 + test/certs/（CMake
  传绝对路径，ctest 工作目录无关）。
- **变异验证**：三 PR 都经变异（忽略 parse / 强制解析失败 / serial 恒空 →
  对应测试变红）。

## 顺带确认

- `load_cert_chain` 即使私钥不匹配也返回 OK——mbedtls 的
  `mbedtls_ssl_conf_own_cert` 只查非空不校验配对（mbedtls 行为非本库缺口）。
- 证书链追加的 `while(current->next)` 遍历经 4 次连续追加验证。

## 剩余缺口（成本高，未做）

- **connection.c 45.3%**：未覆盖是 TLS/WebSocket 回调（mbedtls_bio_recv/send、
  on_websocket_*、tls_decrypt_pending）与 OOM 防御分支——需真实 TLS 连接/
  mbedtls 桩，成本高。
- **context.c 22 行**：全是 OOM/熵源失败（防御性错误分支，不可变异模拟）。
- **server.c health_check_handler**：static handler 的 5 行 body 设置未触发，
  需真实服务器，价值低。


## Timeline

- time: 2026-10-06T00:44:03
  kind: decision
  summary: "Created this page: TLS 覆盖率提升三 PR（#461-#463）：tls.c 60.3%→67.9%"
  source: "2026-10-06 完成"
  affects: [coverage-tls-improvement]

- time: 2026-10-06T00:44:28
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "三 PR 完成"
  affects: [coverage-tls-improvement]
