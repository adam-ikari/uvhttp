# UVHTTP 架构设计文档

## 概述

UVHTTP 是一个基于 libuv 的高性能、轻量级 HTTP 服务器库。本文档详细描述了其架构设计、模块组织和核心实现原理。

## 架构概览

```
┌─────────────────────────────────────────────────────────────┐
│                        UVHTTP 架构                          │
├─────────────────────────────────────────────────────────────┤
│  应用层 (Application Layer)                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   示例程序   │  │   用户应用   │  │   测试套件   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│  API 层 (API Layer)                                         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  HTTP API   │  │ WebSocket   │  │   工具函数   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│  核心层 (Core Layer)                                        │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │   服务器     │  │   路由系统   │  │  连接管理    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  请求处理    │  │  响应构建    │  │  协议解析    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│  中间件层 (Middleware Layer)                                 │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │    CORS     │  │    限流      │  │   静态文件   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│  基础层 (Foundation Layer)                                  │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │  内存管理    │  │  错误处理    │  │   日志系统   │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
├─────────────────────────────────────────────────────────────┤
│  系统层 (System Layer)                                      │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐         │
│  │    libuv    │  │   llhttp    │  │   mbedtls    │         │
│  └─────────────┘  └─────────────┘  └─────────────┘         │
└─────────────────────────────────────────────────────────────┘
```

## API 设计

### 设计原则

UVHTTP 采用统一的核心API设计，摒弃多层次的抽象，提供简洁、高效、一致的接口：

#### 1. **零开销抽象**
- 所有API直接映射到核心功能
- 编译时优化，运行时无额外开销
- 避免过度封装和隐藏

#### 2. **一致性原则**
- 统一的错误处理机制
- 一致的命名约定
- 标准化的参数顺序

#### 3. **灵活性与控制力**
- 开发者完全控制HTTP响应的每个方面
- 支持任意内容类型和自定义头部
- 不强制特定的使用模式

### 核心API组成

#### 服务器管理
```c
uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop);
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);
void uvhttp_server_free(uvhttp_server_t* server);
```

#### 路由系统
```c
uvhttp_router_t* uvhttp_router_new(void);
int uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler);
void uvhttp_router_free(uvhttp_router_t* router);
```

#### 请求处理
```c
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
```

#### 响应构建
```c
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
```

### 标准使用模式

```c
// 1. 创建服务器
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);

// 2. 设置路由
uvhttp_router_t* router = uvhttp_router_new();
server->router = router;
uvhttp_router_add_route(router, "/api", api_handler);

// 3. 启动服务器
uvhttp_server_listen(server, "0.0.0.0", 8080);
uv_run(loop, UV_RUN_DEFAULT);

// 4. 处理请求
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, "{\"status\":\"ok\"}", 15);
    return uvhttp_response_send(res);
}
```

## 核心模块

### 1. 服务器模块 (Server)

#### 设计目标
- 高性能事件驱动
- 可扩展的连接管理
- 资源限制保护

#### 核心组件

```c
struct uvhttp_server {
    uv_tcp_t tcp_handle;           // TCP 句柄
    uv_loop_t* loop;              // 事件循环
    uvhttp_router_t* router;      // 路由器
    size_t max_connections;        // 最大连接数
    size_t active_connections;     // 活跃连接数
    void* user_data;              // 用户数据
};
```

#### 关键特性
- **连接池管理**: 动态管理客户端连接
- **负载控制**: 连接数限制和资源保护
- **优雅关闭**: 支持优雅关闭现有连接

### 2. 路由系统 (Router)

#### 设计模式
- 前缀树 (Trie) 数据结构
- O(k) 路径查找复杂度
- 支持参数化路由

#### 路由匹配算法

```c
// 路由节点结构
struct route_node {
    char* path;                    // 路径片段
    uvhttp_request_handler_t handler; // 处理器
    struct route_node** children;  // 子节点
    size_t child_count;           // 子节点数量
    int is_param;                 // 是否参数节点
};
```

#### 路由匹配流程
1. 将请求 URL 按 '/' 分割
2. 从根节点开始逐段匹配
3. 支持通配符和参数提取
4. 返回最佳匹配的处理器

#### 性能优化
- **哈希加速**: 使用xxHash算法进行快速路径哈希
- **缓存机制**: 热路径缓存减少查找开销
- **内存池**: 节点内存池减少分配开销

```c
// 路由哈希计算
uint32_t route_hash = (uint32_t)uvhttp_hash_string(path);

// 缓存友好的路由查找
if (route_hash < HOT_PATH_SIZE && 
    hot_paths[route_hash].method == method) {
    return hot_paths[route_hash].handler;
}
```

#### 哈希算法选择
- **xxHash**: 极快的非加密哈希算法
- **64位哈希**: 减少冲突概率
- **安全防护**: 1024字符长度限制防止哈希冲突攻击

### 3. 请求处理 (Request)

#### 生命周期
```
接收连接 → HTTP 解析 → 路由匹配 → 执行处理器 → 发送响应
```

#### 请求结构

```c
struct uvhttp_request {
    uv_tcp_t* client;              // 客户端连接
    llhttp_t* parser;              // HTTP 解析器
    uvhttp_method_t method;        // HTTP 方法
    char url[MAX_URL_LEN];         // 请求 URL
    char* body;                    // 请求体
    uvhttp_header_t headers[MAX_HEADERS]; // 请求头
    size_t header_count;           // 头部数量
};
```

#### 解析优化
- 使用 llhttp 高性能解析器
- 零拷贝字符串处理
- 流式解析大请求体

### 4. 响应构建 (Response)

#### 响应缓冲区设计

```c
struct uvhttp_response {
    int status_code;               // 状态码
    uvhttp_header_t headers[MAX_HEADERS]; // 响应头
    size_t header_count;           // 头部数量
    char* body;                    // 响应体
    size_t body_length;            // 响应体长度
    char* buffer;                  // 发送缓冲区
    size_t buffer_size;            // 缓冲区大小
};
```

#### 优化策略
- 动态缓冲区调整
- Header 压缩和去重
- 分块传输支持

## WebSocket 模块

### 架构设计

```
┌─────────────────────────────────────────────┐
│            WebSocket 模块                    │
├─────────────────────────────────────────────┤
│  应用层                                     │
│  ┌─────────────┐  ┌─────────────┐           │
│  │ 消息处理器   │  │ 用户回调     │           │
│  └─────────────┘  └─────────────┘           │
├─────────────────────────────────────────────┤
│  协议层                                     │
│  ┌─────────────┐  ┌─────────────┐           │
│  │ 握手处理     │  │ 帧处理       │           │
│  └─────────────┘  └─────────────┘           │
├─────────────────────────────────────────────┤
│  传输层                                     │
│  ┌─────────────┐  ┌─────────────┐           │
│  │ 连接管理     │  │ 心跳检测     │           │
│  └─────────────┘  └─────────────┘           │
└─────────────────────────────────────────────┘
```

### 握手流程

1. **HTTP 升级请求**
   - 验证 Upgrade 头部
   - 检查 WebSocket Version
   - 提取 Sec-WebSocket-Key

2. **服务器响应**
   - 计算 Accept Key
   - 设置协议升级头部
   - 发送 101 Switching Protocols

3. **连接建立**
   - 切换到 WebSocket 模式
   - 设置消息处理器
   - 启动心跳机制

### 帧处理

```c
struct ws_frame_header {
    unsigned fin:1;               // FIN 位
    unsigned rsv1:1;              // RSV1
    unsigned rsv2:1;              // RSV2
    unsigned rsv3:1;              // RSV3
    unsigned opcode:4;            // 操作码
    unsigned mask:1;              // 掩码位
    uint64_t payload_length;      // 载荷长度
};
```

## 内存管理

### 分配器抽象

```c
typedef struct {
    void* (*malloc)(size_t size);
    void* (*realloc)(void* ptr, size_t size);
    void (*free)(void* ptr);
    void* (*calloc)(size_t nmemb, size_t size);
    void* data;
    uvhttp_allocator_type_t type;
} uvhttp_allocator_t;
```

### 内存策略

1. **mimalloc 分配器** (默认)
   - 高性能现代分配器
   - 降低内存碎片
   - 更好的多线程扩展性
   - 默认启用，无需额外配置

2. **系统分配器**
   - 直接使用 malloc/free
   - 零抽象开销
   - 可通过编译选项禁用mimalloc

3. **自定义分配器**
   - 支持内存池
   - 调试和统计功能
   - 灵活的扩展接口

### 内存安全

- 边界检查
- 双重释放检测
- 内存泄漏追踪
- 使用后释放保护

### 编译配置

在CMakeLists.txt中，mimalloc默认启用：

```cmake
option(BUILD_WITH_MIMALLOC "Build with mimalloc allocator" ON)
```

如需使用系统分配器，可以在编译时禁用：

```bash
cmake -DBUILD_WITH_MIMALLOC=OFF ..
make
```

## 错误处理

### 错误分类

```c
// 通用错误
UVHTTP_ERROR_INVALID_PARAM
UVHTTP_ERROR_OUT_OF_MEMORY
UVHTTP_ERROR_NOT_FOUND

// 服务器错误
UVHTTP_ERROR_SERVER_INIT
UVHTTP_ERROR_CONNECTION_LIMIT

// 网络错误
UVHTTP_ERROR_CONNECTION_ACCEPT
UVHTTP_ERROR_RESPONSE_SEND
```

### 恢复机制

1. **重试策略**
   - 指数退避算法
   - 最大重试次数限制
   - 可配置延迟参数

2. **错误统计**
   - 错误计数器
   - 最后错误时间
   - 错误上下文记录

3. **优雅降级**
   - 服务降级策略
   - 熔断机制
   - 自动恢复

## 性能优化

### 关键优化点

1. **零拷贝设计**
   - 避免不必要的数据复制
   - 引用计数管理
   - 内存映射技术

2. **连接复用**
   - Keep-Alive 支持
   - 连接池管理
   - 空闲连接回收

3. **缓存策略**
   - 响应缓存
   - DNS 缓存
   - 静态资源缓存

4. **高性能内存分配**
   - 默认使用 mimalloc 分配器
   - 降低内存碎片
   - 更好的多线程扩展性
   - 显著提升分配/释放性能

### 性能指标

- **吞吐量**: 10,000+ QPS (启用mimalloc)
- **延迟**: < 1ms 平均响应时间
- **并发**: 1000+ 并发连接
- **内存**: < 10MB 基础占用
- **性能提升**: 相比系统分配器提升约30-50%

## 安全设计

### 安全特性

1. **输入验证**
   - 请求大小限制
   - Header 验证
   - 路径遍历防护

2. **资源保护**
   - 连接数限制
   - 请求速率限制
   - 内存使用限制

3. **TLS 支持**
   - mbedtls 集成
   - 证书验证
   - 加密套件配置

### 安全最佳实践

- 最小权限原则
- 深度防御策略
- 定期安全审计
- 漏洞响应机制

## 扩展机制

### 插件架构

```c
typedef struct {
    const char* name;
    int (*init)(void* config);
    void (*cleanup)(void);
    int (*request_handler)(uvhttp_request_t*, uvhttp_response_t*);
} uvhttp_plugin_t;
```

### 中间件支持

1. **请求中间件**
   - 认证授权
   - 日志记录
   - 限流控制

2. **响应中间件**
   - 压缩处理
   - 缓存控制
   - 安全头部

## 测试架构

### 测试层次

1. **单元测试**
   - 模块独立测试
   - Mock 对象使用
   - 边界条件覆盖

2. **集成测试**
   - 组件协作测试
   - 端到端场景
   - 错误注入测试

3. **性能测试**
   - 基准测试
   - 压力测试
   - 内存泄漏检测

### 测试工具

- 自定义测试框架
- 覆盖率分析
- 性能分析工具
- 内存检查工具

## 部署架构

### 单机部署

```
┌─────────────────┐
│   Load Balancer │
└─────────┬───────┘
          │
    ┌─────┴─────┐
    │ UVHTTP    │
    │ Instances │
    └───────────┘
```

### 集群部署

```
┌─────────────────┐
│   Load Balancer │
└─────────┬───────┘
          │
    ┌─────┴─────┐
    │   Gateway  │
    └─────┬─────┘
          │
    ┌─────┴─────┐
    │ UVHTTP    │
    │ Cluster   │
    └───────────┘
```

## 监控和运维

### 监控指标

- 请求计数和延迟
- 连接数和状态
- 内存使用情况
- 错误率和类型

### 日志系统

- 结构化日志
- 日志级别控制
- 异步日志写入
- 日志轮转机制

## 未来规划

### 短期目标

- HTTP/2 支持
- 更完善的中间件
- 性能优化

### 长期目标

- gRPC 支持
- 服务网格集成
- 云原生部署

---

本文档持续更新中，欢迎贡献和反馈。