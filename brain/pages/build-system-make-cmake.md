---
id: build-system-make-cmake
title: "构建系统定位：CMake + Make，弃用 Just 推荐"
category: decision
status: active
tags: [build-system, cmake, make, just, embedded]
created: "2026-08-21T12:01:42"
updated: "2026-09-29T01:22:57"
---

<!-- compiled_truth -->
## 决策
**CMake 是唯一构建真源**（`CMakeLists.txt` + `cmake/Dependencies.cmake`，`CMAKE_C_STANDARD 11`）。Makefile 只是 cmake 的直接包装（文件头自述 "Direct cmake wrapper — no dependency on GNUmakefile"），提供 `build / build-release / build-coverage / test / bench / verify-memory-safety / docs` 等入口，不含独立构建逻辑。

**`just` 不再作为推荐入口**：`justfile` 仍在仓库里，但 README 的 Prerequisites 已把它写成 "Optional, for developers who prefer `just`"，面向用户的构建文档一律走 cmake/make。维护三套等价入口只会让配置项（36 个编译时裁剪选项）出现口径分裂，尤其影响嵌入者与 CI 的一致性。

## 对嵌入者的含义
嵌入路径以 CMake 为准：`add_subdirectory`（推荐）/ `FetchContent` / 系统安装 + `find_package`，见 `docs/guide/EMBEDDING_GUIDE.md` 与 `embedding-cmake-public-deps`、`embedding-guide-examples`。

## 判据
新增构建便利层可以做，但不得成为第二份配置真源——凡 CMake 有而包装层没有的选项，视为缺陷。


## Timeline

- time: 2026-08-21T12:01:42
  kind: decision
  summary: "Created this page: 构建系统定位：CMake + Make，弃用 Just 推荐"
  source: Build system review
  affects: [build-system-make-cmake]

- time: 2026-09-29T01:22:57
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）回填 — 依据 commit aecb053 / cf1d456 / docs/PERFORMANCE_TARGETS.md"
  affects: [build-system-make-cmake]
