---
id: ci-fuzz-c11-fix
title: "ci-fuzz 修复：C11 对齐 + 工作流补齐"
category: decision
status: active
tags: [ci, fuzz, c11, clang]
created: "2026-08-25T01:24:41"
updated: "2026-08-25T01:25:00"
---

<!-- compiled_truth -->
## 根因（连续失败 5 天：2026-08-20 ~ 08-24）
ci-fuzz 是唯一用 clang 编译的工作流。项目 `USER_WARNINGS_AS_ERRORS` 默认 ON 且 `CMAKE_C_STANDARD 99`，clang 在 C99 下对同 TU 内重复 typedef 报 `-Wtypedef-redefinition`（C11 起才合法）。GCC 不报此警告，所以主 CI 一直绿。

## 修复内容
1. **CMakeLists.txt**: `CMAKE_C_STANDARD` 99 → 11。PHILOSOPHY.md 本来就说"实际构建使用 C11 标准"，代码与文档对齐。
2. **src/uvhttp_connection.c**: 删除从未被调用的 `static inline uvhttp_validate_buffer_state`（clang `-Wunused-function` 报错，被 typedef 错误掩盖）。
3. **.github/workflows/ci-fuzz.yml**: fuzz_router 链接补齐 `libminiz.a`、`libxxhash.a`（此前漏链接，被编译失败掩盖）；fuzz_request 链接命令改用 `$INCLUDES` 补齐 uthash/mbedtls 头路径。

## fuzz_request 过时处理
`test/fuzz/fuzz_request.c` 引用已不存在的内部 API（`uvhttp_request_parse`、`UVHTTP_REQUEST_STATE_DONE`、`req->conn`），无法编译。因 fuzz_router 已覆盖完整请求解析路径（lihttp parse + router + handler），从 CI 移除 fuzz_request 步骤，保留文件待后续重写。

## 验证
- 本地 clang + ASan 复现 typedef 错误 → 修复后零错误
- fuzz_router 链接成功，跑 5s：442K execs, 0 crash, 0 timeout


## Timeline

- time: 2026-08-25T01:24:41
  kind: decision
  summary: "Created this page: ci-fuzz 修复：C11 对齐 + 工作流补齐"
  source: "ci-fuzz 排查会话"
  affects: [ci-fuzz-c11-fix]

- time: 2026-08-25T01:25:00
  kind: decision
  summary: "记录 ci-fuzz 修复根因与方案"
  source: "ci-fuzz 排查会话"
  affects: [ci-fuzz-c11-fix]
