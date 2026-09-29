---
id: embedding-guide-examples
title: "新嵌入者接入：独立嵌入示例 + 双语接入指南"
category: decision
status: active
tags: [embedding, guide, examples, add_subdirectory]
created: "2026-08-27T09:52:30"
updated: "2026-09-29T01:23:22"
---

<!-- compiled_truth -->
## 交付物
v2.8.x「新嵌入者接入」(P1) 交付：完整嵌入式集成文档 + 独立可运行示例。

1. **独立嵌入示例项目** `examples/embedding/`：CMakeLists.txt（add_subdirectory 方式）+ main.c（最小 hello world 服务器，端口可通过 argv 指定，SIGINT/SIGTERM 优雅退出）+ README.md。该目录**不**挂载到主构建（避免 add_subdirectory 递归），用 `cmake -S examples/embedding -B <build>` 独立构建。
2. **双语接入指南** `docs/guide/EMBEDDING_GUIDE.md`（英）+ `docs/zh/guide/EMBEDDING_GUIDE.md`（中）：三种集成方式（add_subdirectory 推荐 / FetchContent / 系统安装 + find_package）、特性裁剪选项表、故障排查、验证步骤。
3. **文档站点** `docs/.vitepress/config.ts` 双语 sidebar 增加「Embedding Guide / 嵌入指南」入口；CHANGELOG 双语加条目；embedding-checklist 双语第 9 节指向示例与指南。

## 关键发现（嵌入者必知）
1. **`uvhttp_server_free` 清理循环会阻塞**：其内部跑 `UV_RUN_ONCE` 清理 tcp_handle 的 close。若调用方 event loop 中仍残留**其他活跃句柄**（如自建的 `uv_signal_t`）且无 timer/io 事件，`uv_backend_timeout` 返回 -1，`uv__io_poll` 无限阻塞。**修复**：`uv_run` 返回后、调 `uvhttp_server_free` 前，先 `uv_close` 自己的句柄。
2. **不要重复释放 router**：`uvhttp_server_free` 会一并释放其持有的 `server->router`。示例若先 `uvhttp_router_free` 再 `uvhttp_server_free` 会 double-free（悬垂后二次 free）。仓库惯例（`examples/01_basics/01_hello_world.c`）只调 `uvhttp_server_free`。
3. **add_subdirectory 嵌入验证**：`add_subdirectory(${UVHTTP_SOURCE_DIR} uvhttp)`（第二参数指定 binary dir 名）配置/构建/运行/优雅退出全链路通过。
4. **测试目标可以用开关裁掉**（2026-09-29 校正，原主张已过期）：本页原写"根 CMakeLists 无条件 `enable_testing()`、测试目标暂无总开关"。v2.8.0 的 PR #377 引入了 `option(BUILD_TESTS ... ON)`（main `CMakeLists.txt:58`，`:672`/`:967` 处守卫），嵌入者现可 `-DBUILD_TESTS=OFF` 连带 `-DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF` 把测试构建全部裁掉。覆盖类目标在 `ENABLE_COVERAGE` 且 `BUILD_TESTS=OFF` 时给出 warning 并停用。

## 验证
- `cmake -S examples/embedding -B /tmp/embed-build` 配置成功；构建 `embedding_server` 成功。
- 运行 `embedding_server <port>`：`curl` 返回 `Hello from embedded uvhttp!`；`kill -TERM` 打印 `Received signal → Shutdown complete` 后正常退出。


## Timeline

- time: 2026-08-27T09:52:30
  kind: decision
  summary: "Created this page: 新嵌入者接入：独立嵌入示例 + 双语接入指南"
  source: "v2.8.x 新嵌入者接入会话"
  affects: [embedding-guide-examples]

- time: 2026-08-27T09:52:41
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: brain update-truth
  affects: [embedding-guide-examples]

- time: 2026-09-29T01:23:22
  kind: decision
  summary: Rewrote compiled_truth to the new best understanding
  source: "brain 审核会话（2026-09-29）— 对照 main 现状校正过期主张"
  affects: [embedding-guide-examples]
