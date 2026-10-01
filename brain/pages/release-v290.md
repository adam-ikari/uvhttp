---
id: release-v290
title: "发布 v2.9.0"
category: decision
status: active
tags: [release, v2.9.0, benchmark, quality]
created: "2026-10-01T05:07:50"
updated: "2026-10-01T05:10:36"
---

<!-- compiled_truth -->
# 发布 v2.9.0

## 事实
- tag `v2.9.0` → `e7764af`（main，#414 合并）
- GitHub Release v2.9.0 = Latest（2026-10-01）
- 上一个 release：v2.8.1（2026-09-29）
- v2.8.1→main 25 提交，61 文件，生产代码真实变更仅 2 处（#404 header 检查、#409 lru_cache dead-store）

## 发布前质量门禁（全绿）
| 门禁 | 状态 |
|---|---|
| ctest | 102/102 |
| ASan (asan-gate) | pass |
| UBSan (ci-nightly test-ubsan, da93630) | pass |
| build-matrix 全配置 | pass |
| format + code-quality (cppcheck) | pass |
| doc-sync + links | pass |
| ci-fuzz 四 harness | 无崩溃（#409 后稳定） |

## benchmark 回归门禁 vs v2.8.1（核心门禁）
两阶段流程：手动 workflow_dispatch（36812792949）验证 + prerelease 事件触发（36815913819）确认。

prerelease 触发的判定（release 事件自动跑）：
| 端点 | head med | base med | ratio | pairs<lim | MAD | verdict |
|---|---|---|---|---|---|---|
| / | 75598 | 78104 | 99.9% | 2/10 | 8.6% | PASS |
| /json | 76521 | 77148 | 98.0% | 1/10 | 3.2% | PASS |
| /large | 10200 | 10153 | 99.8% | 0/10 | 2.7% | PASS |

ALL PASS，无回归。#404 header 检查在噪声内（/ 和 /json 下滑 0-2%，MAD 3.2-8.6%，1 个 MAD 内），符合纳秒级预期。

## 已知问题（不阻塞发布）
- ci-benchmark run 的 trend job 在 v2.9.0 prerelease 上 failure。trend 是趋势数据落库，不是发布门禁——benchmark job（含 Regression gate check）success。#394 记过 trend 推送竞态修复，仍有残留问题。需单独排查。

## 发布内容
- Added: 三 fuzz harness（request/websocket/static_path）、零拷贝 wire 测试、压缩×阈值交互、请求体上限边界
- Fixed: header 名称 control-char 检查（#404，防响应分割）、lru_cache dead-store（#409）、fuzz artifact 上传顺序（#400）
- Changed: integration→manual/（#411）、benchmark base 排除 nightly（#391）、社区贡献指南（#401/#410）
- Internal: 四项评估关闭（#405/#406/#407/#412）

## 发布准备 PR
#414（VERSION 2.8.1→2.9.0、CHANGELOG EN+ZH、CONTRIBUTING 发布流程重写、release-strategy 版本历史表补全）。


## Timeline

- time: 2026-10-01T05:07:50
  kind: decision
  summary: "Created this page: 发布 v2.9.0"
  source: "v2.9.0 发布（2026-10-01）"
  affects: [release-v290]

- time: 2026-10-01T05:08:09
  kind: decision
  summary: "v2.9.0 发布为 Latest。25 提交、生产代码真实变更仅 2 处。全部质量门禁全绿，benchmark 回归门禁 vs v2.8.1 ALL PASS（99.8-99.9%）。trend job failure 是独立问题不阻塞。"
  source: "v2.9.0 发布（2026-10-01）"
  affects: [release-v290]

- time: 2026-10-01T05:10:36
  kind: note
  summary: "trend job 首次触发即失败：直推 main 被 PR-only 规则拒（GH013），需改推专用分支+PR"
  source: "v2.9.0 发布后排查（2026-10-01）"
  affects: [release-v290]
