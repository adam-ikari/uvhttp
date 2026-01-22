# WebSocket 认证指南

## 概述

WebSocket 认证功能已从 UVHTTP 核心库中剥离，完全由应用层实现。这种设计提供了更大的灵活性，允许开发者根据具体需求实现自定义的认证逻辑。

## 为什么由应用层实现？

1. **灵活性**：应用层可以根据业务需求实现任何认证逻辑
2. **可定制性**：支持各种认证方式（JWT、OAuth、数据库验证等）
3. **安全性**：认证逻辑完全由应用控制，避免核心库的安全隐患
4. **轻量级**：核心库保持简洁，只负责 WebSocket 协议处理

## 应用层实现认证

### 1. 认证检查函数

在 WebSocket 连接建立时进行认证检查：

```c
#include "uvhttp.h"
#include "uvhttp_websocket_auth.h"

/* 简单的 Token 验证函数 */
int validate_token(const char* token, void* user_data) {
    /* 验证逻辑 */
    if (strcmp(token, "secret123") == 0) {
        return 0;  /* 认证成功 */
    }
    return -1;  /* 认证失败 */
}

/* 认证配置 */
static uvhttp_ws_auth_config_t* g_auth_config = NULL;

/* 应用层认证检查函数 */
int check_websocket_auth(const char* client_ip, const char* token) {
    if (!g_auth_config) {
        return 1;  /* 没有配置认证，允许连接 */
    }

    uvhttp_ws_auth_result_t result = uvhttp_ws_authenticate(g_auth_config, client_ip, token);
    
    if (result != UVHTTP_WS_AUTH_SUCCESS) {
        printf("认证失败: %s (IP: %s)\n", 
               uvhttp_ws_auth_result_string(result), client_ip);
        return 0;  /* 认证失败 */
    }

    return 1;  /* 认证成功 */
}
```

### 2. 创建认证配置

```c
/* 创建认证配置 */
g_auth_config = uvhttp_ws_auth_config_create();
if (!g_auth_config) {
    fprintf(stderr, "无法创建认证配置\n");
    return 1;
}

/* 启用 Token 认证 */
g_auth_config->enable_token_auth = 1;
uvhttp_ws_auth_set_token_validator(g_auth_config, validate_token, NULL);

/* 添加 IP 白名单（可选） */
uvhttp_ws_auth_add_ip_to_whitelist(g_auth_config, "127.0.0.1");
uvhttp_ws_auth_add_ip_to_whitelist(g_auth_config, "192.168.1.0/24");

/* 添加 IP 黑名单（可选） */
uvhttp_ws_auth_add_ip_to_blacklist(g_auth_config, "192.168.1.100");
```

### 3. 注册 WebSocket 处理器

```c
/* WebSocket 消息回调 */
int on_message(uvhttp_ws_connection_t* ws_conn, const char* data, size_t len, int opcode) {
    printf("收到消息: %.*s\n", (int)len, data);
    uvhttp_server_ws_send(ws_conn, data, len);
    return 0;
}

/* WebSocket 连接回调 */
int on_connect(uvhttp_ws_connection_t* ws_conn) {
    printf("新的 WebSocket 连接已建立\n");
    return 0;
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

uvhttp_server_register_ws_handler(server->server, "/ws", &ws_handler);
```

### 4. 完整示例

参见 `examples/05_advanced/websocket_auth_server.c`。

## 认证功能说明

### 1. Token 认证

Token 认证允许您在 WebSocket 握手时验证客户端身份。

#### Token 传递方式

客户端可以通过两种方式传递 Token：

1. **查询参数**：`ws://localhost:8080/ws?token=secret123`
2. **Authorization 头**：`Authorization: Bearer secret123`

#### Token 验证

```c
int validate_token(const char* token, void* user_data) {
    /* 验证逻辑 */
    if (strcmp(token, "secret123") == 0) {
        return 0;  /* 认证成功 */
    }
    return -1;  /* 认证失败 */
}
```

### 2. IP 白名单

IP 白名单只允许特定 IP 地址的客户端连接。

```c
/* 添加 IP 到白名单 */
uvhttp_ws_auth_add_ip_to_whitelist(g_auth_config, "127.0.0.1");
uvhttp_ws_auth_add_ip_to_whitelist(g_auth_config, "192.168.1.0/24");
```

### 3. IP 黑名单

IP 黑名单拒绝特定 IP 地址的客户端连接。

```c
/* 添加 IP 到黑名单 */
uvhttp_ws_auth_add_ip_to_blacklist(g_auth_config, "192.168.1.100");
uvhttp_ws_auth_add_ip_to_blacklist(g_auth_config, "10.0.0.0/8");
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

### 认证函数

#### `uvhttp_ws_auth_config_create()`

创建认证配置。

```c
uvhttp_ws_auth_config_t* uvhttp_ws_auth_config_create(void);
```

#### `uvhttp_ws_auth_config_destroy()`

销毁认证配置。

```c
void uvhttp_ws_auth_config_destroy(uvhttp_ws_auth_config_t* config);
```

#### `uvhttp_ws_auth_set_token_validator()`

设置 Token 验证回调。

```c
void uvhttp_ws_auth_set_token_validator(
    uvhttp_ws_auth_config_t* config,
    uvhttp_ws_token_validator_callback validator,
    void* user_data
);
```

#### `uvhttp_ws_auth_add_ip_to_whitelist()`

添加 IP 到白名单。

```c
int uvhttp_ws_auth_add_ip_to_whitelist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);
```

#### `uvhttp_ws_auth_add_ip_to_blacklist()`

添加 IP 到黑名单。

```c
int uvhttp_ws_auth_add_ip_to_blacklist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);
```

#### `uvhttp_ws_authenticate()`

执行认证。

```c
uvhttp_ws_auth_result_t uvhttp_ws_authenticate(
    uvhttp_ws_auth_config_t* config,
    const char* client_ip,
    const char* token
);
```

### 认证结果

#### `uvhttp_ws_auth_result_t`

认证结果枚举。

```c
typedef enum {
    UVHTTP_WS_AUTH_SUCCESS = 0,           /* 认证成功 */
    UVHTTP_WS_AUTH_FAILED = -1,           /* 认证失败 */
    UVHTTP_WS_AUTH_NO_TOKEN = -2,         /* 缺少 Token */
    UVHTTP_WS_AUTH_INVALID_TOKEN = -3,    /* Token 无效 */
    UVHTTP_WS_AUTH_EXPIRED_TOKEN = -4,    /* Token 已过期 */
    UVHTTP_WS_AUTH_IP_BLOCKED = -5,       /* IP 被阻止 */
    UVHTTP_WS_AUTH_IP_NOT_ALLOWED = -6,   /* IP 不在白名单中 */
    UVHTTP_WS_AUTH_INTERNAL_ERROR = -7    /* 内部错误 */
} uvhttp_ws_auth_result_t;
```

## 安全建议

1. **使用 HTTPS/WSS**：在生产环境中始终使用加密连接
2. **Token 安全**：使用强随机生成的 Token，定期更换
3. **IP 限制**：结合 IP 白名单增强安全性
4. **日志记录**：记录所有认证尝试，便于审计
5. **速率限制**：防止暴力破解攻击

## 编译示例

```bash
cd build
make websocket_auth_server
./dist/bin/websocket_auth_server 8080
```

## 客户端连接示例

```javascript
// 使用 Token 连接
const ws = new WebSocket('ws://localhost:8080/ws?token=secret123');

// 或使用 Authorization 头
const ws = new WebSocket('ws://localhost:8080/ws', [], {
    headers: {
        'Authorization': 'Bearer secret123'
    }
});
```

## 参考资料

- [WebSocket 协议规范](https://tools.ietf.org/html/rfc6455)
- [示例代码](../examples/05_advanced/websocket_auth_server.c)
- [WebSocket 指南](../WEBSOCKET.md)