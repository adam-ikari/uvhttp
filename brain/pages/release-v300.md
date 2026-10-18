---
id: release-v300
title: "发布 v3.0.0：原子化窗口按 MAJOR 改号，跳号豁免撤销"
category: decision
status: active
tags: [release, versioning, api, roadmap-v2.10]
created: "2026-10-18T07:41:03"
updated: "2026-10-18T07:41:21"
---

<!-- compiled_truth -->
# 发布 v3.0.0

## 决定（2026-10-18，用户拍板）

v2.10 API 原子化窗口按 **v3.0.0** 发布，不保留「版本号留在 2.10」的跳号豁免。
兼容策略 B 的另一半 —— 破坏性改动不留 deprecated 兼容层、不做新旧 API 并行 ——
继续有效。

## 判据

`docs/release-strategy.md` 的版本号规则表把「不兼容 API 变更」定为 MAJOR，把 MINOR
限定为「向后兼容的新功能」。该窗口合并的内容全部是删除与改名，没有一条落在 MINOR
的定义里：

- 删 `uvhttp_config_t` 14 个从不被读取的字段与只写不读的 config 旋钮（#472）
- 删 builder 层与 `uvhttp_serve`：`uvhttp_server_builder_t` / `server_create` /
  `server_run` / `stop_simple` / `simple_free` / 动词函数 `uvhttp_get|post|put|delete|any`
  / `set_max_connections|set_timeout|set_max_body_size`（#479）
- 删不带 request 前缀的取值函数 `uvhttp_get_body|header|param`（#470）
- 改名 `set_router` / `set_context` → `take_router` / `take_context`（#470）
- 删内置限流 7 个 API + 8 个 server 字段 + `UVHTTP_FEATURE_RATE_LIMIT`，并删
  `uvhttp_version_info_t.feature_rate_limit`（#483）

最后一项删除的是公共结构体的字段，其后字段前移 —— 这是 **ABI** 破坏，语义化版本号
是它唯一的对外信号渠道。

## 为什么此刻改号是零成本

最新已发布 tag 是 `v2.9.2`，「2.10」从未发布过。现在改号对下游没有迁移成本；反过来
若以 2.10 发出，任何按 `^2.x` 取依赖的嵌入方会在不升 major 的情况下静默拿到这些删除。

## 落地口径

- `VERSION` → 3.0.0（`VERSION_TYPE=major`，`PREVIOUS_VERSION=2.9.2`）
- CHANGELOG 的 `[Unreleased]` 收口为 `[3.0.0]`，条目补齐窗口内 11 个 PR 并带 PR 号
- `docs/release-strategy.md` 增一条规则：版本号按**实际兼容性**定，不按起草时给窗口
  起的名字定
- 公共 API 的出处标记（`@note Added in ...`、`Renamed from ... in ...`）里的 `v2.10`
  全量改为 `v3.0.0`（约 60 处、47 个文件）。历史叙述保留旧名：CHANGELOG 引言、
  release-strategy 的改号说明、`VERSION_NAME` 里的「原 v2.10」

## 记法约定

brain 页与 roadmap 时间线里此前写下的 `v2.10` 是当时对该窗口的称呼，按原样保留 ——
改号由本条与 `api-atomization-orthogonalization` 的 reversal 时间线项记录，不追溯改写
历史条目。


## Timeline

- time: 2026-10-18T07:41:03
  kind: decision
  summary: "Created this page: 发布 v3.0.0：原子化窗口按 MAJOR 改号，跳号豁免撤销"
  source: "2026-10-18 用户拍板：发布 3.0.0"
  affects: [release-v300]

- time: 2026-10-18T07:41:21
  kind: decision
  summary: "把原 v2.10 原子化窗口的发布号定为 v3.0.0：兼容性事实决定版本号，跳号豁免撤销"
  source: "2026-10-18 用户拍板：发布 3.0.0"
  affects: [release-v300]
