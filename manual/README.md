# Manual test tools

**These are NOT automated tests.** They are long-lived HTTP servers meant to be
started by hand and driven with `curl`/`wrk` from a terminal. They were moved
out of `test/` so nothing in the repo suggests CI runs them.

## How to use

Each `.c` compiles to an executable under `dist/bin/` (CMake globs `manual/*.c`).
Start it, then hit it with curl:

```bash
./build/dist/bin/test_simple &
curl -i http://127.0.0.1:8081/
```

They run `uv_run(loop, UV_RUN_DEFAULT)` and never return — stop with Ctrl+C or
`kill`. `test_static/` holds static assets used by `test_static_files_e2e.c`.

## Why they are not registered

| 为什么 | 细节 |
|---|---|
| 永不返回 | `uv_run(UV_RUN_DEFAULT)` 无限循环，注册进 ctest 会撞 `--timeout 90` 被杀 |
| 需要外部驱动 | 打印 usage 等人用 curl/wrk 打，退出码不代表通过 |
| assert 无效 | 构建是 `Release`，`NDEBUG` 让 `assert()` 全部展开为 no-op |

`test_middleware_compile_time.c`、`test_e2e_simple.c`、`test_e2e_real.c`、
`test_https_e2e.c` 曾经放在这里，它们的 42 处断言在 Release 下全部不执行
（`exit=0` 是「什么都没检查」），已于 2026-09 删除。

想写真正的断言测试 → `test/unit/*.cpp`（gtest，被 ctest + CI 执行），不要
在这里用 `assert()`。
