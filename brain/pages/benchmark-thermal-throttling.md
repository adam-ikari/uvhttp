---
id: benchmark-thermal-throttling
title: "基准测试发现 CPU 热降频主导方差"
category: decision
status: active
tags: [performance, benchmark, methodology]
created: "2026-08-21T04:26:33"
updated: "2026-09-30T12:47:46"
---

<!-- compiled_truth -->
# 禁止在本机做性能测量 —— 连配对 A/B 也不行

## 硬约束

**不在本机跑任何用于下结论的性能测量。** 包括配对 A/B、交替测量、中位数比值——一律不做。本机测出的数字只能用于"确认代码路径被执行到了"这类非性能判断，且不得写进 commit、PR、brain 或任何结论。

这不是"本机数据不可靠所以少用"，而是**本机测量无法产出可复现的结论**，用它下结论本身就是错的。

## 为什么配对 A/B 也不行（2026-09-30 实测反例）

在 `benchmark-thermal-throttling` 原页面里，"本地只作开发参考"被读成了"本地配对 A/B 可以下结论"。据此实测了 #404（header 名称 control-char 检查）的性能影响：

8 轮交替配对 `wrk -t1 -c1 -d4s`，结果：

| 口径 | 中位比值 |
|---|---|
| 全部 8 轮 | 0.9994 |
| 剔除 3 轮"受干扰"轮 | 1.0054 |

**问题不在结论（确实无可测差异），而在方法**：
- 8 轮里 **3 轮**明显受热降频干扰（最低 8787 RPS，稳态约 20K，低一半）
- 要得到可用的结论，必须**先剔除这 3 轮**——而"哪些轮受干扰"是**主观判断**，没有客观阈值
- 一个依赖主观剔除的数据集，下次谁复现都复现不出来
- 更糟的是：剔除后 MAD 仍有 2.0%，即噪声比待测效应大一个数量级，"无差异"这个结论本身就撑不住

同样的坑在原始页面里已记过一次：10 轮里前 3 轮跑在涡轮频率、之后回落，CV 40%+。**热降频不是偶发噪声，是本机的常态行为**——任何依赖连续多轮的本机测量都会踩到。

## 正确做法

1. **不需要测的情况（默认）** — 先算上限。若待测改动的影响上限低于噪声（本项目 runner 跨 run 方差 ~40%，见 `perf-regression-gate`），**直接不测**，结论写"预期影响低于测量阈值"。
2. **需要测的情况** — 用 CI 的同机配对门禁（`perf-regression-gate` 页的设计）：同一 job 内 head/base 构建、交替 10 轮、按 round 配对取比值中位数，fail-closed。那里 head 与 base 在**同一 runner、同一时段**，热状态一致。
3. **临时手测** — 只允许用 `workflow_dispatch` 触发 CI 上的基准，不允许在本机跑。

## 判据

动本机 wrk 之前先问：
- 这个结论会被写进哪里？（commit / PR / brain → 不允许本机测）
- 上限算过吗？（低于 40% 方差 → 不测）
- 真的要测吗？（要 → 走 CI 门禁，不走本机）


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

- time: 2026-09-30T12:47:46
  kind: decision
  summary: "升级为硬约束：禁止本机性能测量，连配对 A/B 也不行（本机热降频使连续多轮不可复现，且剔除受干扰轮次是主观判断）；正确路径是先算上限或走 CI 同机配对门禁"
  source: "用户明确纠正（2026-09-30）"
  affects: [benchmark-thermal-throttling]
