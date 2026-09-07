---
id: embedding-cmake-public-deps
title: "嵌入构建修复：依赖链接可见性 PUBLIC + uthash include 路径"
category: decision
status: active
tags: [embedding, cmake, build, add_subdirectory]
created: "2026-08-25T03:18:24"
updated: "2026-08-27T09:56:04"
---

<!-- compiled_truth -->
## 发现
嵌入验证第二轮（`add_subdirectory` 方式）发现：uvhttp 的 public headers 包含 `llhttp.h`、`<uv.h>`、`xxhash.h`、`uthash.h`、`mbedtls` 头文件，但这些依赖在 CMake 中被标记为 `PRIVATE` 链接，导致嵌入者项目无法编译（找不到头文件）。

## 修复
1. **CMakeLists.txt**: `libuv`, `xxhash`, `llhttp` 从 `PRIVATE` → `PUBLIC`。这些 IMPORTED targets 的 `INTERFACE_INCLUDE_DIRECTORIES` 现在传播给嵌入者。
2. **CMakeLists.txt**: `mbedtls` 从 `PRIVATE` → `PUBLIC`（`uvhttp_tls.h` public header 包含 mbedtls 头文件）。
3. **CMakeLists.txt**: `target_include_directories(uvhttp PUBLIC ...)` 添加 `deps/uthash/src`（uthash 是 header-only 库，无 IMPORTED target）。

## 验证
- 嵌入测试项目（`/tmp/embed-test`）通过 `add_subdirectory` 集成 uvhttp，编译成功。
- 服务器启动/停止正常（timer 回调停止）。
- HTTP 响应验证受限于环境代理问题，但项目自身 e2e 测试套件已覆盖。
- 101/101 单元测试通过（GCC + C11）。

## 结论
嵌入者可以通过 `add_subdirectory` 方式集成 uvhttp，编译正常。`FetchContent` 方式待后续验证。


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
