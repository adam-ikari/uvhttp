---
id: release-v280
title: "发布 v2.8.0"
category: decision
status: active
tags: [release, performance, quality]
created: "2026-09-23T04:16:34"
updated: "2026-09-23T04:17:39"
---

<!-- compiled_truth -->
# v2.8.0 发布记录

## 发布时间
2026-09-23，VERSION_TYPE=minor，版本名「性能优化与质量加固」。走两阶段流程（[[release-process-benchmark-gate]]）：release PR 合并 main → tag `v2.8.0` → `gh release create --prerelease` 触发 ci-benchmark 回归门禁 → 门禁绿后 `gh release edit --latest` 转正式，main push 触发文档部署。

## 包含内容
- **大响应零拷贝 writev（PR #378）**：header+body 组装 iovec 经 `uv_write` 一次 writev 发送，消除每请求 200KB memcpy 与两次 100KB 分配，/large RPS **5140 → 8881（+72.7%）**；TLS 与非法 client 自动回退原拷贝路径；/large 回归基线更新至 8800（PR #385）
- **第二轮代码评审 17 项缺陷修复（PR #380，63f29bd）**：lru_cache OOM 路径 UAF 与淘汰死循环、config 双归属 double-free、`uvhttp_strerror` 覆盖全部 84 个错误码、gzip 替换路径预算绕过、JSON 注入转义、敏感词过滤、版本 fallback 漂移
- **公共 API / 构建系统高危修复（PR #381，8da889f）**：install 不再发布第三方头与静态库、特性宏与分配器类型 PUBLIC 传播消除消费者 ABI 错配、`uvhttp-config.cmake`/`uvhttp.pc` 重写使 `find_package` 与 pkg-config 全链路可用
- **基准新增 SSE / 流式 / WebSocket 维度（PR #379，028f2f1）**：`benchmark/ws_benchmark_client.py`，/sse ~60-68K 流/s、/stream ~9.5-10.8K req/s
- **嵌入与 CI 门禁（PR #373/#375/#376/#377/#385）**：`BUILD_TESTS` 编译选项、UBSan 门禁恢复 101/101 零发现、PR 占位门禁真实化（cppcheck 真实现）、examples 纳入 CI 编译、llhttp submodule `ignore = dirty`

## 发布会话补充（收尾阶段完成）
- **版本引用同步**：`src/uvhttp_version.c` 非 CMake 构建的版本 fallback（2.7.2→2.8.0，MINOR 7→8、PATCH 2→0）、README/README_CN（badge、关键指标标题、示例串、路线图 v2.8.0 转已发布 + 未完成项并入 v2.9.0、版本历史表加行）、`docs/api/API_REFERENCE.md` 版本头、嵌入指南 `GIT_TAG` v2.7.1→v2.8.0（英/中）
- **修复 ci-benchmark 发布门禁回归**：PR #373（e213557）已把触发器改为 `release: [published]`，但 PR #379（028f2f1）基于改动前的文件把它覆盖回 `push: branches: [pre-release]` 死配置——若不恢复，pre-release 不会触发回归门禁，两阶段流程会静默跳过门禁。本次恢复触发器、job `if`、gate 步骤 `if`、趋势落库条件（仅 `prerelease == true`）
- **门禁结果**：Debug / ASan / UBSan 三套 101/101 通过且零 sanitizer 发现；VitePress 文档构建通过（`docs/api/generated` 由 doxygen + `api:generate` 产出，属 gitignore 生成物，本地从主工作区补齐）；`check-doc-sync` 31/31 同步、`check-links` 全绿

## 兼容性
- 零破坏性变更：minor release，运行时 API 完全向后兼容
- 构建行为变化：`make install` 不再安装第三方依赖头文件与静态库；特性宏 PUBLIC 传播后消费者与库的特性配置强制一致
- 回归基线变化：/large 5700 → 8800 RPS

## 下一步
- v2.8.x：io_uring 静态文件路径探索（P2）、内存分配优化（P2）
- 已知待办：`format-check` 门禁在 pull_request 事件下空转（见 [[ci-format-check-gate-noop]]），修复需先清理存量格式漂移

关联 [[release-process-benchmark-gate]]、[[perf-regression-gate]]、[[release-v272]]


## Timeline

- time: 2026-09-23T04:16:34
  kind: decision
  summary: "Created this page: 发布 v2.8.0"
  source: "v2.8.0 发布会话"
  affects: [release-v280]

- time: 2026-09-23T04:17:39
  kind: decision
  summary: "v2.8.0 发布记录：包含内容、发布会话补充（版本同步与门禁回归修复）、门禁结果、兼容性与下一步"
  source: "v2.8.0 发布会话"
  affects: [release-v280]
