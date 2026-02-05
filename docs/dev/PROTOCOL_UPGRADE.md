# UVHTTP 协议升级设计文档

> 版本：2.3.0
> 更新日期：2026-02-03
> 状态：设计阶段

## 目录

1. [概述](#概述)
2. [背景与需求](#背景与需求)
3. [架构设计](#架构设计)
4. [协议检测机制](#协议检测机制)
5. [连接所有权转移](#连接所有权转移)
6. [资源管理](#资源管理)
7. [与 WebSocket 升级的对比](#与-websocket-升级的对比)
8. [实现细节](#实现细节)
9. [最佳实践](#最佳实践)
10. [性能考虑](#性能考虑)

---

## 概述

### 协议升级框架简介

UVHTTP 协议升级框架是一个通用、可扩展的机制，允许将 HTTP/1.1 连接升级为自定义协议（如 IPPS、gRPC-Web 等）。框架设计遵循 UVHTTP 的核心原则：专注核心、零开销、极简工程。

### 设计目标

1. **通用性**：支持任意自定义协议的升级
2. **可扩展性**：通过注册机制支持多个协议
3. **零开销**：不使用协议升级时无性能损失
4. **兼容性**：与现有 WebSocket 升级机制共存
5. **安全性**：确保连接所有权转移的安全性

### 核心特性

- 动态协议检测器注册
- 连接所有权安全转移
- 完整的生命周期管理
- 与 WebSocket 协议共存
- 支持 TLS 连接升级

---

## 背景与需求

### 当前限制

UVHTTP 当前仅支持 WebSocket 协议升级，且实现是硬编码的：

```c
// 当前硬编码的 WebSocket 检测
static int is_websocket_handshake_request(uvhttp_request_t* request) {
    const char* upgrade = uvhttp_request_get_header(request, HTTP_HEADER_UPGRADE);
    const char* connection = uvhttp_request_get_header(request, HTTP_HEADER_CONNECTION);
    const char* ws_key = uvhttp_request_get_header(request, HTTP_HEADER_SEC_WEBSOCKET_KEY);
    
    if (!upgrade || !connection || !ws_key) {
        return FALSE;
    }
    
    if (strcasecmp(upgrade, HTTP_VALUE_WEBSOCKET) != 0) {
        return FALSE;
    }
    
    if (strstr(connection, HTTP_HEADER_UPGRADE) == NULL) {
        return FALSE;
    }
    
    return TRUE;
}
```

### 应用场景

#### 场景 1: IPPS 打印协议集成

IPPS (Internet Printing Protocol over Socket) 是一个用于打印机通信的自定义协议，需要在 HTTP 连接建立后升级为 IPPS 协议。

**需求**：
- 检测 `Upgrade: ipps` 头部
- 验证客户端支持的 IPPS 版本
- 将 HTTP 连接转移给 IPPS 库
- IPPS 库接管连接生命周期

#### 场景 2: gRPC-Web 升级

gRPC-Web 允许浏览器应用直接调用 gRPC 服务，需要协议升级支持。

**需求**：
- 检测 `Content-Type: application/grpc-web` 头部
- 支持 gRPC-Web 协议握手
- 转移连接给 gRPC 处理器

#### 场景 3: 自定义二进制协议

应用层可能需要实现自定义二进制协议，通过 HTTP 升级机制建立连接。

**需求**：
- 灵活的协议检测机制
- 自定义握手验证
- 完整的连接控制

### 需求分析

| 需求 | 优先级 | 描述 |
|------|--------|------|
| 协议检测器注册 | 高 | 允许应用层注册自定义协议检测逻辑 |
| 连接所有权转移 | 高 | 安全地将 TCP 连接转移给外部库 |
| 生命周期管理 | 高 | 协调 UVHTTP 和外部库的资源清理 |
| TLS 支持 | 中 | 支持 TLS 连接的协议升级 |
| 性能优化 | 中 | 确保协议检测不影响现有性能 |
| 错误处理 | 高 | 完整的错误处理和恢复机制 |

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层（Application）                    │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                  │
│  │  IPPS    │  │  gRPC    │  │  Custom  │                  │
│  │  Library │  │  Handler │  │  Protocol│                  │
│  └──────────┘  └──────────┘  └──────────┘                  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                 UVHTTP 协议升级框架（Protocol Upgrade）       │
│  ┌──────────────────────────────────────────────────────┐  │
│  │              协议注册表（Protocol Registry）           │  │
│  │  ┌──────────┐  ┌──────────┐  ┌──────────┐           │  │
│  │  │  IPPS    │  │ WebSocket│  │  Custom  │           │  │
│  │  └──────────┘  └──────────┘  └──────────┘           │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │            协议检测器（Protocol Detectors）            │  │
│  └──────────────────────────────────────────────────────┘  │
│  ┌──────────────────────────────────────────────────────┐  │
│  │          升级处理器（Upgrade Handlers）                │  │
│  └──────────────────────────────────────────────────────┘  │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                  UVHTTP 核心层（Core）                       │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │  Server  │  │  Router  │  │  Request │  │ Response │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐                 │
│  │Connection│  │  Static  │  │   TLS    │                 │
│  └──────────┘  └──────────┘  └──────────┘                 │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    依赖层（Dependencies）                       │
│  ┌─────────┐  ┌─────────┐                                     │
│  │  libuv  │  │ llhttp  │                                     │
│  └─────────┘  └─────────┘                                     │
└─────────────────────────────────────────────────────────────┘
```

### 核心组件

#### 1. 协议注册表（Protocol Registry）

存储所有已注册的协议信息。

```c
typedef struct uvhttp_protocol_info {
    char name[32];                           // 协议名称
    char upgrade_header[64];                 // Upgrade 头部值
    uvhttp_protocol_detector_t detector;     // 协议检测器
    uvhttp_protocol_upgrade_handler_t handler; // 升级处理器
    void* user_data;                         // 用户数据
    struct uvhttp_protocol_info* next;       // 链表指针
} uvhttp_protocol_info_t;

typedef struct {
    uvhttp_protocol_info_t* protocols;       // 协议链表
    size_t protocol_count;                   // 协议数量
} uvhttp_protocol_registry_t;
```

#### 2. 协议检测器（Protocol Detector）

检测请求是否匹配特定协议。

```c
/**
 * @brief 协议检测函数
 * 
 * @param request HTTP 请求对象
 * @param protocol_name 输出参数，检测到的协议名称
 * @param protocol_name_len 协议名称缓冲区大小
 * @return int 1 表示检测到协议，0 表示未检测到
 */
typedef int (*uvhttp_protocol_detector_t)(
    uvhttp_request_t* request,
    char* protocol_name,
    size_t protocol_name_len
);
```

#### 3. 升级处理器（Upgrade Handler）

处理协议升级逻辑。

```c
/**
 * @brief 协议升级处理器
 * 
 * @param conn HTTP 连接对象
 * @param protocol_name 协议名称
 * @param user_data 用户数据
 * @return uvhttp_error_t UVHTTP_OK 表示成功，其他值表示错误
 */
typedef uvhttp_error_t (*uvhttp_protocol_upgrade_handler_t)(
    uvhttp_connection_t* conn,
    const char* protocol_name,
    void* user_data
);
```

#### 4. 连接所有权转移（Ownership Transfer）

安全地将连接转移给外部库。

```c
/**
 * @brief 连接所有权转移回调
 * 
 * @param tcp_handle TCP 句柄
 * @param fd 文件描述符
 * @param user_data 用户数据
 */
typedef void (*uvhttp_connection_ownership_callback_t)(
    uv_tcp_t* tcp_handle,
    int fd,
    void* user_data
);
```

### 协议升级流程

```
┌─────────────────────────────────────────────────────────────┐
│  1. 客户端发送 HTTP 请求                                     │
│     GET / HTTP/1.1                                          │
│     Host: example.com                                       │
│     Upgrade: ipps                                           │
│     Connection: Upgrade                                     │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  2. UVHTTP 接收请求并解析                                    │
│     - on_read 回调接收数据                                   │
│     - llhttp 解析 HTTP 请求                                  │
│     - on_message_complete 触发                               │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  3. 协议检测                                                  │
│     - 遍历协议注册表                                          │
│     - 调用每个协议的检测器                                    │
│     - 确定协议类型（IPPS）                                    │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  4. 调用协议升级处理器                                        │
│     - 发送 101 Switching Protocols 响应                      │
│     - 停止 HTTP 读取                                          │
│     - 获取 TCP 文件描述符                                     │
│     - 转移连接所有权                                          │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│  5. 外部库接管连接                                            │
│     - IPPS 库创建连接对象                                     │
│     - 开始 IPPS 协议处理                                      │
│     - 管理连接生命周期                                        │
└─────────────────────────────────────────────────────────────┘
```

---

## 协议检测机制

### 检测器设计

协议检测器在 HTTP 请求解析完成后（`on_message_complete`）被调用。

### 检测流程

```c
static void on_message_complete(llhttp_t* parser) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    uvhttp_request_t* request = conn->request;
    uvhttp_server_t* server = conn->server;
    
    // 1. 首先检查 WebSocket（向后兼容）
    if (is_websocket_handshake_request(request)) {
        uvhttp_connection_handle_websocket_handshake(conn, ...);
        return;
    }
    
    // 2. 遍历协议注册表
    if (server->protocol_registry) {
        char protocol_name[32];
        
        for (uvhttp_protocol_info_t* proto = server->protocol_registry->protocols;
             proto != NULL; proto = proto->next) {
            
            // 调用协议检测器
            if (proto->detector(request, protocol_name, sizeof(protocol_name))) {
                // 调用升级处理器
                uvhttp_error_t result = proto->handler(conn, protocol_name, proto->user_data);
                
                if (result == UVHTTP_OK) {
                    // 升级成功，不再继续 HTTP 处理
                    return;
                } else {
                    // 升级失败，返回错误响应
                    uvhttp_response_set_status(conn->response, 400);
                    uvhttp_response_send(conn->response);
                    return;
                }
            }
        }
    }
    
    // 3. 未检测到协议升级，继续正常 HTTP 处理
    // ... 路由匹配和请求处理 ...
}
```

### 检测器实现示例

#### IPPS 协议检测器

```c
int ipps_protocol_detector(uvhttp_request_t* request,
                           char* protocol_name,
                           size_t protocol_name_len) {
    // 检查 Upgrade 头部
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    if (!upgrade) {
        return 0;
    }
    
    // 检查是否为 IPPS 协议
    if (strcasecmp(upgrade, "ipps") != 0) {
        return 0;
    }
    
    // 检查 Connection 头部
    const char* connection = uvhttp_request_get_header(request, "Connection");
    if (!connection || strstr(connection, "Upgrade") == NULL) {
        return 0;
    }
    
    // 可选：检查 IPPS 版本头部
    const char* ipps_version = uvhttp_request_get_header(request, "X-IPPS-Version");
    if (ipps_version) {
        // 验证版本是否支持
        // ...
    }
    
    // 设置协议名称
    strncpy(protocol_name, "ipps", protocol_name_len);
    
    return 1;  // 检测到 IPPS 协议
}
```

#### gRPC-Web 协议检测器

```c
int grpc_web_protocol_detector(uvhttp_request_t* request,
                               char* protocol_name,
                               size_t protocol_name_len) {
    // gRPC-Web 通过 Content-Type 检测
    const char* content_type = uvhttp_request_get_header(request, "Content-Type");
    if (!content_type) {
        return 0;
    }
    
    // 检查是否为 gRPC-Web
    if (strstr(content_type, "application/grpc-web") == NULL) {
        return 0;
    }
    
    // 设置协议名称
    strncpy(protocol_name, "grpc-web", protocol_name_len);
    
    return 1;  // 检测到 gRPC-Web 协议
}
```

---

## 连接所有权转移

### 转移机制

连接所有权转移是协议升级的核心，需要确保：

1. **安全性**：避免内存泄漏和双重释放
2. **完整性**：TCP 连接状态保持一致
3. **可控性**：UVHTTP 仍能监控连接状态

### 转移 API

```c
/**
 * @brief 转移连接所有权给外部库
 * 
 * @param conn HTTP 连接对象
 * @param callback 所有权转移回调
 * @param user_data 用户数据
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_connection_transfer_ownership(
    uvhttp_connection_t* conn,
    uvhttp_connection_ownership_callback_t callback,
    void* user_data
);
```

### 转移流程

```c
uvhttp_error_t uvhttp_connection_transfer_ownership(
    uvhttp_connection_t* conn,
    uvhttp_connection_ownership_callback_t callback,
    void* user_data) {
    
    if (!conn || !callback) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 1. 停止 HTTP 读取
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);
    
    // 2. 停止超时定时器
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }
    
    // 3. 获取文件描述符
    int fd = 0;
    uv_fileno((uv_handle_t*)&conn->tcp_handle, &fd);
    
    // 4. 标记连接已转移
    conn->state = UVHTTP_CONN_STATE_PROTOCOL_UPGRADED;
    
    // 5. 调用回调，转移所有权
    callback(&conn->tcp_handle, fd, user_data);
    
    return UVHTTP_OK;
}
```

### 外部库接管示例

```c
// IPPS 升级处理器
uvhttp_error_t ipps_upgrade_handler(uvhttp_connection_t* conn,
                                     const char* protocol_name,
                                     void* user_data) {
    
    ipps_context_t* ipps_ctx = (ipps_context_t*)user_data;
    
    // 1. 发送 101 Switching Protocols 响应
    uvhttp_response_set_status(conn->response, 101);
    uvhttp_response_set_header(conn->response, "Upgrade", "ipps");
    uvhttp_response_set_header(conn->response, "Connection", "Upgrade");
    uvhttp_response_send(conn->response);
    
    // 2. 转移连接所有权
    uvhttp_error_t result = uvhttp_connection_transfer_ownership(
        conn,
        on_transfer_to_ipps,
        ipps_ctx
    );
    
    if (result != UVHTTP_OK) {
        return result;
    }
    
    return UVHTTP_OK;
}

// 所有权转移回调
void on_transfer_to_ipps(uv_tcp_t* tcp_handle, int fd, void* user_data) {
    ipps_context_t* ipps_ctx = (ipps_context_t*)user_data;
    
    // 1. 创建 IPPS 连接
    ipps_connection_t* ipps_conn = ipps_connection_create(ipps_ctx, fd);
    
    // 2. 开始 IPPS 协议处理
    ipps_connection_start(ipps_conn);
    
    // 3. IPPS 库管理连接生命周期
    // ...
}
```

---

## 资源管理

### 生命周期管理

协议升级后，连接的生命周期由外部库管理，但 UVHTTP 需要记录连接状态。

### 状态管理

```c
typedef enum {
    UVHTTP_CONN_STATE_NEW,
    UVHTTP_CONN_STATE_TLS_HANDSHAKE,
    UVHTTP_CONN_STATE_HTTP_READING,
    UVHTTP_CONN_STATE_HTTP_PROCESSING,
    UVHTTP_CONN_STATE_HTTP_WRITING,
    UVHTTP_CONN_STATE_PROTOCOL_UPGRADED,  // 新增：协议升级状态
    UVHTTP_CONN_STATE_CLOSING
} uvhttp_connection_state_t;
```

### 资源清理

#### UVHTTP 侧清理

```c
void uvhttp_connection_free(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }
    
    // 如果连接已升级，不释放 TCP 句柄（由外部库管理）
    if (conn->state == UVHTTP_CONN_STATE_PROTOCOL_UPGRADED) {
        // 只释放 HTTP 相关资源
        if (conn->request) {
            uvhttp_request_free(conn->request);
        }
        if (conn->response) {
            uvhttp_response_free(conn->response);
        }
        uvhttp_free(conn);
        return;
    }
    
    // 正常连接清理
    // ...
}
```

#### 外部库侧清理

```c
void ipps_connection_close(ipps_connection_t* ipps_conn) {
    if (!ipps_conn) {
        return;
    }
    
    // 1. 关闭文件描述符
    close(ipps_conn->fd);
    
    // 2. 清理 IPPS 资源
    // ...
    
    // 3. 释放连接对象
    uvhttp_free(ipps_conn);
}
```

### 引用计数

为确保资源安全，可以使用引用计数：

```c
typedef struct {
    int ref_count;
    uvhttp_connection_t* http_conn;
    void* external_conn;
    void (*on_close)(void* external_conn);
} uvhttp_upgraded_connection_t;

void uvhttp_upgraded_connection_add_ref(uvhttp_upgraded_connection_t* conn) {
    conn->ref_count++;
}

void uvhttp_upgraded_connection_release(uvhttp_upgraded_connection_t* conn) {
    conn->ref_count--;
    if (conn->ref_count == 0) {
        // 双方都释放了，清理资源
        if (conn->on_close) {
            conn->on_close(conn->external_conn);
        }
        uvhttp_free(conn);
    }
}
```

---

## 与 WebSocket 升级的对比

### 相似之处

| 特性 | WebSocket | 通用协议升级 |
|------|-----------|-------------|
| 协议检测 | 检查特定头部 | 检测器函数 |
| 升级响应 | 101 Switching Protocols | 101 Switching Protocols |
| 连接转移 | 内部转移 | 支持外部转移 |
| 状态管理 | `is_websocket` 标志 | `PROTOCOL_UPGRADED` 状态 |

### 主要区别

| 特性 | WebSocket | 通用协议升级 |
|------|-----------|-------------|
| 实现方式 | 硬编码 | 动态注册 |
| 协议数量 | 单一 | 多个 |
| 扩展性 | 低 | 高 |
| 控制权 | UVHTTP 内部 | 外部库 |
| 复杂度 | 简单 | 中等 |

### 迁移策略

为保持向后兼容，WebSocket 升级将继续使用硬编码实现，但可以迁移到通用框架：

```c
// 可选：将 WebSocket 迁移到通用框架
void uvhttp_server_register_websocket_upgrade(uvhttp_server_t* server) {
    uvhttp_server_register_protocol_upgrade(
        server,
        "websocket",
        "websocket",
        is_websocket_handshake_request,  // 复用现有检测器
        uvhttp_connection_handle_websocket_handshake,  // 复用现有处理器
        NULL
    );
}
```

---

## 实现细节

### 头文件设计

```c
// include/uvhttp_protocol_upgrade.h

#ifndef UVHTTP_PROTOCOL_UPGRADE_H
#define UVHTTP_PROTOCOL_UPGRADE_H

#include "uvhttp_common.h"
#include "uvhttp_connection.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 协议检测器函数类型 */
typedef int (*uvhttp_protocol_detector_t)(
    uvhttp_request_t* request,
    char* protocol_name,
    size_t protocol_name_len
);

/* 协议升级处理器函数类型 */
typedef uvhttp_error_t (*uvhttp_protocol_upgrade_handler_t)(
    uvhttp_connection_t* conn,
    const char* protocol_name,
    void* user_data
);

/* 连接所有权转移回调函数类型 */
typedef void (*uvhttp_connection_ownership_callback_t)(
    uv_tcp_t* tcp_handle,
    int fd,
    void* user_data
);

/* 连接生命周期回调 */
typedef struct {
    void* user_data;
    void (*on_close)(void* user_data);
} uvhttp_connection_lifecycle_t;

/* ========== 协议注册 API ========== */

/**
 * @brief 注册协议升级处理器
 * 
 * @param server 服务器对象
 * @param protocol_name 协议名称
 * @param upgrade_header Upgrade 头部值（可选，用于快速匹配）
 * @param detector 协议检测器
 * @param handler 升级处理器
 * @param user_data 用户数据
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_server_register_protocol_upgrade(
    uvhttp_server_t* server,
    const char* protocol_name,
    const char* upgrade_header,
    uvhttp_protocol_detector_t detector,
    uvhttp_protocol_upgrade_handler_t handler,
    void* user_data
);

/**
 * @brief 注销协议升级处理器
 * 
 * @param server 服务器对象
 * @param protocol_name 协议名称
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_server_unregister_protocol_upgrade(
    uvhttp_server_t* server,
    const char* protocol_name
);

/* ========== 连接所有权转移 API ========== */

/**
 * @brief 转移连接所有权给外部库
 * 
 * @param conn HTTP 连接对象
 * @param callback 所有权转移回调
 * @param user_data 用户数据
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_connection_transfer_ownership(
    uvhttp_connection_t* conn,
    uvhttp_connection_ownership_callback_t callback,
    void* user_data
);

/**
 * @brief 设置连接生命周期回调
 * 
 * @param conn HTTP 连接对象
 * @param lifecycle 生命周期回调结构
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_connection_set_lifecycle(
    uvhttp_connection_t* conn,
    uvhttp_connection_lifecycle_t* lifecycle
);

/* ========== 辅助函数 ========== */

/**
 * @brief 获取连接文件描述符
 * 
 * @param conn HTTP 连接对象
 * @param fd 输出参数，文件描述符
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_connection_get_fd(
    uvhttp_connection_t* conn,
    int* fd
);

/**
 * @brief 获取客户端地址
 * 
 * @param conn HTTP 连接对象
 * @param addr 输出参数，客户端地址
 * @param addr_len 输入输出参数，地址长度
 * @return uvhttp_error_t UVHTTP_OK 表示成功
 */
uvhttp_error_t uvhttp_connection_get_peer_address(
    uvhttp_connection_t* conn,
    struct sockaddr_storage* addr,
    socklen_t* addr_len
);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_PROTOCOL_UPGRADE_H */
```

### 服务器结构扩展

```c
// include/uvhttp_server.h

struct uvhttp_server {
    /* ... 现有字段 ... */
    
    /* 协议升级注册表 */
    uvhttp_protocol_registry_t* protocol_registry;
    
    /* ... 其他字段 ... */
};
```

### 连接结构扩展

```c
// include/uvhttp_connection.h

typedef enum {
    UVHTTP_CONN_STATE_NEW,
    UVHTTP_CONN_STATE_TLS_HANDSHAKE,
    UVHTTP_CONN_STATE_HTTP_READING,
    UVHTTP_CONN_STATE_HTTP_PROCESSING,
    UVHTTP_CONN_STATE_HTTP_WRITING,
    UVHTTP_CONN_STATE_PROTOCOL_UPGRADED,  // 新增
    UVHTTP_CONN_STATE_CLOSING
} uvhttp_connection_state_t;

struct uvhttp_connection {
    /* ... 现有字段 ... */
    
    /* 协议升级相关 */
    char protocol_name[32];              // 升级的协议名称
    uvhttp_connection_lifecycle_t* lifecycle;  // 生命周期回调
    
    /* ... 其他字段 ... */
};
```

---

## 最佳实践

### 1. 协议检测器设计

**原则**：
- 快速失败：尽早返回 0
- 避免复杂计算：检测器应该轻量级
- 缓存友好：避免内存分配

**示例**：

```c
// 好的设计：快速失败
int good_detector(uvhttp_request_t* request, char* name, size_t len) {
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    if (!upgrade) {
        return 0;  // 快速失败
    }
    
    if (strcasecmp(upgrade, "ipps") != 0) {
        return 0;  // 快速失败
    }
    
    strncpy(name, "ipps", len);
    return 1;
}

// 不好的设计：复杂计算
int bad_detector(uvhttp_request_t* request, char* name, size_t len) {
    // 遍历所有头部
    for (size_t i = 0; i < request->header_count; i++) {
        const char* header_name = request->headers[i].name;
        const char* header_value = request->headers[i].value;
        
        // 复杂的字符串处理
        char* lower_name = to_lower_case(header_name);
        char* lower_value = to_lower_case(header_value);
        
        // ... 更多复杂逻辑 ...
    }
    
    return 1;
}
```

### 2. 连接所有权转移

**原则**：
- 立即停止 HTTP 读取
- 清理 HTTP 相关资源
- 确保文件描述符有效

**示例**：

```c
uvhttp_error_t safe_transfer(uvhttp_connection_t* conn, void* user_data) {
    // 1. 验证连接状态
    if (conn->state != UVHTTP_CONN_STATE_HTTP_PROCESSING) {
        return UVHTTP_ERROR_INVALID_STATE;
    }
    
    // 2. 停止 HTTP 读取
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);
    
    // 3. 停止定时器
    uv_timer_stop(&conn->timeout_timer);
    
    // 4. 获取文件描述符
    int fd = 0;
    uvhttp_error_t result = uvhttp_connection_get_fd(conn, &fd);
    if (result != UVHTTP_OK) {
        return result;
    }
    
    // 5. 验证文件描述符
    if (fd < 0) {
        return UVHTTP_ERROR_INVALID_STATE;
    }
    
    // 6. 转移所有权
    // ...
    
    return UVHTTP_OK;
}
```

### 3. 资源清理

**原则**：
- 明确所有权：谁分配谁释放
- 避免双重释放：使用引用计数或标志
- 错误处理：清理失败也要释放资源

**示例**：

```c
void cleanup_resources(uvhttp_connection_t* conn, void* external_conn) {
    // 1. 清理 HTTP 资源
    if (conn->request) {
        uvhttp_request_free(conn->request);
        conn->request = NULL;
    }
    
    if (conn->response) {
        uvhttp_response_free(conn->response);
        conn->response = NULL;
    }
    
    // 2. 清理外部资源
    if (external_conn) {
        external_cleanup(external_conn);
    }
    
    // 3. 清理连接对象
    uvhttp_free(conn);
}
```

### 4. 错误处理

**原则**：
- 检查所有返回值
- 提供有意义的错误信息
- 支持错误恢复

**示例**：

```c
uvhttp_error_t robust_upgrade(uvhttp_connection_t* conn, void* user_data) {
    uvhttp_error_t result;
    
    // 1. 发送升级响应
    result = uvhttp_response_set_status(conn->response, 101);
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to set status: %s", uvhttp_error_string(result));
        return result;
    }
    
    result = uvhttp_response_set_header(conn->response, "Upgrade", "ipps");
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to set header: %s", uvhttp_error_string(result));
        return result;
    }
    
    result = uvhttp_response_send(conn->response);
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to send response: %s", uvhttp_error_string(result));
        return result;
    }
    
    // 2. 转移所有权
    result = uvhttp_connection_transfer_ownership(conn, on_transfer, user_data);
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to transfer ownership: %s", uvhttp_error_string(result));
        // 尝试关闭连接
        uvhttp_connection_close(conn);
        return result;
    }
    
    return UVHTTP_OK;
}
```

### 5. TLS 支持

**原则**：
- 检查 TLS 状态
- 正确处理加密数据
- 确保安全上下文传递

**示例**：

```c
uvhttp_error_t tls_aware_upgrade(uvhttp_connection_t* conn, void* user_data) {
    // 1. 检查 TLS 状态
    if (conn->tls_enabled) {
        // TLS 连接需要特殊处理
        // 获取 SSL 上下文
        mbedtls_ssl_context* ssl = (mbedtls_ssl_context*)conn->ssl;
        
        // 将 SSL 上下文传递给外部库
        // ...
    } else {
        // 非 TLS 连接
        // ...
    }
    
    return UVHTTP_OK;
}
```

---

## 性能考虑

### 协议检测开销

协议检测在每次请求完成后执行，需要确保开销最小：

| 操作 | 开销 | 优化策略 |
|------|------|----------|
| 遍历协议注册表 | O(n) | 限制协议数量，使用哈希表 |
| 调用检测器 | O(1) x n | 检测器应该快速失败 |
| 字符串比较 | O(m) | 使用快速哈希比较 |

### 优化策略

1. **协议数量限制**：建议最多注册 10 个协议
2. **快速失败**：检测器应该尽早返回 0
3. **缓存结果**：对相同路径的请求缓存检测结果
4. **避免内存分配**：检测器不应该分配内存

### 性能测试

```c
// 性能测试示例
void benchmark_protocol_detection() {
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 注册 10 个协议
    for (int i = 0; i < 10; i++) {
        uvhttp_server_register_protocol_upgrade(
            server,
            protocol_names[i],
            upgrade_headers[i],
            detectors[i],
            handlers[i],
            NULL
        );
    }
    
    // 测试协议检测性能
    uint64_t start = uv_hrtime();
    
    for (int i = 0; i < 1000000; i++) {
        // 模拟请求检测
        // ...
    }
    
    uint64_t end = uv_hrtime();
    uint64_t elapsed = end - start;
    
    printf("Protocol detection: %.2f ns per request\n", elapsed / 1000000.0);
}
```

---

## 总结

UVHTTP 协议升级框架提供了：

1. **通用性**：支持任意自定义协议
2. **可扩展性**：通过注册机制支持多个协议
3. **零开销**：不使用协议升级时无性能损失
4. **兼容性**：与现有 WebSocket 升级机制共存
5. **安全性**：确保连接所有权转移的安全性

---

**文档版本**：1.0  
**最后更新**：2026-02-03  
**维护者**：UVHTTP 开发团队