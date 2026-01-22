# WebSocket 认证指南

## 概述

WebSocket 认证功能已从 UVHTTP 核心库中剥离，完全由应用层实现。这种设计提供了更大的灵活性，允许开发者根据具体需求实现自定义的认证逻辑。

## 设计思想

### 为什么由应用层实现？

1. **灵活性**：应用层可以根据业务需求实现任何认证逻辑（JWT、OAuth、数据库验证等）
2. **可定制性**：支持各种认证方式，不受核心库限制
3. **安全性**：认证逻辑完全由应用控制，避免核心库的安全隐患
4. **轻量级**：核心库保持简洁，只负责 WebSocket 协议处理

### 架构设计

```
┌─────────────────────────────────────────┐
│         应用层（Application）           │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │  WebSocket 认证模块             │   │
│  │  - Token 验证                  │   │
│  │  - IP 白名单/黑名单            │   │
│  │  - 自定义认证逻辑              │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
                    │
                    │ 调用
                    ▼
┌─────────────────────────────────────────┐
│         UVHTTP 核心库                   │
│                                         │
│  ┌─────────────────────────────────┐   │
│  │  WebSocket 协议处理             │   │
│  │  - 握手处理                    │   │
│  │  - 消息收发                    │   │
│  │  - 连接管理                    │   │
│  └─────────────────────────────────┘   │
└─────────────────────────────────────────┘
```

### 认证流程

```
客户端请求
    │
    ▼
WebSocket 握手
    │
    ├─→ 提取 Token（查询参数或 Authorization 头）
    │
    ├─→ 提取客户端 IP
    │
    ├─→ 应用层认证检查
    │       │
    │       ├─ Token 验证
    │       ├─ IP 白名单检查
    │       └─ IP 黑名单检查
    │
    ├─→ 认证成功？
    │       │
    │       ├─ 是 → 建立连接
    │       └─ 否 → 拒绝连接
    │
    ▼
连接建立/拒绝
```

## 认证功能说明

### 1. Token 认证

Token 认证允许在 WebSocket 握手时验证客户端身份。

**设计考虑**：
- 支持多种 Token 传递方式（查询参数、HTTP 头）
- 应用层可自定义 Token 验证逻辑
- 支持各种 Token 类型（JWT、Bearer Token、自定义 Token）

**Token 传递方式**：
1. **查询参数**：`ws://localhost:8080/ws?token=secret123`
2. **Authorization 头**：`Authorization: Bearer secret123`

### 2. IP 白名单

IP 白名单只允许特定 IP 地址的客户端连接。

**设计考虑**：
- 支持单个 IP 地址（如 `127.0.0.1`）
- 支持 CIDR 表示法（如 `192.168.1.0/24`）
- 应用层可动态管理白名单

### 3. IP 黑名单

IP 黑名单拒绝特定 IP 地址的客户端连接。

**设计考虑**：
- 支持单个 IP 地址（如 `192.168.1.100`）
- 支持 CIDR 表示法（如 `10.0.0.0/8`）
- 应用层可动态管理黑名单

## API 参考

### 认证配置

#### `uvhttp_ws_auth_config_t`

认证配置结构体，用于配置认证策略。

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

## 示例代码

完整的示例代码请参考：
- `examples/05_advanced/websocket_auth_server.c` - WebSocket 认证服务器示例

## 参考资料

- [WebSocket 协议规范](https://tools.ietf.org/html/rfc6455)
- [WebSocket 指南](./WEBSOCKET.md)