# WebSocket 认证指南

## 概述

UVHTTP 提供了完整的 WebSocket 连接认证功能，支持：

- **Token 认证**：通过查询参数或 HTTP 头传递 Token
- **IP 白名单**：只允许特定 IP 地址连接
- **IP 黑名单**：拒绝特定 IP 地址连接

## 认证机制

### 1. Token 认证

Token 认证允许您在 WebSocket 握手时验证客户端身份。

#### Token 传递方式

客户端可以通过两种方式传递 Token：

1. **查询参数**：`ws://localhost:8080/ws?token=secret123`
2. **Authorization 头**：`Authorization: Bearer secret123`

#### 启用 Token 认证

```c
#include "uvhttp.h"
#include "uvhttp_websocket_auth.h"

/* Token 验证函数 */
int validate_token(const char* token, void* user_data) {
    /* 验证逻辑 */
    if (strcmp(token, "secret123") == 0) {
        return 0;  /* 认证成功 */
    }
    return -1;  /* 认证失败 */
}

/* 创建服务器 */
uvhttp_server_builder_t* server = uvhttp_server_create("0.0.0.0", 8080);

/* 注册 WebSocket 处理器 */
uvhttp_ws_handler_t ws_handler = {
    .on_connect = on_connect,
    .on_message = on_message,
    .on_close = on_close,
    .on_error = on_error,
    .user_data = NULL
};

uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);

/* 启用 Token 认证 */
uvhttp_server_ws_enable_token_auth(server, "/ws", validate_token, NULL);

/* 启动服务器 */
uvhttp_server_listen(server, "0.0.0.0", 8080);
```

### 2. IP 白名单

IP 白名单只允许特定 IP 地址的客户端连接。

```c
/* 添加 IP 到白名单 */
uvhttp_server_ws_add_ip_to_whitelist(server, "/ws", "127.0.0.1");
uvhttp_server_ws_add_ip_to_whitelist(server, "/ws", "192.168.1.0/24");
```

### 3. IP 黑名单

IP 黑名单拒绝特定 IP 地址的客户端连接。

```c
/* 添加 IP 到黑名单 */
uvhttp_server_ws_add_ip_to_blacklist(server, "/ws", "192.168.1.100");
uvhttp_server_ws_add_ip_to_blacklist(server, "/ws", "10.0.0.0/8");
```

## API 参考

### 认证配置

#### `uvhttp_ws_auth_config_t`

认证配置结构体。

```c
typedef struct {
    int enable_token_auth;                    /* 是否启用 Token 认证 */
    char token_param_name[64];                /* Token 参数名 */
    uvhttp_ws_token_validator_callback token_validator;  /* Token 验证回调 */
    void* token_validator_data;               /* Token 验证回调的用户数据 */
    int enable_ip_whitelist;                  /* 是否启用 IP 白名单 */
    ip_entry_t* ip_whitelist;                 /* IP 白名单链表 */
    int enable_ip_blacklist;                  /* 是否启用 IP 黑名单 */
    ip_entry_t* ip_blacklist;                 /* IP 黑名单链表 */
    int send_auth_failed_response;            /* 是否发送认证失败响应 */
    char auth_failed_message[256];            /* 认证失败消息 */
} uvhttp_ws_auth_config_t;
```

### Token 验证回调

#### `uvhttp_ws_token_validator_callback`

Token 验证回调函数类型。

```c
typedef int (*uvhttp_ws_token_validator_callback)(const char* token, void* user_data);
```

**参数**：
- `token`：客户端提供的 Token
- `user_data`：用户数据

**返回值**：
- `0`：认证成功
- 非 0：认证失败

### 服务器 API

#### `uvhttp_server_ws_enable_token_auth`

启用 Token 认证。

```c
uvhttp_error_t uvhttp_server_ws_enable_token_auth(
    uvhttp_server_t* server,
    const char* path,
    uvhttp_ws_token_validator_callback validator,
    void* user_data
);
```

**参数**：
- `server`：服务器实例
- `path`：WebSocket 路径
- `validator`：Token 验证回调函数
- `user_data`：用户数据

**返回值**：
- `UVHTTP_OK`：成功
- 其他值：失败

#### `uvhttp_server_ws_add_ip_to_whitelist`

添加 IP 到白名单。

```c
uvhttp_error_t uvhttp_server_ws_add_ip_to_whitelist(
    uvhttp_server_t* server,
    const char* path,
    const char* ip
);
```

**参数**：
- `server`：服务器实例
- `path`：WebSocket 路径
- `ip`：IP 地址或 CIDR（如 "192.168.1.0/24"）

**返回值**：
- `UVHTTP_OK`：成功
- 其他值：失败

#### `uvhttp_server_ws_add_ip_to_blacklist`

添加 IP 到黑名单。

```c
uvhttp_error_t uvhttp_server_ws_add_ip_to_blacklist(
    uvhttp_server_t* server,
    const char* path,
    const char* ip
);
```

**参数**：
- `server`：服务器实例
- `path`：WebSocket 路径
- `ip`：IP 地址或 CIDR（如 "10.0.0.0/8"）

**返回值**：
- `UVHTTP_OK`：成功
- 其他值：失败

#### `uvhttp_server_ws_set_auth_config`

设置完整的认证配置。

```c
uvhttp_error_t uvhttp_server_ws_set_auth_config(
    uvhttp_server_t* server,
    const char* path,
    uvhttp_ws_auth_config_t* config
);
```

**参数**：
- `server`：服务器实例
- `path`：WebSocket 路径
- `config`：认证配置

**返回值**：
- `UVHTTP_OK`：成功
- 其他值：失败

## 认证流程

1. **客户端发起握手**
   - 客户端发送 WebSocket 握手请求
   - 可以在查询参数或 Authorization 头中包含 Token

2. **服务器提取认证信息**
   - 服务器从请求中提取客户端 IP
   - 服务器从查询参数或 Authorization 头中提取 Token

3. **执行认证检查**
   - 检查 IP 黑名单（如果启用）
   - 检查 IP 白名单（如果启用）
   - 验证 Token（如果启用）

4. **认证结果**
   - 认证成功：继续握手流程，创建 WebSocket 连接
   - 认证失败：拒绝连接，关闭 TCP 连接

## 认证结果

认证可能返回以下结果：

| 结果 | 说明 |
|------|------|
| `UVHTTP_WS_AUTH_SUCCESS` | 认证成功 |
| `UVHTTP_WS_AUTH_FAILED` | 认证失败 |
| `UVHTTP_WS_AUTH_NO_TOKEN` | 缺少 Token |
| `UVHTTP_WS_AUTH_INVALID_TOKEN` | Token 无效 |
| `UVHTTP_WS_AUTH_EXPIRED_TOKEN` | Token 已过期 |
| `UVHTTP_WS_AUTH_IP_BLOCKED` | IP 被阻止 |
| `UVHTTP_WS_AUTH_IP_NOT_ALLOWED` | IP 不在白名单中 |
| `UVHTTP_WS_AUTH_INTERNAL_ERROR` | 内部错误 |

## 示例

### 完整示例

参见 `examples/websocket_auth_server.c`。

```bash
# 编译
cd build
make websocket_auth_server

# 运行
./dist/bin/websocket_auth_server 8080
```

### 客户端连接示例

#### 使用查询参数

```javascript
// JavaScript
const ws = new WebSocket('ws://localhost:8080/ws?token=secret123');
```

#### 使用 Authorization 头

```javascript
// JavaScript 需要使用支持自定义头的 WebSocket 库
const ws = new WebSocket('ws://localhost:8080/ws', {
    headers: {
        'Authorization': 'Bearer secret123'
    }
});
```

#### Python

```python
import websockets
import asyncio

async def connect():
    # 使用查询参数
    uri = "ws://localhost:8080/ws?token=secret123"
    async with websockets.connect(uri) as websocket:
        await websocket.send("Hello, Server!")
        response = await websocket.recv()
        print(response)

asyncio.run(connect())
```

## 安全建议

1. **使用 HTTPS/WSS**：在生产环境中，始终使用 WSS（WebSocket Secure）而不是 WS
2. **Token 安全**：
   - 使用强随机 Token
   - 定期轮换 Token
   - 为 Token 设置过期时间
3. **IP 过滤**：
   - 结合 IP 白名单和黑名单使用
   - 定期审查 IP 列表
4. **日志记录**：记录所有认证失败尝试，用于安全审计
5. **速率限制**：对认证失败尝试实施速率限制，防止暴力破解

## 常见问题

### Q: 如何实现 Token 过期？

A: 在 Token 验证函数中检查 Token 的过期时间：

```c
int validate_token(const char* token, void* user_data) {
    /* 从数据库或缓存中查找 Token */
    token_info_t* info = find_token(token);

    if (!info) {
        return -1;  /* Token 不存在 */
    }

    /* 检查过期时间 */
    if (info->expires_at < time(NULL)) {
        return -1;  /* Token 已过期 */
    }

    return 0;  /* 认证成功 */
}
```

### Q: 如何支持多种认证方式？

A: 在 Token 验证函数中实现多种验证逻辑：

```c
int validate_token(const char* token, void* user_data) {
    /* 尝试 JWT 验证 */
    if (validate_jwt(token) == 0) {
        return 0;
    }

    /* 尝试数据库验证 */
    if (validate_from_database(token) == 0) {
        return 0;
    }

    /* 尝试 Redis 缓存验证 */
    if (validate_from_redis(token) == 0) {
        return 0;
    }

    return -1;  /* 所有验证方式都失败 */
}
```

### Q: 如何动态更新 IP 白名单？

A: 使用 `uvhttp_server_ws_set_auth_config` 更新配置：

```c
/* 创建新配置 */
uvhttp_ws_auth_config_t* new_config = uvhttp_ws_auth_config_create();
uvhttp_ws_auth_add_ip_to_whitelist(new_config, "192.168.1.100");

/* 更新配置 */
uvhttp_server_ws_set_auth_config(server, "/ws", new_config);

/* 释放旧配置 */
uvhttp_ws_auth_config_destroy(old_config);
```

## 参考资源

- [WebSocket RFC 6455](https://tools.ietf.org/html/rfc6455)
- [API 参考](API_REFERENCE.md)
- [WebSocket 示例](../examples/websocket_auth_server.c)