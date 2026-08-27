---
id: release-v271
title: "发布 v2.7.1"
category: decision
status: active
tags: [release, embedding, ci]
created: "2026-08-26T11:03:17"
updated: "2026-08-27T07:57:45"
---

<!-- compiled_truth -->
# v2.7.1 发布记录

## 发布时间
2026-08-26，VERSION_TYPE=patch，质量与嵌入修复补丁版（PR #368 合并，c46b9cf）

## 包含内容
- **ci-fuzz CI 修复（PR #364）**：ci-fuzz 连续失败 5 天（2026-08-20~24）。根因：ci-fuzz 是唯一用 clang 编译的工作流，项目默认 C99 + `-Werror`，clang 对同 TU 内重复 typedef 报 `-Wtypedef-redefinition`（GCC 不报，主 CI 一直绿）
  - `CMAKE_C_STANDARD` 99 → 11（与 PHILOSOPHY.md "实际构建使用 C11" 对齐）
  - 删除未使用的 `uvhttp_validate_buffer_state`（clang `-Wunused-function` 报错）
  - fuzz_router 链接补齐 `libminiz.a` / `libxxhash.a`
  - 移除引用已不存在 API 的过时 `fuzz_request` harness
  - 验证：fuzz_router 5s 跑 442K execs，0 crash，0 timeout
- **嵌入构建修复（PR #365）**：public headers 依赖（`llhttp.h`、`<uv.h>`、`xxhash.h`、`uthash.h`、mbedtls）在 CMake 中被标为 `PRIVATE`，嵌入者 `add_subdirectory` 集成时找不到头文件
  - `libuv`/`xxhash`/`llhttp`/`mbedtls` 从 `PRIVATE` → `PUBLIC` 链接
  - PUBLIC include dirs 添加 `deps/uthash/src`（uthash 为 header-only 库）
  - 验证：嵌入测试项目 add_subdirectory 构建成功，服务器启动/停止正常
- **性能回归门禁（PR #366）**：新增 `scripts/performance/regression_check.py`
  - CI 基准测试后自动对比内置基线（`/` 83K, `/json` 81K, `/large` 5.7K RPS）
  - 阈值 10%，RPS 低于基线 90% 则 CI 失败
  - 仅在 PR 和 pre-release push 触发，`workflow_dispatch` 不阻塞（开发期手动基准）
  - 验证：PASS 场景 exit=0；FAIL 场景（70K RPS）exit=1 正确报错

## 门禁结果
- 单元测试 101/101 通过
- ASan gate PASS、UBSan gate PASS
- build-matrix（5 组合）全部通过
- ci-fuzz 修复后本地验证通过
- 嵌入验证 add_subdirectory 构建成功

## 兼容性
- 零破坏性变更：patch release，API 完全向后兼容，现有代码升级无需修改

## 文档
- `docs/guide/CHANGELOG.md`：新增 v2.7.1 条目
- `docs/embedding-checklist.md`：勾选 4 项构建集成验证项
- Brain roadmap：更新 v2.7.x 完成项（ci-fuzz / 嵌入验证第二轮 / 性能回归门禁）

## 下一步
- v2.8.x 聚焦性能优化与平台扩展：io_uring 探索（P2）、内存分配优化（P2）、新嵌入者接入（P1）、macOS/FreeBSD 支持（P3）


## Timeline

- time: 2026-08-26T11:03:17
  kind: decision
  summary: "Created this page: 发布 v2.7.1"
  source: "v2.7.1 发布会话"
  affects: [release-v271]

- time: 2026-08-27T07:57:45
  kind: decision
  summary: "填写 v2.7.1 发布记录：ci-fuzz C11 修复（PR #364）、嵌入 CMake 依赖可见性 PUBLIC（PR #365）、性能回归门禁（PR #366）"
  source: docs/releases/v2.7.1-release-notes.md
  affects: [release-v271]
