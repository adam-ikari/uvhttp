# 架构设计

## 概述

UVHTTP 采用模块化设计，基于 libuv 事件驱动架构，提供高性能的 HTTP/1.1 和 WebSocket 服务。

## 核心架构

```
┌─────────────────────────────────────────────────┐
│              Application Layer                  │
│  (User Handlers, Middleware, Business Logic)    │
└─────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────┐
│              HTTP/WS Protocol Layer             │
│  (Request Parsing, Response Generation)         │
└─────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────┐
│              Router & Middleware Layer          │
│  (Route Matching, Middleware Chain)             │
└─────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────┐
│              Connection Management              │
│  (Connection Pool, Keep-Alive)                  │
└─────────────────────────────────────────────────┘
                         ↓
┌─────────────────────────────────────────────────┐
│              libuv Event Loop                   │
│  (I/O Multiplexing, Async Operations)           │
└─────────────────────────────────────────────────┘
```

## 核心组件

### 1. 服务器 (Server)

服务器是 UVHTTP 的核心组件，负责：

- 接受客户端连接
- 管理连接池
- 调度请求处理
- 生命周期管理

### 2. 路由 (Router)

路由负责：

- URL 路径匹配
- HTTP 方法匹配
- 参数提取
- 中间件链管理

### 3. 请求 (Request)

请求对象包含：

- HTTP 方法
- URL 路径
- 请求头
- 请求体
- 查询参数

### 4. 响应 (Response)

响应对象提供：

- 状态码设置
- 响应头设置
- 响应体设置
- 自动发送

### 5. 连接 (Connection)

连接管理：

- Keep-Alive 支持
- 连接池管理
- 超时处理
- 错误恢复

## 数据流

### 请求处理流程

```
Client Request
    ↓
TCP Connection (libuv)
    ↓
Connection Manager
    ↓
HTTP Parser (llhttp)
    ↓
Router
    ↓
Middleware Chain
    ↓
Handler
    ↓
Response Builder
    ↓
TCP Send (libuv)
    ↓
Client Response
```

### 响应处理流程

```
Handler
    ↓
Response Builder
    ↓
Middleware Chain (post-processing)
    ↓
Response Serializer
    ↓
TCP Send (libuv)
    ↓
Client
```

## 性能优化

### 1. 零拷贝

- 使用 sendfile 进行大文件传输
- 避免不必要的数据复制
- 使用内存池减少分配开销

### 2. 缓存机制

- LRU 缓存静态文件
- 路由缓存
- 连接复用

### 3. 事件驱动

- 基于 libuv 的异步 I/O
- 非阻塞操作
- 高并发处理

### 4. 内存管理

- 使用 mimalloc 分配器
- 内存池管理
- 避免内存碎片

## 模块化设计

### 可选模块

通过编译宏控制：

```c
UVHTTP_FEATURE_WEBSOCKET      // WebSocket 支持
UVHTTP_FEATURE_STATIC_FILES   // 静态文件服务
UVHTTP_FEATURE_TLS            // TLS/SSL 支持
UVHTTP_FEATURE_RATE_LIMIT     // 限流功能
UVHTTP_FEATURE_LOGGING        // 日志系统
UVHTTP_FEATURE_LRU_CACHE      // LRU 缓存
UVHTTP_FEATURE_ROUTER_CACHE   // 路由缓存
```

### 中间件系统

- 日志中间件
- CORS 中间件
- 限流中间件
- 自定义中间件

## 错误处理

### 错误类型

- 系统错误
- 网络错误
- 协议错误
- 应用错误

### 错误处理策略

- 统一错误码
- 错误上下文
- 错误恢复
- 错误日志

## 安全性

### 安全特性

- 缓冲区溢出保护
- 输入验证
- SQL 注入防护
- XSS 防护
- CSRF 防护
- TLS 1.3 支持

### 最佳实践

- 最小权限原则
- 安全编码规范
- 定期安全审计
- 漏洞扫描

## 扩展性

### 扩展点

- 自定义处理器
- 自定义中间件
- 自定义协议
- 插件系统

### 集成能力

- 数据库集成
- 缓存集成
- 消息队列集成
- 监控集成

## 监控和调试

### 性能监控

- 请求计数
- 响应时间
- 错误率
- 资源使用

### 调试工具

- 日志系统
- 性能分析
- 内存分析
- 网络抓包