# UVHTTP 错误码参考

## 概述

UVHTTP 使用整数错误码来表示不同的错误情况。所有错误码都是负数，`UVHTTP_OK (0)` 表示成功。

## 错误码分类

### 通用错误 (0 到 -9)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| 0 | `UVHTTP_OK` | 操作成功 | - |
| -1 | `UVHTTP_ERROR_INVALID_PARAM` | 无效参数 | 是 |
| -2 | `UVHTTP_ERROR_OUT_OF_MEMORY` | 内存不足 | 否 |
| -3 | `UVHTTP_ERROR_NOT_FOUND` | 资源未找到 | 是 |
| -4 | `UVHTTP_ERROR_ALREADY_EXISTS` | 资源已存在 | 是 |
| -5 | `UVHTTP_ERROR_NULL_POINTER` | 空指针 | 否 |
| -6 | `UVHTTP_ERROR_BUFFER_TOO_SMALL` | 缓冲区太小 | 是 |
| -7 | `UVHTTP_ERROR_TIMEOUT` | 操作超时 | 是 |
| -8 | `UVHTTP_ERROR_CANCELLED` | 操作被取消 | 是 |

### 服务器错误 (-100 到 -109)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -100 | `UVHTTP_ERROR_SERVER_INIT` | 服务器初始化失败 | 否 |
| -101 | `UVHTTP_ERROR_SERVER_LISTEN` | 服务器监听失败 | 否 |
| -102 | `UVHTTP_ERROR_SERVER_STOP` | 服务器停止失败 | 是 |
| -103 | `UVHTTP_ERROR_CONNECTION_LIMIT` | 达到连接数限制 | 是 |
| -104 | `UVHTTP_ERROR_SERVER_ALREADY_RUNNING` | 服务器已在运行 | 否 |
| -105 | `UVHTTP_ERROR_SERVER_NOT_RUNNING` | 服务器未运行 | 否 |
| -106 | `UVHTTP_ERROR_SERVER_INVALID_CONFIG` | 服务器配置无效 | 否 |

### 连接错误 (-200 到 -209)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -200 | `UVHTTP_ERROR_CONNECTION_INIT` | 连接初始化失败 | 否 |
| -201 | `UVHTTP_ERROR_CONNECTION_ACCEPT` | 接受连接失败 | 是 |
| -202 | `UVHTTP_ERROR_CONNECTION_START` | 启动连接失败 | 是 |
| -203 | `UVHTTP_ERROR_CONNECTION_CLOSE` | 关闭连接失败 | 是 |
| -204 | `UVHTTP_ERROR_CONNECTION_RESET` | 连接被重置 | 是 |
| -205 | `UVHTTP_ERROR_CONNECTION_TIMEOUT` | 连接超时 | 是 |
| -206 | `UVHTTP_ERROR_CONNECTION_REFUSED` | 连接被拒绝 | 是 |
| -207 | `UVHTTP_ERROR_CONNECTION_BROKEN` | 连接断开 | 是 |

### 请求/响应错误 (-300 到 -309)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -300 | `UVHTTP_ERROR_REQUEST_INIT` | 请求初始化失败 | 否 |
| -301 | `UVHTTP_ERROR_RESPONSE_INIT` | 响应初始化失败 | 否 |
| -302 | `UVHTTP_ERROR_RESPONSE_SEND` | 发送响应失败 | 是 |
| -303 | `UVHTTP_ERROR_INVALID_HTTP_METHOD` | 无效的 HTTP 方法 | 是 |
| -304 | `UVHTTP_ERROR_INVALID_HTTP_VERSION` | 无效的 HTTP 版本 | 是 |
| -305 | `UVHTTP_ERROR_HEADER_TOO_LARGE` | 请求头过大 | 是 |
| -306 | `UVHTTP_ERROR_BODY_TOO_LARGE` | 请求体过大 | 是 |
| -307 | `UVHTTP_ERROR_MALFORMED_REQUEST` | 格式错误的请求 | 是 |

### TLS 错误 (-400 到 -409)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -400 | `UVHTTP_ERROR_TLS_INIT` | TLS 初始化失败 | 否 |
| -401 | `UVHTTP_ERROR_TLS_CONTEXT` | TLS 上下文错误 | 否 |
| -402 | `UVHTTP_ERROR_TLS_HANDSHAKE` | TLS 握手失败 | 是 |
| -403 | `UVHTTP_ERROR_TLS_CERT_LOAD` | 加载证书失败 | 否 |
| -404 | `UVHTTP_ERROR_TLS_KEY_LOAD` | 加载密钥失败 | 否 |
| -405 | `UVHTTP_ERROR_TLS_VERIFY_FAILED` | TLS 验证失败 | 是 |
| -406 | `UVHTTP_ERROR_TLS_EXPIRED` | 证书已过期 | 否 |
| -407 | `UVHTTP_ERROR_TLS_NOT_YET_VALID` | 证书尚未生效 | 否 |

### 路由错误 (-500 到 -509)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -500 | `UVHTTP_ERROR_ROUTER_INIT` | 路由器初始化失败 | 否 |
| -501 | `UVHTTP_ERROR_ROUTER_ADD` | 添加路由失败 | 否 |
| -502 | `UVHTTP_ERROR_ROUTE_NOT_FOUND` | 路由未找到 | 是 |
| -503 | `UVHTTP_ERROR_ROUTE_ALREADY_EXISTS` | 路由已存在 | 否 |
| -504 | `UVHTTP_ERROR_INVALID_ROUTE_PATTERN` | 无效的路由模式 | 否 |

### 分配器错误 (-600 到 -609)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -600 | `UVHTTP_ERROR_ALLOCATOR_INIT` | 分配器初始化失败 | 否 |
| -601 | `UVHTTP_ERROR_ALLOCATOR_SET` | 设置分配器失败 | 否 |
| -602 | `UVHTTP_ERROR_ALLOCATOR_NOT_INITIALIZED` | 分配器未初始化 | 否 |

### WebSocket 错误 (-700 到 -709)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -700 | `UVHTTP_ERROR_WEBSOCKET_INIT` | WebSocket 初始化失败 | 否 |
| -701 | `UVHTTP_ERROR_WEBSOCKET_HANDSHAKE` | WebSocket 握手失败 | 是 |
| -702 | `UVHTTP_ERROR_WEBSOCKET_FRAME` | WebSocket 帧错误 | 是 |
| -703 | `UVHTTP_ERROR_WEBSOCKET_TOO_LARGE` | WebSocket 消息过大 | 是 |
| -704 | `UVHTTP_ERROR_WEBSOCKET_INVALID_OPCODE` | 无效的操作码 | 是 |
| -705 | `UVHTTP_ERROR_WEBSOCKET_NOT_CONNECTED` | WebSocket 未连接 | 否 |
| -706 | `UVHTTP_ERROR_WEBSOCKET_ALREADY_CONNECTED` | WebSocket 已连接 | 否 |
| -707 | `UVHTTP_ERROR_WEBSOCKET_CLOSED` | WebSocket 已关闭 | 否 |

### HTTP/2 错误 (-800 到 -809)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -800 | `UVHTTP_ERROR_HTTP2_INIT` | HTTP/2 初始化失败 | 否 |
| -801 | `UVHTTP_ERROR_HTTP2_STREAM` | HTTP/2 流错误 | 是 |
| -802 | `UVHTTP_ERROR_HTTP2_SETTINGS` | HTTP/2 设置错误 | 是 |
| -803 | `UVHTTP_ERROR_HTTP2_FLOW_CONTROL` | HTTP/2 流控错误 | 是 |
| -804 | `UVHTTP_ERROR_HTTP2_HEADER_COMPRESS` | HTTP/2 头部压缩错误 | 是 |
| -805 | `UVHTTP_ERROR_HTTP2_PRIORITY` | HTTP/2 优先级错误 | 是 |

### 配置错误 (-900 到 -909)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -900 | `UVHTTP_ERROR_CONFIG_PARSE` | 配置解析失败 | 否 |
| -901 | `UVHTTP_ERROR_CONFIG_INVALID` | 配置无效 | 否 |
| -902 | `UVHTTP_ERROR_CONFIG_FILE_NOT_FOUND` | 配置文件未找到 | 否 |
| -903 | `UVHTTP_ERROR_CONFIG_MISSING_REQUIRED` | 缺少必需配置 | 否 |

### 中间件错误 (-1000 到 -1009)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -1000 | `UVHTTP_ERROR_MIDDLEWARE_INIT` | 中间件初始化失败 | 否 |
| -1001 | `UVHTTP_ERROR_MIDDLEWARE_REGISTER` | 注册中间件失败 | 否 |
| -1002 | `UVHTTP_ERROR_MIDDLEWARE_EXECUTE` | 执行中间件失败 | 是 |
| -1003 | `UVHTTP_ERROR_MIDDLEWARE_NOT_FOUND` | 中间件未找到 | 是 |

### 日志错误 (-1100 到 -1109)

| 错误码 | 名称 | 说明 | 可恢复 |
|--------|------|------|--------|
| -1100 | `UVHTTP_ERROR_LOG_INIT` | 日志初始化失败 | 否 |
| -1101 | `UVHTTP_ERROR_LOG_WRITE` | 日志写入失败 | 是 |
| -1102 | `UVHTTP_ERROR_LOG_FILE_OPEN` | 打开日志文件失败 | 否 |
| -1103 | `UVHTTP_ERROR_LOG_NOT_INITIALIZED` | 日志未初始化 | 否 |

## 错误码解读 API

### 获取错误名称

```c
const char* name = uvhttp_error_string(error_code);
// 示例: uvhttp_error_string(-101) 返回 "UVHTTP_ERROR_SERVER_LISTEN"
```

### 获取错误分类

```c
const char* category = uvhttp_error_category_string(error_code);
// 示例: uvhttp_error_category_string(-101) 返回 "Server Error"
```

### 获取错误描述

```c
const char* description = uvhttp_error_description(error_code);
// 示例: uvhttp_error_description(-101) 返回 "Failed to listen on the specified port"
```

### 获取修复建议

```c
const char* suggestion = uvhttp_error_suggestion(error_code);
// 示例: uvhttp_error_suggestion(-101) 返回 "Check if the port is already in use or if you have permission to bind to it"
```

### 检查错误是否可恢复

```c
if (uvhttp_error_is_recoverable(error_code)) {
    // 尝试恢复操作
}
```

## 错误处理最佳实践

### 1. 基本错误处理

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "描述: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "建议: %s\n", uvhttp_error_suggestion(result));
    return 1;
}
```

### 2. 错误恢复

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    if (uvhttp_error_is_recoverable(result)) {
        // 尝试使用不同的端口
        result = uvhttp_server_listen(server, "0.0.0.0", 8081);
        if (result != UVHTTP_OK) {
            fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
            return 1;
        }
    } else {
        fprintf(stderr, "致命错误: %s\n", uvhttp_error_string(result));
        fprintf(stderr, "描述: %s\n", uvhttp_error_description(result));
        return 1;
    }
}
```

### 3. 错误日志

```c
#if UVHTTP_FEATURE_LOGGING
uvhttp_log_middleware_t* log_middleware = uvhttp_log_get_global_middleware();
if (log_middleware) {
    UVHTTP_LOG_ERROR(log_middleware, "操作失败: %s (%d)", 
                    uvhttp_error_string(result), result);
}
#endif
```

## 错误码范围

- **0**: 成功
- **-1 到 -9**: 通用错误
- **-100 到 -199**: 服务器错误
- **-200 到 -299**: 连接错误
- **-300 到 -399**: 请求/响应错误
- **-400 到 -499**: TLS 错误
- **-500 到 -599**: 路由错误
- **-600 到 -699**: 分配器错误
- **-700 到 -799**: WebSocket 错误
- **-800 到 -899**: HTTP/2 错误
- **-900 到 -999**: 配置错误
- **-1000 到 -1099**: 中间件错误
- **-1100 到 -1199**: 日志错误

## 相关文档

- [API 参考](API_REFERENCE.md)
- [开发者指南](DEVELOPER_GUIDE.md)
- [错误处理最佳实践](DEVELOPER_GUIDE.md#错误处理)