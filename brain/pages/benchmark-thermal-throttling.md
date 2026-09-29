---
id: benchmark-thermal-throttling
title: "基准测试发现 CPU 热降频主导方差"
category: decision
status: active
tags: [performance, benchmark, methodology]
created: "2026-08-21T04:26:33"
updated: "2026-09-29T01:22:56"
---

<!-- compiled_truth -->
## 发现（2026-08-21，多轮基准 + CPU 频率/温度监控）
本地基准主机 AMD Ryzen 7 5800H 上，**热降频主导测量方差**，不是代码：10 轮连续压测里前 3 轮跑在涡轮频率（~33K RPS，Gold），之后温度触顶、频率回落，稳态只有 ~15K RPS（Silver），整体 CV 40%+。

对策（当时）：
1. 基准脚本同时采 CPU 频率与温度，把"峰值"和"稳态"分开记录，不再混成一个数字。
2. 权威基准迁到 GitHub CI（见 `performance-benchmark-update`），本地只作开发参考。

## 后续修正（2026-09-28）
"换到 CI 就稳定"只对了一半：CI **单次运行内** 10 轮 CV 只有 0.4–2.4%，但**跨运行**同一 commit 的中位数可漂 ~40%（runner 是共享 VM，邻居/CPU 型号/频率每次分配都不同）。所以方差治理的正解不是换机器，而是**同机配对比较**——见 `perf-regression-gate`。

## 可迁移的判据
任何"性能回归"结论，先排除采样层：本地看频率/温度，CI 看是否同一 runner 内的配对。绝对值跨环境、跨 run 都不可比。


## Timeline

- time: 2026-08-21T04:26:33
  kind: decision
  summary: "Created this page: 基准测试发现 CPU 热降频主导方差"
  source: Multi-round benchmark session
  affects: [benchmark-thermal-throttling]

- time: 2026-09-29T01:22:56
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）回填 — 依据 commit aecb053 / cf1d456 / docs/PERFORMANCE_TARGETS.md"
  affects: [benchmark-thermal-throttling]
