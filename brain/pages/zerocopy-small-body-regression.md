---
id: zerocopy-small-body-regression
title: "零拷贝 writev 对小 body 是负优化（阈值 4096）"
category: decision
status: active
tags: [performance, benchmark, writev, methodology]
created: "2026-09-28T18:56:30"
updated: "2026-09-29T01:51:22"
---

<!-- compiled_truth -->
## 结论
writev 双 iovec（header + body 一次 `uv_write`）只在 body 够大时才是优化。小 body 下它比"memcpy 到单缓冲 + 一次 write"**慢约 14%**：多一个 iov 让 libuv 走 writev 而非单缓冲快路径，省下的那次小 memcpy 抵不过 syscall 侧的开销。因此 v2.8.0 把 writev 无条件用于全部非 TLS 响应后，`/`（几十字节 body）出现真实回归。

修复：`src/uvhttp_response.c` 的 writev 分支加 `body_length >= UVHTTP_ZEROCOPY_MIN_BODY`（新具名常量，默认 4096，CMake 可调）。`/large`（~100KB）保持 writev 增益 ~1.51x。

## 测量条件（换环境数字不成立，方法可复用）
- 服务端绑到单核（CPU 0），`wrk -t4 -c100` 放 CPU 1-9 —— **必须保证是 server-bound**。反例：把 server 与 wrk 一起绑在 2 个核上时客户端先饱和，三个变体都停在 ~13.6K，差异被完全掩盖。
- 同一台机器交替跑 A/B/A/B，取中位数比**配对比值**，不比绝对值；本机噪声大（内存高占用），绝对值不可信。
- 单变量实验定位根因：只加阈值、不改其他，`/` 从 v2.7.2 的 0.861 恢复到 0.987，`/large` 仍 1.51x。
- 排除项：router-cache 默认值翻转（`include/uvhttp_constants.h`）不是原因——CMake 总是显式定义该宏。

## 状态
PR #387（`fix/zerocopy-small-body`）；v2.8.0 仍为 pre-release，待 #387/#388 处置后再决定是否 promote。


## Timeline

- time: 2026-09-28T18:56:30
  kind: decision
  summary: "Created this page: 零拷贝 writev 对小 body 是负优化（阈值 4096）"
  source: created via brain create-page
  affects: [zerocopy-small-body-regression]

- time: 2026-09-28T18:57:17
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "v2.8.0 门禁取证会话（2026-09-28）"
  affects: [zerocopy-small-body-regression]

- time: 2026-09-29T01:14:08
  kind: note
  summary: "阈值判定用的是压缩前的 response->body_length：uvhttp_response_send() 在调用 prepare()（gzip 会就地改写 body_length）之前就决定走 writev 还是拷贝路径。后果仅限性能——原始 body ≥4096 但压缩后 <4096 的可压缩响应仍走双 iovec writev，落在小 body 慢 14% 的区间；正确性不受影响（prepare 返回的压缩 buffer 才是实际发送内容，Content-Encoding 一致）。暂不改：收窄判定需把 prepare 提到决策前，代价大于收益。"
  source: "PR #387 自评审（代码路径核对 src/uvhttp_response.c:1044-1080 / :962-970 / :615-743）"
  affects: [zerocopy-small-body-regression]

- time: 2026-09-29T01:51:22
  kind: evidence
  summary: "PR #387 已合入 main（cda4815）。合入前用同机配对门禁实测：base = 含全量 writev 的 main、head = 加阈值，10 轮配对给出 / 123.4%、/json 127.6%、/large 101.8%，三端点 MAD 2–4%、0/10 配对低于限——与本地单核绑核测得的 +14%（小 body 回到拷贝路径）方向一致、幅度更大（CI 是 -t2 -c10 轻载，拷贝路径优势更明显）。v2.8.0 处置仍待定（原地重打 tag vs v2.8.1）"
  source: "PR #387 合并（2026-09-29）+ 配对门禁实测"
  affects: [zerocopy-small-body-regression]
