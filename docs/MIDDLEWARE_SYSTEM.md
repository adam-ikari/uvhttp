# UVHTTP 中间件系统

## 概述

UVHTTP 采用模块化的中间件架构，提供可配置的功能模块，包括静态文件服务、WebSocket 等。这些功能模块通过编译时宏进行控制，实现按需编译和功能裁剪。

## 中间件架构

### 核心特性

- **模块化设计**：每个中间件独立实现，互不干扰
- **编译时控制**：通过 CMake 宏启用/禁用功能模块
- **统一接口**：所有中间件通过 `uvhttp_http_middleware_t` 统一接口
- **优先级控制**：支持中间件优先级，控制执行顺序
- **零开销**：未启用的功能模块不会增加运行时开销

### 中间件类型

| 功能模块 | 宏定义 | 默认状态 | 实现方式 |
|----------|--------|----------|----------|
| 静态文件服务 | `UVHTTP_FEATURE_STATIC_FILES` | 启用 | HTTP 中间件 |
| WebSocket | `BUILD_WITH_WEBSOCKET` | 启用 | WebSocket 中间件 |

### 中间件工作流程

```
HTTP 请求
    ↓
中间件链（按优先级执行）
    ↓
路由系统
    ↓
业务处理器
    ↓
响应
```

## 编译时控制

### 启用/禁用功能模块

```bash
# 启用所有功能模块（默认）
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# 禁用 WebSocket
cmake -DBUILD_WITH_WEBSOCKET=OFF ..

# 禁用日志系统
cmake -DUVHTTP_FEATURE_LOGGING=OFF ..

# 静态文件服务始终启用
```

### 特性检查

在代码中使用条件编译检查功能模块是否可用：

```c
#if UVHTTP_FEATURE_STATIC_FILES
// 静态文件服务相关代码
#endif

#if UVHTTP_FEATURE_WEBSOCKET
// WebSocket 相关代码
#endif

#if UVHTTP_FEATURE_LOGGING
// 日志系统相关代码
#endif
```

## 功能模块使用

### 日志中间件

日志中间件提供灵活的日志功能，支持多种输出方式和格式：

```c
#include "uvhttp_log_middleware.h"

#if UVHTTP_FEATURE_LOGGING

int main() {
    // 创建日志中间件
    uvhttp_log_context_t log_context = UVHTTP_LOG_DEFAULT_CONTEXT;
    log_context.level = UVHTTP_LOG_LEVEL_DEBUG;
    log_context.output = UVHTTP_LOG_OUTPUT_FILE;
    strcpy(log_context.file_path, "uvhttp.log");
    
    uvhttp_log_middleware_t* log_middleware = uvhttp_log_middleware_create(&log_context);
    
    // 设置为全局日志中间件
    uvhttp_log_set_global_middleware(log_middleware);
    
    // 使用日志
    UVHTTP_LOG_INFO(log_middleware, "服务器启动");
    UVHTTP_LOG_DEBUG(log_middleware, "调试信息: %s", "详细内容");
    UVHTTP_LOG_ERROR(log_middleware, "发生错误: %d", error_code);
    
    // 清理
    uvhttp_log_middleware_destroy(log_middleware);
    
    return 0;
}

#endif
```

### 日志级别

| 级别 | 说明 | 使用场景 |
|------|------|----------|
| `TRACE` | 最详细的跟踪信息 | 调试非常详细的问题 |
| `DEBUG` | 调试信息 | 开发和调试 |
| `INFO` | 一般信息 | 正常运行信息 |
| `WARN` | 警告信息 | 潜在问题 |
| `ERROR` | 错误信息 | 错误发生 |
| `FATAL` | 致命错误 | 程序无法继续运行 |

### 日志配置选项

| 选项 | 类型 | 说明 |
|------|------|------|
| `level` | `uvhttp_log_level_t` | 日志级别 |
| `format` | `uvhttp_log_format_t` | 日志格式（文本/JSON） |
| `output` | `uvhttp_log_output_t` | 输出目标 |
| `file_path` | `char[256]` | 文件路径（如果输出到文件） |
| `enable_colors` | `int` | 启用颜色 |
| `enable_timestamp` | `int` | 启用时间戳 |
| `enable_source_location` | `int` | 启用源代码位置 |
| `thread_safe` | `int` | 线程安全 |

### WebSocket 中间件

WebSocket 使用独立的中间件模块：

```c
#include "uvhttp.h"
#include "uvhttp_websocket_middleware.h"

#if UVHTTP_FEATURE_WEBSOCKET

// WebSocket 消息处理回调
int ws_message_handler(uvhttp_ws_connection_t* ws_conn,
                       const char* data,
                       size_t len,
                       int opcode,
                       void* user_data) {
    printf("收到 WebSocket 消息: %.*s\n", (int)len, data);
    
    // 回显消息
    uvhttp_ws_middleware_send(ws_middleware, ws_conn, data, len);
    
    return 0;
}

// WebSocket 连接建立回调
int ws_connect_handler(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    printf("WebSocket 连接建立\n");
    return 0;
}

// WebSocket 连接关闭回调
int ws_close_handler(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    printf("WebSocket 连接关闭\n");
    return 0;
}

int main() {
    int port = 8080;
    
    // 使用统一 API 创建服务器
    uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", port);
    if (!server) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }
    
    // 创建 WebSocket 中间件
    uvhttp_ws_middleware_config_t ws_config = UVHTTP_WS_MIDDLEWARE_DEFAULT_CONFIG;
    ws_config.max_connections = 1000;
    
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_create("/ws", &ws_config);
    if (!ws_middleware) {
        fprintf(stderr, "WebSocket 中间件创建失败\n");
        return 1;
    }
    
    // 设置回调函数
    uvhttp_ws_middleware_callbacks_t callbacks = {
        .on_connect = ws_connect_handler,
        .on_message = ws_message_handler,
        .on_close = ws_close_handler,
        .user_data = NULL
    };
    uvhttp_ws_middleware_set_callbacks(ws_middleware, &callbacks);
    
    // 注册中间件到服务器
    uvhttp_http_middleware_t* http_middleware = uvhttp_ws_middleware_get_http_middleware(ws_middleware);
    uvhttp_server_add_middleware(server->server, http_middleware);
    
    printf("服务器运行中，按 Ctrl+C 停止...\n");
    printf("WebSocket URL: ws://localhost:%d/ws\n", port);
    
    // 运行服务器
    int result = uvhttp_server_run(server);
    
    // 清理
    uvhttp_ws_middleware_destroy(ws_middleware);
    uvhttp_server_simple_free(server);
    
    return result;
}

#endif
```

## 功能模块组合

多个中间件可以同时使用，按照注册顺序执行：

```c
int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
#if UVHTTP_FEATURE_WEBSOCKET
    // 1. 创建并注册 WebSocket 中间件
    uvhttp_ws_middleware_config_t ws_config = UVHTTP_WS_MIDDLEWARE_DEFAULT_CONFIG;
    uvhttp_ws_middleware_t* ws_middleware = uvhttp_ws_middleware_create("/ws", &ws_config);
    
    uvhttp_ws_middleware_callbacks_t ws_callbacks = { /* ... */ };
    uvhttp_ws_middleware_set_callbacks(ws_middleware, &ws_callbacks);
    
    uvhttp_server_add_middleware(server, uvhttp_ws_middleware_get_http_middleware(ws_middleware));
#endif
    
    // 2. 注册业务路由
    uvhttp_router_add_route(router, "/api/status", status_handler);
    uvhttp_router_add_route(router, "/api/data", data_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 3. 清理
#if UVHTTP_FEATURE_WEBSOCKET
    if (ws_middleware) {
        uvhttp_ws_middleware_destroy(ws_middleware);
    }
#endif
    
    return 0;
}
```

## 性能影响

### 编译时优化

- **未启用的功能模块**：完全不编译，零运行时开销
- **条件编译**：使用 `#if` 宏确保未启用的代码不会进入二进制
- **链接时优化**：未使用的函数会被链接器移除

### 运行时开销

| 功能模块 | 内存开销 | CPU 开销 | 说明 |
|----------|----------|----------|------|
| 静态文件 | 低 | 低 | 仅在访问文件时消耗资源，支持 LRU 缓存 |
| WebSocket | 中 | 中 | 每个连接维护状态 |

## API 参考

### 中间件 API

```c
// 创建 HTTP 中间件
uvhttp_http_middleware_t* uvhttp_http_middleware_create(
    const char* path,
    uvhttp_http_middleware_handler_t handler,
    uvhttp_middleware_priority_t priority
);

// 销毁中间件
void uvhttp_http_middleware_destroy(uvhttp_http_middleware_t* middleware);

// 添加中间件到服务器
uvhttp_error_t uvhttp_server_add_middleware(uvhttp_server_t* server, 
                                              uvhttp_http_middleware_t* middleware);

// 移除中间件
uvhttp_error_t uvhttp_server_remove_middleware(uvhttp_server_t* server, 
                                                 const char* path);
```

### WebSocket 中间件 API

```c
// 创建 WebSocket 中间件
uvhttp_ws_middleware_t* uvhttp_ws_middleware_create(
    const char* path,
    const uvhttp_ws_middleware_config_t* config
);

// 设置回调函数
void uvhttp_ws_middleware_set_callbacks(
    uvhttp_ws_middleware_t* middleware,
    const uvhttp_ws_middleware_callbacks_t* callbacks
);

// 获取 HTTP 中间件（用于注册）
uvhttp_http_middleware_t* uvhttp_ws_middleware_get_http_middleware(
    uvhttp_ws_middleware_t* middleware
);

// 发送消息
int uvhttp_ws_middleware_send(
    uvhttp_ws_middleware_t* middleware,
    uvhttp_ws_connection_t* ws_conn,
    const char* data,
    size_t len
);

// 关闭连接
int uvhttp_ws_middleware_close(
    uvhttp_ws_middleware_t* middleware,
    uvhttp_ws_connection_t* ws_conn,
    int code,
    const char* reason
);

// 获取连接数
int uvhttp_ws_middleware_get_connection_count(
    uvhttp_ws_middleware_t* middleware
);

// 广播消息
int uvhttp_ws_middleware_broadcast(
    uvhttp_ws_middleware_t* middleware,
    const char* data,
    size_t len
);
```

## 最佳实践

1. **按需启用**：只编译需要的功能，减小二进制体积
2. **路径规划**：为不同功能模块使用不同的 URL 路径前缀
3. **资源限制**：配置合理的缓存大小和连接数
4. **错误处理**：为每个功能模块设置适当的错误处理
5. **内存管理**：确保正确创建和销毁上下文对象
6. **线程安全**：在多线程环境中正确使用功能模块

## 示例程序

完整示例请参考：

- `examples/websocket_echo_server.c` - WebSocket 服务器（使用中间件）
- `examples/05_advanced/03_websocket_middleware.c` - WebSocket 中间件示例

## 故障排查

### 静态文件 404

- 确认 `root_directory` 路径正确
- 检查文件权限
- 验证 URL 路径前缀（如 `/static/*`）
- 确认静态文件上下文已正确创建

### WebSocket 连接失败

- 检查 WebSocket 路径配置
- 确认防火墙设置
- 验证处理器回调函数已正确设置
- 查看服务器日志

### 编译错误

- 确认已启用相应的功能宏
- 检查头文件包含是否正确
- 验证 CMake 配置选项

## 相关文档

- [教程 - 静态文件服务](TUTORIAL.md#6-静态文件中间件)
- [教程 - WebSocket](TUTORIAL.md#6-websocket-中间件)
- [开发指南](DEVELOPER_GUIDE.md)
- [API 参考](API_REFERENCE.md)
- [静态文件服务指南](STATIC_FILE_SERVER.md)
