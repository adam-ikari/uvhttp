---
id: code-quality-l3-l5-fixes
title: "代码质量修复 L3-L5"
category: decision
status: active
tags: [code-quality, gzip-cache, comments]
created: "2026-08-21T03:55:25"
updated: "2026-09-29T01:22:30"
---

<!-- compiled_truth -->
## 内容（v2.7.0，commit aecb053）
代码质量评审按层级出问题时，L3–L5 三项随 TLS 会话缓存一起修掉：

- **L3 — gzip 缓存内存记账漏了条目结构体开销**：`uvhttp_gzip_cache` 的 `total_memory` 原本只累计压缩数据字节，不计 entry 结构本身，导致按内存上限淘汰时实际占用高于名义值。改为把 entry 结构开销一并计入。
- **L4 — `uvhttp_gzip_cache_set_max_entries()` 不扩容 entries 数组**：运行时上调条目上限时数组仍按旧容量，写入越界风险。改为按需增长 entries 数组。
- **L5 — 注释拼写**：`paddingto32bytes` → `padding to 32 bytes`。

## 可复用的判断
L3 是这一类里唯一有长期价值的教训：**缓存的内存核算必须包含元数据自身**。同类问题在 `src/uvhttp_lru_cache.c` / `src/uvhttp_gzip_cache.c` 上重复出现过，审查缓存实现时优先看"记账口径 vs 真实占用"。L5 属噪音，不再单列。


## Timeline

- time: 2026-08-21T03:55:25
  kind: decision
  summary: "Created this page: 代码质量修复 L3-L5"
  source: Code quality fix session
  affects: [code-quality-l3-l5-fixes]

- time: 2026-09-29T01:22:30
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）回填 — 依据 commit aecb053 / cf1d456 / 仓库现状"
  affects: [code-quality-l3-l5-fixes]
