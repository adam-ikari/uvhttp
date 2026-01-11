# UVHTTP 项目上下文

## 项目概述

UVHTTP 是一个基于 libuv 的高性能、轻量级 HTTP/1.1 和 WebSocket 服务器库，采用 C11 标准编写。项目专注于核心 HTTP 功能，提供简洁的 API、完善的文档和生产就绪的质量保证。

### 核心特性

- **高性能**: 基于 libuv 事件驱动架构，集成 xxHash 极快哈希算法，峰值吞吐量达 16,832 RPS
- **零拷贝优化**: 支持大文件零拷贝传输（sendfile），性能提升 50%+
- **智能缓存**: LRU 缓存 + 缓存预热机制，显著提升重复请求性能
- **安全**: 缓冲区溢出保护、输入验证、TLS 1.3 支持（通过 mbedtls）
- **生产就绪**: 零编译警告、完整错误处理、性能监控
- **模块化**: 支持静态文件服务、WebSocket、限流等功能模块，通过编译宏控制
- **内存优化**: 默认使用 mimalloc 分配器，可选系统分配器

### 技术栈

- **语言**: C11
- **核心依赖**: libuv（异步 I/O）、llhttp（HTTP 解析）
- **可选依赖**: mbedtls（TLS）、mimalloc（内存分配）、cjson（JSON）
- **构建系统**: CMake 3.10+
- **测试框架**: Google Test
- **性能测试**: wrk、ab

## 项目结构

```
uvhttp/
├── include/           # 公共头文件
│   ├── uvhttp.h      # 主头文件
│   ├── uvhttp_*.h    # 模块头文件（34个）
│   └── uvhttp_features.h  # 特性配置
├── src/              # 源代码实现
│   ├── uvhttp_*.c    # 核心模块（18个）
│   └── uvhttp_websocket_native.c  # WebSocket 实现
├── docs/             # 文档（19个）
│   ├── API_REFERENCE.md
│   ├── ARCHITECTURE.md
│   ├── DEVELOPER_GUIDE.md
│   ├── TUTORIAL.md
│   ├── PERFORMANCE_BENCHMARK.md
│   ├── PERFORMANCE_TESTING_STANDARD.md
│   ├── RATE_LIMIT_API.md
│   ├── STATIC_FILE_SERVER.md
│   ├── MIDDLEWARE_SYSTEM.md
│   └── LIBUV_DATA_POINTER.md
├── examples/         # 示例程序
│   ├── 01_basics/    # 基础示例
│   ├── 02_routing/   # 路由示例
│   ├── 04_responses/ # 响应处理示例
│   ├── 05_advanced/  # 高级功能示例
│   └── performance_*.c  # 性能测试示例
├── test/             # 测试
│   ├── unit/         # 单元测试（8个）
│   ├── gtest/        # Google Test 框架
│   └── performance/  # 性能测试脚本
├── public/           # 静态文件测试目录
├── deps/             # 第三方依赖（子模块）
│   ├── libuv/
│   ├── llhttp/
│   ├── mbedtls/
│   ├── cjson/
│   ├── mimalloc/
│   ├── uthash/
│   └── xxhash/
├── build/            # 构建输出目录
├── CMakeLists.txt    # 主构建配置
└── run_tests.sh      # 测试脚本
```

## 构建和运行

### 基本构建

```bash
# 创建构建目录
mkdir build && cd build

# 配置项目（默认选项）
cmake ..

# 编译
make -j$(nproc)
```

### 构建选项

```bash
# 启用/禁用 WebSocket 支持
cmake -DBUILD_WITH_WEBSOCKET=ON ..
cmake -DBUILD_WITH_WEBSOCKET=OFF ..

# 启用/禁用 mimalloc 分配器
cmake -DBUILD_WITH_MIMALLOC=ON ..
cmake -DBUILD_WITH_MIMALLOC=OFF ..

# Debug 模式（禁用优化）
cmake -DENABLE_DEBUG=ON ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..

# 启用示例程序编译
cmake -DBUILD_EXAMPLES=ON ..
```

### 运行示例

```bash
# 编译示例程序
cd build
make

# 运行 Hello World 示例
./dist/bin/helloworld

# 运行性能测试服务器
./dist/bin/performance_static_server -d ./public -p 8080

# 访问 http://127.0.0.1:8080
```

### 运行测试

```bash
# 使用测试脚本（推荐）
./run_tests.sh

# 或手动运行
cd build
ctest

# 运行特定测试
./uvhttp_unit_tests

# 生成覆盖率报告
./run_tests.sh --detailed
```

### 测试脚本选项

```bash
./run_tests.sh -h, --help     # 显示帮助
./run_tests.sh -c, --clean    # 仅清理构建文件
./run_tests.sh -b, --build    # 仅构建项目
./run_tests.sh -t, --test     # 仅运行测试
./run_tests.sh -f, --fast     # 快速模式（跳过性能测试）
./run_tests.sh -d, --detailed  # 生成详细覆盖率报告
```

### 性能测试

```bash
# 使用 wrk 进行性能测试
wrk -t4 -c100 -d30s http://localhost:8080/

# 使用 ab 进行性能测试
ab -n 10000 -c 100 http://localhost:8080/
```

## 开发约定

### 代码风格

- **标准**: 使用 C11 标准
- **命名约定**:
  - 函数: `uvhttp_module_action`（如 `uvhttp_server_new`）
  - 类型: `uvhttp_name_t`（如 `uvhttp_server_t`）
  - 常量: `UVHTTP_UPPER_CASE`（如 `UVHTTP_MAX_HEADERS`）
  - 全局变量: `g_` 前缀
- **缩进**: 4 个空格，不使用制表符
- **大括号**: K&R 风格

### API 使用规范

UVHTTP 采用统一的核心 API 设计，所有开发应直接使用核心 API：

```c
// 标准服务器创建流程
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;

// 添加路由
uvhttp_router_add_route(router, "/api", api_handler);

// 启动服务器
uvhttp_server_listen(server, "0.0.0.0", 8080);
uv_run(loop, UV_RUN_DEFAULT);
```

### 响应处理标准模式

所有响应处理应遵循以下模式：
1. 设置状态码（`uvhttp_response_set_status`）
2. 设置响应头（`uvhttp_response_set_header`）
3. 设置响应体（`uvhttp_response_set_body`）
4. 发送响应（`uvhttp_response_send`）

### 内存管理

- 使用统一分配器宏: `UVHTTP_MALLOC`、`UVHTTP_FREE`
- 检查分配是否成功
- 确保每个分配都有对应的释放
- 默认使用 mimalloc，可通过编译宏切换到系统分配器
- **严禁混用** `malloc/free` 和 `UVHTTP_MALLOC/UVHTTP_FREE`

### 错误处理

- 检查所有可能失败的函数调用
- 使用统一的错误类型 `uvhttp_error_t`
- 返回 `UVHTTP_OK` 表示成功，其他值表示错误
- **错误码机制**：
  - 所有错误码都是负数，`UVHTTP_OK (0)` 表示成功
  - 错误码按功能分类（服务器、连接、请求、TLS、路由等）
  - 提供错误码解读 API：
    - `uvhttp_error_string()` - 获取错误名称
    - `uvhttp_error_category_string()` - 获取错误分类
    - `uvhttp_error_description()` - 获取错误描述
    - `uvhttp_error_suggestion()` - 获取修复建议
    - `uvhttp_error_is_recoverable()` - 检查是否可恢复
  - 详细的错误码参考：[错误码参考](docs/ERROR_CODES.md)

### 错误处理示例

```c
// 基本错误处理
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "描述: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "建议: %s\n", uvhttp_error_suggestion(result));
    return 1;
}

// 错误恢复
if (result != UVHTTP_OK && uvhttp_error_is_recoverable(result)) {
    // 尝试恢复操作
    result = uvhttp_server_listen(server, "0.0.0.0", 8081);
}
```

### 性能优化最佳实践

#### 1. 零拷贝文件传输

对于大文件（> 1MB），使用 sendfile 零拷贝优化：

```c
// 自动集成：在 uvhttp_static_handle_request 中自动使用
// 文件 > 1MB 时自动使用 sendfile

// 手动使用：
uvhttp_result_t result = uvhttp_static_sendfile("/path/to/file", response);
```

#### 2. 缓存预热

在服务器启动时预热常用文件：

```c
// 预热单个文件
uvhttp_static_prewarm_cache(ctx, "/static/index.html");

// 预热整个目录（最多100个文件）
uvhttp_static_prewarm_directory(ctx, "/static", 100);
```

#### 3. 路由优化

使用快速路由匹配，避免通配符路由：

```c
// 推荐：具体路由
uvhttp_router_add_route(router, "/api/users", users_handler);
uvhttp_router_add_route(router, "/api/posts", posts_handler);

// 避免：通配符路由（性能较差）
// uvhttp_router_add_route(router, "/api/*", api_handler);
```

### 功能模块

项目采用功能模块架构，通过编译宏控制：

- **WebSocket**: `BUILD_WITH_WEBSOCKET`（默认启用）
  - 使用 `uvhttp_ws_middleware_t` 创建中间件
  - 通过 `uvhttp_ws_middleware_set_callbacks()` 设置回调
  - 注册到服务器: `uvhttp_server_add_middleware()`
  - 发送消息: `uvhttp_ws_middleware_send()`

- **日志系统**: `UVHTTP_FEATURE_LOGGING`（默认启用）
  - 使用 `uvhttp_log_middleware_t` 创建日志中间件
  - 支持多种日志级别（TRACE, DEBUG, INFO, WARN, ERROR, FATAL）
  - 支持多种输出方式（标准输出、文件、自定义回调）
  - 支持文本和 JSON 格式
  - 可通过编译宏完全关闭以零开销

- **静态文件服务**: `UVHTTP_FEATURE_STATIC_FILES`（默认启用）
  - 使用 `uvhttp_static_create()` 创建上下文
  - 通过路由集成: `uvhttp_router_add_route(router, "/static/*", handler)`
  - 处理函数: `uvhttp_static_handle_request()`
  - 支持缓存预热: `uvhttp_static_prewarm_cache()`
  - 支持零拷贝: `uvhttp_static_sendfile()`

- **限流功能**: `UVHTTP_FEATURE_RATE_LIMIT`（默认启用）
  - 使用 `uvhttp_rate_limit_t` 创建限流器
  - 支持令牌桶算法
  - 支持白名单机制
  - 详细的 API 文档：[限流 API](docs/RATE_LIMIT_API.md)

### 编译配置

所有功能模块通过 `include/uvhttp_features.h` 控制：

```c
// 功能模块
UVHTTP_FEATURE_WEBSOCKET      // WebSocket 支持
UVHTTP_FEATURE_STATIC_FILES   // 静态文件服务
UVHTTP_FEATURE_TLS            // TLS/SSL 支持
UVHTTP_FEATURE_RATE_LIMIT     // 限流功能

// 其他特性
UVHTTP_FEATURE_LRU_CACHE      // LRU 缓存
UVHTTP_FEATURE_ROUTER_CACHE   // 路由缓存
UVHTTP_FEATURE_LOGGING        // 日志系统
```

### 框架职责

UVHTTP 专注于框架核心功能，以下功能由应用层实现：
- JSON 解析（应用层可选择 cJSON 或其他库）
- 数据库集成（应用层根据需求选择）
- 业务逻辑（完全由应用层控制）

### 避免全局变量

推荐使用 libuv 数据指针模式避免全局变量：

```c
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
} app_context_t;

// 创建并设置上下文
app_context_t* ctx = app_context_create(loop);
loop->data = ctx;

// 在处理器中访问
app_context_t* ctx = (app_context_t*)loop->data;
```

详细教程请参考 `docs/LIBUV_DATA_POINTER.md`。

## 性能特性

### 性能优化措施

1. **Keep-Alive 连接管理**: 复用连接，性能提升约 1000 倍
2. **mimalloc 内存分配器**: 更快的内存分配和释放
3. **TCP 套接字优化**: TCP_NODELAY 和 TCP_KEEPALIVE
4. **路由匹配优化**: O(1) 快速前缀匹配
5. **零拷贝优化**: sendfile 大文件传输
6. **LRU 缓存**: 静态文件内容缓存
7. **缓存预热**: 减少首次请求延迟

### 性能指标

- **峰值吞吐量**: 16,832 RPS（主页）
- **静态文件**: 12,510 RPS（中等并发）
- **API 路由**: 13,950 RPS
- **平均延迟**: 2.92ms - 43.59ms
- **错误率**: < 0.1%

详细性能测试结果：[性能基准测试](docs/PERFORMANCE_BENCHMARK.md)

## 文档资源

### 核心文档

- **[API 参考](docs/API_REFERENCE.md)**: 完整的 API 文档
- **[架构设计](docs/ARCHITECTURE.md)**: 系统架构说明
- **[开发者指南](docs/DEVELOPER_GUIDE.md)**: 开发指南和最佳实践
- **[教程](docs/TUTORIAL.md)**: 从基础到高级的渐进式教程
- **[功能模块系统](docs/MIDDLEWARE_SYSTEM.md)**: 功能模块使用指南
- **[libuv 数据指针](docs/LIBUV_DATA_POINTER.md)**: 避免全局变量的最佳实践

### 专项文档

- **[错误码参考](docs/ERROR_CODES.md)**: 完整的错误码列表和解读
- **[依赖说明](docs/DEPENDENCIES.md)**: 第三方依赖说明
- **[变更日志](docs/CHANGELOG.md)**: 版本变更历史
- **[安全指南](docs/SECURITY.md)**: 安全相关说明
- **[性能基准](docs/PERFORMANCE_BENCHMARK.md)**: 性能测试结果
- **[性能测试标准](docs/PERFORMANCE_TESTING_STANDARD.md)**: 性能测试规范
- **[限流 API](docs/RATE_LIMIT_API.md)**: 限流功能 API 文档
- **[静态文件服务](docs/STATIC_FILE_SERVER.md)**: 静态文件服务指南
- **[服务器配置性能指南](docs/SERVER_CONFIG_PERFORMANCE_GUIDE.md)**: 服务器性能配置

## 测试

### 测试类型

- **单元测试**: 测试单个函数和模块
- **集成测试**: 测试模块间交互
- **性能测试**: 基准测试和性能回归
- **压力测试**: 高并发和长时间运行测试

### 测试文件

- 单元测试: `test/unit/`
- 测试辅助: `test/gtest/`
- 测试可执行文件: `build/uvhttp_unit_tests`

### 代码覆盖率

当前代码覆盖率目标: 80%

使用 `./run_tests.sh` 运行测试并生成覆盖率报告。

## 常见任务

### 添加新功能

1. 在 `include/` 中添加头文件
2. 在 `src/` 中实现功能
3. 在 `include/uvhttp_features.h` 中添加特性宏（如需要）
4. 添加单元测试
5. 更新相关文档

### 修复 Bug

1. 在 `test/unit/` 中添加重现测试
2. 修复代码
3. 确保所有测试通过
4. 更新文档（如需要）

### 优化性能

1. 使用性能测试工具（如 `wrk`、`ab`、`perf`、`valgrind`）
2. 添加性能基准测试
3. 实施优化
4. 运行性能回归测试
5. 更新性能文档

### 性能测试

```bash
# 启动测试服务器
./build/dist/bin/performance_static_server -d ./public -p 8080

# 运行 wrk 测试
wrk -t4 -c100 -d30s http://localhost:8080/

# 运行 ab 测试
ab -n 10000 -c 100 http://localhost:8080/
```

## 版本信息

- **当前版本**: 1.2.0
- **最低 CMake 版本**: 3.10
- **C 标准**: C11
- **最新提交**: 943bc2c - "perf: 完成性能优化和代码质量改进"

## 相关链接

- 项目主页: https://github.com/adam-ikari/uvhttp
- 问题反馈: https://github.com/adam-ikari/uvhttp/issues
- 许可证: MIT License

## 重要提示

- 所有依赖都包含在 `deps/` 目录中，无需额外安装系统依赖
- 项目采用自包含的依赖管理方式
- 编译时默认启用所有安全选项和警告
- 遵循零编译警告原则
- 所有文档基于实际代码库实现，确保准确性
- 性能优化已完成，峰值吞吐量达 16,832 RPS
- 代码质量评分: 9/10，已准备好生产部署