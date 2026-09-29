---
id: perf-regression-gate
title: "性能回归门禁：10% RPS 阈值 CI 失败"
category: decision
status: active
tags: [performance, benchmark, ci, gate]
created: "2026-08-26T04:20:15"
updated: "2026-09-29T02:26:09"
---

<!-- compiled_truth -->
## 现状
配对门禁已合入 main（PR #388，merge commit `d51be09`，2026-09-29）。`ci-benchmark.yml` 在同一 runner、同一 job 内测量 head 与 base，门禁判据是**两者的比值**，不再是绝对 RPS。

- base 解析：PR → `pull_request.base.sha`；release → 上一个已发布 release tag；`workflow_dispatch` → `base_ref` 输入。解析不到才退回绝对基线门禁。
- 测量：base 检出到 `bench-base/` 并构建；两台 `benchmark_unified`（head 18081 / base 18082）先预检端点双方均 200，丢弃 3s 预热，再按轮次交替跑 `/`、`/json`、`/large`（各 10 轮；奇数轮先压 head、偶数轮先压 base），产出 `head-paired.csv` / `base-paired.csv`。
- 判定：`regression_check.py --compare` 按 round 配对取每轮 head/base 比值。**失败需同时满足两条**：比值中位数 < `1 − threshold`（默认 10%），且**多数**单个配对也低于该限。中位数在限内但多数配对不在 → 打印 `NOISE`，不判失败。
- **fail closed（四种测不出结论的情形都算失败）**：base 无数据、配对 < 3 轮、被 gate 的端点在 head CSV 缺失、在 base CSV 缺失。端点清单**单一来源**是 workflow 的 job env `GATE_ENDPOINTS`，以 `--gate` 传给脚本——否则"脚本默认基线键"与"实测端点"两处真源会漂移。
- 绝对基线（`/` 83K、`/json` 81K、`/large` 8.8K）在配对模式下仍打印，但只是报告信息。
- 触发条件（2026-09-07 修正、09-28 复核）：`release: [published]` + PR 侧 `benchmark` label；pre-release 分支 push 是死配置，见 [[release-process-benchmark-gate]]。**该 label 在仓库里直到 2026-09-28 才被创建**，所以 PR 侧门禁历史上等于没跑过；今天不带 label 的 PR 依旧静默跳过——审 PR 时先看 label。

## exit code 契约（CI 依赖它，改动要同步）
`regression_check.py`：`0` = 通过；`1` = 门禁判红；`2` = **数据不可用**（CSV 解析不到结果、配对模式下任一侧没有 round 数据）。`2` 与 `1` 都必须让 job 失败，但语义不同：`1` 是"测出了回归"，`2` 是"根本没测到"。空 CSV 走 `2` 而不是静默通过，是 fail-closed 链条的第一环。

## 从初版（v2.7.1，PR #366）保留、至今仍成立的约定
- **内置基线，不依赖外部文件**：`DEFAULT_BASELINE` 写在脚本里，随代码版本一起更新；引入外部 baseline JSON 会让基线与代码版本脱钩（`--baseline` 参数保留给临时实验）。
- **比中位数，不比均值**：单轮 wrk 掉尾会把均值拉偏，median 对离群轮鲁棒。配对模式继承了这一点（比的是逐轮比值的**中位数**）。
- **每端点取 10 轮的 median**，不是单次采样。
- **`workflow_dispatch` 不阻塞**：手动跑基准只出报告、不判红，开发期看趋势用；门禁只在 `pull_request`（带 label）与 `release` 上生效。

## 三层噪声，各自对应一个对策
1. **run 间**（跨机器）：同一 commit 小响应中位数可从记录的 83K 漂到 40–54K（~40%）。绝对阈值 gate 的是这个 → 同机配对消除。旧口径"CI CV < 5%"只在**单次运行内**成立。
2. **配对内残差，且重尾**：配对后 `/` 仍有大抖动。证据都来自 PR #388 自身（**零 C 代码改动**）：6 轮给出 `/` 89.6%；10 轮给出 `/` 中位数 97.4%、MAD 14.4%（原始比值 93 80 103 72 97 123 100 88 100 109 %），而 `/large` 同期 MAD 2–3%。
3. **判据选择本身**：先试的中位数 − 1.7·MAD-SE 稳健下界**已被否决**——MAD-SE 继承重尾，14.4% 的 MAD 把下界压到 88.2%，同一构建仍被误判红。现规则对同码噪声宽容，对真实回退保持敏感：writev 小 body 案例是 −14% 且 spread ~2%，几乎每个配对都在限下，仍会 FAIL。

即：宁可用一个非参数的双条件规则，也不要在 n=10 的重尾样本上估置信区间。

## 顺带修掉的长期潜伏缺陷（都在这个 workflow 里）
- 仓库 `default_workflow_permissions = read`，而 benchmark job 从未声明 `permissions:` → PR 评论一直 403、趋势落库从未成功过。现 job 声明 `contents: read / issues: write / pull-requests: write`，**写仓库权限单独给 `trend` job**（`contents: write`），构建 PR-head 代码的 job 不持有可写 token。
- `wrk` 不加 `-L` 就不打印延迟分布 → 报告里 p99 列恒空。两处压测循环均已加 `-L`。
- `upload-artifact` 的 path 里混了绝对 `/tmp` 路径会保留绝对路径、产物套进 `home/runner/.../tmp/` 嵌套 → 加"Stage logs into workspace"步骤，趋势 job 用 `find … -print -quit` 两种布局都能取到。
- 产物上传与 PR 评论步骤带 `if: always()` —— 门禁判红的 run 恰恰是最需要配对比对证据的那次。

## 验证
- 合成样本 7 个场景 + fail-closed 场景（gate 端点缺失 → exit 1；空 CSV → exit 2，不会变绿）。
- GitHub 同码自测连续 4 轮绿：run 5 `/` 103.3%（1/10）、run 6 `/` 103.5%（0/10）、run 7（docs-only）、run 8（fail-closed 规则上线后首轮）`/` 103.9%、`/json` 115.2%、`/large` 101.6%。同批绝对基线报 `/` 73–80% —— 正是被降级为报告信息的那个跨 run 层。
- 首次实测真实修复（PR #387，base = 含全量 writev 的 main）：`/` **123.4%**、`/json` **127.6%**、`/large` 101.8%，MAD 2–4%，0/10 配对在限下。


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

- time: 2026-09-29T01:47:41
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）— #388 已合入 main 后的现状"
  affects: [perf-regression-gate]

- time: 2026-09-29T01:49:10
  kind: reversal
  summary: "绝对 RPS 阈值门禁被否决（2026-09-28 取证）：同一 commit 跨 run 小响应中位数从 83K 漂到 40–54K，阈值 gate 的是 runner 而非代码；改为同机 head/base 配对，绝对基线降级为报告信息"
  source: "v2.8.0 门禁取证会话（2026-09-28）"
  affects: [perf-regression-gate]

- time: 2026-09-29T01:49:11
  kind: reversal
  summary: "配对判据两轮否决：6 轮硬阈值在中码 97.4%/MAD 14.4% 的同码样本上误报；改试中位数 − 1.7·MAD-SE 稳健下界仍误报（下界被压到 88.2%）。终版规则＝中位数低于限 **且** 多数配对也低于限，配重尾宽容、对 −14%/2%-spread 的真实回退保持敏感"
  source: "PR #388 自测 run 3/4（2026-09-28）"
  affects: [perf-regression-gate]

- time: 2026-09-29T01:49:11
  kind: evidence
  summary: "顺带修掉两个长期潜伏缺陷：仓库默认 workflow token 为 read 导致 PR 评论与趋势落库从未成功；wrk 缺 -L 导致报告 p99 恒空。同码自测连续绿（103.3%/103.5%/docs-only/fail-closed 后 103.9%）"
  source: "PR #388 自测 run 5–8（2026-09-28~29）"
  affects: [perf-regression-gate]

- time: 2026-09-29T02:26:09
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #389 第二轮修订 — 找回初版仍成立的约定 + 补 exit code 契约"
  affects: [perf-regression-gate]
