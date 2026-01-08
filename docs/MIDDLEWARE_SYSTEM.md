# UVHTTP 功能模块系统

## 概述

UVHTTP 采用模块化的功能架构，提供可配置的功能模块，包括静态文件服务、WebSocket 等。这些功能模块通过编译时宏进行控制，实现按需编译和功能裁剪。

## 功能模块架构

### 核心特性

- **模块化设计**：每个功能模块独立实现，互不干扰
- **编译时控制**：通过 CMake 宏启用/禁用功能模块
- **路由集成**：静态文件服务通过路由系统集成
- **处理器注册**：WebSocket 通过处理器注册机制实现
- **零开销**：未启用的功能模块不会增加运行时开销

### 功能模块类型

| 功能模块 | 宏定义 | 默认状态 | 实现方式 |
|----------|--------|----------|----------|
| 静态文件服务 | `UVHTTP_FEATURE_STATIC_FILES` | 启用 | 路由集成 + 上下文 |
| WebSocket | `BUILD_WITH_WEBSOCKET` | 启用 | 处理器注册 |

## 编译时控制

### 启用/禁用功能模块

```bash
# 启用所有功能模块（默认）
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# 禁用 WebSocket
cmake -DBUILD_WITH_WEBSOCKET=OFF ..

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
```

## 功能模块使用

### 静态文件服务

静态文件服务使用上下文模式和路由集成：

```c
#include "uvhttp.h"
#include "uvhttp_static.h"

#if UVHTTP_FEATURE_STATIC_FILES

static uvhttp_static_context_t* g_static_ctx = NULL;

int static_file_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!g_static_ctx) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Static file service not initialized", 35);
        uvhttp_response_send(response);
        return -1;
    }
    
    int result = uvhttp_static_handle_request(g_static_ctx, request, response);
    if (result != 0) {
        const char* error_body = "Error processing static file request";
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error_body, strlen(error_body));
    }
    
    uvhttp_response_send(response);
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    
    // 创建静态文件上下文
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,
        .cache_ttl = 3600
    };
    
    g_static_ctx = uvhttp_static_create(&static_config);
    
    // 创建服务器和路由
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 注册静态文件路由
    uvhttp_router_add_route(router, "/static/*", static_file_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    if (g_static_ctx) {
        uvhttp_static_destroy(g_static_ctx);
    }
    
    return 0;
}

#endif
```

### WebSocket

WebSocket 使用处理器注册机制：

```c
#include "uvhttp.h"

#if UVHTTP_FEATURE_WEBSOCKET

// WebSocket 消息处理回调
int ws_message_handler(uvhttp_ws_connection_t* ws_conn,
                       const char* data,
                       size_t len,
                       int opcode) {
    printf("收到 WebSocket 消息: %.*s\n", (int)len, data);
    
    // 回显消息
    uvhttp_server_ws_send(ws_conn, data, len);
    
    return 0;
}

// WebSocket 连接建立回调
int ws_connect_handler(uvhttp_ws_connection_t* ws_conn) {
    printf("WebSocket 连接建立\n");
    return 0;
}

// WebSocket 连接关闭回调
int ws_close_handler(uvhttp_ws_connection_t* ws_conn) {
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
    
    // 注册 WebSocket 处理器
    uvhttp_ws_handler_t ws_handler;
    ws_handler.on_connect = ws_connect_handler;
    ws_handler.on_message = ws_message_handler;
    ws_handler.on_close = ws_close_handler;
    
    uvhttp_server_register_ws_handler(server->server, "/ws", &ws_handler);
    
    printf("服务器运行中，按 Ctrl+C 停止...\n");
    printf("WebSocket URL: ws://localhost:%d/ws\n", port);
    
    // 运行服务器
    int result = uvhttp_server_run(server);
    
    // 清理
    uvhttp_server_simple_free(server);
    
    return result;
}

#endif
```

## 功能模块组合

多个功能模块可以同时使用：

```c
int main() {
    uv_loop_t* loop = uv_default_loop();
    
#if UVHTTP_FEATURE_STATIC_FILES
    // 1. 创建静态文件上下文
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html"
    };
    uvhttp_static_context_t* static_ctx = uvhttp_static_create(&static_config);
#endif

    // 2. 创建服务器和路由
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
#if UVHTTP_FEATURE_STATIC_FILES
    // 3. 注册静态文件路由
    uvhttp_router_add_route(router, "/static/*", static_file_handler);
#endif

#if UVHTTP_FEATURE_WEBSOCKET
    // 4. 注册 WebSocket 处理器
    uvhttp_ws_handler_t ws_handler = { /* ... */ };
    uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
#endif
    
    // 5. 注册其他业务路由
    uvhttp_router_add_route(router, "/api/status", status_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 6. 清理
#if UVHTTP_FEATURE_STATIC_FILES
    if (static_ctx) {
        uvhttp_static_destroy(static_ctx);
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

### 静态文件服务 API

```c
// 创建静态文件上下文
uvhttp_static_context_t* uvhttp_static_create(const uvhttp_static_config_t* config);

// 处理静态文件请求
int uvhttp_static_handle_request(uvhttp_static_context_t* ctx, 
                                 uvhttp_request_t* request, 
                                 uvhttp_response_t* response);

// 销毁静态文件上下文
void uvhttp_static_destroy(uvhttp_static_context_t* ctx);
```

### WebSocket API

```c
// 注册 WebSocket 处理器
uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server, 
                                                 const char* path, 
                                                 uvhttp_ws_handler_t* handler);

// 发送 WebSocket 消息
int uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn, 
                          const char* data, 
                          size_t len);

// 关闭 WebSocket 连接
int uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, 
                          int code, 
                          const char* reason);
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

- `examples/static_file_server.c` - 静态文件服务
- `examples/websocket_echo_server.c` - WebSocket 服务器

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
