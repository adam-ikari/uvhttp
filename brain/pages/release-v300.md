---
id: release-v300
title: "发布 v3.0.0：原子化窗口按 MAJOR 改号，跳号豁免撤销"
category: decision
status: active
tags: [release, versioning, api, roadmap-v2.10]
created: "2026-10-18T07:41:03"
updated: "2026-10-27T14:17:31"
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

## 发布结果（2026-10-20）

v3.0.0 已发布并转为 latest，`--post` 6/6 通过（线上 `versions.json` current=3.0.0）。

过程比预期多出一次 tag 重打，原因值得记住：

1. **首发**：tag `045a160` → prerelease 07:12Z。两条 release 门禁零运行记录。
   直接原因是 tag 内 workflow 写着无效的 `types: [prerelease]`（0dafa69 引入）。
2. **第一次修复**（#485）：改 `types: [prereleased]`。重建 prerelease 后仍然零运行 ——
   `prereleased` 不覆盖「从 draft 发布的预发布」，而 `gh release create --prerelease` 正走该路径。
3. **第二次修复**（#486）：改回 `published`。但 release 事件使用的是 **tag 提交上的
   workflow 文件**，不是默认分支；改 main 对已存在的 tag 无效。用临时探针 tag
   （`05ed4a3`）验证后，把 v3.0.0 重打到 `8376f1b`（#486 的 merge commit）。
4. **重打后的 tag** 相对 `045a160` 只多出 CI/构建修复：cJSON 的 CMake 4 policy flag、
   空 CLOSE 帧缓冲区零初始化、workflow 触发器与 env、文档/大脑。无 API 变化。

门禁结果（tag `8376f1b`）：

- `ci-release-gate`：9/9 job 全绿（fast / 各配置矩阵 / ASan）
- `ci-benchmark`：paired base = `v2.9.2`；`/` 100.8%、`/json` 102.0%、`/large` 101.4%，全部 PASS
- `scripts/ci/release_checklist.sh --gates`：5/5
- 转 latest 后 `--post`：6/6


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

- time: 2026-10-19T14:08:01
  kind: decision
  summary: "评审两处修正：(1) CHANGELOG 的 3.0.0 段补齐 2.9.2 之后漏记的行为修复（#449 prewarm 泄漏、#457 拒绝空 header 名、#464 手工 Content-Length 与压缩的分帧错位）与测试/CI 两项分段，EN/ZH 分段条数逐项对齐；详见 [[release-checklist-enforced]] 的覆盖面缺口。(2) docs/zh/dev/ROADMAP.md 把 v3.0.0 预留给 2027 Q1 的「重大更新（计划中）」，与本次发布撞号，改为其后一号；该文件其余内容是废弃状态（自称 v2.0.0 为当前版本、v2.1.0 计划发布于 2026-03-15），维护版路线图是 docs/guide/ROADMAP.md，废弃件留待单独决定重写或删除，发布 PR 里不顺手清理。"
  source: "v3.0.0 发布评审（2026-10-19）"
  affects: [release-v300]

- time: 2026-10-20T17:16:29
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "v3.0.0 发布完成（2026-10-20）"
  affects: [release-v300]

- time: 2026-10-20T17:16:29
  kind: decision
  summary: "v3.0.0 正式发布：tag 重打至 8376f1b（含 CI/构建修复，无 API 变化）；release 门禁 9/9、benchmark 对 v2.9.2 全 PASS、--gates 5/5、--post 6/6，已转 latest"
  source: "GitHub Release v3.0.0；runs 39460475038 / 39460475053"
  affects: [release-v300, runner-toolchain-drift, perf-regression-gate]

- time: 2026-10-27T14:17:31
  kind: note
  summary: "发布正文修正：PR 计数 35→37，#485/#486 在 tag 内但正文漏记；补 CMake 4 构建修复（CMAKE_POLICY_VERSION_MINIMUM）与 published 触发器修正两条。日期口径写入 release-strategy：仓库内（CHANGELOG/versions.json/版本历史）记打 tag 日期 2026-10-18，GitHub publishedAt 是转 latest 日期 2026-10-20，两者不必一致。"
  source: "v3.0.0 发布后评审（本次 PR）"
  affects: [release-v300]
