---
id: websocket-tls-pr335-merge
title: "WebSocket TLS 修复与 PR #335 合并"
category: decision
status: active
tags: [websocket, tls, pr]
created: "2026-08-18T03:12:20"
updated: "2026-08-18T03:12:39"
---

<!-- compiled_truth -->
## 背景

uvhttp 的 WebSocket 层曾有多处内存安全/协议合规缺陷。PR #336（fix/ws-rfc-compliance）已在 main 上修复全部 code-review 缺陷（分片 CONTINUATION 重组、max_frame_size 溢出防护、协议合规检查 M1-M5、S1 double-free、S4 累积上限）。PR #335（fix/ws-tls-large-frames）最初基于 #336 之前的旧 main 创建，因此（1）缺失这些修复，（2）还引入了 3 个回归：删除 process_data 的 max_frame_size 安全 guard（触发 asan-gate heap-buffer-overflow）、ws_close 状态顺序错误（close 帧发不出去）、recv_frame 扩展长度解析顺序错误。

## 决策：PR #335 采用 rebase 方式合并

- 将 PR #335 rebase 到最新 main（已含 #336 全部修复），丢弃 3 个回归。
- 只保留 PR #335 的两个真实 TLS 修复：on_websocket_read 每次解密 chunk 立即喂 WS parser（修复 >16KB 大 TLS 帧卡在 mbedtls 的问题）；uvhttp_connection_websocket_close 在 free 前触发 on_close 回调链（修复 TLS 错误/EOF 路径 wrapper 与 embedder 状态泄漏）。
- send_frame 的大帧循环写已在 main 上存在，不重复实现。
- 合并前追加修复：process_data 的 CLOSE 分支存在顺序 bug —— 先触发 on_close（on_websocket_close 会 free wrapper 并置 user_data=NULL），随后读取 user_data 做 close 帧 echo 得到 NULL，导致 server 永不回 close 帧（RFC 6455 §5.5.1）。修复：在 on_close 之前捕获 server context，echo 用保存的引用。

## 关键洞察

- on_close 双重触发是安全的：on_websocket_close 释放 wrapper 并置 user_data=NULL，第二次触发时 wrapper 为 NULL 提前返回。
- CI 的 code-quality-check / format-check 偶发在 setup-build 步骤（sudo apt-get update）超时，这是 GitHub runner 环境问题与代码无关，重跑即可通过（曾重跑 2-3 次）。


## Timeline

- time: 2026-08-18T03:12:20
  kind: decision
  summary: "Created this page: WebSocket TLS 修复与 PR #335 合并"
  source: "PR #335 处理会话"
  affects: [websocket-tls-pr335-merge]

- time: 2026-08-18T03:12:39
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #335 处理会话"
  affects: [websocket-tls-pr335-merge]
