---
id: v300-mechanism-audit
title: "v3.0.0 后机制有效性审计：开关双向生效、入口唯一、文档可执行"
category: decision
status: active
tags: [audit, cmake, packaging, api, docs]
created: "2026-10-28T14:54:26"
updated: "2026-10-28T14:54:38"
---

<!-- compiled_truth -->
# v3.0.0 后机制有效性审计

## 判据（用户要求「每一个机制都是有效而且极简的」）

- **有效**：开关必须**双向**生效（ON 有增量，OFF 真能关）；公共入口必须有真实
  执行路径；文档给出的接入方式必须能原样跑通。
- **极简**：一个能力只有一个入口；不被任何构建路径 include 的文件、没有调用方
  的脚本、与实现不符的文档示例，都算机制债务，删掉或修正，而不是留着「备用」。

审计范围：CMake option / 常量、公共头文件与导出符号、安装与 find_package /
pkg-config、Makefile 目标、scripts、构建产物、活跃文档示例。

## 已修的机制缺陷（2026-10-28）

### CMake 开关
- `ENABLE_HARDENING` / `ENABLE_STACK_PROTECTION` / `ENABLE_FORTIFY_SOURCE`
  此前只有一半有效：`-fstack-protector-strong` 与 `-Wl,-z,relro,-z,now` 被
  **无条件**写进 CMAKE_C_FLAGS/链接标志，选项块里又重复加一遍。结果 ON 无增量、
  OFF 关不掉。现在基础 flags 只保留编译警告，硬化标志全部由开关 gate；
  已用 ON/OFF 两次 configure 对比 flags.make 与 link.txt 验证。
- `CMAKE_BUILD_TYPE=Debug` 此前走 else 分支得到 `-O2 -DNDEBUG -g`（`make build`
  默认就是 Debug，等于假 Debug，assert 全失效）。现在 Debug 分支输出
  `-O0 -g` 且不定义 NDEBUG；Release 仍为 `-O2 -DNDEBUG`。
- 删除未声明且指向不存在目录的 `BUILD_INTEGRATION_TESTS` /
  `BUILD_PERFORMANCE_TESTS` 条件块（`test/integration`、`test/performance`
  已不存在）。
- `BUILD_TESTS` 描述去掉「integration」。

### 安装包与消费者路径
- **find_package 消费者必须链接 `uvhttp::uvhttp`**。裸 `uvhttp` 会被 CMake 当成
  `-luvhttp`，既不传播 INTERFACE include dirs，也不传播特性宏；导入目标本身
  一直是对的，错的是 `UVHTTP_LIBRARIES` 变量与 INSTALL_CMAKE 文档教了裸名。
  `cmake/uvhttp-config.in.cmake` 的 `UVHTTP_LIBRARIES` 已改为
  `uvhttp::uvhttp`，文档同步。已用临时前缀安装 + 最小消费者
  `find_package(uvhttp REQUIRED)` 编译运行验证（输出 `version=3.0.0 static=0
  ws=1 tls=1 lru=0`，与库编译配置一致）。
- pkg-config 的 `Libs.private` 已包含静态依赖链（libuv/xxhash/llhttp/
  mbedtls/miniz/pthread/m/dl），Cflags 带全部 ABI 宏。

### 删除的未接线/僵尸机制
- `cmake/UserOptions.cmake`：README 教用户复制成 `UserOptions.local.cmake` 并用
  `-DCMAKE_USER_CONFIG=ON`，但顶层 CMakeLists 从未 include 它；模板里还有
  不存在的 `BUILD_WITH_TLS`、只打印不应用的开关。文件与 README 段落一起删除。
- `test/CMakeLists.txt`：顶层从不 `add_subdirectory(test)`，单元测试由顶层
  GLOB 直接建目标，该文件是死文件（原内容还指向不存在的集成/性能目录）。
- `test/unit/test_static_api_new.c`：gtest 代码写成 `.c`，顶层 GLOB 只收
  `.cpp`，从未编译，且引用已删除的 `enable_etag`/`enable_last_modified`。
  其覆盖已被 `test_static_api_coverage.cpp` 重叠。
- `scripts/check_links.sh`：外部 URL 检查脚本，只打印 ✗ 不设失败退出码，无任何
  调用方；与有效的 `scripts/check-links.sh`（内部链接、失败退出 1）重名近似。
  删除旧脚本并修正 `scripts/README.md`。
- Makefile：`docs-preview` 调不存在的 `npm run dev` → `npm run docs:dev`；
  `bench` 提示不存在的 `test_performance_e2e` → `benchmark_unified`；
  `check-syntax` 用 c99 且缺 xxhash include 路径，修好后才能通过；
  `cmake-options` 补全现存开关。
- 公共代码里已在本轮更早删除：TLS stats API/struct、静态 `enable_etag` /
  `enable_last_modified` / `enable_sendfile` 与 cache 开关、WebSocket
  `send_buffer` 字段、`uvhttp_context_t.config_callback`。对应测试同步删除。

### 文档可执行性
- `ENGINEERING_STANDARDS.md`（EN/ZH）：`server->stats.*` 不存在 → 改为
  context 的 `active_connections` / `total_requests` / `total_connections`；
  `config->keep_alive_timeout` 不存在 → 删除并说明 Keep-Alive 自动管理；
  `xxhash64()` → `uvhttp_hash()`。
- `INSTALL_CMAKE.md`（EN/ZH）：find_package 后改用 `uvhttp::uvhttp`；删掉
  冗余的 include/宏手动设置；删除已不存在的「Feature Flags」手动
  `add_definitions` 示例（target 已 PUBLIC 传播）。
- `REQUEST_FILTER_API.md`（EN/ZH）：迁移表中的 `uvhttp_has_feature` 改为真实
  存在的 `uvhttp_is_feature_enabled`；CHANGELOG 与 v3.0.0 release notes 同步修正。
- `EMBEDDING_GUIDE.md`（EN/ZH）：FetchContent 示例 tag 从 v2.8.1 更新到 v3.0.0。
- 静态 `max_file_size=0` 现在表示「使用内置 1GB 默认值」，缓存与请求路径共用
  同一上限（`uvhttp_lru_cache_set_max_file_size`），并有测试锁定。

## 保留的取舍（有意不修）

- **静态 Cache-Control 的 max-age 用 `UVHTTP_CACHE_DEFAULT_TTL`**：它是可配置
  的编译期常量，且与 LRU 条目 TTL 共用一个值；没有独立的 HTTP 缓存头配置项，
  加一个只会扩大表面。语义耦合已在文档说明。
- **middleware STOP 不短路 handler 体**：见
  [[middleware-stop-does-not-short-circuit-handler]]，仍是已知行为。
- **pkg-config 的 prefix 在 configure 时固定**：`cmake --install --prefix`
  不会重写 .pc 中的 prefix。自定义前缀应在 configure 时传
  `CMAKE_INSTALL_PREFIX`。
- **约 87 个公共函数未被库内部调用**：这是对外 API 面（router、请求/响应、
  WebSocket 发送等），有测试覆盖，不是死代码；`uvhttp_custom_alloc*` 是
  allocator type 2 的扩展点，保留。
- `justfile` 仍存在但不再作为推荐入口（[[build-system-make-cmake]]）；内容未
  引用已删除选项。

## 验证证据

- Debug（static files ON）与 Release（默认配置）各 119/119 ctest 通过。
- ASan 构建 119/119 通过。
- VitePress `DEPLOY=pages npm run docs:build` 成功；`scripts/check-links.sh`
  报告所有内部链接有效。
- find_package 最小消费者编译并运行，特性查询与库配置一致。
- CMake 开关 ON/OFF 的 flags.make / link.txt 对比验证。
- doc-sync 门禁仅剩 `guide/introduction.md` 的时间戳差异，需随提交更新
  ZH 文件后由 CI 判定。


## Timeline

- time: 2026-10-28T14:54:26
  kind: decision
  summary: "Created this page: v3.0.0 后机制有效性审计：开关双向生效、入口唯一、文档可执行"
  source: "2026-10-28 用户要求：确保 uvhttp 的每一个机制都是有效而且极简的"
  affects: [v300-mechanism-audit]

- time: 2026-10-28T14:54:38
  kind: decision
  summary: "写入审计判据、已修机制缺陷清单、保留取舍与验证证据"
  source: "2026-10-28 机制审计（代码、CMake、打包、文档）"
  affects: [v300-mechanism-audit]
