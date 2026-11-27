---
id: version-310-decision
title: "版本 3.1.0：3.0 破坏性窗口收尾，不升 4.0"
category: decision
status: active
tags: [version, semver, release, decision]
created: "2026-11-26T14:37:25"
updated: "2026-11-26T17:22:51"
---

<!-- compiled_truth -->
# 版本决策（2026-11-26）— 3.1.1

## 调整

用户说"调"。VERSION 从 3.1.0 改为 **3.1.1**（patch），PREVIOUS_VERSION=3.1.0，NEXT_VERSION=3.1.2，VERSION_NAME 改为"3.1.0 收尾：TLS WANT_WRITE 与非 WS 升级残留字节修复"。

理由：ownership 回调签名变更是源码级破坏，不该藏在 3.1.0 的 MINOR 条目里。把它从 3.1.0 的 `Removed (breaking)` 移到新 3.1.1 条目的 `Removed (breaking)`，TLS WANT_WRITE 与首帧保留作为 3.1.1 的 Fixed。中英文 CHANGELOG 同步。

## 与 SemVer 的张力（延续 3.1.0 决策）

3.1.1 是 patch 却含破坏项——严格 SemVer 下不合规。这是对 3.1.0 决策的延续：用户要求保持 3.x、不消耗 MAJOR，代价由 CHANGELOG 的明确破坏清单补偿。这个取舍在 [[version-310-decision]] 已记录，不重复。

## 验证

所有构建目录重新 configure 后：Release 134/134、Static 135/135、ASan 134/134、UBSan 134/134、32-bit 134/134；`make check-docs` 31 对同步。


## Timeline

- time: 2026-11-26T14:37:25
  kind: decision
  summary: "Created this page: 版本 3.1.0：3.0 破坏性窗口收尾，不升 4.0"
  source: "2026-11-26 用户质疑'没生产级就 4.0'，决定保持 3.x"
  affects: [version-310-decision]

- time: 2026-11-26T17:22:51
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-26 用户要求调整，VERSION 3.1.0 → 3.1.1，ownership 签名变更从 3.1.0 条目拆到 3.1.1"
  affects: [version-310-decision]
