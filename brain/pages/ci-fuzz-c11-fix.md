---
id: ci-fuzz-c11-fix
title: "ci-fuzz 修复：C11 对齐 + 工作流补齐"
category: decision
status: active
tags: [ci, fuzz, c11, clang]
created: "2026-08-25T01:24:41"
updated: "2026-11-09T17:51:56"
---

<!-- compiled_truth -->
## 现状（2026-11-09 起）
`CMAKE_C_STANDARD 99`，`CMAKE_C_EXTENSIONS OFF`，构建与 `make check-syntax` 都用严格 C99。clang 在 C99 下对同 TU 内重复前向 typedef 报 `-Wtypedef-redefinition`，GCC 不报；构建用 `-Wno-typedef-redefinition` 显式压制（头文件里的重复定义内容一致，属便利性写法）。`UVHTTP_STATIC_ASSERT` 在编译器不支持 `_Static_assert` 时回退为负数组长度 `typedef`，因此 C99 下仍有编译期断言。GCC 与 clang 的 C99 构建均已通过 119/119 测试。

## 历史（2026-08-25 的修复）
ci-fuzz 连续失败 5 天（2026-08-20 ~ 08-24）时，项目为 clang 的 `-Wtypedef-redefinition` 把标准从 99 升到 11。2026-11-09 按要求切回 C99，改用抑制告警 + 静态断言回退，不再依赖 C11 的 typedef 重定义规则。同期修复了测试目标里 `-Wno-error-unused-function` 的非法写法（应为 `-Wno-error=unused-function`），该错误此前只在 clang 构建测试时暴露。


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

- time: 2026-11-09T17:51:56
  kind: decision
  summary: "标准切回 C99：clang typedef 告警改为显式抑制，静态断言加 C99 回退"
  source: "C99 构建切换"
  affects: [ci-fuzz-c11-fix]

- time: 2026-11-09T17:51:56
  kind: reversal
  summary: "2026-11-09：CMAKE_C_STANDARD 11→99；用 -Wno-typedef-redefinition 与 UVHTTP_STATIC_ASSERT 回退替代 C11 依赖；GCC/Clang C99 均过 119 测试"
  source: "C99 构建切换"
  affects: [ci-fuzz-c11-fix]
