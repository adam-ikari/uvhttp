---
slug: roadmap
title: Roadmap
role: milestones
updated: "2026-10-09T00:53:21"
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
  文档多语言完善            :done, c1, after b2, 14d
  Fuzz 测试增强            :done, c4, after b2, 14d
  社区贡献指南             :done, c4b, after c4, 3d
  测试基建清理              :done, c4c, after c4b, 1d
  section v2.10.x — API 原子化与正交化（特性冻结）
  router 迁移致路由全丢（修复） :active, d1, 2026-10-09, 2d
  配置静默失效全线（14 字段）  :active, d2, after d1, 4d
  限流语义修正 per-client      :pending, d3, after d2, 3d
  set_body 原子性 + cache setter 校验 :pending, d4, after d3, 2d
  构造/释放原子化              :pending, d5, after d4, 5d
  所有权进类型 + builder 收敛   :pending, d6, after d5, 5d
  子模块独立生命周期            :pending, d7, after d6, 7d
  API 参考与示例同步           :pending, d8, after d7, 3d
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

### v2.9.x — 生态扩展（2026 Q4 — 2027 Q1）

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| 文档完善 | P1 | ✅ 已完成 | v2.8.0/v2.8.1 release notes、CHANGELOG 双语、API 参考版本头均已同步 |
| Fuzz 测试增强 | P1 | ✅ 已完成 | `fuzz_static_path` 补路径解析面（#408）——变异验证证明 harness 有牙齿（移除包含检查后 1s 抓逃逸），60s 自由 fuzz 未发现可利用穿越；`lru_cache` dead-store 修复（#409）让四 harness 在 CI 稳定跑 |
| 社区贡献指南 | P1 | ✅ 已完成 | 代码审查清单 + 测试形态选择（#401）；补 integration 真相、assert 在 Release 下失效、本地构建盲区（#410） |
| 测试基建清理 | P2 | ✅ 已完成 | `test/integration/` 19 个文件移到 `manual/`，删 4 个纯 assert 文件（42 处断言在 NDEBUG 下全失效，exit=0 是虚假绿灯）；CMake glob 同步（#411） |
| 行为测试补全 | P1 | ✅ 已完成 | 11 组 132 个测试（v2.9.2）：JSON 错误转义、连接 close、TLS 证书三 PR（#461-#463） |
| 测试有效性治理 | P0 | ✅ 已完成 | 静默跳过 101 → 18（#453-#456）；恒绿零验证修复（#457）；TLS 覆盖率 60.3% → 67.9%（#461-#463）；PR 测试移至 pre-release 门禁（#465） |
| Sanitizer 盲区修复 | P0 | ✅ 已完成 | STATIC_FILES（#449）与 ROUTER_CACHE（#468）纳入 ASan/UBSan/coverage——此前 feature 默认 OFF 致守卫源文件整文件不编译 |

### v2.10.x — API 原子化与正交化（特性冻结）

**方向已定：不加新特性。** 详见 [[api-atomization-orthogonalization]]。
理由：性能、测试、CI 三条抓手在 v2.9.x 已各自走到实测收益为零或治理完毕，
剩下唯一有价值的抓手是 API 自身的形状。

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| **router 迁移致路由全丢** | **P0** | **📋 下一步（已实测复现）** | **13+ 个不同顶层路由 + 任意 `:param` 路由 → `migrate_to_trie` 中途失败 → 全部已注册路由静默丢失，`find_handler` 返回 NULL。** 机制：`child_indices[12]` 定长上限撞顶后，失败分支 detach 并释放 `old_routes`，但已迁入 trie 的条目因 `use_trie` 仍为 0 而永不可达。当前**无测试覆盖** |
| 配置静默失效全线 | P0 | 📋 待办 | 诊断已修正：`uvhttp_config_t` 26 字段中 **14 个从不被执行路径读取**（3 个"仅写不读" + 11 个零读点）。`update_size_limits` / `set_max_body_size` 返回 `UVHTTP_OK` 并打 INFO 日志，实际执行用编译期常量 —— **零应用且报告成功**。非"双通道矛盾" |
| set_body 原子性 | P1 | 📋 待办 | `uvhttp_response_set_body` 先 `free` 旧 body 再 `alloc`，OOM 下旧值不可恢复。同对象上 `set_header` 的 realloc 路径却是原子的 —— 两个 setter 失败语义不一致 |
| cache setter 入参校验 | P2 | 📋 待办 | `lru_cache_set_max_entries` / `set_cache_ttl` 等直接赋值无范围校验，不变量不在写入点建立（response 系已是正确范式，可作模板） |
| 限流语义修正 | P0 | 📋 待办 | 签名 per-client（`client_ip` 参数）实现 per-server 全局单计数；`reset_rate_limit_client` 注释自承 "simplified implementation: reset entire server's"。A 打满会连带 429 B |
| 构造/释放原子化 | P1 | 📋 待办 | `uvhttp_server_create` 错误回滚靠手工置 `server->config/router = NULL`，相邻分支一会儿置一会儿不置，正确性靠注释维护 |
| 所有权进类型 + builder 收敛 | P1 | 📋 待办 | **破坏性**：`set_router`/`set_context` 接管所有权只写在 `@note`；`uvhttp_get/post/put/delete` 等 10 个全局函数当 builder 方法，**随 builder 一并移除**（`uvhttp_delete` 命名空间污染随之消失） |
| 子模块独立生命周期 | P2 | 📋 待办 | 限流 / TLS / WS / 压缩内联在同一个 6-cache-line `server` struct 内，`#if` + `_padding` 拼布局 |
| 构造入口收敛 | P2 | 📋 待办 | **破坏性**：`server_new` / `server_new_with_loop` / `server_create` 三个平级入口删成只剩一个，差别只在"谁管 loop" |
| API 参考与示例同步 | P1 | 📋 待办 | 改造后 `docs/` API 参考与 `examples/` 需同步 |

> 完整的原子性判据、12 条违约点与清理清单见 [[api-atomicity-audit]]。

> ✅ **兼容策略已定：直接破坏性改（B）**（2026-10-09）。不留 deprecated 兼容层，
> 不做新旧 API 并行 —— 双份测试与"哪个是新的"心智负担正是本轮要消灭的东西。
> 一次改干净，v2.10 即干净的新 API，3.0 不再背清理义务。版本号保留 2.10
> 只为不跳号，靠 CHANGELOG + 迁移说明交代。

### 最低优先级（长期 / 按需）

> macOS / FreeBSD 平台支持已降级为最低优先级：不进入近期里程碑排期，按需推进。
> 在 API 原子化完成前，平台适配会放大改动面，故排在后面。

| 目标 | 优先级 | 状态 | 说明 |
|------|--------|------|------|
| macOS 支持 | 最低 | 📋 待办（无排期） | kqueue 适配、sendfile 兼容、CI 测试；按需推进 |
| FreeBSD 支持 | 最低 | 📋 待办（无排期） | kqueue 已有经验，适配 FreeBSD 差异；按需推进 |

### 完成项（v2.6.x ~ v2.9.x）

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
