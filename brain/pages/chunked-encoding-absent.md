---
id: chunked-encoding-absent
title: "响应侧无 chunked encoding：能力存在（send_raw），缺的是封装，故不做推测性实现"
category: decision
status: active
tags: [http, chunked, sse, rfc7230]
created: "2026-10-03T10:15:10"
updated: "2026-10-03T10:15:10"
---

<!-- compiled_truth -->
# 响应侧 chunked encoding 缺失：现状与判断

## 事实

- `Transfer-Encoding` 在 `src/` 中出现 **0 次** —— 响应侧无 chunked 实现
- `conn->chunked_encoding` 只写不读（`uvhttp_connection.c:531, 619` 置 0），
  是死状态
- 无 Trailer 支持

## 关键：能力其实存在

`uvhttp_response_send_raw(data, len, client, resp)` 是**字节级**写入
（`src/uvhttp_response.c:834`），调用方可自行构造 chunk 格式：
`<hex-size>\r\n<data>\r\n`。

所以缺的不是**能力**，是**正确封装**。

## 为什么 2026-10-03 没有实现它

SSE 是项目里唯一的流式用例。实测发现它的问题是**声明了
`Connection: keep-alive` 却不定界消息体**（违反 RFC 7230 §3.3.3），而不是
「没有 chunked 所以做不了」。改用第三种界定方式（连接关闭）后完全合规。

新增公开 chunk API 的实际成本：
- chunk 状态机（begin / write / end）挂在 response 还是 connection 上
- 与 `Content-Length` 的互斥（两者都是长度界定，不能共存）
- 与压缩的叠加顺序（压缩后 chunk 还是先 chunk 再压）
- 与 zerocopy writev 路径的交互
- keep-alive 复用（chunked 结束后连接可复用，需正确重置状态）

这些都有真实设计成本，而**当前无用例**。按 YAGNI 不做推测性功能。

## 何时该做

出现下列任一需求时：

1. 边生成边发送且**需要复用连接**（`Connection: close` 方案做不到）
2. 大响应分块（避免一次性缓冲整个 body）
3. 反向代理转发上游的 chunked 响应
4. HTTP/2 或 HTTP/3（需要 trailer / 流式多路复用）

## RFC 依据

RFC 7230 §3.3.3 规定 HTTP/1.1 消息体长度只有三种界定方式：

1. `Content-Length`
2. `Transfer-Encoding: chunked`
3. 以连接关闭结束 —— 此时**不得**声明 keep-alive

第 3 种是 SSE 在 HTTP/1.1 下的合法做法：流一直开着，服务端决定何时关闭，
浏览器 EventSource 在连接断开后自动重连。


## Timeline

- time: 2026-10-03T10:15:10
  kind: decision
  summary: "Created this page: 响应侧无 chunked encoding：能力存在（send_raw），缺的是封装，故不做推测性实现"
  source: "2026-10-03 评估，PR #450 修 SSE"
  affects: [chunked-encoding-absent]

- time: 2026-10-03T10:15:10
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #450"
  affects: [chunked-encoding-absent]
