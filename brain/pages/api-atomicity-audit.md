---
id: api-atomicity-audit
title: "API 原子化审计：按原子性判据逐条定位违约点（不考虑兼容）"
category: decision
status: active
tags: [api, atomicity, orthogonality, audit, v2.10]
created: "2026-10-09T00:49:13"
updated: "2026-11-25T17:43:29"
---

<!-- compiled_truth -->
# API 原子化审计 — 状态更新（2026-11-25）

N1/N2/N3 已落地（见先前 timeline）。本轮：

- **O5 已落地**：删除 `uvhttp_response_set_compress_by_filename` / `_by_content_type` 两个纯包装（内部只是调用已有谓词 + set_compress），消除重复入口。迁移 benchmark、示例与测试到「谓词 + 显式 set_compress + 阈值」。测试改为直接验证 `should_compress_*` 谓词驱动 `set_compress`。
- **原子 API 家族补齐**：新增 `uvhttp_respond_with_headers(response, status, header_view[], count, body, len)` 与 `uvhttp_header_view_t`；`uvhttp_respond` 改为它的一 header 特例，两条路径共享全量校验与失败回滚。新增无发送变体 `uvhttp_response_prepare`，同样校验/回滚但不发送。wire 测试覆盖双 header；变异「跳过 header 写入」→ 红。
- **策略 B 已落地（本轮）**：`set_status` / `set_header` / `set_body` / `send` 从 `include/uvhttp_response.h` 删除，公共 API 只剩 `uvhttp_respond` / `uvhttp_respond_with_headers` / `uvhttp_response_prepare` 三个构造入口。实现保留在 `src/uvhttp_response.c`，声明移到 `src/uvhttp_response_internal.h` 供库内增量场景（压缩追加 header、静态文件多 header、原子入口回滚）和白盒单测使用；符号名未改，避免二进制/源码双重破坏。迁移覆盖 9 个 src 文件、45 个示例、benchmark、35 个单测/手动工具、docs 与 docs/zh 成对代码样例。中间件守卫宏改为通过 `uvhttp_respond` 发送已准备响应，并对 `resp == NULL` 保持无操作（原 `send(NULL)` 语义）。验证：默认构建 0 错误、ctest 130/130、STATIC_FILES=ON 构建 0 错误、BUILD_EXAMPLES/BENCHMARKS 构建 0 错误、`make check-docs` 通过；未删用例，测试数不变。
- **O2（LRU / gzip cache 合并）本轮未执行，留判断依据**：两者 key/value 形状不同（路径+元数据 vs hash+压缩字节），所有操作签名虽逐字同构，但合并需要运行时类型化 value 或把 gzip 重写为 LRU 后端，属于重写而非"还债"，且会触及静态文件与压缩两条热路径。保留两套。

关联 [[production-readiness-push]]、[[api-atomization-orthogonalization]]。


## Timeline

- time: 2026-10-09T00:49:13
  kind: decision
  summary: "Created this page: API 原子化审计：按原子性判据逐条定位违约点（不考虑兼容）"
  source: "2026-10-09 按原子性判据审计全量公共 API"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:49:47
  kind: decision
  summary: "原子化判据与 12 条违约点定位：构造非原子、迁移无回滚、set_body 先释放后分配、无全局状态（唯一干净维度）"
  source: "2026-10-09 逐函数审计 src/ 实现"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:52:29
  kind: evidence
  summary: "A2 实测确认（比原判断更严重）：13 个不同顶层路由 + 任意参数路由 → migrate_to_trie 中途失败 → 全部路由丢失且 find_handler 返回 NULL，无任何报错"
  source: "2026-10-09 独立探针程序实测（4 个 probe，逐个排除后定位）"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:52:56
  kind: decision
  summary: "A2 实测升级为已复现的数据丢失：13+ 顶层路由 + 参数路由致全部路由静默丢失，附机制解剖与修复方向"
  source: "2026-10-09 探针实测 + 机制追至 child_indices[12] 定长上限"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:53:01
  kind: reversal
  summary: "修正早前判断：初版称 A2 仅『面临丢失风险』且迁移后新路由缺失，量级判断不足 —— 实测为已注册路由全部丢失"
  source: "2026-10-09 探针实测推翻"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:56:28
  kind: decision
  summary: "按用户定义重写：原子性=API 不可再分割，正交性=API 无重合。原 C1-C4 事务语义判据作废"
  source: "2026-10-09 用户纠正判据定义后重写"
  affects: [api-atomicity-audit]

- time: 2026-10-09T00:56:32
  kind: reversal
  summary: "判据定义作废重写：此前用的 C1-C4（提交性/顺序无关/幂等/跨对象半状态）是事务原子性，与用户所指（API 不可再分割）无关"
  source: "2026-10-09 用户纠正定义"
  affects: [api-atomicity-audit]

- time: 2026-10-09T01:08:06
  kind: evidence
  summary: "N1 的实证升级：examples/embedding/main.c 的 listen 失败回滚分支在 ASan 下确认 double-free（set_router 已转移所有权，示例仍 router_free）"
  source: "2026-10-09 ASan 探针复现，栈指向 uvhttp_server_free→uvhttp_router_free"
  affects: [api-atomicity-audit]

- time: 2026-10-18T04:23:07
  kind: decision
  summary: "审计第五节两项（config 14 死字段、router 迁移丢路由）标注已修复及对应 commit"
  source: brain update-truth
  affects: [api-atomicity-audit]

- time: 2026-11-24T14:45:56
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 核实 N1/N2 落地状态，纠正过期描述"
  affects: [api-atomicity-audit]

- time: 2026-11-24T17:12:46
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-24 API 原子化推进：N3 落地，O1/O3/O4 状态核实，O4 判据推翻"
  affects: [api-atomicity-audit]

- time: 2026-11-25T09:54:57
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "2026-11-25 全部续：O5 删除便捷包装、原子 API 家族补齐，策略 B/O2 保留判断"
  affects: [api-atomicity-audit]

- time: 2026-11-25T17:43:29
  kind: decision
  summary: "策略 B 已落地：4 个公共 setter/send 移入内部头，公共 API 收敛到 3 个原子入口；全量迁移并验证"
  source: "2026-11-25 策略 B 完整迁移 + 构建/ctest/check-docs 验证"
  affects: [api-atomicity-audit]

- time: 2026-11-25T17:43:29
  kind: decision
  summary: "策略 B 从『破坏面过大未执行』改为已执行：内部头 + 全量迁移，130/130 测试通过"
  source: "2026-11-25 用户授权执行策略 B 后的实现与验证"
  affects: [api-atomicity-audit]
