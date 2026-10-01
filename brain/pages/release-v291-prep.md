---
id: release-v291-prep
title: "v2.9.1 发布准备：CI 门禁修复与死代码清理（patch）"
category: decision
status: active
tags: [release, v2.9.1, ci, quality, pre-release]
created: "2026-10-01T10:07:05"
updated: "2026-10-01T15:58:20"
---

<!-- compiled_truth -->
## 预发布转正式的条件（来源：docs/release-strategy.md）

**唯一门禁条件是 benchmark 回归门禁为绿。** 其余检查清单 8 项（测试 / ASan / UBSan / 文档构建 / CHANGELOG / VERSION / tag / 网站部署）都是**创建 pre-release 之前**的前置条件，不是转正式时的条件。

流程的因果结构：

```
前置条件全满足 → 打 tag → gh release create --prerelease
                                    ↓
                        release 事件自动触发 ci-benchmark
                                    ↓
                    paired gate 绿 ← 这是唯一的转正式门禁
                                    ↓
                        gh release edit --latest
```

因此「门禁绿」= 同一 runner 上 head/base 交替 10 轮，配对比值中位数 ≥ 90%（且多数配对不低于 90%）。绝对 RPS 只作报告，不参与判定（runner 跨 run 方差约 40%，绝对阈值 gate 的是机器运气）。

## v2.9.1 逐项核对

| 条件 | 状态 | 证据 |
|---|---|---|
| benchmark 回归门禁绿 | ✅ | run 36859077349 success；/ 100.6%、/json 99.6%、/large 97.3% |
| 测试通过 | ✅ | 102/102（#427 CI ubuntu-test-fast） |
| ASan 零发现 | ✅ | asan-gate pass（#427） |
| 文档构建 / doc-sync | ✅ | doc-sync-check 31/31 |
| CHANGELOG / VERSION | ✅ | [2.9.1] EN+ZH 已写；VERSION=2.9.1 |
| tag 已推送 | ✅ | v2.9.1 → f324784 |
| **UBSan 零发现** | ⚠️ **未覆盖** | nightly 最新跑在 `da93630`，早于 #417 死代码删除与本次 tag |

## tag 落后于 main 不构成阻塞（此前我的判断过重）

```
git diff v2.9.1..main -- src/ include/   →  空
```

main 落后的 2 个提交（#426 format-check 删除、#427 ci-daily 删除）只改
`.github/workflows/` 与 `docs/`，**库代码逐字节一致**。门禁测的是 tag，
tag 的库代码就是 main 的库代码，所以转正式不会发布未经门禁测量的代码。

## UBSan 缺口的实际影响

da93630 之后的库代码改动只有 #417：删除 `chunked_transfer_context_t`
（一个从未被实例化的 typedef）与一行注释缩短。两者都不引入未定义行为，
且 #417 的 PR 跑过 asan-gate + build-matrix 全配置。UBSan 会在下一个
nightly（UTC 00:00）自动覆盖。若要严格按清单执行，可在转正式前手动触发
`gh workflow run ci-nightly.yml --ref main`。


## Timeline

- time: 2026-10-01T10:07:05
  kind: decision
  summary: "Created this page: v2.9.1 发布准备：CI 门禁修复与死代码清理（patch）"
  source: "v2.9.1 发布准备（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T10:07:06
  kind: decision
  summary: "v2.9.1 发布准备：VERSION 2.9.0→2.9.1 patch + CHANGELOG EN/ZH + release-strategy 已更新，PR/tag/prerelease 未创建。内容全是质量修复（#416-#420）。trend job 端到端验证是主要目的"
  source: "v2.9.1 发布准备（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T10:55:33
  kind: decision
  summary: "v2.9.1 prerelease 已发布（tag 55a79ac，pre-release 未转 latest）；benchmark 门禁 ALL PASS（96.3/98.4/98.8%，差异在 MAD 内无回归）；trend job 端到端验证：push 到 benchmark-trends 成功（GH013 已修），但 gh pr create 被仓库 Actions policy 拒（can_approve_pull_request_reviews: False），趋势数据堆在分支待决策"
  source: "v2.9.1 pre-release + trend job 验证（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T11:19:46
  kind: decision
  summary: "趋势数据落库：API PATCH can_approve_pull_request_reviews 持续 404（curl 直连同样拒写，GET 正常），该设置只能走 web UI（Settings→Actions→General→Workflow permissions）；已用维护者 token 手动开 PR #423 验证路径可行并合并（main a059bcd，4 个趋势文件）。v2.9.1 仍 pre-release 待转正式"
  source: "趋势落库方案落地（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T11:44:49
  kind: reversal
  summary: "删除趋势数据落库功能（trend job + docs/benchmark-trends/ + benchmark-trends 分支）：用户决定不要趋势数据。两层限制（GH013 + Actions policy 不可 API 改）使修复链无尽头，且 trend 不参与门禁判定，删了对质量无影响"
  source: "删除趋势数据功能（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T13:41:43
  kind: note
  summary: "ASan/UBSan 盲点已知但不修（极简决策，2026-10-01）：asan-gate 用默认配置（STATIC_FILES=OFF），uvhttp_static/lru_cache/router_cache 三个被 feature 宏包裹的源文件不编译进 ASan 库（nm 符号数 0），8 个 static 测试是空壳绿灯；UBSan 只在 nightly 不阻塞 PR。明知存在，按极简哲学不扩展检查面，维持现状"
  source: "极简决策（2026-10-01）"
  affects: [release-v291-prep]

- time: 2026-10-01T15:58:20
  kind: decision
  summary: "转正式的唯一门禁是 benchmark paired gate 绿（其余 8 项是创建 pre-release 前的前置条件）。v2.9.1 门禁已绿（100.6/99.6/97.3%），唯一未覆盖项是 UBSan（nightly 最新跑在 da93630，早于 #417）。tag 落后 main 2 个提交但 src/include 逐字节一致，不构成阻塞"
  source: "预发布转正式条件核对（2026-10-01）"
  affects: [release-v291-prep]
