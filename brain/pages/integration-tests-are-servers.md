---
id: integration-tests-are-servers
title: "test/integration 下 19 个文件不是测试：长驻 server + 无自验证，其中 4 个的 assert 在 Release 下全失效"
category: decision
status: active
tags: [test, integration, assert, ci, cleanup]
created: "2026-09-30T16:31:48"
updated: "2026-10-01T01:40:43"
---

<!-- compiled_truth -->
# test/integration 下 19 个文件不是测试

## 结论

`test/integration/*.c` 的 19 个文件**没有一个是自动化测试**。它们全部是「启动一个长驻 HTTP server，等外部 curl/wrk 驱动，靠人看输出或计数」的手动工具。

CMake 的 `add_test` 只注册 `test/unit/*.cpp`（gtest）。integration 目录的
文件被 `file(GLOB INTEGRATION_TEST_FILES ...)` 编译进 `dist/bin/`，但**从不注册
进 ctest**——这是有意的：它们跑 `uv_run(loop, UV_RUN_DEFAULT)` 永不返回，
注册进去会撞 `ctest --timeout 90` 被杀。

所以 CI 一次都不执行它们。这本身不算错（它们是工具），**但放在 `test/` 目录
下会让人误以为 CI 在跑**。

## 更严重的一类：assert 在 Release 下全部失效

其中 4 个文件（42 处）只依赖 `assert()` 做验证：

| 文件 | assert 数 |
|---|---|
| `test_middleware_compile_time.c` | 15 |
| `test_e2e_real.c` | 14 |
| `test_https_e2e.c` | 11 |
| `test_e2e_simple.c` | 2 |

CI 与本地都用 `-DCMAKE_BUILD_TYPE=Release`，而 Release 定义 `NDEBUG`，
`assert()` 全部展开为 no-op。验证：

    nm build/dist/bin/test_middleware_compile_time | grep -c __assert_fail
    0

**这 4 个文件在 Release 下一个断言都不执行**。它们 exit=0 不是"通过"，是
"什么都没检查"。人手动跑它们时会得到虚假的绿灯——这比"没有测试"更糟，
因为它伪装成验证。

## 分类（按实际结构，不是按文件名）

读 `test_rate_limit_e2e.c` 等文件确认：全部是

```c
uvhttp_server_listen(...);
printf("按 Ctrl+C 停止服务器\n");
uv_run(loop, UV_RUN_DEFAULT);   /* 永不返回 */
```

无论它叫 `_e2e`、`_integration` 还是 `test_route`，结构完全一样。**"e2e"
这个名字不改变它需要外部驱动的事实**。

## 处置建议（未执行，待决策）

删除/移动属于破坏性操作，且这些文件不是本会话写的，所以只记录不执行。

1. **4 个纯 assert 文件**：断言在 Release 下不存在，留着只会给人虚假绿灯。
   删掉，或改写成 gtest（放 `test/unit/`，那样才会被 CI 跑）。
2. **其余 15 个长驻 server**：移到 `examples/` 或新目录 `manual/`，并在该
   目录 README 写明"这些不是 CI 测试，需手动启动 + curl"。留在 `test/` 下
   是持续误导。
3. 无论怎么处置，CMake 的 `INTEGRATION_TEST_FILES` glob 与
   `add_test` 的缺口值得在 CONTRIBUTING 里写清（已有一节「选择测试形态」，
   应补充"integration 目录的文件不会被执行"这一点，见下）。

## 顺带：本地构建的盲区（本次实际踩到）

`src/uvhttp_lru_cache.c` 的**整个内容**被 `#if UVHTTP_FEATURE_STATIC_FILES` 包裹。
本地默认 `build/` 是 `STATIC_FILES=OFF`，所以**这个文件在本地根本不编译**——
在本地 build/ 下做任何验证都碰不到它。只有 `build-matrix` 的 `static-files`
条目会编它。

后果实测（#409）：给该文件加 `UVHTTP_UNUSED` 后，本地 `make uvhttp` 通过
（因为文件没编），CI 的 static-files 矩阵立刻报 `UVHTTP_UNUSED undeclared`
（该文件没 include `uvhttp_features.h`，靠间接包含拿到 `UVHTTP_FEATURE_STATIC_FILES`
却拿不到 `UVHTTP_UNUSED`）。

**判据**：改某个 `.c` 之前先确认它在**哪些配置下会被编译**。
`grep -rn 'if UVHTTP_FEATURE' src/<file>.c` 就能看出来。若被 feature 宏
整体包裹，本地默认构建的通过**不构成任何证据**，必须用该 feature 开启的
配置单独构建一次。


## Timeline

- time: 2026-09-30T16:31:48
  kind: decision
  summary: "Created this page: test/integration 下 19 个文件不是测试：长驻 server + 无自验证，其中 4 个的 assert 在 Release 下全失效"
  source: "integration 测试清理调研（2026-09-30）"
  affects: [integration-tests-are-servers]

- time: 2026-09-30T16:32:08
  kind: decision
  summary: "19 个 integration 文件全是长驻 server 无自验证；其中 4 个纯 assert 文件的 42 处断言在 Release（NDEBUG）下全部不执行，exit=0 是虚假绿灯；处置建议待决策"
  source: "integration 测试清理调研（2026-09-30）"
  affects: [integration-tests-are-servers]

- time: 2026-10-01T01:40:43
  kind: decision
  summary: "19 个 integration 文件处置完毕：删 4 个纯 assert + 移 15 个长驻 server 到 manual/，test/integration 目录删除，全部文档同步"
  source: "处置执行（2026-09-30）"
  affects: [integration-tests-are-servers]
