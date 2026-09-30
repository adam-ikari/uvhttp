---
slug: roadmap
title: Roadmap
role: milestones
updated: "2026-09-30T14:32:13"
---

# Roadmap

## 里程碑

```mermaid
gantt
  title Roadmap
  dateFormat YYYY-MM-DD
  section v2.7.x — 质量与嵌入
  性能基准更新与优化        :done, b1, 2026-08-21, 1d
  ci-fuzz 修复（C11对齐）  :done, b1b, 2026-08-25, 1d
  嵌入验证第二轮            :done, a3, 2026-08-25, 3d
  性能回归门禁建设          :done, b0, 2026-08-26, 1d
  代码评审缺陷修复          :done, b5, 2026-09-07, 1d
  section v2.8.x — 性能与平台
  新嵌入者接入              :done, a4, 2026-08-27, 1d
  v2.8.0 发布               :done, b6, 2026-09-23, 1d
  回归门禁改同机配对        :done, b7, 2026-09-28, 1d
  v2.8.1 补丁发布与收尾     :done, b8, 2026-09-29, 3d
  io_uring 静态文件路径     :cancelled, b2, after b8, 21d
  section v2.9.x — 生态
  文档多语言完善            :c1, after b2, 14d
  Fuzz 测试增强            :c4, after b2, 14d
```

### v2.7.x — 质量巩固与嵌入验证（2026 Q3）

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| TLS 会话缓存 | P0 | ✅ 已完成 | 重新启用 session cache，默认 2048 条目/24h 超时 |
| 代码质量修复 | P1 | ✅ 已完成 | 修复 L3-L5：gzip 缓存开销追踪、set_max_entries 扩容、注释拼写 |
| 性能基准更新 | P2 | ✅ 已完成 | 10 轮多轮测试，稳态 15K RPS (Silver)，峰值 33K RPS (Gold) |
| ci-fuzz 修复 | P0 | ✅ 已完成 | C11 对齐 + 链接补齐 + fuzz_request 移除 |
| 嵌入验证第二轮 | P1 | ✅ 已完成 | add_subdirectory 集成验证，CMake 依赖可见性修复 |
| 性能回归门禁 | P0 | ✅ 已完成 | v2.7.1 初版为绝对 RPS 阈值；v2.8.x 已替换为同机 head/base 配对（见 v2.8.x 表） |
| 代码评审缺陷修复 | P1 | ✅ 已完成 | 34 项缺陷修复（13 P0/P1 + 21 P2/P3），v2.7.2 |

### v2.8.x — 性能优化与平台扩展（2026 Q4）

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| 新嵌入者接入 | P1 | ✅ 已完成 | 完整嵌入式集成文档（英/中）+ 独立示例 `examples/embedding/` |
| v2.8.0 发布 | P0 | ✅ 已完成 | 零拷贝 writev（/large +72.7%）、第二轮评审 17 项修复、公共 API/构建系统修复（2026-09-23） |
| 回归门禁改造为同机配对 | P0 | ✅ 已完成 | 绝对 RPS 阈值 gate 的是 runner 跨 run 方差（~40%）；改为同 runner head/base 交替 10 轮 + 中位数与多数规则 + fail closed（PR #388） |
| 零拷贝小 body 阈值 | P0 | ✅ 已完成 | writev 对 body < 4096 是负优化（−14%），新增 `UVHTTP_ZEROCOPY_MIN_BODY`（PR #387）；配对门禁实测 `/` +23.4%、`/json` +27.6% |
| 门禁 base 解析排除 nightly | P0 | ✅ 已完成 | nightly 预发布构建自移动中的 main，作 base 会让 head/base ≈ 100% 空测绿灯（PR #391） |
| v2.8.1 补丁发布 | P0 | ✅ 已完成 | tag `b26e9ac`，设为 Latest；v2.8.0 保持 pre-release 不回退。配对门禁实测 vs v2.8.0：`/` +27.7%、`/json` +21.5%、`/large` +1.2% |
| 零拷贝阈值边界测试 | P1 | ✅ 已完成 | 4095/4096/4097 wire 等价性（PR #393），含提前退出与 fd 泄漏修复（PR #396） |
| trend 推送竞态修复 | P1 | ✅ 已完成 | concurrency group + rebase-retry，三次失败 exit 1 而非静默通过（PR #394） |
| brain lint 空占位守卫 | P1 | ✅ 已完成 | `scripts/check-brain.sh` 检查占位/空 compiled_truth/断链，挂 doc-sync-check（PR #394） |
| io_uring 探索 | P2 | ❌ 已关闭（架构不可达） | libuv 1.52 不覆盖 sendfile，uvhttp 静态文件热路径全走 sendfile+线程池；要受益需绕过 libuv 自管 ring fd，属架构重构。基准门禁 3 端点均为内存 body 也验证不到。见 [[io-uring-evaluation]] |
| 内存分配优化 | P2 | ❌ 已关闭（实测收益过低） | keep-alive 3 次分配/请求且无泄漏；唯一可省的 1024B scratch 上限 **0.026%**（12ns ÷ 45.7µs），远低于 runner 方差 ~40%，不做。见 [[alloc-hotpath-measured]] |

### v2.9.x — 生态扩展（2027 Q1）

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| 文档完善 | P1 | ✅ 已完成 | v2.8.0/v2.8.1 release notes、CHANGELOG 双语、API 参考版本头均已同步 |
| Fuzz 测试增强 | P1 | 📋 待办 | 扩展 fuzz 测试覆盖更多协议路径 |
| 社区贡献指南 | P1 | 📋 待办 | 完善 CONTRIBUTING.md、代码评审流程 |

### 最低优先级（长期 / 按需）

> macOS / FreeBSD 平台支持已降级为最低优先级：不进入近期里程碑排期，按需推进。

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| macOS 支持 | 最低 | 📋 待办（无排期） | kqueue 适配、sendfile 兼容、CI 测试；按需推进 |
| FreeBSD 支持 | 最低 | 📋 待办（无排期） | kqueue 已有经验，适配 FreeBSD 差异；按需推进 |

### 完成项（v2.6.x ~ v2.8.x）

- ✅ HTTP/1.1 服务器（CI 记录基线 ~83K RPS；绝对值仅作趋势记录，不当门禁）
- ✅ WebSocket 全双工通信（RFC 6455）
- ✅ TLS 1.2/1.3（mbedtls）
- ✅ TLS 会话缓存（默认 2048 条目/24h，可配置）
- ✅ 零拷贝静态文件（sendfile）
- ✅ LRU 缓存（静态文件 + 压缩）
- ✅ gzip 压缩（RFC 1952）
- ✅ 32-bit 嵌入式支持
- ✅ 限流（令牌桶 + 白名单）
- ✅ ASan/UBSan CI 门禁（101/101 测试通过）
- ✅ 编译时裁剪（36 个选项）
