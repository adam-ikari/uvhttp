---
id: embedding-cmake-public-deps
title: "嵌入构建修复：依赖链接可见性 PUBLIC + uthash include 路径"
category: decision
status: active
tags: [embedding, cmake, build, add_subdirectory]
created: "2026-08-25T03:18:24"
updated: "2026-09-29T02:26:32"
---

<!-- compiled_truth -->
## 发现
嵌入验证第二轮（`add_subdirectory` 方式）发现：uvhttp 的 public headers 包含 `llhttp.h`、`<uv.h>`、`xxhash.h`、`uthash.h`、mbedtls 头文件，但这些依赖在 CMake 中被标记为 `PRIVATE` 链接，导致嵌入者项目无法编译（找不到头文件）。

## 修复
1. **CMakeLists.txt**: `libuv`, `xxhash`, `llhttp` 从 `PRIVATE` → `PUBLIC`，其 IMPORTED target 的 `INTERFACE_INCLUDE_DIRECTORIES` 传播给嵌入者。
2. **CMakeLists.txt**: `mbedtls` 从 `PRIVATE` → `PUBLIC`（`uvhttp_tls.h` 是 public header 且包含 mbedtls 头）。
3. **CMakeLists.txt**: `target_include_directories(uvhttp PUBLIC ...)` 添加 `deps/uthash/src`（uthash 是 header-only，无 IMPORTED target）。

## 验证边界（照此判断证据强度，别把"编译通过"读成"功能验证通过"）
- 当时用独立测试项目（`/tmp/embed-test`）经 `add_subdirectory` 集成：**编译通过**、服务器启停正常（timer 回调停止）、101/101 单元测试通过（GCC + C11）。
- **当时的 HTTP 实际响应验证没做成**，受环境代理干扰；那部分覆盖靠仓库自身 e2e 测试套件。
- 运行时缺口后来由 `examples/embedding/` 补齐：`curl` 拿到 `Hello from embedded uvhttp!`、`SIGTERM` 优雅退出（见 `embedding-guide-examples`）。

## 结论与状态
- `add_subdirectory` 集成：验证通过（配置/构建/起停/响应）。
- `FetchContent`：**两种方式都已验证**（2026-09-28 补）。`SOURCE_DIR` 模式端到端通过；git-fetch 模式必须显式给 `GIT_TAG main`（默认 `master` 不存在），代价是要拉 8 个 submodule（mbedtls 100MB+），重。
- 裁剪测试构建用 `BUILD_TESTS=OFF`（v2.8.0 PR #377），见 `embedding-guide-examples`。


## Timeline

- time: 2026-08-25T03:18:24
  kind: decision
  summary: "Created this page: 嵌入构建修复：依赖链接可见性 PUBLIC + uthash include 路径"
  source: "嵌入验证第二轮会话"
  affects: [embedding-cmake-public-deps]

- time: 2026-08-25T03:18:46
  kind: decision
  summary: "嵌入验证第二轮发现：依赖链接可见性修复"
  source: "嵌入验证第二轮会话"
  affects: [embedding-cmake-public-deps]

- time: 2026-08-27T09:56:04
  kind: decision
  summary: "FetchContent 集成方式已验证可用：SOURCE_DIR 模式端到端通过；git-fetch 模式需显式 GIT_TAG main（默认 master 不存在），8 个 submodule 拉取较重（mbedtls 100MB+）"
  source: "v2.8.x 新嵌入者接入会话"
  affects: [embedding-cmake-public-deps]

- time: 2026-09-29T01:23:22
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）— 对照 main 现状校正过期主张"
  affects: [embedding-cmake-public-deps]

- time: 2026-09-29T02:26:32
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "PR #389 第二轮修订 — 补回被压缩掉的验证边界（哪些是编译验证、哪些是运行时验证）"
  affects: [embedding-cmake-public-deps]
