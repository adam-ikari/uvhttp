# UVHTTP 测试指南

本文讲 UVHTTP 3.x 的测试如何编译、注册和运行。

## 测试如何被发现

顶层 `CMakeLists.txt` 使用 glob 匹配 `test/unit/test_*.cpp`。每个文件编译成
`build/dist/bin/<test_name>` 下的独立 GoogleTest 可执行文件，并通过
`add_test` 注册，`ctest` 会自动发现。

项目没有 `test/CMakeLists.txt`，不要新增它，也不要手工维护测试文件清单。

`manual/` 下的文件会被编译，但**故意不注册**进 ctest，因为它们运行
`uv_run(UV_RUN_DEFAULT)` 且永不返回。断言测试放在 `test/unit/`。

## 测试目录结构

```text
test/
├── unit/                 # gtest *.cpp，自动 glob 并注册
├── mock/                 # libuv_mock.c/.h，供 link wrap 测试使用
├── certs/                # TLS 测试证书
├── scripts/              # 证书与辅助脚本
├── coverage/             # 覆盖率辅助文件
└── README.md
manual/                   # 长驻手动 server，不进 ctest
```

文件名会影响构建：

- `test_*_mock*.cpp` 与 `test_*_libuv_fail*.cpp` 会自动链接 `libuv_mock`，
  并自动获得 `-Wl,--wrap=uv_*` 选项。
- 文件名匹配 `*websocket*`、`*tls*`、`*compression*` 的测试，在对应特性关闭时
  会被跳过。

## 运行测试

```bash
# Debug 构建并运行全部已注册测试
make build
make test

# 等价的直接调用
ctest --test-dir build --output-on-failure

# 按名称运行单个测试（或直接运行二进制）
ctest --test-dir build -R test_router --output-on-failure
build/dist/bin/test_router_full_coverage

# 内存安全门禁：全套 ASan + UBSan
make verify-memory-safety

# 覆盖率报告：build_coverage/coverage_html/index.html
make build-coverage
make coverage
```

`make test` 是发布门禁测试的本地等价物。PR CI 只做编译和静态分析，不在 PR
阶段执行测试，因此 PR 变绿不代表测试通过。

## 编写测试

```cpp
#include <gtest/gtest.h>

#include "uvhttp_router.h"

TEST(RouterTest, CreateRouter) {
    uvhttp_router_t* router = nullptr;

    uvhttp_error_t result = uvhttp_router_new(&router);

    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    uvhttp_router_free(router);
}
```

本仓库必须遵守的规则：

1. **直接 include 自己调用的头。** 裁剪构建会关掉特性并移除传递包含；测试不能
   依赖间接引入的头文件。
2. **使用真实 API。** 不要自造函数，也不要驱动普通嵌入代码无法触达的内部状态。
3. **每条路径都清理资源。** 所有提前返回路径都要释放 connection/server 并关闭
   socket fd；ASan 查不到 fd 泄漏。
4. **必须有断言。** 无断言、断言被注释、或断言被无条件控制流罩住的测试会被
   `scripts/ci/check_test_validity.py --strict` 拒绝。
5. **新测试做变异验证。** 把测试本应捕获的 bug 造回去，确认测试变红。改坏实现
   仍然绿的测试没有保护作用。

## Mock libuv

`test/mock/libuv_mock.c` 通过链接期符号替换接管 libuv。CMake 会根据文件名自动
应用 wrap 选项，不要把 wrap 选项复制进测试文件。

```cpp
#include "libuv_mock.h"

TEST(ServerTest, ListenFailure) {
    libuv_mock_reset();
    libuv_mock_set_enabled(true);
    libuv_mock_set_uv_listen_result(-1);

    /* ...驱动被测代码... */

    libuv_mock_reset();
}
```

Mock 还会记录调用次数并可手动触发回调：

```cpp
size_t calls = 0;
libuv_mock_get_call_count("uv_listen", &calls);
EXPECT_EQ(calls, 1);
```

完整控制、返回值与回调触发接口见 `test/mock/libuv_mock.h`。

## Sanitizer 与覆盖率

| 命令 | 构建目录 | 检查内容 |
|---|---|---|
| `make verify-memory-safety` | `build_asan`、`build_ubsan` | 泄漏、use-after-free、越界、未定义行为 |
| `make build-coverage && make coverage` | `build_coverage` | 行/函数/分支覆盖率 |

`make verify-memory-safety` 先以 ASan 构建 Debug，再以 UBSan 构建 Debug，并在两个
构建树中运行全部已注册测试。CI 发布门禁另有独立的 ASan job（启用 static files
与 router cache）；nightly 额外跑覆盖率、压力和 UBSan。

查看单个问题：

```bash
cd build_asan
ctest --output-on-failure -R <test_name>
```

## CI 中的测试职责

| 阶段 | 是否执行测试 |
|---|---|
| PR（`ci-pr.yml`） | 否——只做编译、cppcheck、文档同步和配置矩阵编译 |
| 预发布（`ci-release-gate.yml`） | 快速套件、配置矩阵、ASan |
| Nightly（`ci-nightly.yml`） | 覆盖率、压力、UBSan |

由于 PR CI 不执行测试，开 PR 前必须在本地跑 `make test`。

## 常见问题

### Q1：如何测试异步回调？

用 mock 注入字节并直接触发读回调。wrap 机制见 [Mock libuv](#mock-libuv)。

```cpp
const char* data = "GET / HTTP/1.1\r\n\r\n";
libuv_mock_set_read_data(data, strlen(data));
uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
libuv_mock_trigger_read_cb((uv_stream_t*)&conn->tcp_handle, strlen(data), &buf);
EXPECT_NE(conn->request, nullptr);
```

### Q2：如何测试超时场景？

启动定时器，再调用其回调强制走超时路径。

```cpp
uvhttp_connection_start_timeout(conn);
uv_timer_t* timer = &conn->timeout_timer;
timer->cb(timer);
EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
```

### Q3：如何测试内存泄漏？

[Sanitizer 与覆盖率](#sanitizer-与覆盖率) 通过 `make verify-memory-safety`
在 ASan 和 UBSan 下跑全套测试。手动构建 ASan：

```bash
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc)
cd build_asan && ctest --output-on-failure
```

### Q4：如何测试并发？

让多个连接走同一个 server，断言各自到达预期状态。

```cpp
for (int i = 0; i < num_connections; i++) {
    uvhttp_connection_new(server, &conns[i]);
    EXPECT_EQ(conns[i]->state, UVHTTP_CONN_STATE_NEW);
}
```

### Q5：如何测试 WebSocket？

通过 mock 注入握手请求，再断言连接已升级。

```cpp
libuv_mock_set_read_data(request, strlen(request));
uv_buf_t buf = uv_buf_init((char*)request, strlen(request));
libuv_mock_trigger_read_cb((uv_stream_t*)&conn->tcp_handle, strlen(request), &buf);
EXPECT_EQ(conn->is_websocket, 1);
EXPECT_NE(conn->ws_connection, nullptr);
```

## 提升覆盖率

1. **分析未覆盖代码。** 构建并打开报告：

   ```bash
   make build-coverage && make coverage
   ```

   HTML 报告生成在 `build_coverage/coverage_html/index.html`。

2. **识别场景**：针对未覆盖行——错误返回、空指针/零/最大值边界、状态转换、
   并发连接。

3. **编写测试**，遵循 [编写测试](#编写测试)。

4. **验证提升：**

   ```bash
   make test
   lcov --list build_coverage/coverage.info | grep uvhttp_connection
   ```

## 延伸阅读

- [开发者指南](DEVELOPER_GUIDE.md)
- [测试规范](../dev/TESTING_STANDARDS.md)
- [可测试性指南](../dev/TESTABILITY_GUIDE.md)
- [CI/CD 指南](../dev/CI_CD.md)
- [手动测试工具](https://github.com/adam-ikari/uvhttp/blob/main/manual/README.md)
