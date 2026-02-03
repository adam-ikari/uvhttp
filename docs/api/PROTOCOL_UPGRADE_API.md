# UVHTTP 协议升级 API 参考

> 版本：2.3.0  
> 更新日期：2026-02-03

## 目录

1. [概述](#概述)
2. [核心类型](#核心类型)
3. [协议注册 API](#协议注册-api)
4. [连接所有权转移 API](#连接所有权转移-api)
5. [辅助函数](#辅助函数)
6. [使用示例](#使用示例)
7. [错误码](#错误码)

---

## 概述

UVHTTP 协议升级 API 提供了一套完整的接口，用于实现 HTTP 协议升级到自定义协议（如 IPPS、gRPC-Web 等）。

### 设计原则

- **零开销**：不使用协议升级时无性能损失
- **类型安全**：使用强类型函数指针
- **错误处理**：完整的错误码体系
- **向后兼容**：与现有 WebSocket 升级共存

### 使用流程

1. 定义协议检测器函数
2. 定义协议升级处理器函数
3. 注册协议升级处理器
4. 在升级处理器中转移连接所有权
5. 外部库接管连接生命周期

---

## 核心类型

### uvhttp_protocol_detector_t

协议检测器函数类型。

```c
typedef int (*uvhttp_protocol_detector_t)(
    uvhttp_request_t* request,
    char* protocol_name,
    size_t protocol_name_len
);
```

**参数**:
- `request`: HTTP 请求对象
- `protocol_name`: 输出参数，检测到的协议名称
- `protocol_name_len`: 协议名称缓冲区大小

**返回值**:
- `1`: 检测到协议
- `0`: 未检测到协议

**示例**:
```c
int ipps_protocol_detector(uvhttp_request_t* request,
                           char* protocol_name,
                           size_t protocol_name_len) {
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    if (!upgrade || strcasecmp(upgrade, "ipps") != 0) {
        return 0;
    }
    
    strncpy(protocol_name, "ipps", protocol_name_len);
    return 1;
}
```

### uvhttp_protocol_upgrade_handler_t

协议升级处理器函数类型。

```c
typedef uvhttp_error_t (*uvhttp_protocol_upgrade_handler_t)(
    uvhttp_connection_t* conn,
    const char* protocol_name,
    void* user_data
);
```

**参数**:
- `conn`: HTTP 连接对象
- `protocol_name`: 协议名称
- `user_data`: 用户数据（注册时传入）

**返回值**:
- `UVHTTP_OK`: 升级成功
- 其他值: 升级失败

**示例**:
```c
uvhttp_error_t ipps_upgrade_handler(uvhttp_connection_t* conn,
                                     const char* protocol_name,
                                     void* user_data) {
    // 发送 101 响应
    uvhttp_response_set_status(conn->response, 101);
    uvhttp_response_set_header(conn->response, "Upgrade", "ipps");
    uvhttp_response_send(conn->response);
    
    // 转移连接所有权
    uvhttp_connection_transfer_ownership(conn, on_transfer_to_ipps, user_data);
    
    return UVHTTP_OK;
}
```

### uvhttp_connection_ownership_callback_t

连接所有权转移回调函数类型。

```c
typedef void (*uvhttp_connection_ownership_callback_t)(
    uv_tcp_t* tcp_handle,
    int fd,
    void* user_data
);
```

**参数**:
- `tcp_handle`: TCP 句柄
- `fd`: 文件描述符
- `user_data`: 用户数据

**示例**:
```c
void on_transfer_to_ipps(uv_tcp_t* tcp_handle, int fd, void* user_data) {
    ipps_context_t* ctx = (ipps_context_t*)user_data;
    ipps_connection_t* ipps_conn = ipps_connection_create(ctx, fd);
    ipps_connection_start(ipps_conn);
}
```

### uvhttp_connection_lifecycle_t

连接生命周期回调结构。

```c
typedef struct {
    void* user_data;
    void (*on_close)(void* user_data);
} uvhttp_connection_lifecycle_t;
```

**字段**:
- `user_data`: 用户数据
- `on_close`: 连接关闭时的回调函数

**示例**:
```c
uvhttp_connection_lifecycle_t lifecycle = {
    .user_data = ipps_conn,
    .on_close = on_ipps_connection_close
};

uvhttp_connection_set_lifecycle(conn, &lifecycle);
```

---

## 协议注册 API

### uvhttp_server_register_protocol_upgrade

注册协议升级处理器。

```c
uvhttp_error_t uvhttp_server_register_protocol_upgrade(
    uvhttp_server_t* server,
    const char* protocol_name,
    const char* upgrade_header,
    uvhttp_protocol_detector_t detector,
    uvhttp_protocol_upgrade_handler_t handler,
    void* user_data
);
```

**参数**:
- `server`: 服务器对象
- `protocol_name`: 协议名称（如 "ipps"、"grpc-web"）
- `upgrade_header`: Upgrade 头部值（可选，用于快速匹配）
- `detector`: 协议检测器函数
- `handler`: 协议升级处理器函数
- `user_data`: 用户数据，将传递给处理器

**返回值**:
- `UVHTTP_OK`: 注册成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效
- `UVHTTP_ERROR_OUT_OF_MEMORY`: 内存不足

**示例**:
```c
uvhttp_server_register_protocol_upgrade(
    server,
    "ipps",
    "ipps",
    ipps_protocol_detector,
    ipps_upgrade_handler,
    ipps_context
);
```

### uvhttp_server_unregister_protocol_upgrade

注销协议升级处理器。

```c
uvhttp_error_t uvhttp_server_unregister_protocol_upgrade(
    uvhttp_server_t* server,
    const char* protocol_name
);
```

**参数**:
- `server`: 服务器对象
- `protocol_name`: 协议名称

**返回值**:
- `UVHTTP_OK`: 注销成功
- `UVHTTP_ERROR_NOT_FOUND`: 协议未找到

**示例**:
```c
uvhttp_server_unregister_protocol_upgrade(server, "ipps");
```

---

## 连接所有权转移 API

### uvhttp_connection_transfer_ownership

转移连接所有权给外部库。

```c
uvhttp_error_t uvhttp_connection_transfer_ownership(
    uvhttp_connection_t* conn,
    uvhttp_connection_ownership_callback_t callback,
    void* user_data
);
```

**参数**:
- `conn`: HTTP 连接对象
- `callback`: 所有权转移回调函数
- `user_data`: 用户数据

**返回值**:
- `UVHTTP_OK`: 转移成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效
- `UVHTTP_ERROR_INVALID_STATE`: 连接状态无效

**说明**:
此函数会：
1. 停止 HTTP 读取
2. 停止超时定时器
3. 获取 TCP 文件描述符
4. 标记连接为已升级状态
5. 调用回调函数转移所有权

**示例**:
```c
uvhttp_connection_transfer_ownership(
    conn,
    on_transfer_to_ipps,
    ipps_context
);
```

### uvhttp_connection_set_lifecycle

设置连接生命周期回调。

```c
uvhttp_error_t uvhttp_connection_set_lifecycle(
    uvhttp_connection_t* conn,
    uvhttp_connection_lifecycle_t* lifecycle
);
```

**参数**:
- `conn`: HTTP 连接对象
- `lifecycle`: 生命周期回调结构

**返回值**:
- `UVHTTP_OK`: 设置成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**说明**:
生命周期回调会在连接关闭时被调用，用于通知外部库清理资源。

**示例**:
```c
uvhttp_connection_lifecycle_t lifecycle = {
    .user_data = ipps_conn,
    .on_close = on_ipps_connection_close
};

uvhttp_connection_set_lifecycle(conn, &lifecycle);
```

---

## 辅助函数

### uvhttp_connection_get_fd

获取连接文件描述符。

```c
uvhttp_error_t uvhttp_connection_get_fd(
    uvhttp_connection_t* conn,
    int* fd
);
```

**参数**:
- `conn`: HTTP 连接对象
- `fd`: 输出参数，文件描述符

**返回值**:
- `UVHTTP_OK`: 获取成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效

**示例**:
```c
int fd;
uvhttp_error_t result = uvhttp_connection_get_fd(conn, &fd);
if (result == UVHTTP_OK) {
    printf("Connection FD: %d\n", fd);
}
```

### uvhttp_connection_get_peer_address

获取客户端地址。

```c
uvhttp_error_t uvhttp_connection_get_peer_address(
    uvhttp_connection_t* conn,
    struct sockaddr_storage* addr,
    socklen_t* addr_len
);
```

**参数**:
- `conn`: HTTP 连接对象
- `addr`: 输出参数，客户端地址
- `addr_len`: 输入输出参数，地址长度

**返回值**:
- `UVHTTP_OK`: 获取成功
- `UVHTTP_ERROR_INVALID_PARAM`: 参数无效
- `UVHTTP_ERROR_IO_ERROR`: IO 错误

**示例**:
```c
struct sockaddr_storage addr;
socklen_t addr_len = sizeof(addr);

uvhttp_error_t result = uvhttp_connection_get_peer_address(conn, &addr, &addr_len);
if (result == UVHTTP_OK) {
    if (addr.ss_family == AF_INET) {
        struct sockaddr_in* addr_in = (struct sockaddr_in*)&addr;
        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &addr_in->sin_addr, ip, sizeof(ip));
        printf("Client IP: %s\n", ip);
    }
}
```

---

## 使用示例

### 完整的 IPPS 集成示例

```c
#include <uvhttp.h>
#include <ipps.h>

// IPPS 上下文
typedef struct {
    uv_loop_t* loop;
    ipps_server_t* ipps_server;
} ipps_context_t;

// IPPS 协议检测器
int ipps_protocol_detector(uvhttp_request_t* request,
                           char* protocol_name,
                           size_t protocol_name_len) {
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    if (!upgrade || strcasecmp(upgrade, "ipps") != 0) {
        return 0;
    }
    
    const char* connection = uvhttp_request_get_header(request, "Connection");
    if (!connection || strstr(connection, "Upgrade") == NULL) {
        return 0;
    }
    
    strncpy(protocol_name, "ipps", protocol_name_len);
    return 1;
}

// IPPS 升级处理器
uvhttp_error_t ipps_upgrade_handler(uvhttp_connection_t* conn,
                                     const char* protocol_name,
                                     void* user_data) {
    ipps_context_t* ctx = (ipps_context_t*)user_data;
    
    // 1. 发送 101 Switching Protocols 响应
    uvhttp_response_set_status(conn->response, 101);
    uvhttp_response_set_header(conn->response, "Upgrade", "ipps");
    uvhttp_response_set_header(conn->response, "Connection", "Upgrade");
    uvhttp_response_send(conn->response);
    
    // 2. 转移连接所有权
    uvhttp_error_t result = uvhttp_connection_transfer_ownership(
        conn,
        on_transfer_to_ipps,
        ctx
    );
    
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to transfer ownership: %s", 
                         uvhttp_error_string(result));
        return result;
    }
    
    return UVHTTP_OK;
}

// 所有权转移回调
void on_transfer_to_ipps(uv_tcp_t* tcp_handle, int fd, void* user_data) {
    ipps_context_t* ctx = (ipps_context_t*)user_data;
    
    // 1. 创建 IPPS 连接
    ipps_connection_t* ipps_conn = ipps_connection_create(ctx->ipps_server, fd);
    if (!ipps_conn) {
        close(fd);
        return;
    }
    
    // 2. 设置生命周期回调
    uvhttp_connection_lifecycle_t lifecycle = {
        .user_data = ipps_conn,
        .on_close = on_ipps_connection_close
    };
    
    // 3. 开始 IPPS 协议处理
    ipps_connection_start(ipps_conn);
}

// IPPS 连接关闭回调
void on_ipps_connection_close(void* user_data) {
    ipps_connection_t* ipps_conn = (ipps_connection_t*)user_data;
    ipps_connection_free(ipps_conn);
}

// HTTP 请求处理器（用于普通 HTTP 请求）
int http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello from HTTP", 17);
    uvhttp_response_send(response);
    return 0;
}

int main() {
    // 1. 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 2. 创建 IPPS 上下文
    ipps_context_t ctx;
    ctx.loop = loop;
    ctx.ipps_server = ipps_server_create(loop);
    
    // 3. 创建 HTTP 服务器
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    
    // 4. 注册 HTTP 路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", http_handler);
    server->router = router;
    
    // 5. 注册 IPPS 协议升级
    uvhttp_server_register_protocol_upgrade(
        server,
        "ipps",
        "ipps",
        ipps_protocol_detector,
        ipps_upgrade_handler,
        &ctx
    );
    
    // 6. 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    // 7. 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 8. 清理资源
    uvhttp_server_free(server);
    ipps_server_free(ctx.ipps_server);
    
    return 0;
}
```

### gRPC-Web 集成示例

```c
// gRPC-Web 协议检测器
int grpc_web_protocol_detector(uvhttp_request_t* request,
                               char* protocol_name,
                               size_t protocol_name_len) {
    const char* content_type = uvhttp_request_get_header(request, "Content-Type");
    if (!content_type) {
        return 0;
    }
    
    if (strstr(content_type, "application/grpc-web") == NULL) {
        return 0;
    }
    
    strncpy(protocol_name, "grpc-web", protocol_name_len);
    return 1;
}

// gRPC-Web 升级处理器
uvhttp_error_t grpc_web_upgrade_handler(uvhttp_connection_t* conn,
                                         const char* protocol_name,
                                         void* user_data) {
    grpc_context_t* ctx = (grpc_context_t*)user_data;
    
    // gRPC-Web 不发送 101 响应，直接升级
    uvhttp_connection_transfer_ownership(
        conn,
        on_transfer_to_grpc,
        ctx
    );
    
    return UVHTTP_OK;
}

// 注册 gRPC-Web 协议
uvhttp_server_register_protocol_upgrade(
    server,
    "grpc-web",
    NULL,  // 不使用 Upgrade 头部
    grpc_web_protocol_detector,
    grpc_web_upgrade_handler,
    &grpc_ctx
);
```

---

## 错误码

### 协议注册错误

| 错误码 | 描述 | 解决方案 |
|--------|------|----------|
| `UVHTTP_ERROR_INVALID_PARAM` | 参数无效 | 检查服务器、协议名称等参数是否有效 |
| `UVHTTP_ERROR_OUT_OF_MEMORY` | 内存不足 | 检查系统内存，减少注册的协议数量 |
| `UVHTTP_ERROR_NOT_FOUND` | 协议未找到 | 确认协议名称正确 |

### 连接所有权转移错误

| 错误码 | 描述 | 解决方案 |
|--------|------|----------|
| `UVHTTP_ERROR_INVALID_PARAM` | 参数无效 | 检查连接和回调函数是否有效 |
| `UVHTTP_ERROR_INVALID_STATE` | 连接状态无效 | 确保连接处于正确的状态（HTTP_PROCESSING） |
| `UVHTTP_ERROR_IO_ERROR` | IO 错误 | 检查文件描述符是否有效 |

### 辅助函数错误

| 错误码 | 描述 | 解决方案 |
|--------|------|----------|
| `UVHTTP_ERROR_INVALID_PARAM` | 参数无效 | 检查连接和输出参数是否有效 |
| `UVHTTP_ERROR_IO_ERROR` | IO 错误 | 检查网络连接状态 |

---

**文档版本**：1.0  
**最后更新**：2026-02-03  
**维护者**：UVHTTP 开发团队