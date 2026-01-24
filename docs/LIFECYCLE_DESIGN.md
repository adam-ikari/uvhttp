# UVHTTP 生命周期设计方案

## 1. 概述

本文档描述 UVHTTP 的完整生命周期管理方案，包括连接生命周期、请求/响应生命周期、资源管理生命周期和错误处理生命周期。

## 2. 连接生命周期

### 2.1 连接状态机

```
+-------------+      accept      +------------+      start      +-------------+
|   NEW       | --------------> | ACCEPTED   | ------------> | READING     |
+-------------+                   +------------+                   +-------------+
      ^                               |                               |
      |                               | close/timeout                 | complete/error
      |                               v                               v
+-------------+    close/timeout   +------------+    send_response   +-------------+
|  CLOSING    | <---------------- |  CLOSED    | <---------------- |  SENDING    |
+-------------+                   +------------+                   +-------------+
      ^                                                                   |
      |                                                                   v
      +---------------------------------------------------------------+
                              keep-alive restart
```

### 2.2 连接生命周期阶段

#### 2.2.1 NEW 状态
- **触发**: `uvhttp_connection_new()` 创建连接对象
- **操作**:
  - 分配连接对象内存
  - 初始化 TCP 句柄
  - 初始化请求和响应对象
  - 初始化读缓冲区
  - 初始化 idle 句柄
- **状态转换**: ACCEPTED (当 libuv 接受连接)

#### 2.2.2 ACCEPTED 状态
- **触发**: `on_connection()` 回调被调用
- **操作**:
  - 接受连接
  - 增加活动连接计数
  - 启动连接读取
- **状态转换**: 
  - READING (成功启动读取)
  - CLOSING (启动失败或连接关闭)

#### 2.2.3 READING 状态
- **触发**: `uvhttp_connection_start()` 启动读取
- **操作**:
  - 等待客户端数据
  - 解析 HTTP 请求
  - 处理请求
- **状态转换**:
  - SENDING (请求解析完成，开始发送响应)
  - CLOSING (连接关闭或错误)

#### 2.2.4 SENDING 状态
- **触发**: `uvhttp_response_send()` 发送响应
- **操作**:
  - 构建响应数据
  - 发送响应数据
  - 等待发送完成
- **状态转换**:
  - READING (keep-alive 连接，重启读取)
  - CLOSING (非 keep-alive 或错误)

#### 2.2.5 CLOSING 状态
- **触发**: 连接关闭或错误
- **操作**:
  - 停止读取
  - 关闭 TCP 句柄
  - 延迟释放连接对象
- **状态转换**: CLOSED (TCP 句柄关闭完成)

#### 2.2.6 CLOSED 状态
- **触发**: `on_close()` 回调被调用
- **操作**:
  - 减少活动连接计数
  - 延迟释放连接对象
- **状态转换**: NEW (连接对象被释放，可以重新使用)

### 2.3 连接生命周期关键点

#### 2.3.1 连接创建
```c
uvhttp_connection_t* uvhttp_connection_new(uvhttp_server_t* server) {
    // 1. 分配连接对象
    uvhttp_connection_t* conn = uvhttp_alloc(sizeof(uvhttp_connection_t));
    
    // 2. 初始化连接对象
    memset(conn, 0, sizeof(uvhttp_connection_t));
    conn->server = server;
    conn->state = UVHTTP_CONN_STATE_NEW;
    
    // 3. 初始化 idle 句柄
    uv_idle_init(server->loop, &conn->idle_handle);
    conn->idle_handle.data = conn;
    
    // 4. 初始化 TCP 句柄
    uv_tcp_init(server->loop, &conn->tcp_handle);
    conn->tcp_handle.data = conn;
    
    // 5. 分配读缓冲区
    conn->read_buffer = uvhttp_alloc(conn->read_buffer_size);
    
    // 6. 创建请求和响应对象
    conn->request = uvhttp_alloc(sizeof(uvhttp_request_t));
    uvhttp_request_init(conn->request, &conn->tcp_handle);
    
    conn->response = uvhttp_alloc(sizeof(uvhttp_response_t));
    uvhttp_response_init(conn->response, &conn->tcp_handle);
    
    return conn;
}
```

#### 2.3.2 连接接受
```c
static void on_connection(uv_stream_t* server_handle, int status) {
    if (status < 0) {
        return;
    }
    
    uvhttp_server_t* server = (uvhttp_server_t*)server_handle->data;
    
    // 检查连接数限制
    if (server->active_connections >= server->config->max_connections) {
        // 发送 503 响应
        send_503_response(server_handle);
        return;
    }
    
    // 创建新连接
    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    if (!conn) {
        return;
    }
    
    // 接受连接
    if (uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle) != 0) {
        uvhttp_connection_free(conn);
        return;
    }
    
    // 增加活动连接计数
    server->active_connections++;
    
    // 启动连接读取
    if (uvhttp_connection_start(conn) != 0) {
        uvhttp_connection_close(conn);
        return;
    }
}
```

#### 2.3.3 连接读取
```c
static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    
    if (nread < 0) {
        // 连接关闭或错误
        uvhttp_connection_close(conn);
        return;
    }
    
    if (nread == 0) {
        return;
    }
    
    // 解析 HTTP 请求
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    enum llhttp_errno err = llhttp_execute(parser, buf->base, nread);
    
    if (err != HPE_OK) {
        // 解析错误
        uvhttp_connection_close(conn);
        return;
    }
}
```

#### 2.3.4 连接关闭
```c
void uvhttp_connection_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    // 设置状态为 CLOSING
    conn->state = UVHTTP_CONN_STATE_CLOSING;
    
    // 停止读取
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);
    
    // 关闭 TCP 句柄
    if (!uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
        uv_close((uv_handle_t*)&conn->tcp_handle, on_close);
    }
}

static void on_close(uv_handle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (conn) {
        // 减少活动连接计数
        if (conn->server) {
            conn->server->active_connections--;
        }
        
        // 延迟释放连接对象
        uv_idle_t* idle = &conn->idle_handle;
        idle->data = conn;
        uv_idle_start(idle, on_idle_free_connection);
    }
}

static void on_idle_free_connection(uv_idle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (conn) {
        // 停止 idle 句柄
        uv_idle_stop(handle);
        
        // 释放连接对象
        uvhttp_connection_free(conn);
    }
}
```

#### 2.3.5 连接释放
```c
void uvhttp_connection_free(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    // 释放请求对象
    if (conn->request) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
        conn->request = NULL;
    }
    
    // 释放响应对象
    if (conn->response) {
        uvhttp_response_cleanup(conn->response);
        uvhttp_free(conn->response);
        conn->response = NULL;
    }
    
    // 释放读缓冲区
    if (conn->read_buffer) {
        uvhttp_free(conn->read_buffer);
        conn->read_buffer = NULL;
    }
    
    // 释放连接对象
    uvhttp_free(conn);
}
```

## 3. 请求/响应生命周期

### 3.1 请求生命周期

```
+-------------+      init      +------------+      parse     +-------------+
|  CREATED    | -----------> |  PARSING   | -----------> |  COMPLETE   |
+-------------+                   +------------+                   +-------------+
                                           |                               |
                                           | error                          | process
                                           v                               v
                                    +------------+                   +-------------+
                                    |  ERROR     |                   |  HANDLED    |
                                    +------------+                   +-------------+
                                                                           |
                                                                           | cleanup
                                                                           v
                                                                    +-------------+
                                                                    |  CLEANED    |
                                                                    +-------------+
```

### 3.2 请求生命周期阶段

#### 3.2.1 CREATED 状态
- **触发**: `uvhttp_request_init()` 初始化请求对象
- **操作**:
  - 清零请求对象
  - 初始化 HTTP 解析器
  - 设置解析器回调
- **状态转换**: PARSING (开始解析 HTTP 请求)

#### 3.2.2 PARSING 状态
- **触发**: `on_read()` 回调接收数据
- **操作**:
  - 解析 HTTP 请求行
  - 解析 HTTP 头部
  - 解析 HTTP 请求体
- **状态转换**:
  - COMPLETE (解析完成)
  - ERROR (解析错误)

#### 3.2.3 COMPLETE 状态
- **触发**: `on_message_complete()` 回调被调用
- **操作**:
  - 设置 HTTP 方法
  - 标记解析完成
  - 触发请求处理
- **状态转换**: HANDLED (请求被处理)

#### 3.2.4 HANDLED 状态
- **触发**: 请求处理器完成
- **操作**:
  - 发送响应
  - 准备下一个请求（keep-alive）
- **状态转换**: CLEANED (请求被清理)

#### 3.2.5 CLEANED 状态
- **触发**: `uvhttp_request_cleanup()` 被调用
- **操作**:
  - 释放请求体
  - 释放解析器
  - 重置请求状态
- **状态转换**: CREATED (请求可以重新使用)

### 3.3 响应生命周期

```
+-------------+      init      +------------+      build     +-------------+
|  CREATED    | -----------> |  BUILDING  | -----------> |  READY      |
+-------------+                   +------------+                   +-------------+
                                           |                               |
                                           | error                          | send
                                           v                               v
                                    +------------+                   +-------------+
                                    |  ERROR     |                   |  SENDING    |
                                    +------------+                   +-------------+
                                                                           |
                                                                           | complete
                                                                           v
                                                                    +-------------+
                                                                    |  SENT       |
                                                                    +-------------+
                                                                           |
                                                                           | cleanup
                                                                           v
                                                                    +-------------+
                                                                    |  CLEANED    |
                                                                    +-------------+
```

### 3.4 响应生命周期阶段

#### 3.4.1 CREATED 状态
- **触发**: `uvhttp_response_init()` 初始化响应对象
- **操作**:
  - 清零响应对象
  - 设置默认值
  - 初始化 keep-alive
- **状态转换**: BUILDING (开始构建响应)

#### 3.4.2 BUILDING 状态
- **触发**: 请求处理器设置响应
- **操作**:
  - 设置状态码
  - 设置响应头
  - 设置响应体
- **状态转换**:
  - READY (响应构建完成)
  - ERROR (构建错误)

#### 3.4.3 READY 状态
- **触发**: `uvhttp_response_build_data()` 构建响应数据
- **操作**:
  - 构建响应头
  - 计算响应总大小
  - 分配响应数据缓冲区
- **状态转换**: SENDING (开始发送响应)

#### 3.4.4 SENDING 状态
- **触发**: `uvhttp_response_send_raw()` 发送响应
- **操作**:
  - 发送响应数据
  - 等待发送完成
- **状态转换**: SENT (发送完成)

#### 3.4.5 SENT 状态
- **触发**: `uvhttp_free_write_data()` 回调被调用
- **操作**:
  - 释放响应数据
  - 检查 keep-alive
- **状态转换**: CLEANED (响应被清理)

#### 3.4.6 CLEANED 状态
- **触发**: `uvhttp_response_cleanup()` 被调用
- **操作**:
  - 释放响应体
  - 释放额外头部
  - 重置响应状态
- **状态转换**: CREATED (响应可以重新使用)

## 4. 资源管理生命周期

### 4.1 内存管理原则

#### 4.1.1 统一分配器
- 使用 `UVHTTP_MALLOC` 和 `UVHTTP_FREE` 宏
- 严禁混用 `malloc/free` 和 `UVHTTP_MALLOC/UVHTTP_FREE`
- 默认使用 mimalloc 分配器

#### 4.1.2 内存分配策略
```c
// 连接对象
uvhttp_connection_t* conn = uvhttp_alloc(sizeof(uvhttp_connection_t));

// 请求对象
uvhttp_request_t* request = uvhttp_alloc(sizeof(uvhttp_request_t));

// 响应对象
uvhttp_response_t* response = uvhttp_alloc(sizeof(uvhttp_response_t));

// 读缓冲区
char* buffer = uvhttp_alloc(buffer_size);

// 响应数据
char* response_data = uvhttp_alloc(total_size);
```

#### 4.1.3 内存释放策略
```c
// 连接对象释放
void uvhttp_connection_free(uvhttp_connection_t* conn) {
    if (conn->request) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
    }
    
    if (conn->response) {
        uvhttp_response_cleanup(conn->response);
        uvhttp_free(conn->response);
    }
    
    if (conn->read_buffer) {
        uvhttp_free(conn->read_buffer);
    }
    
    uvhttp_free(conn);
}

// 请求对象清理
void uvhttp_request_cleanup(uvhttp_request_t* request) {
    if (request->body) {
        uvhttp_free(request->body);
    }
    
    if (request->parser) {
        uvhttp_free(request->parser);
    }
    
    if (request->parser_settings) {
        uvhttp_free(request->parser_settings);
    }
    
    if (request->headers_extra) {
        uvhttp_free(request->headers_extra);
    }
}

// 响应对象清理
void uvhttp_response_cleanup(uvhttp_response_t* response) {
    if (response->body) {
        uvhttp_free(response->body);
    }
    
    if (response->headers_extra) {
        uvhttp_free(response->headers_extra);
    }
}
```

### 4.2 句柄管理原则

#### 4.2.1 句柄初始化
```c
// TCP 句柄初始化
uv_tcp_init(server->loop, &conn->tcp_handle);
conn->tcp_handle.data = conn;

// Idle 句柄初始化
uv_idle_init(server->loop, &conn->idle_handle);
conn->idle_handle.data = conn;
```

#### 4.2.2 句柄关闭
```c
// 检查句柄是否已经关闭
if (!uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
    uv_close((uv_handle_t*)&conn->tcp_handle, on_close);
}

if (!uv_is_closing((uv_handle_t*)&conn->idle_handle)) {
    uv_idle_stop(&conn->idle_handle);
    uv_close((uv_handle_t*)&conn->idle_handle, NULL);
}
```

#### 4.2.3 句柄清理
```c
static void on_close(uv_handle_t* handle) {
    // 延迟释放连接对象，避免 libuv 内部队列访问已释放的内存
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    
    uv_idle_t* idle = &conn->idle_handle;
    idle->data = conn;
    uv_idle_start(idle, on_idle_free_connection);
}

static void on_idle_free_connection(uv_idle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    
    // 停止 idle 句柄
    uv_idle_stop(handle);
    
    // 释放连接对象
    uvhttp_connection_free(conn);
}
```

## 5. 错误处理生命周期

### 5.1 错误处理流程

```
+-------------+      error      +------------+      recover   +-------------+
|  DETECTED   | -----------> |  HANDLED   | -----------> |  RECOVERED  |
+-------------+                   +------------+                   +-------------+
                                           |                               |
                                           | unrecoverable                 | unrecoverable
                                           v                               v
                                    +------------+                   +-------------+
                                    |  CLOSED    |                   |  CLOSED     |
                                    +------------+                   +-------------+
```

### 5.2 错误处理原则

#### 5.2.1 错误检测
```c
// 检查函数返回值
int result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "Error: %s\n", uvhttp_error_string(result));
    return 1;
}

// 检查指针
if (!request || !response) {
    return UVHTTP_ERROR_INVALID_PARAM;
}

// 检查状态
if (conn->state == UVHTTP_CONN_STATE_CLOSING) {
    return UVHTTP_ERROR_CONNECTION_CLOSE;
}
```

#### 5.2.2 错误处理
```c
// 可恢复错误
if (result != UVHTTP_OK && uvhttp_error_is_recoverable(result)) {
    // 尝试恢复
    result = uvhttp_server_listen(server, "0.0.0.0", 8081);
} else {
    // 不可恢复错误
    uvhttp_server_free(server);
    return 1;
}

// 连接错误
if (nread < 0) {
    if (nread != UV_EOF) {
        uvhttp_log_safe_error(nread, "connection_read", NULL);
    }
    uvhttp_connection_close(conn);
    return;
}

// 解析错误
if (err != HPE_OK) {
    uvhttp_log_safe_error(err, "http_parse", llhttp_errno_name(err));
    uvhttp_connection_close(conn);
    return;
}
```

#### 5.2.3 错误恢复
```c
// 重试机制
int retry_count = 0;
int max_retries = 3;

while (retry_count < max_retries) {
    int result = uvhttp_server_listen(server, host, port);
    if (result == UVHTTP_OK) {
        break;
    }
    
    retry_count++;
    if (retry_count < max_retries) {
        sleep(1);
    }
}

// 回退机制
if (result != UVHTTP_OK) {
    // 回退到默认端口
    result = uvhttp_server_listen(server, "0.0.0.0", 8080);
}
```

## 6. 生命周期管理最佳实践

### 6.1 连接管理最佳实践

1. **延迟释放连接对象**
   - 使用 idle 句柄延迟释放连接对象
   - 避免 libuv 内部队列访问已释放的内存

2. **连接数限制**
   - 检查活动连接数
   - 超过限制时发送 503 响应

3. **Keep-Alive 管理**
   - 支持 HTTP/1.1 Keep-Alive
   - 重用连接以提高性能
   - 设置合理的 keep-alive 超时

### 6.2 请求/响应管理最佳实践

1. **请求解析**
   - 使用 llhttp 解析 HTTP 请求
   - 支持流式解析
   - 支持大请求体

2. **响应构建**
   - 使用纯函数构建响应数据
   - 支持流式响应
   - 支持大响应体

3. **资源清理**
   - 及时释放请求/响应资源
   - 支持 keep-alive 时重用请求/响应对象

### 6.3 内存管理最佳实践

1. **统一分配器**
   - 使用 `UVHTTP_MALLOC` 和 `UVHTTP_FREE` 宏
   - 严禁混用 `malloc/free`

2. **内存泄漏预防**
   - 确保每个分配都有对应的释放
   - 使用 valgrind 检测内存泄漏

3. **内存优化**
   - 使用混合分配策略（内联 + 动态）
   - 减少内存占用

### 6.4 错误处理最佳实践

1. **错误检测**
   - 检查所有可能失败的函数调用
   - 使用统一的错误类型

2. **错误处理**
   - 区分可恢复和不可恢复错误
   - 提供有意义的错误信息

3. **错误恢复**
   - 实现重试机制
   - 实现回退机制

## 7. 总结

本文档描述了 UVHTTP 的完整生命周期管理方案，包括：

1. **连接生命周期**: NEW → ACCEPTED → READING → SENDING → CLOSING → CLOSED
2. **请求生命周期**: CREATED → PARSING → COMPLETE → HANDLED → CLEANED
3. **响应生命周期**: CREATED → BUILDING → READY → SENDING → SENT → CLEANED
4. **资源管理**: 统一分配器、句柄管理、内存管理
5. **错误处理**: 错误检测、错误处理、错误恢复

通过遵循这个生命周期方案，可以确保 UVHTTP 服务器能够正确地处理请求和发送响应，同时避免内存泄漏和资源泄漏。