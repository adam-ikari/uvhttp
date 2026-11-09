---
title: 内存安全 — ASan 与 UBSan 验证
description: UVHTTP 的内存安全保证：119 项测试通过 ASan 与 UBSan 验证，每夜 CI 持续验证，一键复现。涵盖已修复的 bug 类别、纵深防御，以及对长期运行与嵌入式场景的意义。
---

# 内存安全

UVHTTP 不比单机峰值吞吐，nginx 和 h2o 在那一项上更强。它比的是另一件事：在一个轻量、可嵌入、支持 32 位的 C 库里把内存安全做扎实。生产服务一跑几个月，嵌入式设备又不能靠重启清掉泄漏，一个慢速泄漏或一次罕见的 use-after-free 就足以致命。多数轻量 C HTTP 库根本没有针对这类 bug 的测试。

这一页给证据，不给口号。

## 承诺

完整的 119 项测试套件在以下**两种**配置下均验证通过：

| 消毒器 | 检测内容 | 结果 |
|--------|----------|------|
| **AddressSanitizer**（含内存泄漏检测） | 堆内存释放后使用、堆缓冲区溢出、栈缓冲区溢出、内存泄漏 | **119/119 pass，零发现** |
| **UndefinedBehaviorSanitizer** | 有符号整数溢出、非法移位、空指针解引用、对齐错误、越界访问 | **119/119 pass，零发现** |

ASan 与 UBSan 无法在同一个构建里共存，只能分两次跑。两者都得通过：发布门禁跑 ASan，UBSan 每夜跑，也可以本地一键复现。

## 自行复现（一条命令）

```bash
make verify-memory-safety
```

这条命令会分别在 ASan 与 UBSan 下配置、构建并运行完整的 119 项测试。任一测试失败，或任一消毒器报告发现，命令就以非零状态退出。干净跑完时输出：

```
==> PASS: full suite clean under ASan and UBSan (119/119 each).
```

等价的手动命令：

```bash
# AddressSanitizer（内存泄漏、堆内存释放后使用、缓冲区溢出）
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc)
(cd build_asan && ctest --output-on-failure)

# UndefinedBehaviorSanitizer
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc)
(cd build_ubsan && ctest --output-on-failure)
```

消毒器构建保留调试符号，不传 `-s`，所以任何发现都能落到具体文件和行号。

## 持续验证

内存安全得一直成立，检查因此分布在发布门禁和每夜流水线里：

> **覆盖率**：项目代码（不含 deps/）行覆盖率 86%、函数覆盖率 99%。

- **PR CI**（`.github/workflows/ci-pr.yml`）对进入 `main` 的每次改动做编译和静态检查，但不跑测试套件。
- **发布前门禁**（ci-release-gate.yml）：`asan-gate` 作业用 AddressSanitizer 构建并运行完整套件，有发现就阻断发布。
- **每夜 CI**（`.github/workflows/ci-nightly.yml`）：`test-ubsan` 作业每夜在 UBSan 下跑完整套件。`test-memory` 已移除，因为它和发布门禁里的 ASan 作业重复。
- **每夜模糊测试**（`.github/workflows/ci-fuzz.yml`）：libFuzzer 测试桩（`test/fuzz/fuzz_router.c`）在 ASan 下用数百万条任意输入驱动路由器，能走到固定测试输入到不了的状态，目前已经发现过一个 bug（见下表）。
- **构建系统护栏**：消毒器和 Debug 构建从不 strip，CI 里的发现始终可调试。

## 模糊测试

消毒器覆盖测试套件走得到的路径，模糊测试补上剩下的。UVHTTP 为路由器提供了 libFuzzer 测试桩（`test/fuzz/fuzz_router.c`），每夜在 CI 里跑。

```bash
# Build (clang + libFuzzer + ASan)
cmake -B build_fuzz -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON \
  -DBUILD_TESTING=OFF -DBUILD_EXAMPLES=OFF
cmake --build build_fuzz -j$(nproc) --target uvhttp
clang -g -O1 -fsanitize=fuzzer,address -fno-omit-frame-pointer \
  -Iinclude -Ideps/llhttp/include -Ideps/uthash/src \
  -Ideps/mbedtls/include -Ideps/cjson -Ideps/libuv/include \
  test/fuzz/fuzz_router.c build_fuzz/dist/lib/libuvhttp.a \
  -Wl,--start-group \
  deps/llhttp/build/libllhttp.a deps/cjson/build/libcjson.a \
  deps/mbedtls/build/library/libmbedtls.a deps/mbedtls/build/library/libmbedx509.a \
  deps/mbedtls/build/library/libmbedcrypto.a deps/libuv/build/libuv.a \
  -Wl,--end-group -lpthread -lm -ldl -o fuzz_router

# Run
./fuzz_router -max_total_time=60 -max_len=256
```

干净运行以 0 退出，且不打印 `ERROR:` 行。下表里 `add_route_method` 的堆缓冲区溢出就是这个测试桩找到的：迁移之后注册非参数路由，单元测试从没按这个顺序走过。模糊测试的价值正在这里。

## 为达成此承诺而修复的缺陷类别

测试套件一开始并不干净。要把消毒器发现降到零，得修库代码里的真实缺陷：它们能通过功能测试，却在消毒器下破坏内存。下面列代表性修复，完整列表见[更新日志](../guide/CHANGELOG.md)。

| 缺陷类别 | 位置 | 详情 |
|----------|------|------|
| 堆缓冲区溢出 | `uvhttp_router.c` `add_route_method` | **由 libFuzzer 发现。** 在一条参数路由触发 trie 迁移（将 `array_capacity` 重置为 0）后，后续的一条非参数路由落入 `add_array_route`，其 `0*2` 容量计算出一个零大小的 realloc，`strncpy` 随之溢出。常规测试套件从未触及这一路由注册顺序。 |
| 堆内存释放后使用 | `uvhttp_router.c` `find_or_create_child` | 指向节点池的缓存指针在 `realloc` 扩展池后悬空 |
| 堆内存释放后使用 | `uvhttp_server.c` `uvhttp_server_free` | 双重释放时从已释放内存读取 `freed` 标志 |
| 堆内存释放后使用 | `uvhttp_connection.c` | 关闭后读取；`close_pending` 计数在重入时出错 |
| 堆缓冲区溢出 | `uvhttp_response.c` `build_response_headers` | `snprintf` 返回值累加使 `pos` 超出缓冲区，下溢突破边界 |
| 栈缓冲区下溢 | `uvhttp_websocket.c` `uvhttp_ws_close` | 写入已超出作用域的栈上连接 |
| 内存泄漏 | `uvhttp_connection.c` `restart_read` | 请求体指针被置空却未释放 |
| 内存泄漏 | `uvhttp_server.c` `on_connection` 503 路径 | `temp_client`（`uv_tcp_t`）从未关闭 / 释放 |
| 内存泄漏 | `uvhttp_server.c` `ws_disable_connection_management` | 管理结构体（含内嵌 `uv_timer_t`）在关闭回调触发前被释放 |
| 内存泄漏 | `uvhttp_error_helpers.c` | 对非 libuv errno 调用 `uv_strerror` 触发了无法释放的 libuv `uv__strdup` |
| 未定义行为 | `uvhttp_connection.c` `switch_to_websocket` | 重入破坏 CLOSING 状态，导致关闭计数错乱（表现为 UBSan 下的挂起） |

每一项都追到了源头再修，没有靠屏蔽绕过去。

## 为何重要（以及为何多数轻量级 C 库缺失此项）

一个“通过测试”的 C HTTP 库，照样可能每次连接漏掉几 KB、在罕见的关闭竞态里解引用已释放的内存，或者把一段没有结尾的字符串拼进响应。功能测试不会失败。问题要到运行一段时间后才显形：

- RSS 一天天涨，持续数天或数周，而嵌入式设备没法重启。
- 在测试套件没造出来的负载下间歇崩溃。
- 头部或主体解析中的缓冲区溢出，从网络侧就能触发内存破坏。

消毒器在部署前就能抓住这些。多数轻量 C HTTP 库不宣传 sanitizer-clean，是因为它们没跑过这项检查，不是因为代码干净。UVHTTP 在发布门禁和每夜流水线里都跑，任何一项发现都按发布阻断处理。

## 纵深防御

消毒器之外，代码库还有这些约束：

- **`-Werror`** 配合 `-Wall -Wextra -Wformat=2 -Wformat-security`：零警告才算编译通过。
- **`-fstack-protector-strong`**、`-fno-common`、完整 RELRO（`-Wl,-z,relro,now`）。
- **HTTP 响应拆分防护**（`contains_control_chars`），所有头部值在发出前检查。
- **输入校验**：URL 长度 / 路径穿越、头部大小 / 格式、主体大小限制。
- **幂等资源清理**：`*_cleanup` / `*_free_resources` 路径释放后置空指针，并用 `freed` 标志兜底，避免 API 边界上的双重释放读到已释放内存。

## 当报告了消毒器发现时

1. 用 `make verify-memory-safety`（或对应的消毒器构建）复现。
2. 对着源码级栈回溯找原因；消毒器构建没有 strip。
3. 修根因，不要抑制。
4. 合并前确认 ASan 与 UBSan 都是绿的。

与安全相关的发现，按[安全指南](./guide/SECURITY.md)里的流程报告。
