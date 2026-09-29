---
id: performance-benchmark-update
title: "性能基准更新 2026-08-21"
category: decision
status: active
tags: [performance, benchmark, release-v2.6.2]
created: "2026-08-21T04:12:43"
updated: "2026-09-29T01:22:56"
---

<!-- compiled_truth -->
## 变更（v2.7.0，commit aecb053，2026-08-21）
基准口径从"本地机器"整体迁到 **GitHub Actions `ubuntu-latest`**，并引入 CV 作为 KPI、新增 Platinum tier（80K RPS）。

当时的权威基线（Release 构建、系统分配器、`benchmark_unified`、`-t2 -c10 -d10s`、每端点 10 轮）：

| 端点 | RPS |
|---|---|
| `/` | 83,099 |
| `/json` | 81,848 |
| `/large` | 5,721（v2.7.x 零拷贝后由 PR #385 上调至 8800） |

本地 5800H 的数字保留为"开发参考"，明确标注非权威：稳态 ~15K（Silver）、前 3 轮涡轮峰值 ~33K（Gold），CV 40%。

## 2026-09-28 的重要限定（勿照抄本页当门禁依据）
"CI 基线权威"只对**趋势记录**成立，不对**门禁阈值**成立。在同一 commit 上重跑 `ci-benchmark.yml`，小响应中位数从记录的 83K 落到 ~40–54K 区间——共享 runner 的跨 run 漂移约 40%，而上表的 2.0% CV 是**单次运行内 10 轮**的离散度。因此绝对 RPS 阈值已降级为报告信息，回归门禁改为同机 head/base 配对，见 `perf-regression-gate`。


## Timeline

- time: 2026-08-21T04:12:43
  kind: decision
  summary: "Created this page: 性能基准更新 2026-08-21"
  source: Performance benchmark session
  affects: [performance-benchmark-update]

- time: 2026-09-29T01:22:56
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）回填 — 依据 commit aecb053 / cf1d456 / docs/PERFORMANCE_TARGETS.md"
  affects: [performance-benchmark-update]
