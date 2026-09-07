---
id: perf-regression-gate
title: "性能回归门禁：10% RPS 阈值 CI 失败"
category: decision
status: active
tags: [performance, benchmark, ci, gate]
created: "2026-08-26T04:20:15"
updated: "2026-09-07T07:13:56"
---

<!-- compiled_truth -->
## 方案
在 ci-benchmark.yml 中添加 Regression gate check 步骤：
- 脚本：`scripts/performance/regression_check.py`
- 输入：`benchmark-raw.csv`（ci-benchmark 自动生成）
- 基线：内置默认值（`/` 83K, `/json` 81K, `/large` 5.7K）
- 阈值：10%（RPS 低于基线 90% 则 exit 1 → CI 失败）
- 触发条件：仅 PR 和 pre-release push（workflow_dispatch 不阻塞）
- 对比指标：每个 endpoint 的中位数 RPS（10 轮取 median）

## 设计决策
- 10% 阈值：CI runner 一致性好（CV < 5%），10% 足够宽松避免误报，同时防严重退化
- 中位数对比（非均值）：对异常值更鲁棒
- 内置基线：避免外部文件依赖，随代码版本更新
- workflow_dispatch 不阻塞：开发期间可手动跑基准看趋势

## 验证
- PASS 场景：83K RPS → exit 0
- FAIL 场景：70K RPS → exit 1，报 "FAIL: / — 70000 RPS (baseline 83000, limit 74700, 84.3%)"
- PR #366 合并，CI 全部通过


## Timeline

- time: 2026-08-26T04:20:15
  kind: decision
  summary: "Created this page: 性能回归门禁：10% RPS 阈值 CI 失败"
  source: "性能回归门禁建设会话"
  affects: [perf-regression-gate]

- time: 2026-08-26T04:23:36
  kind: decision
  summary: "性能回归门禁：10% RPS 阈值 CI 失败"
  source: "性能回归门禁建设会话"
  affects: [perf-regression-gate]

- time: 2026-09-07T07:13:56
  kind: note
  summary: "触发条件修正：pre-release 分支 push 是死配置，改为 release 事件（published）+ PR benchmark 标签；趋势数据仅 pre-release 落库。详见 [[release-process-benchmark-gate]]"
  source: "CI/发布配置与真实流程脱节修复"
  affects: [release-process-benchmark-gate]
