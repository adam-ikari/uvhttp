# UVHTTP AI Agent 指南

本文档专为 AI Agent 设计，提供结构化的项目信息，帮助 AI Agent 快速理解项目结构、开发规范和最佳实践。

## 项目概览

### 基本信息
- **项目名称**: UVHTTP
- **版本**: 2.0.0
- **语言**: C11
- **类型**: 高性能 HTTP/1.1 和 WebSocket 服务器库
- **核心依赖**: libuv, llhttp, mbedtls (可选), mimalloc (可选)
- **构建系统**: CMake 3.10+
- **测试框架**: Google Test

### 项目目标
为 C 语言应用提供高性能、轻量级、生产就绪的 HTTP/1.1 和 WebSocket 服务器库。

### 核心特性
- 事件驱动架构（基于 libuv）
- 零拷贝优化（sendfile）
- 智能缓存（LRU + 预热）
- TLS 1.3 支持（mbedtls）
- 模块化设计（编译宏控制）
- 统一内存管理（UVHTTP_MALLOC/FREE）
- 零编译警告

## 项目结构

### 目录布局
```
uvhttp/
├── include/           # 公共头文件（34个）
│   ├── uvhttp.h      # 主头文件
│   ├── uvhttp_*.h    # 模块头文件
│   └── uvhttp_features.h  # 特性配置
├── src/              # 源代码实现（18个）
│   └── uvhttp_*.c    # 核心模块实现
├── docs/             # 文档
├── examples/         # 示例程序
├── test/             # 测试
│   ├── unit/         # 单元测试
│   └── performance/  # 性能测试
├── deps/             # 第三方依赖（子模块）
└── build/            # 构建输出
```

### 核心模块

#### 服务器模块
- `uvhttp_server.h/c` - 服务器核心
- `uvhttp_router.h/c` - 路由系统
- `uvhttp_connection.h/c` - 连接管理
- `uvhttp_request.h/c` - 请求处理
- `uvhttp_response.h/c` - 响应处理

#### 功能模块
- `uvhttp_websocket_native.h/c` - WebSocket 支持
- `uvhttp_tls.h/c` - TLS 支持
- `uvhttp_static.h/c` - 静态文件服务
- `uvhttp_middleware.h/c` - 中间件系统
- `uvhttp_lru_cache.h/c` - LRU 缓存
- `uvhttp_rate_limit.h/c` - 限流功能

#### 基础模块
- `uvhttp_error.h/c` - 错误处理
- `uvhttp_allocator.h/c` - 内存分配
- `uvhttp_config.h/c` - 配置管理
- `uvhttp_utils.h/c` - 工具函数
- `uvhttp_validation.h/c` - 输入验证

## 开发规范

### 命名约定
```c
// 函数：uvhttp_module_action
uvhttp_server_new()
uvhttp_router_add_route()

// 类型：uvhttp_name_t
uvhttp_server_t
uvhttp_router_t

// 常量：UVHTTP_UPPER_CASE
UVHTTP_MAX_HEADERS
UVHTTP_OK

// 全局变量：g_前缀（避免使用）
g_server_instance
```

### 代码风格
- **标准**: C11
- **缩进**: 4 个空格
- **大括号**: K&R 风格
- **编译警告**: 零警告原则
- **注释**: 解释"为什么"而非"是什么"

### 内存管理
```c
// 统一分配器
void* ptr = UVHTTP_MALLOC(size);
UVHTTP_FREE(ptr);

// 检查分配结果
if (!ptr) {
    return UVHTTP_ERR_OUT_OF_MEMORY;
}

// 确保每个分配都有对应的释放
```

### 错误处理
```c
// 检查所有可能失败的函数
uvhttp_error_t result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return result;
}

// 错误码机制
// UVHTTP_OK (0) = 成功
// 其他值 = 错误（负数）
```

### 响应处理标准模式
```c
// 1. 设置状态码
uvhttp_response_set_status(response, 200);

// 2. 设置响应头
uvhttp_response_set_header(response, "Content-Type", "text/html");

// 3. 设置响应体
uvhttp_response_set_body(response, body, length);

// 4. 发送响应
uvhttp_response_send(response);
```

## API 使用规范

### 标准服务器创建流程
```c
// 1. 创建事件循环
uv_loop_t* loop = uv_default_loop();

// 2. 创建服务器
uvhttp_server_t* server = uvhttp_server_new(loop);

// 3. 创建路由
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;

// 4. 添加路由
uvhttp_router_add_route(router, "/api", api_handler);

// 5. 启动服务器
uvhttp_server_listen(server, "0.0.0.0", 8080);

// 6. 运行事件循环
uv_run(loop, UV_RUN_DEFAULT);
```

### 避免全局变量
```c
// 推荐使用 libuv 数据指针模式
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

## 功能模块配置

### 编译宏控制
```c
// include/uvhttp_features.h
UVHTTP_FEATURE_WEBSOCKET      // WebSocket 支持
UVHTTP_FEATURE_TLS            // TLS/SSL 支持
UVHTTP_FEATURE_STATIC_FILES   // 静态文件服务
UVHTTP_FEATURE_RATE_LIMIT     // 限流功能
UVHTTP_FEATURE_LRU_CACHE      // LRU 缓存
UVHTTP_FEATURE_ROUTER_CACHE   // 路由缓存
UVHTTP_FEATURE_LOGGING        // 日志系统
UVHTTP_FEATURE_ALLOCATOR      // 统一分配器
```

### CMake 构建选项
```bash
# 启用/禁用 WebSocket
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# 启用/禁用 mimalloc
cmake -DBUILD_WITH_MIMALLOC=ON ..

# Debug 模式
cmake -DENABLE_DEBUG=ON ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..

# 启用示例程序
cmake -DBUILD_EXAMPLES=ON ..
```

## 测试规范

### 测试类型
- **单元测试**: 测试单个函数和模块
- **集成测试**: 测试模块间交互
- **性能测试**: 基准测试和性能回归
- **压力测试**: 高并发和长时间运行

### 测试文件位置
- 单元测试: `test/unit/`
- 测试辅助: `test/gtest/`
- 性能测试: `test/performance/`

### 运行测试
```bash
# 使用测试脚本（推荐）
./run_tests.sh

# 或手动运行
cd build
ctest

# 生成覆盖率报告
./run_tests.sh --detailed
```

### 代码覆盖率目标
- **当前覆盖率**: ~42.7%
- **目标覆盖率**: 80%

## 性能优化

### 性能指标
- **峰值吞吐量**: 16,832 RPS
- **静态文件**: 12,510 RPS
- **API 路由**: 13,950 RPS
- **平均延迟**: 2.92ms - 43.59ms
- **错误率**: < 0.1%

### 优化技术
1. **零拷贝传输**: 大文件使用 sendfile
2. **智能缓存**: LRU 缓存 + 缓存预热
3. **连接复用**: Keep-Alive 连接池
4. **异步 I/O**: 基于 libuv 事件驱动
5. **高效哈希**: xxHash 算法
6. **内存优化**: mimalloc 分配器

### 性能测试
```bash
# 使用 wrk
wrk -t4 -c100 -d30s http://localhost:8080/

# 使用 ab
ab -n 10000 -c 100 http://localhost:8080/
```

## 文档结构

### 使用者文档 (`/guide/`)
- 快速开始指南
- API 参考文档
- 教程
- 最佳实践
- 性能优化指南

### 开发者文档 (`/dev/`)
- 开发指南
- 架构设计
- 代码规范
- 测试标准
- 贡献指南

### 专项文档
- `API_REFERENCE.md` - 完整 API 参考
- `ARCHITECTURE.md` - 架构设计
- `ERROR_CODES.md` - 错误码参考
- `PERFORMANCE_BENCHMARK.md` - 性能基准
- `MIDDLEWARE_SYSTEM.md` - 中间件系统
- `STATIC_FILE_SERVER.md` - 静态文件服务

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

### 性能优化
1. 使用性能测试工具（wrk、ab、perf、valgrind）
2. 添加性能基准测试
3. 实施优化
4. 运行性能回归测试
5. 更新性能文档

## 重要提示

### 依赖管理
- 所有依赖都包含在 `deps/` 目录中
- 项目采用自包含的依赖管理方式
- 无需额外安装系统依赖

### 编译要求
- 遵循零编译警告原则
- 启用所有安全编译选项
- 使用 C11 标准

### 代码质量
- 代码质量评分: 9/10
- 已准备好生产部署
- 完整的错误处理
- 详细的性能监控

### 文档原则
- 所有文档基于实际代码库实现
- 确保准确性
- 性能优化已完成
- 提供完整的使用示例

## 相关资源

- **项目主页**: https://github.com/adam-ikari/uvhttp
- **问题反馈**: https://github.com/adam-ikari/uvhttp/issues
- **许可证**: MIT License

## AI Agent 快速参考

### 关键文件位置
- 主头文件: `include/uvhttp.h`
- 特性配置: `include/uvhttp_features.h`
- 服务器实现: `src/uvhttp_server.c`
- 错误处理: `src/uvhttp_error.c`
- 构建配置: `CMakeLists.txt`
- 测试脚本: `run_tests.sh`

### 关键模式
1. **libuv 循环注入模式** - 避免全局变量
2. **统一内存管理** - UVHTTP_MALLOC/FREE
3. **错误处理模式** - 检查所有返回值
4. **响应处理模式** - 状态码 -> 头部 -> 主体 -> 发送

### 关键约束
1. **零编译警告** - 所有警告必须修复
2. **统一内存分配** - 禁止混用 malloc/free
3. **错误处理** - 检查所有可能失败的函数
4. **命名约定** - 严格遵循命名规范

### 关键指标
1. **代码覆盖率目标**: 80%
2. **性能基准**: 16,832 RPS
3. **错误率**: < 0.1%
4. **P99 延迟**: ~10ms - ~100ms

---

**最后更新**: 2026-01-23
**文档版本**: 1.0.0