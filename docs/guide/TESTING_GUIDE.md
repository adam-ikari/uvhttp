# UVHTTP Testing Guide

This document provides testing guidance for the UVHTTP project, including the test framework, mock mechanisms, testing standards, and coverage improvement strategies.

## Table of Contents

1. [Test Framework](#test-framework)
2. [Mock Framework](#mock-framework)
3. [Test Organization](#test-organization)
4. [Test Writing Standards](#test-writing-standards)
5. [Coverage Improvement Strategy](#coverage-improvement-strategy)
6. [Common Questions](#common-questions)

---

## Test Framework

### Google Test Framework

UVHTTP uses Google Test as its unit testing framework, providing the following features:

- **Assertion macros**: `EXPECT_EQ`, `ASSERT_EQ`, `EXPECT_TRUE`, `ASSERT_TRUE`, etc.
- **Test suites**: Define test cases using the `TEST()` macro
- **Parameterized tests**: Supports parameterized testing
- **Death tests**: Supports testing crash scenarios

#### Basic Test Structure

```cpp
#include <gtest/gtest.h>
#include "uvhttp_module.h"

TEST(ModuleNameTest, TestCaseName) {
    // Arrange: Prepare test data
    uvhttp_module_t* obj = nullptr;
    
    // Act: Execute the operation under test
    uvhttp_error_t result = uvhttp_module_new(&obj);
    
    // Assert: Verify the result
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_NE(obj, nullptr);
    
    // Cleanup: Free resources
    uvhttp_module_free(obj);
}
```

#### Assertion Types

- `EXPECT_*`: Continues executing subsequent assertions after failure
- `ASSERT_*`: Terminates the current test immediately after failure

```cpp
EXPECT_EQ(result, UVHTTP_OK);      // Continues after failure
ASSERT_NE(ptr, nullptr);            // Terminates after failure
EXPECT_STREQ(str, "expected");     // String comparison
EXPECT_NEAR(val, expected, 0.001);  // Floating-point comparison
```

#### Helper Functions

Use helper functions to simplify repetitive test setup and cleanup logic:

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
    
    // Test logic...
    
    teardown_server(loop, server);
}
```

---

## Mock Framework

### libuv Mock Framework

UVHTTP provides a complete libuv mock framework that achieves zero-overhead test isolation through link-time symbol replacement (linker wrap).

#### Mock Framework Features

- **Return value control**: Set return values of libuv functions
- **Callback trigger control**: Manually trigger libuv callback functions
- **Call recording**: Record all function calls
- **Error simulation**: Simulate various error scenarios

#### Mock Framework Usage

##### 1. Enabling Mock

Add the linker wrap option in CMakeLists.txt:

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

##### 2. Initializing Mock

```cpp
#include "test/mock/libuv_mock.h"

TEST(ModuleTest, WithMock) {
    // Enable mock
    libuv_mock_set_enabled(true);
    libuv_mock_set_record_calls(true);
    
    // Set return values
    libuv_mock_set_uv_tcp_init_result(0);
    libuv_mock_set_uv_listen_result(0);
    
    // Execute test...
    
    // Cleanup
    libuv_mock_reset();
}
```

##### 3. Verifying Calls

```cpp
TEST(ModuleTest, VerifyCalls) {
    libuv_mock_set_enabled(true);
    libuv_mock_set_record_calls(true);
    
    // Execute test...
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // Verify calls
    size_t call_count = 0;
    libuv_mock_get_call_count("uv_listen", &call_count);
    EXPECT_EQ(call_count, 1);
    
    libuv_mock_reset();
}
```

##### 4. Simulating Errors

```cpp
TEST(ModuleTest, SimulateError) {
    libuv_mock_set_enabled(true);
    
    // Set error return value
    libuv_mock_set_uv_tcp_init_result(-1);
    
    // Execute test
    uvhttp_error_t result = uvhttp_server_new(loop, &server);
    EXPECT_NE(result, UVHTTP_OK);
    
    libuv_mock_reset();
}
```

##### 5. Triggering Callbacks

```cpp
TEST(ModuleTest, TriggerCallbacks) {
    libuv_mock_set_enabled(true);
    
    // Set callback data
    const char* test_data = "GET / HTTP/1.1\r\n\r\n";
    libuv_mock_set_read_data(test_data, strlen(test_data));
    
    // Trigger read callback
    uvhttp_connection_t* conn = create_connection();
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(test_data), &buf);
    
    // Verify processing results...
    
    libuv_mock_reset();
}
```

#### Mock Framework API

**Control interface:**

```cpp
void libuv_mock_reset(void);                          // Reset all state
void libuv_mock_set_enabled(bool enabled);            // Enable/disable mock
void libuv_mock_set_record_calls(bool record);        // Record calls
void libuv_mock_get_call_count(const char* func_name, size_t* call_count); // Get call count
```

**Return value control:**

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

**Callback control:**

```cpp
void libuv_mock_trigger_connection_cb(uv_stream_t* server, int status);
void libuv_mock_trigger_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);
void libuv_mock_trigger_write_cb(uv_write_t* req, int status);
void libuv_mock_trigger_close_cb(uv_handle_t* handle);
```

**Error simulation:**

```cpp
void libuv_mock_set_next_error(int error_code);
void libuv_mock_set_function_result(const char* func_name, int result);
```

---

## Test Organization

### Directory Structure

```
test/
├── unit/                    # Unit tests
│   ├── test_connection_*.cpp
│   ├── test_server_*.cpp
│   ├── test_request_*.cpp
│   └── ...
├── integration/             # Integration tests
│   ├── test_concurrency_e2e.c        # Concurrency end-to-end tests
│   ├── test_http_methods_e2e.c       # HTTP method end-to-end tests
│   ├── test_websocket_e2e.c          # WebSocket end-to-end tests
│   ├── test_static_files_e2e.c       # Static file end-to-end tests
│   ├── test_tls_e2e.c                # TLS end-to-end tests
│   ├── test_performance_e2e.c        # Performance tests
│   ├── test_error_handling_e2e.c     # Error handling tests
│   ├── test_rate_limit_e2e.c         # Rate limit tests
│   ├── test_simple.c                 # Simple test
│   ├── test_route.c                  # Routing test
│   ├── test_no_router.c              # No-router test
│   ├── test_include.c                # Header include test
│   ├── test_middleware_compile_time.c # Middleware compile-time test
│   ├── test_websocket_callback.c     # WebSocket callback test
│   ├── test_websocket_integration.c  # WebSocket integration test
│   ├── test_tls_simple.c             # TLS simple test
│   └── test_static/                  # Static file test directory
├── performance/             # Performance tests
│   ├── test_benchmark.cpp
│   └── test_stress.cpp
├── mock/                    # Mock framework
│   ├── libuv_mock.c
│   └── libuv_mock.h
└── CMakeLists.txt
```

### Naming Conventions

**Test file naming:**

- Unit tests: `test_{module}_{type}.cpp`
  - `test_connection_api_coverage.cpp` - API coverage test
  - `test_server_error_coverage.cpp` - Error handling test
  - `test_request_full_coverage.cpp` - Full feature test

**Test case naming:**

```cpp
TEST(ModuleNameTest, TestCaseName) {
    // ModuleNameTest: module name + Test
    // TestCaseName: test scenario description (camelCase)
}

// Examples:
TEST(ConnectionTest, NewSuccess) {
    // Test successful connection creation
}

TEST(ConnectionTest, NewNullServer) {
    // Test NULL server argument
}

TEST(ConnectionTest, CloseSuccess) {
    // Test successful connection close
}
```

**Test suite naming:**

```cpp
class ConnectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialization before each test
    }
    
    void TearDown() override {
        // Cleanup after each test
    }
    
    uv_loop_t* loop;
    uvhttp_server_t* server;
};
```

---

## Test Writing Standards

### Test Types

#### 1. API Coverage Test

Test the basic functionality of all public APIs:

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

#### 2. Error Handling Test

Test NULL arguments, boundary conditions, and error scenarios:

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
    // Should not crash
    uvhttp_connection_close(NULL);
    SUCCEED();
}
```

#### 3. State Management Test

Test state transitions and field access:

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

#### 4. Integration Test

Test interactions between modules:

```cpp
TEST(HttpIntegrationTest, FullRequestResponse) {
    // Create server
    uvhttp_server_t* server = create_server();
    uvhttp_router_add_route(server->router, "/api", api_handler);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // Send request
    uvhttp_connection_t* conn = create_connection(server);
    const char* request = "GET /api HTTP/1.1\r\n\r\n";
    libuv_mock_set_read_data(request, strlen(request));
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(request), &buf);
    
    // Verify response
    EXPECT_EQ(conn->response->status_code, 200);
    
    // Cleanup
    uvhttp_connection_free(conn);
    uvhttp_server_free(server);
}
```

### Test Standards

#### 1. Test Independence

Each test should run independently without relying on other tests:

```cpp
// Good practice: each test sets up and cleans up independently
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

// Bad practice: relying on global state
static uvhttp_connection_t* g_conn;

TEST(ConnectionTest, Setup) {
    g_conn = create_connection();
}

TEST(ConnectionTest, Test) {
    EXPECT_NE(g_conn, nullptr);  // Depends on another test
}
```

#### 2. Test Readability

Use clear test names and comments:

```cpp
// Good practice: clear test name
TEST(ConnectionTest, CloseConnectionWithPendingWrites) {
    // Test the close behavior of a connection with pending write data
}

// Bad practice: vague test name
TEST(ConnectionTest, Test1) {
    // Test connection close
}
```

#### 3. Test Completeness

Test both normal and exceptional flows:

```cpp
// Good practice: test both normal and exceptional flows
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

#### 4. Route Method Registration

In integration tests, when testing multiple HTTP methods, you must use the correct API to register handlers in order to avoid compiler warnings:

```cpp
// Good practice: use uvhttp_router_add_route_method to register a specific HTTP method
static int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // POST handling logic
    return 0;
}

static int put_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // PUT handling logic
    return 0;
}

// Register routes in main
uvhttp_router_add_route(ctx->router, "/api", json_handler);  // Default GET
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_POST, post_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_PUT, put_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_DELETE, delete_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_PATCH, patch_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_HEAD, head_handler);
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_OPTIONS, options_handler);
```

**Important notes**:
- Use enum values (`UVHTTP_POST`, `UVHTTP_PUT`, etc.) instead of string constants (`UVHTTP_METHOD_POST`, etc.)
- String constants (`UVHTTP_METHOD_POST`) serve other purposes and cannot be used as arguments to `uvhttp_router_add_route_method`
- The correct enum values are defined by the `uvhttp_method_t` enum type in `uvhttp_request.h`

```cpp
// Bad practice: using string constants causes a type mismatch error
uvhttp_router_add_route_method(ctx->router, "/api", UVHTTP_METHOD_POST, post_handler);  // Compile error
```

#### 5. Resource Management

Ensure all resources are properly released:

```cpp
// Good practice: use a helper function to manage resources
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

// Bad practice: resource leak
TEST(ConnectionTest, CreateConnection) {
    uvhttp_connection_t* conn = create_connection();
    EXPECT_NE(conn, nullptr);
    // Forgot to free conn
}
```

---

## Coverage Improvement Strategy

### Current Coverage Status

| Module | Line Coverage | Function Coverage | Priority |
|--------|---------------|--------------------|----------|
| uvhttp_connection.c | 21.9% | 52.2% | High |
| uvhttp_server.c | 10.3% | 5.6% | High |
| uvhttp_static.c | 0% | 0% | High |
| uvhttp_websocket.c | 0% | 0% | High |
| uvhttp_router.c | 31.1% | 73.7% | Medium |
| uvhttp_request.c | 12.3% | 11.1% | Medium |
| uvhttp_response.c | 5.7% | 11.8% | Medium |
| uvhttp_error.c | 1.2% | 16.7% | Medium |

### Coverage Improvement Steps

#### 1. Analyze Uncovered Code

Use lcov to generate a coverage report and inspect uncovered code lines:

```bash
make coverage
open build/coverage_html/index.html
```

#### 2. Identify Test Scenarios

Based on uncovered code, identify scenarios that need testing:

- **Error handling paths**: error return values, error callbacks
- **Boundary conditions**: null pointers, zero length, maximum values
- **State transitions**: transitions between various states
- **Concurrency scenarios**: multiple connections operating simultaneously

#### 3. Write Test Cases

Write corresponding test cases for each scenario:

```cpp
// Example: write a test for an uncovered error handling path
TEST(ConnectionErrorTest, StartWithInvalidHandle) {
    uvhttp_connection_t* conn = create_connection();
    conn->tcp_handle.data = nullptr;  // Simulate an invalid handle
    
    uvhttp_error_t result = uvhttp_connection_start(conn);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_connection_free(conn);
}
```

#### 4. Verify Coverage Improvement

Run the tests and check the coverage improvement:

```bash
make test-all
make coverage
lcov --list coverage.info | grep uvhttp_connection
```

### Coverage Targets

- **Short-term target (1-2 weeks)**: core module coverage > 50%
  - uvhttp_connection.c: 21.9% → 50%
  - uvhttp_server.c: 10.3% → 50%
  - uvhttp_static.c: 0% → 50%

- **Medium-term target (1-2 months)**: overall coverage > 60%

- **Long-term target (3-6 months)**: overall coverage > 80%

---

## Common Questions

### Q1: How to test asynchronous callbacks?

Use the mock framework to manually trigger callbacks:

```cpp
TEST(ConnectionTest, ReadCallback) {
    uvhttp_connection_t* conn = create_connection();
    
    // Set read data
    const char* data = "GET / HTTP/1.1\r\n\r\n";
    libuv_mock_set_read_data(data, strlen(data));
    
    // Trigger read callback
    uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(data), &buf);
    
    // Verify processing results
    EXPECT_NE(conn->request, nullptr);
    
    uvhttp_connection_free(conn);
}
```

### Q2: How to test timeout scenarios?

Use the mock framework to set up timeout callbacks:

```cpp
TEST(ConnectionTest, TimeoutCallback) {
    uvhttp_connection_t* conn = create_connection();
    
    // Start the timeout timer
    uvhttp_connection_start_timeout(conn);
    
    // Simulate timeout
    uv_timer_t* timer = &conn->timeout_timer;
    timer->cb(timer);
    
    // Verify the connection is closed
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);
    
    uvhttp_connection_free(conn);
}
```

### Q3: How to test memory leaks?

Use Valgrind or ASan to detect memory leaks:

```bash
# Using Valgrind
valgrind --leak-check=full ./dist/bin/test_connection_full_api_coverage

# Using ASan
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON ..
make
./dist/bin/test_connection_full_api_coverage
```

### Q4: How to test concurrency scenarios?

Use multiple connections to simulate concurrency:

```cpp
TEST(ServerTest, MultipleConnections) {
    uvhttp_server_t* server = create_server();
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // Create multiple connections
    const int num_connections = 10;
    uvhttp_connection_t* connections[num_connections];
    
    for (int i = 0; i < num_connections; i++) {
        connections[i] = create_connection(server);
        EXPECT_NE(connections[i], nullptr);
    }
    
    // Verify all connections are healthy
    for (int i = 0; i < num_connections; i++) {
        EXPECT_EQ(connections[i]->state, UVHTTP_CONN_STATE_HTTP_READING);
    }
    
    // Cleanup all connections
    for (int i = 0; i < num_connections; i++) {
        uvhttp_connection_free(connections[i]);
    }
    
    uvhttp_server_free(server);
}
```

### Q5: How to test WebSocket?

Use the mock framework to simulate the WebSocket handshake and data transfer:

```cpp
TEST(WebsocketTest, Handshake) {
    uvhttp_connection_t* conn = create_connection();
    
    // Simulate a WebSocket handshake request
    const char* request = "GET /ws HTTP/1.1\r\n"
                         "Upgrade: websocket\r\n"
                         "Connection: Upgrade\r\n"
                         "Sec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n";
    libuv_mock_set_read_data(request, strlen(request));
    
    // Trigger the read callback
    uv_buf_t buf = uv_buf_init((char*)request, strlen(request));
    libuv_mock_trigger_read_cb(&conn->tcp_handle, strlen(request), &buf);
    
    // Verify the WebSocket handshake succeeded
    EXPECT_EQ(conn->is_websocket, 1);
    EXPECT_NE(conn->ws_connection, nullptr);
    
    uvhttp_connection_free(conn);
}
```

---

## Summary

This testing guide provides the testing standards and best practices for the UVHTTP project, including:

1. **Test framework**: Use Google Test for unit testing
2. **Mock framework**: Use libuv_mock to isolate external dependencies
3. **Test standards**: naming conventions, test types, resource management
4. **Coverage improvement**: a complete analyze-identify-write-verify workflow
5. **Common questions**: async callbacks, timeouts, memory leaks, concurrency, WebSocket testing

Following these standards and best practices allows you to write high-quality, maintainable test code and improve the overall code quality and reliability of the project.

---

**Document version**: 1.0  
**Last updated**: 2026-02-05  
**Maintained by**: UVHTTP development team
