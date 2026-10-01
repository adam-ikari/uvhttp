---
id: zerocopy-small-body-regression
title: "零拷贝 writev 对小 body 是负优化（阈值 4096）"
category: decision
status: active
tags: [performance, benchmark, writev, methodology]
created: "2026-09-28T18:56:30"
updated: "2026-10-01T01:54:54"
---

<!-- compiled_truth -->
评估完毕（2026-09-30）：维持「代价大于收益」，正式关闭重构。与 alloc-hotpath-measured 同理——影响上限低于测量噪声，不值得为它动手。

## 收益上限量化

受益请求 = 可压缩 + 原始 body ≥ 4096 + 压缩后 < 4096。这类响应从 writev 双 iovec 改为单 buffer，单个请求快约 14%（brain 已实测 writev 对小 body 的负优化幅度）。

- 乐观假设这类请求占 10%
- 整体影响上限 ≈ 1.4%
- 1.4% << runner 跨 run 方差 ~40%（perf-regression-gate）
- **不可测，也无从验证** —— writev 与单 buffer 对 1.4% 级差异，CI 配对门禁是盲的

## 为什么不能低成本改

关键约束（代码核对 src/uvhttp_response.c）：uvhttp_response_prepare 压缩成功后**就地改写 response->body_length** 为压缩后值，但 response->body 仍指向原始 buffer。因此：

- send_copy → build_data → prepare 这条链，若在 send() 先调一次 prepare 再走 copy，第二次 prepare 会**重新压缩**（非幂等）
- 要让「先 prepare 再用压缩后长度判定」不重复压缩，必须把 writev 与 copy 两条路径都内联进 uvhttp_response_send()——中等重构，owned/headers 释放的错误清理是风险点

## 决策

改对了是纯增益但无门禁能验证；改错了有 bug 风险（现有 wire 测试只保证正确性，不保证这条路径的性能形态）。收益 ~1.4% 无法测量，重构风险真实存在。**YAGNI 关闭**。

## 重启条件

- 出现可压缩且原始 ≥4096 但压缩后 <4096 的响应成为主导负载（届时先量化占比，再按 perf-regression-gate 走 CI 门禁验证）
- 有证据表明 writev 对小 body 的负优化在本项目实际负载下 >10% 且可测


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

- time: 2026-09-29T05:37:19
  kind: note
  summary: "v2.8.1 已发布为 Latest（tag b26e9ac），本页阈值修复随 v2.8.1 上线。v2.8.0 pre-release 保持不回退。PR #391（base 跳过 nightly）合入后 release 事件门禁 base 解析不再漂移。"
  source: "v2.8.1 发布会话（2026-09-29）"
  affects: [zerocopy-small-body-regression]

- time: 2026-10-01T01:54:54
  kind: decision
  summary: "评估完毕正式关闭：收益上限 ~1.4%（仅可压缩且跨阈值的响应，乐观占10%）<< 40% 方差不可测；且 prepare 非幂等使低成本改法不可行（需内联双路径，风险真实）。YAGNI 关闭"
  source: "阶段 3 评估（2026-09-30）"
  affects: [zerocopy-small-body-regression]
