# UVHTTP 工程规范文档

> 版本：2.2.0  
> 更新日期：2026-01-31  
> 状态：正式发布

## 目录

1. [概述](#概述)
2. [代码风格规范](#代码风格规范)
3. [编译规范](#编译规范)
4. [内存管理规范](#内存管理规范)
5. [错误处理规范](#错误处理规范)
6. [测试规范](#测试规范)
7. [提交规范](#提交规范)
8. [性能规范](#性能规范)
9. [文档规范](#文档规范)
10. [安全规范](#安全规范)
11. [架构设计原则](#架构设计原则)

---

## 概述

UVHTTP 是一个基于 libuv 的高性能、轻量级 HTTP/1.1 和 WebSocket 服务器库，采用 C11 标准编写。本规范文档定义了项目开发过程中必须遵循的工程规范。

### 核心设计理念

- **专注核心**：只实现 HTTP 协议处理，不内置业务逻辑
- **零开销**：生产环境无任何抽象层成本
- **极简工程**：移除所有不必要的复杂度
- **测试分离**：测试代码与生产代码完全分离
- **零全局变量**：支持多实例和单元测试
- **生产就绪**：完整的错误处理和资源管理

---

## 代码风格规范

### C 语言标准

- **标准版本**：C11
- **最低要求**：支持 C11 的编译器（GCC 4.8+, Clang 3.3+）
- **编译器要求**：
  ```cmake
  set(CMAKE_C_STANDARD 11)
  set(CMAKE_C_STANDARD_REQUIRED ON)
  ```

### 缩进和格式

- **缩进**：4 个空格，不使用制表符
- **行宽限制**：80 字符（.clang-format 配置）
- **大括号风格**：K&R 风格
- **格式化工具**：clang-format（基于 Google 风格）

#### .clang-format 配置

```yaml
BasedOnStyle: google
IndentWidth: 4
TabWidth: 4
UseTab: Never
ColumnLimit: 80
BreakBeforeBraces: Attach
AllowShortFunctionsOnASingleLine: Empty
AllowShortIfStatementsOnASingleLine: Never
AllowShortLoopsOnASingleLine: false
AllowShortBlocksOnASingleLine: false
SortIncludes: true
PointerAlignment: Left
```

#### 代码示例

```c
/* K&R 风格大括号 */
if (condition) {
    do_something();
} else {
    do_other();
}

/* 函数定义 */
static void process_request(uvhttp_request_t* request) {
    if (!request) {
        return;
    }
    
    const char* method = uvhttp_request_get_method(request);
    if (strcmp(method, "GET") == 0) {
        handle_get(request);
    }
}
```

### 命名约定

#### 函数命名

- **格式**：`uvhttp_module_action`
- **示例**：
  ```c
  uvhttp_server_new()
  uvhttp_server_listen()
  uvhttp_router_add_route()
  uvhttp_request_get_method()
  uvhttp_response_set_status()
  ```

#### 类型命名

- **格式**：`uvhttp_name_t`
- **示例**：
  ```c
  typedef struct uvhttp_server uvhttp_server_t;
  typedef struct uvhttp_router uvhttp_router_t;
  typedef struct uvhttp_request uvhttp_request_t;
  ```

#### 常量命名

- **格式**：`UVHTTP_UPPER_CASE`
- **示例**：
  ```c
  #define UVHTTP_MAX_HEADERS 64
  #define UVHTTP_MAX_URL_SIZE 2048
  #define UVHTTP_OK 0
  ```

#### 宏命名

- **格式**：`UVHTTP_UPPER_CASE`
- **示例**：
  ```c
  #define UVHTTP_MALLOC(size) uvhttp_alloc(size)
  #define UVHTTP_FREE(ptr) uvhttp_free(ptr)
  #define UVHTTP_LIKELY(x) __builtin_expect(!!(x), 1)
  ```

#### 变量命名

- **格式**：`snake_case`
- **示例**：
  ```c
  uvhttp_server_t* server;
  size_t buffer_size;
  const char* header_name;
  ```

### 注释规范

#### 文件头注释

```c
/*
 * UVHTTP server module
 *
 * Provides core HTTP server functionality including connection management,
 * request routing, and response processing
 * Implements high-performance asynchronous I/O based on libuv
 *
 * @author UVHTTP Team
 * @version 2.2.0
 */
```

#### 函数注释（Doxygen 风格）

```c
/**
 * @brief Create a new HTTP server
 *
 * @param loop The libuv event loop
 * @return uvhttp_server_t* Server object, or NULL on failure
 *
 * @note The server does not start listening until uvhttp_server_listen() is called
 * @see uvhttp_server_listen()
 * @see uvhttp_server_free()
 */
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```

#### 行内注释

```c
/* Check if connection limit is reached */
if (server->active_connections >= max_connections) {
    UVHTTP_LOG_WARN("Connection limit reached: %zu/%zu\n",
                    server->active_connections, max_connections);
    return;
}
```

---

## 编译规范

### 编译选项

#### 调试模式

```cmake
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
endif()
```

#### 发布模式（默认）

```cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2 -DNDEBUG -ffunction-sections -fdata-sections")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -s")
```

**重要**：
- **禁用 -O3 优化**：由于测试超时问题，统一使用 -O2 优化
- **-O3 优化导致的问题**：
  1. 循环展开导致测试超时
  2. 激进的优化可能引入未定义行为
  3. 调试困难

#### 强制覆盖 Release 选项

```cmake
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
```

### 安全编译选项

```cmake
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall \
    -Wextra \
    -Wformat=2 \
    -Wformat-security \
    -fstack-protector-strong \
    -fno-common \
    -Werror \
    -Werror=implicit-function-declaration \
    -Werror=format-security \
    -Werror=return-type \
    -D_FORTIFY_SOURCE=2 \
")
```

**说明**：
- `-Werror`：将所有警告视为错误
- `-Werror=implicit-function-declaration`：禁止隐式函数声明
- `-Werror=format-security`：禁止不安全的格式化字符串
- `-Werror=return-type`：禁止缺少返回值
- `-fstack-protector-strong`：启用栈保护
- `-D_FORTIFY_SOURCE=2`：启用缓冲区溢出检查

### 链接选项

```cmake
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} \
    -Wl,-z,relro \
    -Wl,-z,now \
")
```

**说明**：
- `-Wl,-z,relro`：只读重定位
- `-Wl,-z,now`：立即绑定

### 覆盖率编译

```cmake
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
```

### 编译宏定义

#### 功能模块

```cmake
add_definitions(-DUVHTTP_FEATURE_MIDDLEWARE=1)
add_definitions(-DUVHTTP_FEATURE_STATIC_FILES=1)
add_definitions(-DUVHTTP_FEATURE_TLS=1)
add_definitions(-DUVHTTP_FEATURE_LRU_CACHE=1)
add_definitions(-DUVHTTP_FEATURE_RATE_LIMIT=1)
```

#### 平台检测

```cmake
check_type_size("void*" SIZEOF_VOID_PTR)
if(SIZEOF_VOID_PTR EQUAL 4)
    add_definitions(-DUVHTTP_32BIT)
elseif(SIZEOF_VOID_PTR EQUAL 8)
    add_definitions(-DUVHTTP_64BIT)
endif()
```

#### 可配置常量

```cmake
set(UVHTTP_MAX_HEADER_NAME_SIZE 256 CACHE STRING "Max HTTP header name size")
set(UVHTTP_MAX_HEADERS 64 CACHE STRING "Max number of HTTP headers")
set(UVHTTP_MAX_URL_SIZE 2048 CACHE STRING "Max URL size")
set(UVHTTP_MAX_CONNECTIONS_DEFAULT 2048 CACHE STRING "Default max connections")
```

---

## 内存管理规范

### 统一分配器

UVHTTP 使用统一的内存分配接口，支持编译期选择系统分配器或 mimalloc。

#### 分配器选择

```cmake
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

#### 分配函数

```c
/* 分配内存 */
void* uvhttp_alloc(size_t size);

/* 释放内存 */
void uvhttp_free(void* ptr);

/* 重新分配内存 */
void* uvhttp_realloc(void* ptr, size_t size);

/* 分配并初始化内存 */
void* uvhttp_calloc(size_t nmemb, size_t size);
```

#### 使用示例

```c
/* 正确使用 */
void* buffer = uvhttp_alloc(buffer_size);
if (!buffer) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}

/* 使用 buffer... */

uvhttp_free(buffer);
```

### 内存管理规则

#### 规则 1：严禁混用分配器

```c
/* 错误：混用 malloc 和 uvhttp_free */
void* ptr = malloc(100);
uvhttp_free(ptr);  /* 错误！ */

/* 正确：使用统一的分配器 */
void* ptr = uvhttp_alloc(100);
uvhttp_free(ptr);  /* 正确 */
```

#### 规则 2：检查分配结果

```c
/* 正确：检查分配是否成功 */
void* buffer = uvhttp_alloc(size);
if (!buffer) {
    return UVHTTP_ERROR_OUT_OF_MEMORY;
}
```

#### 规则 3：配对分配和释放

```c
/* 确保每个分配都有对应的释放 */
void init_connection(uvhttp_connection_t** conn_out) {
    uvhttp_connection_t* conn = uvhttp_alloc(sizeof(uvhttp_connection_t));
    if (!conn) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    /* 初始化... */
    
    *conn_out = conn;
    return UVHTTP_OK;
}

void cleanup_connection(uvhttp_connection_t* conn) {
    if (conn) {
        /* 清理资源... */
        uvhttp_free(conn);
    }
}
```

#### 规则 4：避免内存泄漏

```c
/* 错误：可能泄漏 */
void process_request(uvhttp_request_t* request) {
    char* buffer = uvhttp_alloc(1024);
    if (error) {
        return;  /* 泄漏 buffer */
    }
    
    /* 处理... */
    uvhttp_free(buffer);
}

/* 正确：确保释放 */
void process_request(uvhttp_request_t* request) {
    char* buffer = uvhttp_alloc(1024);
    if (!buffer) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    if (error) {
        uvhttp_free(buffer);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 处理... */
    uvhttp_free(buffer);
    return UVHTTP_OK;
}
```

### 内存泄漏检测

#### 使用 Valgrind

```bash
# 编译调试版本
cmake -DENABLE_DEBUG=ON ..
make

# 运行 Valgrind
valgrind --leak-check=full --show-leak-kinds=all ./your_server
```

#### 使用 AddressSanitizer

```bash
# 编译时启用 ASan
cmake -DCMAKE_C_FLAGS="-g -O1 -fsanitize=address" ..
make

# 运行测试
./your_server
```

### 性能考虑

- **内联优化**：所有分配函数都是内联函数，零运行时开销
- **编译期选择**：分配器类型在编译期确定，无运行时开销
- **mimalloc 优势**：
  - 更快的内存分配和释放
  - 更好的多线程性能
  - 更少的内存碎片

---

## 错误处理规范

### 错误类型

UVHTTP 使用统一的错误类型 `uvhttp_error_t`：

```c
typedef enum {
    UVHTTP_OK = 0,

    /* General errors */
    UVHTTP_ERROR_INVALID_PARAM = -1,
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,
    UVHTTP_ERROR_NOT_FOUND = -3,
    UVHTTP_ERROR_NULL_POINTER = -5,
    UVHTTP_ERROR_TIMEOUT = -7,

    /* Server errors */
    UVHTTP_ERROR_SERVER_INIT = -100,
    UVHTTP_ERROR_SERVER_LISTEN = -101,
    UVHTTP_ERROR_CONNECTION_LIMIT = -103,

    /* Connection errors */
    UVHTTP_ERROR_CONNECTION_INIT = -200,
    UVHTTP_ERROR_CONNECTION_TIMEOUT = -205,

    /* Request/Response errors */
    UVHTTP_ERROR_REQUEST_INIT = -300,
    UVHTTP_ERROR_RESPONSE_SEND = -302,
    UVHTTP_ERROR_HEADER_TOO_LARGE = -305,
    UVHTTP_ERROR_BODY_TOO_LARGE = -306,

    /* TLS errors */
    UVHTTP_ERROR_TLS_INIT = -400,
    UVHTTP_ERROR_TLS_HANDSHAKE = -402,

    /* ... 更多错误码 */
} uvhttp_error_t;
```

### 错误处理模式

#### 模式 1：基本错误检查

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(result));
    return 1;
}
```

#### 模式 2：详细错误信息

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "Category: %s\n", uvhttp_error_category_string(result));
    fprintf(stderr, "Description: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "Suggestion: %s\n", uvhttp_error_suggestion(result));
    return 1;
}
```

#### 模式 3：错误恢复

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    if (uvhttp_error_is_recoverable(result)) {
        /* 尝试使用备用端口 */
        result = uvhttp_server_listen(server, "0.0.0.0", 8081);
        if (result == UVHTTP_OK) {
            fprintf(stderr, "Using fallback port 8081\n");
        } else {
            fprintf(stderr, "Failed to listen on fallback port\n");
            return 1;
        }
    } else {
        fprintf(stderr, "Fatal error: %s\n", uvhttp_error_string(result));
        return 1;
    }
}
```

#### 模式 4：资源清理

```c
uvhttp_error_t init_server(uvhttp_server_t** server_out) {
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        uvhttp_server_free(server);
        return result;
    }

    *server_out = server;
    return UVHTTP_OK;
}
```

### 错误处理 API

```c
/* 获取错误名称 */
const char* uvhttp_error_string(uvhttp_error_t error);

/* 获取错误分类 */
const char* uvhttp_error_category_string(uvhttp_error_t error);

/* 获取错误描述 */
const char* uvhttp_error_description(uvhttp_error_t error);

/* 获取修复建议 */
const char* uvhttp_error_suggestion(uvhttp_error_t error);

/* 检查是否可恢复 */
int uvhttp_error_is_recoverable(uvhttp_error_t error);
```

### 错误处理最佳实践

1. **检查所有可能失败的函数调用**
2. **使用统一的错误类型**
3. **提供有意义的错误信息**
4. **正确处理可恢复错误**
5. **确保资源正确清理**

---

## 测试规范

### 测试类型

#### 单元测试

- **目的**：测试单个函数和模块
- **工具**：Google Test
- **位置**：`test/unit/`
- **命名**：`test_<module>_<functionality>.cpp`

```cpp
/* 示例：test_router_add_route.cpp */
TEST(RouterTest, AddRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);
    
    uvhttp_router_add_route(router, "/api", test_handler);
    
    EXPECT_EQ(router->route_count, 1);
    
    uvhttp_router_free(router);
}
```

#### 集成测试

- **目的**：测试模块间交互
- **工具**：Google Test + libuv
- **位置**：`test/integration/`
- **命名**：`test_<feature>_integration.c`

#### 性能测试

- **目的**：验证性能指标
- **工具**：wrk, ab, benchmark/
- **位置**：`benchmark/`
- **命名**：`benchmark_<metric>.c`

### 覆盖率目标

- **目标覆盖率**：80%
- **当前覆盖率**：42.7%（需提升）
- **低覆盖率模块**：
  - uvhttp_websocket_wrapper.c (5.1%)
  - uvhttp_server.c (16.7%)
  - uvhttp_request.c (28.3%)
  - uvhttp_tls_openssl.c (28.1%)

### 测试命名约定

#### 测试文件

```bash
test_<module>_<functionality>.cpp
test_<module>_<functionality>_coverage.cpp
test_<module>_simple_api_coverage.cpp
```

#### 测试用例

```cpp
/* Google Test 风格 */
TEST(TestCaseName, TestName) {
    /* 测试代码 */
}

/* 参数化测试 */
TEST_P(TestCaseName, TestName) {
    /* 测试代码 */
}
```

### 测试配置

#### 超时设置

```cmake
/* 慢速测试（360 秒） */
set_tests_properties(${test_name} PROPERTIES 
    TIMEOUT 360 
    LABELS "slow" 
    RUN_SERIAL TRUE
)

/* 快速测试（90 秒） */
set_tests_properties(${test_name} PROPERTIES 
    TIMEOUT 90 
    LABELS "fast"
)
```

#### 慢速测试列表

- test_server_full_coverage
- test_deps_full_coverage
- test_server_rate_limit_coverage
- test_server_simple_api_coverage
- test_static_prewarm_coverage
- test_stress
- test_memory

### 运行测试

#### 完整测试

```bash
./run_tests.sh
```

#### 快速测试

```bash
./run_tests.sh --fast
```

#### 详细覆盖率报告

```bash
./run_tests.sh --detailed
```

### 测试最佳实践

1. **每个新功能必须有测试**
2. **Bug 修复必须有回归测试**
3. **测试覆盖率必须 >= 80%**
4. **测试命名清晰易懂**
5. **使用断言验证预期行为**
6. **避免测试之间的依赖**

---

## 提交规范

### 提交信息格式

使用 [Conventional Commits](https://www.conventionalcommits.org/) 格式：

```
<type>(<scope>): <subject>

<body>

<footer>
```

#### 类型（type）

- `feat`: 新功能
- `fix`: Bug 修复
- `docs`: 文档更新
- `style`: 代码格式（不影响功能）
- `refactor`: 重构
- `test`: 测试相关
- `chore`: 构建/工具链相关
- `perf`: 性能优化
- `ci`: CI/CD 相关

#### 提交信息示例

```
feat(server): 添加 WebSocket 连接池支持

- 实现连接池管理
- 添加连接超时检测
- 优化连接复用逻辑

Closes #123
```

```
fix(connection): 修复空指针导致的崩溃

问题：当连接对象为 NULL 时，on_read 回调未检查导致崩溃
修复：添加 NULL 指针检查

Fixes #456
```

```
docs: 更新 API 文档

- 添加新函数文档
- 修正示例代码错误
- 更新版本号
```

### 分支策略

#### 主要分支

```
develop → main → pre-release → release
   ↓        ↓         ↓            ↓
 开发中   预发布测试  发布准备     生产环境
```

#### 分支命名

- `feature/功能名称` - 新功能开发
- `fix/问题描述` - Bug 修复
- `refactor/重构描述` - 代码重构
- `docs/文档更新` - 文档更新
- `test/测试相关` - 测试相关
- `hotfix/问题描述` - 紧急修复

#### 开发流程

1. 从 `develop` 创建功能分支
2. 开发和测试
3. 提交更改（遵循提交信息格式）
4. 推送分支
5. 创建 PR 到 `develop`
6. 代码审查（至少 1 人）
7. 合并到 `develop`

#### 发布流程

1. `develop` → `main`（PR，运行完整 CI/CD）
2. `main` → `pre-release`（合并）
3. `pre-release` → `release`（合并）
4. `release` → 创建 Git 标签
5. `release` → `develop`（合并回开发分支）

### 代码审查清单

在提交 PR 前，请确保：

- [ ] 代码遵循项目风格规范
- [ ] 添加了必要的单元测试
- [ ] 所有测试通过
- [ ] 没有编译警告
- [ ] 更新了相关文档
- [ ] 提交信息格式正确
- [ ] 没有引入新的安全漏洞
- [ ] 内存管理正确（使用 UVHTTP_MALLOC/UVHTTP_FREE）
- [ ] 错误处理完整
- [ ] 无全局变量（使用 server->context 或 loop->data）

### CI/CD 检查

#### ci-pr.yml

- **触发**：PR 到 `main` 或 `develop`
- **用途**：快速验证（20 分钟）
- **检查**：构建、单元测试、代码质量、性能回归检测

#### ci-push.yml

- **触发**：Push 到任何分支
- **用途**：完整验证（45 分钟）
- **检查**：多平台构建、完整测试、安全扫描、性能测试

#### ci-nightly.yml

- **触发**：每天运行
- **用途**：深度测试（120 分钟）
- **检查**：代码覆盖率、内存泄漏、压力测试、完整性能测试

---

## 性能规范

### 性能目标

| 指标 | 目标值 | 当前值 |
|------|--------|--------|
| 峰值吞吐量 | 25,000 RPS | 23,226 RPS |
| 平均延迟 | < 5 ms | 2.92 ms |
| 内存使用 | < 100 MB | 50 MB |
| CPU 使用 | < 80% | 60% |
| 错误率 | < 0.1% | < 0.1% |

### 性能测试方法

#### 使用 wrk

```bash
# 启动测试服务器
./build/dist/bin/benchmark_rps &
SERVER_PID=$!
sleep 3

# 运行性能测试
wrk -t4 -c100 -d30s http://localhost:18081/

# 清理
kill $SERVER_PID
```

#### 使用 ab

```bash
ab -n 10000 -c 100 http://localhost:8080/
```

#### 性能基准测试

```bash
cd benchmark
./run_benchmarks.sh
```

### 性能优化技术

#### 1. 零拷贝优化

```c
/* 大文件使用 sendfile */
if (file_size > UVHTTP_FILE_SIZE_SMALL) {
    uvhttp_static_sendfile(file_path, response);
}
```

**性能提升**：50%+

#### 2. 智能缓存

```c
/* 预热缓存 */
uvhttp_static_prewarm_cache(ctx, "/static/index.html");
```

**性能提升**：300%+

#### 3. 连接池

```c
/* Keep-Alive 连接自动管理 */
config->keep_alive_timeout = 60;
```

**性能提升**：1000x

#### 4. 快速哈希

```c
/* 使用 xxHash */
uint64_t hash = xxhash64(key, key_len);
```

**性能提升**：10x

### 性能回归测试

#### 基线配置

```yaml
# config/performance-baseline.yml
baseline:
  rps: 23000
  latency: 3.0
  memory: 50

threshold:
  rps: 0.95  # 允许 5% 下降
  latency: 1.1  # 允许 10% 增加
  memory: 1.2  # 允许 20% 增加
```

#### 回归检测

```bash
# 运行性能回归测试
python scripts/performance_regression.py
```

### 性能监控

#### 内置指标

```c
/* 获取连接统计 */
size_t active_connections = server->stats.active_connections;
size_t total_requests = server->stats.total_requests;
```

#### 外部工具

```bash
# CPU 使用
top

# 内存使用
valgrind --tool=massif ./your_server

# 网络性能
netstat -s
```

---

## 文档规范

### API 文档要求

#### Doxygen 注释

```c
/**
 * @brief Create a new HTTP server
 *
 * @param loop The libuv event loop
 * @return uvhttp_server_t* Server object, or NULL on failure
 *
 * @note The server does not start listening until uvhttp_server_listen() is called
 * @see uvhttp_server_listen()
 * @see uvhttp_server_free()
 * 
 * @code
 * uv_loop_t* loop = uv_default_loop();
 * uvhttp_server_t* server = uvhttp_server_new(loop);
 * uvhttp_server_listen(server, "0.0.0.0", 8080);
 * @endcode
 */
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
```

#### API 文档结构

```
docs/api/
├── API_REFERENCE.md       # 完整 API 参考
├── SERVER_API.md          # 服务器 API
├── ROUTER_API.md          # 路由 API
├── REQUEST_API.md         # 请求 API
├── RESPONSE_API.md        # 响应 API
└── WEBSOCKET_API.md       # WebSocket API
```

### 代码注释要求

#### 文件头注释

```c
/*
 * UVHTTP server module
 *
 * Provides core HTTP server functionality including connection management,
 * request routing, and response processing
 * Implements high-performance asynchronous I/O based on libuv
 *
 * @author UVHTTP Team
 * @version 2.2.0
 */
```

#### 函数注释

```c
/**
 * @brief Process incoming HTTP request
 *
 * @param request The request object
 * @return uvhttp_error_t Error code (UVHTTP_OK on success)
 */
static uvhttp_error_t process_request(uvhttp_request_t* request);
```

#### 行内注释

```c
/* Check if connection limit is reached */
if (server->active_connections >= max_connections) {
    return UVHTTP_ERROR_CONNECTION_LIMIT;
}
```

### README 要求

#### 主 README (README.md)

```markdown
# UVHTTP

## 简介
UVHTTP 是一个基于 libuv 的高性能 HTTP 服务器库。

## 特性
- 高性能
- 轻量级
- 易于使用

## 快速开始
\`\`\`c
#include "uvhttp.h"

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    return 0;
}
\`\`\`

## 文档
- [API 参考](docs/api/API_REFERENCE.md)
- [开发者指南](docs/guide/DEVELOPER_GUIDE.md)
- [教程](docs/guide/TUTORIAL.md)
```

### 文档更新流程

1. **代码变更**：修改代码时同步更新文档
2. **API 变更**：更新 API_REFERENCE.md
3. **新功能**：添加教程和示例
4. **版本发布**：更新 CHANGELOG.md

---

## 安全规范

### 输入验证

#### 验证所有输入

```c
/* 验证指针参数 */
if (!request || !response) {
    return UVHTTP_ERROR_NULL_POINTER;
}

/* 验证字符串长度 */
if (strlen(header_name) > UVHTTP_MAX_HEADER_NAME_SIZE) {
    return UVHTTP_ERROR_HEADER_TOO_LARGE;
}

/* 验证数值范围 */
if (port < 0 || port > 65535) {
    return UVHTTP_ERROR_INVALID_PARAM;
}
```

### 缓冲区溢出保护

#### 使用安全的字符串函数

```c
/* 错误：不安全的 strcpy */
strcpy(dest, src);  /* 可能溢出 */

/* 正确：使用 strncpy */
strncpy(dest, src, sizeof(dest) - 1);
dest[sizeof(dest) - 1] = '\0';
```

#### 边界检查

```c
/* 检查数组边界 */
if (index >= array_size) {
    return UVHTTP_ERROR_INVALID_PARAM;
}

/* 检查缓冲区大小 */
if (buffer_size > UVHTTP_MAX_BUFFER_SIZE) {
    return UVHTTP_ERROR_BUFFER_TOO_SMALL;
}
```

### 内存安全

#### 避免内存泄漏

```c
/* 确保资源释放 */
void cleanup() {
    if (buffer) {
        uvhttp_free(buffer);
        buffer = NULL;
    }
    
    if (server) {
        uvhttp_server_free(server);
        server = NULL;
    }
}
```

#### 避免悬空指针

```c
/* 释放后置空 */
uvhttp_free(ptr);
ptr = NULL;
```

### 错误处理

#### 检查所有返回值

```c
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    /* 处理错误 */
    return result;
}
```

### 安全编译选项

```cmake
# 启用所有安全选项
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} \
    -Wall \
    -Wextra \
    -Werror \
    -fstack-protector-strong \
    -D_FORTIFY_SOURCE=2 \
")
```

### 依赖安全

#### 定期更新依赖

```bash
# 更新子模块
git submodule update --remote

# 检查安全漏洞
npm audit  # 如果使用 npm
```

#### 使用 Dependabot

Dependabot 自动检测依赖漏洞并创建 PR。

---

## 架构设计原则

### 1. 专注核心（Focus on Core）

**原则**：只实现核心功能，不内置业务逻辑

**实践**：
- 不内置认证、数据库、缓存等业务功能
- 应用层完全控制业务逻辑
- 库只提供 HTTP 协议处理和 WebSocket 支持

**收益**：
- 库更小、更快、更易维护

### 2. 零开销（Zero Overhead）

**原则**：生产环境无任何抽象层成本

**实践**：
- 直接调用 libuv，无包装层
- 使用内联函数和编译器优化
- 避免虚函数表和动态分发
- 编译期宏实现日志、中间件等功能

**收益**：
- 性能提升 30%+
- 内存占用减少 88%

### 3. 极简工程（Minimalist Engineering）

**原则**：少即是多，移除所有不必要的复杂度

**实践**：
- 移除未使用的抽象层（如 network_interface、network_type）
- 移除测试模式代码（UVHTTP_TEST_MODE）
- 移除未使用的宏（UVHTTP_RETURN_IF_ERROR、UVHTTP_GOTO_IF_ERROR）
- 移除自定义内存池，使用 mimalloc
- 移除 WebSocket 认证模块（应用层实现）

**收益**：
- 代码更简洁，维护成本更低

### 4. 测试分离（Test Separation）

**原则**：库代码中无任何测试专用代码

**实践**：
- 使用链接时注入（linker wrap）实现 mock
- 测试代码完全独立于生产代码
- 不在库中添加测试钩子或调试代码
- 禁用 32 个使用旧 API 的测试文件

**收益**：
- 库代码纯净，生产环境零影响

### 5. 零全局变量（Zero Global Variables）

**原则**：避免全局变量，支持多实例和单元测试

**实践**：
- 使用 libuv 数据指针模式（loop->data 或 server->context）
- 所有状态通过参数传递
- 支持多实例并发运行
- 移除 g_uvhttp_context 全局变量

**收益**：
- 线程安全、可测试、云原生友好

### 6. 上下文传递（Context Passing）

**原则**：避免独占 loop->data，允许其他应用共享 loop

**实践**：
- 使用 server->context 传递上下文，而非 loop->data
- 避免独占 loop->data 影响其他功能
- 支持多应用共享同一个 libuv 循环

**收益**：
- 更好的兼容性，允许 loop->data 用于其他目的

---

## 附录

### A. 常用命令

```bash
# 构建项目
mkdir build && cd build
cmake ..
make -j$(nproc)

# 运行测试
./run_tests.sh

# 生成覆盖率报告
./run_tests.sh --detailed

# 格式化代码
clang-format -i src/*.c include/*.h

# 检查内存泄漏
valgrind --leak-check=full ./your_server

# 性能测试
wrk -t4 -c100 -d30s http://localhost:8080/
```

### B. 相关文档

- [API 参考](docs/api/API_REFERENCE.md)
- [开发者指南](docs/guide/DEVELOPER_GUIDE.md)
- [教程](docs/guide/TUTORIAL.md)
- [分支策略](docs/BRANCH_STRATEGY.md)
- [发布检查清单](docs/RELEASE_CHECKLIST.md)
- [性能文档](docs/performance.md)

### C. 联系方式

- 项目主页：https://github.com/adam-ikari/uvhttp
- 问题反馈：https://github.com/adam-ikari/uvhttp/issues
- 讨论：https://github.com/adam-ikari/uvhttp/discussions

---

**文档版本**：2.2.0  
**最后更新**：2026-01-31  
**维护者**：UVHTTP Team