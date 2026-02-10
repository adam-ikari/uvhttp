# UVHTTP 测试指南

本文档提供 UVHTTP 项目的测试编写指南，包括测试框架、Mock 机制、测试规范和覆盖率提升策略。

## 目录

1. [测试框架](#测试框架)
2. [Mock 框架](#mock-框架)
3. [测试组织结构](#测试组织结构)
4. [测试编写规范](#测试编写规范)
5. [覆盖率提升策略](#覆盖率提升策略)
6. [常见问题](#常见问题)

---

## 测试框架

### Google Test 框架

UVHTTP 使用 Google Test 作为单元测试框架，提供以下功能：

- **断言宏**：`EXPECT_EQ`、`ASSERT_EQ`、`EXPECT_TRUE`、`ASSERT_TRUE` 等
- **测试套件**：使用 `TEST()` 宏定义测试用例
- **参数化测试**：支持参数化测试
- **死亡测试**：支持测试崩溃场景

#### 基本测试结构

```cpp
#include <gtest/gtest.h>
#include "uvhttp_module.h"

TEST(ModuleNameTest, TestCaseName) {
    // Arrange: 准备测试数据
    uvhttp_module_t* obj = nullptr;
    
    // Act: 执行被测试的操作
    uvhttp_error_t result = uvhttp_module_new(&obj);
    
    // Assert: 验证结果
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(obj, nullptr);
    
    // Cleanup: 清理资源
    uvhttp_module_free(obj);
}
```

#### 断言类型

- `EXPECT_*`：失败后继续执行后续断言
- `ASSERT_*`：失败后立即终止当前测试

```cpp
EXPECT_EQ(result, UVHTTP_OK);      // 失败后继续
ASSERT_NE(ptr, nullptr);            // 失败后终止
EXPECT_STREQ(str, "expected");     // 字符串比较
EXPECT_NEAR(val, expected, 0.001);  // 浮点数比较
```

#### 辅助函数

使用辅助函数简化重复的测试设置和清理逻辑：

```cpp
static void setup_server(uv_loop_t** loop, uvhttp_server_t** server) {
    *loop = uv_loop_new();
    ASSERT_NE(*loop, nullptr);
    uvhttp_error_t result = uvhttp_server_new(*loop, server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(*server, nullptr);
}

static void teardown_server(uv_loop_t* loop, uvhttp_server_t* server) {
    if (server) {
        uvhttp_server_free(server);
    }
    if (loop) {
        uv_loop_close(loop);
        uvhttp_free(loop);
    }
}

TEST(ServerTest, CreateServer) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    setup_server(&loop, &server);
    
    // 测试逻辑...
    
    teardown_server(loop, server);
}
```

---

## Mock 框架

### libuv Mock 框架

UVHTTP 提供了完整的 libuv Mock 框架，通过链接时符号替换（linker wrap）实现零开销的测试隔离。

#### Mock 框架功能

- **函数返回值控制**：设置 libuv 函数的返回值
- **回调触发控制**：手动触发 libuv 回调函数
- **调用记录**：记录所有函数调用
- **错误模拟**：模拟各种错误场景

#### Mock 框架使用

##### 1. 启用 Mock

在 CMakeLists.txt 中添加链接器 wrap 选项：

```cmake
target_link_options(test_module_name PRIVATE
    -Wl,--wrap=uv_loop_init
    -Wl,--wrap=uv_loop_close
    -Wl,--wrap=uv_tcp_init
    -Wl,--wrap=uv_tcp_bind
    -Wl,--wrap=uv_listen
    -Wl,--wrap=uv_read_start
    -Wl,--wrap=uv_read_stop
    -Wl,--wrap=uv_write
    -Wl,--wrap=uv_close
)
```

##### 2. 初始化 Mock

```cpp
#include "test/mock/libuv_mock.h"

TEST(ModuleTest, WithMock) {
    // 启用 mock
    libuv_mock_set_enabled(true);
    libuv_mock_set_record_calls(true);
    
    // 设置返回值
    libuv_mock_set_uv_tcp_init_result(0);
    libuv_mock_set_uv_listen_result(0);
    
    // 执行测试...
    
    // 清理
    libuv_mock_reset();
}
```

##### 3. 验证调用

```cpp
TEST(ModuleTest, VerifyCalls) {
    libuv_mock_set_enabled(true);
    libuv_mock_set_record_calls(true);
    
    // 执行测试...
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // 验证调用
    size_t call_count = 0;
    libuv_mock_get_call_count("uv_listen", &call_count);
    EXPECT_EQ(call_count, 1);
    
    libuv_mock_reset();
}
```

##### 4. 模拟错误

```cpp
TEST(ModuleTest, SimulateError) {
    libuv_mock_set_enabled(true);
    
    // 设置错误返回值
    libuv_mock_set_uv_tcp_init_result(-1);
    
    // 执行测试
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    EXPECT_NE(result, UVHTTP_OK);
    
    libuv_mock_reset();
}
```

##### 5. 触发回调

```cpp
TEST(ModuleTest, TriggerCallbacks) {
    libuv_mock_set_enabled(true);
    
    // 设置回调数据
    const char* test_data = "GET / HTTP/1.1\r\n\r\n";
    libuv_mock_set_read_data(test_data, strlen(test_data));
    
    // 触发 read 回调
    uvhttp_connection_t* conn = create_connection();
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(test_data), &buf);
    
    // 验证处理结果...
    
    libuv_mock_reset();
}
```

#### Mock 框架 API

**控制接口：**

```cpp
void libuv_mock_reset(void);                          // 重置所有状态
void libuv_mock_set_enabled(bool enabled);            // 启用/禁用 mock
void libuv_mock_set_record_calls(bool record);        // 记录调用
void libuv_mock_get_call_count(const char* func_name, size_t* call_count); // 获取调用次数
```

**返回值控制：**

```cpp
void libuv_mock_set_uv_loop_init_result(int result);
void libuv_mock_set_uv_loop_close_result(int result);
void libuv_mock_set_uv_tcp_init_result(int result);
void libuv_mock_set_uv_tcp_bind_result(int result);
void libuv_mock_set_uv_listen_result(int result);
void libuv_mock_set_uv_read_start_result(int result);
void libuv_mock_set_uv_read_stop_result(int result);
void libuv_mock_set_uv_write_result(int result);
```

**回调控制：**

```cpp
void libuv_mock_trigger_connection_cb(uv_stream_t* server, int status);
void libuv_mock_trigger_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void libuv_mock_trigger_write_cb(uv_write_t* req, int status);
void libuv_mock_trigger_close_cb(uv_handle_t* handle);
```

**错误模拟：**

```cpp
void libuv_mock_set_next_error(int error_code);
void libuv_mock_set_function_result(const char* func_name, int result);
```

---

## 测试组织结构

### 目录结构

```
test/
├── unit/                    # 单元测试
│   ├── test_connection_*.cpp
│   ├── test_server_*.cpp
│   ├── test_request_*.cpp
│   └── ...
├── integration/             # 集成测试
│   ├── test_concurrency_e2e.c        # 并发测试端到端测试
│   ├── test_http_methods_e2e.c       # HTTP 方法端到端测试
│   ├── test_websocket_e2e.c          # WebSocket 端到端测试
│   ├── test_static_files_e2e.c       # 静态文件端到端测试
│   ├── test_tls_e2e.c                # TLS 端到端测试
│   ├── test_performance_e2e.c        # 性能测试
│   ├── test_error_handling_e2e.c     # 错误处理测试
│   ├── test_rate_limit_e2e.c         # 限流测试
│   ├── test_simple.c                 # 简单测试
│   ├── test_route.c                  # 路由测试
│   ├── test_no_router.c              # 无路由测试
│   ├── test_include.c                # 头文件包含测试
│   ├── test_middleware_compile_time.c # 中间件编译测试
│   ├── test_websocket_callback.c     # WebSocket 回调测试
│   ├── test_websocket_integration.c  # WebSocket 集成测试
│   ├── test_tls_simple.c             # TLS 简单测试
│   └── test_static/                  # 静态文件测试目录
├── performance/             # 性能测试
│   ├── test_benchmark.cpp
│   └── test_stress.cpp
├── mock/                    # Mock 框架
│   ├── libuv_mock.c
│   └── libuv_mock.h
└── CMakeLists.txt
```

### 命名规范

**测试文件命名：**

- 单元测试：`test_{module}_{type}.cpp`
  - `test_connection_api_coverage.cpp` - API 覆盖率测试
  - `test_server_error_coverage.cpp` - 错误处理测试
  - `test_request_full_coverage.cpp` - 完整功能测试

**测试用例命名：**

```cpp
TEST(ModuleNameTest, TestCaseName) {
    // ModuleNameTest: 模块名 + Test
    // TestCaseName: 测试场景描述（驼峰命名）
}

// 示例：
TEST(ConnectionTest, NewSuccess) {
    // 测试成功创建连接
}

TEST(ConnectionTest, NewNullServer) {
    // 测试 NULL 服务器参数
}

TEST(ConnectionTest, CloseSuccess) {
    // 测试成功关闭连接
}
```

**测试套件命名：**

```cpp
class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 测试前初始化
    }
    
    void TearDown() override {
        // 测试后清理
    }
    
    uv_loop_t* loop;
    uvhttp_server_t* server;
};
```

---

## 测试编写规范

### 测试类型

#### 1. API 覆盖率测试

测试所有公开 API 的基本功能：

```cpp
TEST(ConnectionApiTest, NewSuccess) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(conn, nullptr);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_free(conn);
}

TEST(ConnectionApiTest, NewNullServer) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(conn, nullptr);
}
```

#### 2. 错误处理测试

测试 NULL 参数、边界条件、错误场景：

```cpp
TEST(ConnectionErrorTest, NewNullServer) {
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(NULL, &conn);
    
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST(ConnectionErrorTest, StartNullConnection) {
    uvhttp_error_t result = uvhttp_connection_start(NULL);
    
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST(ConnectionErrorTest, CloseNullConnection) {
    // 不应该崩溃
    uvhttp_connection_close(NULL);
    SUCCEED();
}
```

#### 3. 状态管理测试

测试状态转换和字段访问：

```cpp
TEST(ConnectionStateTest, SetState) {
    uvhttp_connection_t* conn = create_connection();
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_NEW);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);
    
    uvhttp_connection_free(conn);
}

TEST(ConnectionStateTest, KeepaliveFlag) {
    uvhttp_connection_t* conn = create_connection();
    
    conn->keepalive = 0;
    EXPECT_EQ(conn->keepalive, 0);
    
    conn->keepalive = 1;
    EXPECT_EQ(conn->keepalive, 1);
    
    uvhttp_connection_free(conn);
}
```

#### 4. 集成测试

测试模块间的交互：

```cpp
TEST(HttpIntegrationTest, FullRequestResponse) {
    // 创建服务器
    uvhttp_server_t* server = create_server();
    uvhttp_router_add_route(server->router, "/api", api_handler);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // 发送请求
    uvhttp_connection_t* conn = create_connection(server);
    const char* request = "GET /api HTTP/1.1\r\n\r\n";
    libuv_mock_set_read_data(request, strlen(request));
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(request), &buf);
    
    // 验证响应
    EXPECT_EQ(conn->response->status_code, 200);
    
    // 清理
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}
```

### 测试规范

#### 1. 测试独立性

每个测试应该独立运行，不依赖其他测试：

```cpp
// ✅ 好的实践：每个测试独立设置和清理
TEST(ConnectionTest, CreateConnection) {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_server_t* server = create_server(loop);
    uvhttp_connection_t* conn = create_connection(server);
    
    EXPECT_NE(conn, nullptr);
    
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

// ❌ 不好的实践：依赖全局状态
static uvhttp_connection_t* g_conn;

TEST(ConnectionTest, Setup) {
    g_conn = create_connection();
}

TEST(ConnectionTest, Test) {
    EXPECT_NE(g_conn, nullptr);  // 依赖其他测试
}
```

#### 2. 测试可读性

使用清晰的测试名称和注释：

```cpp
// ✅ 好的实践：清晰的测试名称
TEST(ConnectionTest, CloseConnectionWithPendingWrites) {
    // 测试连接在有待写入数据时的关闭行为
}

// ❌ 不好的实践：模糊的测试名称
TEST(ConnectionTest, Test1) {
    // 测试连接关闭
}
```

#### 3. 测试完整性

测试正常流程和异常流程：

```cpp
// ✅ 好的实践：测试正常和异常流程
TEST(ConnectionTest, StartSuccess) {
    uvhttp_connection_t* conn = create_connection();
    uvhttp_error_t result = uvhttp_connection_start(conn);
    EXPECT_EQ(result, UVHTTP_OK);
    uvhttp_connection_free(conn);
}

TEST(ConnectionTest, StartNullConnection) {
    uvhttp_error_t result = uvhttp_connection_start(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}
```

#### 4. 路由方法注册

在集成测试中，当需要测试多个 HTTP 方法时，必须使用正确的 API 注册处理器以避免编译警告：

```cpp
// ✅ 好的实践：使用 uvhttp_router_add_route_method 注册特定 HTTP 方法
static int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // POST 处理逻辑
    return 0;
}

static int put_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // PUT 处理逻辑
    return 0;
}

// 在 main 函数中注册路由
uvhttp_router_add_route(ctx->router, "/api", json_handler);  // 默认 GET
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_POST, post_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_PUT, put_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_DELETE, delete_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_PATCH, patch_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_HEAD, head_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_OPTIONS, options_handler);
```

**重要注意事项**：
- 使用枚举值（`UVHTTP_POST`、`UVHTTP_PUT` 等）而不是字符串常量（`UVHTTP_METHOD_POST` 等）
- 字符串常量（`UVHTTP_METHOD_POST`）用于其他目的，不能作为 `uvhttp_router_add_route_method` 的参数
- 正确的枚举值定义在 `uvhttp_request.h` 中的 `uvhttp_method_t` 枚举类型

```cpp
// ❌ 错误实践：使用字符串常量会导致类型不匹配错误
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_METHOD_POST, post_handler);  // 编译错误
```

#### 5. 资源管理

确保所有资源都被正确释放：

```cpp
// ✅ 好的实践：使用辅助函数管理资源
static void cleanup_resources(uv_loop_t* loop, uvhttp_server_t* server, uvhttp_connection_t* conn) {
    if (conn) uvhttp_connection_free(conn);
    if (server) uvhttp_server_free(server);
    if (loop) {
        uv_loop_close(loop);
        uvhttp_free(loop);
    }
}

TEST(ConnectionTest, CreateConnection) {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_server_t* server = create_server(loop);
    uvhttp_connection_t* conn = create_connection(server);
    
    EXPECT_NE(conn, nullptr);
    
    cleanup_resources(loop, server, conn);
}

// ❌ 不好的实践：资源泄漏
TEST(ConnectionTest, CreateConnection) {
    uvhttp_connection_t* conn = create_connection();
    EXPECT_NE(conn, nullptr);
    // 忘记释放 conn
}
```

---

## 覆盖率提升策略

### 当前覆盖率状况

| 模块 | 行覆盖率 | 函数覆盖率 | 优先级 |
|------|---------|-----------|--------|
| uvhttp_connection.c | 21.9% | 52.2% | 高 |
| uvhttp_server.c | 10.3% | 5.6% | 高 |
| uvhttp_static.c | 0% | 0% | 高 |
| uvhttp_websocket.c | 0% | 0% | 高 |
| uvhttp_router.c | 31.1% | 73.7% | 中 |
| uvhttp_request.c | 12.3% | 11.1% | 中 |
| uvhttp_response.c | 5.7% | 11.8% | 中 |
| uvhttp_error.c | 1.2% | 16.7% | 中 |

### 覆盖率提升步骤

#### 1. 分析未覆盖代码

使用 lcov 生成覆盖率报告，查看未覆盖的代码行：

```bash
make coverage
open build/coverage_html/index.html
```

#### 2. 识别测试场景

根据未覆盖的代码，识别需要测试的场景：

- **错误处理路径**：错误返回值、错误回调
- **边界条件**：空指针、零长度、最大值
- **状态转换**：各种状态之间的转换
- **并发场景**：多个连接同时操作

#### 3. 编写测试用例

为每个场景编写相应的测试用例：

```cpp
// 示例：为未覆盖的错误处理路径编写测试
TEST(ConnectionErrorTest, StartWithInvalidHandle) {
    uvhttp_connection_t* conn = create_connection();
    conn->tcp_handle.data = nullptr;  // 模拟无效句柄
    
    uvhttp_error_t result = uvhttp_connection_start(conn);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
}
```

#### 4. 验证覆盖率提升

运行测试并检查覆盖率提升：

```bash
make test-all
make coverage
lcov --list coverage.info | grep uvhttp_connection
```

### 覆盖率目标

- **短期目标（1-2 周）**：核心模块覆盖率 > 50%
  - uvhttp_connection.c: 21.9% → 50%
  - uvhttp_server.c: 10.3% → 50%
  - uvhttp_static.c: 0% → 50%

- **中期目标（1-2 月）**：整体覆盖率 > 60%

- **长期目标（3-6 月）**：整体覆盖率 > 80%

---

## 常见问题

### Q1: 如何测试异步回调？

使用 Mock 框架手动触发回调：

```cpp
TEST(ConnectionTest, ReadCallback) {
    uvhttp_connection_t* conn = create_connection();
    
    // 设置读取数据
    const char* data = "GET / HTTP/1.1\r\n\r\n";
    libuv_mock_set_read_data(data, strlen(data));
    
    // 触发读取回调
    uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(data), &buf);
    
    // 验证处理结果
    EXPECT_NE(conn->request, nullptr);
    
    uvhttp_connection_free(conn);
}
```

### Q2: 如何测试超时场景？

使用 Mock 框架设置超时回调：

```cpp
TEST(ConnectionTest, TimeoutCallback) {
    uvhttp_connection_t* conn = create_connection();
    
    // 启动超时定时器
    uvhttp_connection_start_timeout(conn);
    
    // 模拟超时
    uv_timer_t* timer = &conn->timeout_timer;
    timer->cb(timer);
    
    // 验证连接已关闭
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
}
```

### Q3: 如何测试内存泄漏？

使用 Valgrind 或 ASan 检测内存泄漏：

```bash
# 使用 Valgrind
valgrind --leak-check=full ./dist/bin/test_connection_full_api_coverage

# 使用 ASan
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make
./dist/bin/test_connection_full_api_coverage
```

### Q4: 如何测试并发场景？

使用多个连接模拟并发：

```cpp
TEST(ServerTest, MultipleConnections) {
    uvhttp_server_t* server = create_server();
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // 创建多个连接
    const int num_connections = 10;
    uvhttp_connection_t* connections[num_connections];
    
    for (int i = 0; i < num_connections; i++) {
        connections[i] = create_connection(server);
        EXPECT_NE(connections[i], nullptr);
    }
    
    // 验证所有连接都正常
    for (int i = 0; i < num_connections; i++) {
        EXPECT_EQ(connections[i]->state, UVHTTP_CONN_STATE_HTTP_READING);
    }
    
    // 清理所有连接
    for (int i = 0; i < num_connections; i++) {
        uvhttp_connection_free(connections[i]);
    }
    
    uvhttp_server_free(server);
}
```

### Q5: 如何测试 WebSocket？

使用 Mock 框架模拟 WebSocket 握手和数据传输：

```cpp
TEST(WebsocketTest, Handshake) {
    uvhttp_connection_t* conn = create_connection();
    
    // 模拟 WebSocket 握手请求
    const char* request = "GET /ws HTTP/1.1\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
    libuv_mock_set_read_data(request, strlen(request));
    
    // 触发读取回调
    uv_buf_t buf = uv_buf_init((char*)request, strlen(request));
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(request), &buf);
    
    // 验证 WebSocket 握手成功
    EXPECT_EQ(conn->is_websocket, 1);
    EXPECT_NE(conn->ws_connection, nullptr);
    
    uvhttp_connection_free(conn);
}
```

---

## 总结

本测试指南提供了 UVHTTP 项目的测试编写规范和最佳实践，包括：

1. **测试框架**：使用 Google Test 进行单元测试
2. **Mock 框架**：使用 libuv_mock 隔离外部依赖
3. **测试规范**：命名规范、测试类型、资源管理
4. **覆盖率提升**：分析、识别、编写、验证的完整流程
5. **常见问题**：异步回调、超时、内存泄漏、并发、WebSocket 测试

遵循这些规范和最佳实践，可以编写高质量、可维护的测试代码，提升项目的代码质量和可靠性。

---

**文档版本**：1.0  
**最后更新**：2026-02-05  
**维护者**：UVHTTP 开发团队