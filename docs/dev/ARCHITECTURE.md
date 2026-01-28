# UVHTTP 架构设计文档

## 概述

UVHTTP 是一个基于 libuv 的高性能 HTTP/1.1 服务器库，采用模块化设计，专注于核心功能。

## 核心架构

### 分层架构

```
┌─────────────────────────────────────────┐
│         应用层 (Application)            │
├─────────────────────────────────────────┤
│         API 层 (API Layer)              │
│  uvhttp_server, uvhttp_router, etc.     │
├─────────────────────────────────────────┤
│      业务逻辑层 (Business Logic)        │
│  请求处理, 路由, 中间件, 静态文件       │
├─────────────────────────────────────────┤
│      核心服务层 (Core Services)         │
│  连接管理, 错误处理, 内存管理           │
├─────────────────────────────────────────┤
│      网络层 (Network Layer)             │
│         libuv (异步 I/O)                │
└─────────────────────────────────────────┘
```

### 模块依赖关系

```
uvhttp_server
    ├── uvhttp_router
    ├── uvhttp_connection
    ├── uvhttp_request
    ├── uvhttp_response
    └── uvhttp_context

uvhttp_connection
    ├── uvhttp_request
    ├── uvhttp_response
    └── libuv

uvhttp_static
    ├── uvhttp_response
    └── uvhttp_lru_cache

uvhttp_websocket
    ├── uvhttp_connection
    └── uvhttp_response
```

## 核心模块

### 1. 服务器模块 (uvhttp_server)

**职责**:
- 服务器生命周期管理
- 连接接受和管理
- 请求分发

**关键数据结构**:
```c
typedef struct uvhttp_server {
    uv_loop_t* loop;
    uv_tcp_t tcp_handle;
    uvhttp_router_t* router;
    uvhttp_context_t* context;
    // ... 其他字段
} uvhttp_server_t;
```

**核心流程**:
1. 创建服务器对象
2. 绑定地址和端口
3. 监听连接
4. 接受新连接
5. 分发请求到路由

### 2. 路由模块 (uvhttp_router)

**职责**:
- URL 路径匹配
- HTTP 方法路由
- 中间件链管理

**路由匹配算法**:
- 前缀匹配（O(1)）
- 支持通配符
- 支持参数提取

### 3. 连接模块 (uvhttp_connection)

**职责**:
- 连接生命周期管理
- 读写缓冲区管理
- HTTP 解析

**状态机**:
```
NEW → TLS_HANDSHAKE → HTTP_READING → HTTP_PROCESSING → HTTP_WRITING → CLOSING
```

**内存布局优化**:
- 热路径字段在前（频繁访问）
- 指针字段 8 字节对齐
- 大块缓冲区在结构体末尾

### 4. 请求/响应模块 (uvhttp_request/uvhttp_response)

**职责**:
- HTTP 请求/响应解析
- 头部管理
- 主体处理

**零拷贝优化**:
- 使用 libuv 缓冲区
- 避免数据复制
- sendfile 支持

## 内存管理

### 分配器设计原则

UVHTTP 采用**编译期优化**的内存管理策略，通过编译宏选择分配器，实现零运行时开销。

### 分配器类型

#### 1. 系统分配器（默认）

```c
static inline void* uvhttp_alloc(size_t size) {
    return malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    free(ptr);
}
```

**特点**:
- 稳定可靠，无额外依赖
- 零抽象开销
- 适合大多数场景

#### 2. mimalloc 分配器

```c
static inline void* uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    mi_free(ptr);
}
```

**特点**:
- 高性能现代分配器
- 内置小对象优化
- 更好的多线程扩展性
- 降低内存碎片

### 编译配置

通过 CMake 编译宏选择分配器：

```cmake
# 系统分配器（默认）
cmake -DUVHTTP_ALLOCATOR_TYPE=0 ..

# mimalloc 分配器
cmake -DUVHTTP_ALLOCATOR_TYPE=1 ..
```

### 使用方式

```c
// 分配内存
void* ptr = uvhttp_alloc(size);

// 重新分配
ptr = uvhttp_realloc(ptr, new_size);

// 释放内存
uvhttp_free(ptr);

// 分配并初始化
ptr = uvhttp_calloc(count, size);
```

### 性能特性

- **零运行时开销**: 所有函数都是内联函数
- **编译期优化**: 编译器可以完全优化
- **类型安全**: 编译期类型检查
- **可预测性**: 无动态分发

### 最佳实践

1. **统一使用**: 始终使用 `uvhttp_alloc/uvhttp_free`，不要混用 `malloc/free`
2. **成对分配**: 每个分配都有对应的释放
3. **检查返回值**: 检查分配是否成功
4. **避免泄漏**: 确保所有路径都释放内存

## 错误处理

### 错误码设计

```c
typedef enum {
    UVHTTP_OK = 0,                           /* 成功 */
    UVHTTP_ERROR_INVALID_PARAM = -1,        /* 无效参数 */
    UVHTTP_ERROR_OUT_OF_MEMORY = -2,       /* 内存不足 */
    UVHTTP_ERROR_IO = -3,                   /* I/O 错误 */
    // ... 更多错误码
} uvhttp_error_t;
```

### 错误处理原则

1. **检查所有可能失败的函数调用**
2. **使用统一的错误类型**
3. **提供有意义的错误信息**
4. **支持错误恢复**

## 性能优化

### 1. 零拷贝优化

- 使用 libuv 缓冲区
- sendfile 支持
- 避免数据复制

### 2. 缓存策略

- LRU 缓存静态文件
- 路由缓存
- 连接复用

### 3. 内存优化

- 内联函数
- 编译期优化
- 内存池（可选）

### 4. I/O 优化

- 异步非阻塞 I/O
- 批量操作
- 零拷贝传输

## 安全特性

### 1. 输入验证

- 参数检查
- 边界检查
- 类型验证

### 2. 内存安全

- 边界检查
- 双重释放检测
- 使用后释放保护

### 3. 网络安全

- TLS 支持（mbedtls）
- 配置验证
- 错误处理

## 扩展性

### 1. 中间件系统

- 请求前处理
- 请求后处理
- 自定义中间件

### 2. 插件系统

- 自定义路由
- 自定义处理器
- 自定义分配器

### 3. 配置系统

- 运行时配置
- 编译时配置
- 环境变量配置

## 测试策略

### 1. 单元测试

- 测试单个函数
- 测试边界条件
- 测试错误处理

### 2. 集成测试

- 测试模块交互
- 测试端到端流程
- 测试性能

### 3. 性能测试

- 基准测试
- 压力测试
- 内存分析

## 文档结构

```
docs/
├── api/                    # API 文档
│   └── API_REFERENCE.md
├── dev/                    # 开发者文档
│   ├── ARCHITECTURE.md     # 架构设计（本文档）
│   ├── DEVELOPER_GUIDE.md  # 开发指南
│   └── ROADMAP.md          # 路线图
└── guide/                  # 用户指南
    ├── TUTORIAL.md         # 教程
    └── DEVELOPER_GUIDE.md  # 开发指南
```

## 版本历史

- **v2.0.0**: 重构架构，移除抽象层
- **v1.4.0**: 添加 WebSocket 支持
- **v1.3.0**: 添加 TLS 支持
- **v1.2.0**: 添加静态文件服务
- **v1.1.0**: 添加路由功能
- **v1.0.0**: 初始版本

## 参考资料

- [libuv 文档](https://docs.libuv.org/)
- [HTTP/1.1 规范](https://tools.ietf.org/html/rfc7230)
- [WebSocket 规范](https://tools.ietf.org/html/rfc6455)