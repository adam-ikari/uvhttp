# UVHTTP 架构设计文档

## 概述

UVHTTP 是一个基于 libuv 的 HTTP/1.1 服务器库。网络事件交给 libuv，HTTP 协议、路由、连接和内存管理由库自己分层处理。

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

服务器模块负责三件事：管理服务器自身的生命周期，接受并管理连接，把请求分发出去。

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

启动流程分五步：
1. 创建服务器对象
2. 绑定地址和端口
3. 开始监听
4. 接受新连接
5. 把请求交给路由

### 2. 路由模块 (uvhttp_router)

路由模块负责 URL 路径匹配、按 HTTP 方法分流，以及管理中间件链。

匹配以前缀匹配为主，查找复杂度是 O(1)。路由支持通配符，也能从路径里提取参数。

### 3. 连接模块 (uvhttp_connection)

连接模块管连接的生命周期、读写缓冲区，以及 HTTP 解析。连接在状态机里按下面的顺序流转：
```
NEW → TLS_HANDSHAKE → HTTP_READING → HTTP_PROCESSING → HTTP_WRITING → CLOSING
```

连接结构体的字段按访问频率排布。热路径字段放在最前面，指针字段保持 8 字节对齐，大块缓冲区挪到结构体末尾，免得把常用字段挤出缓存行。

### 4. 请求/响应模块 (uvhttp_request/uvhttp_response)

请求和响应模块负责解析报文、管理头部、处理主体。数据尽量走 libuv 缓冲区，不做多余复制；发送文件时交给 sendfile。

## 内存管理

### 分配器设计原则

分配器在编译期选定，运行时不再判断该调哪个函数。uvhttp_alloc 这类接口写成内联函数，编译器直接把底层调用替换进来，不留下额外的函数调用开销。

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

系统分配器就是 malloc 和 free，没有额外依赖，行为稳定，抽象层也被内联掉了。大多数场景用它就够。

#### 2. mimalloc 分配器

```c
static inline void* uvhttp_alloc(size_t size) {
    return mi_malloc(size);
}

static inline void uvhttp_free(void* ptr) {
    mi_free(ptr);
}
```

mimalloc 是小对象分配做得更细的现代分配器，多线程扩展性更好，长期运行时的内存碎片也更少。代价是构建时多一个依赖。

### 编译配置

用 CMake 变量 UVHTTP_ALLOCATOR_TYPE 选择分配器：默认走系统分配器，改成 mimalloc 时启用它，另一个取值留给调用方自己提供的分配器实现。

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

这些函数都声明为内联，调用点在编译后直接落到分配器实现上，没有运行时分支，也没有动态分发。类型检查在编译期完成，编译器还能顺着内联看到更多上下文做优化。

### 最佳实践

1. 始终使用 `uvhttp_alloc/uvhttp_free`，不要和 `malloc/free` 混用。分配器一旦换掉，混用的那部分会落到另一套实现上。
2. 每个分配都要有对应的释放。
3. 分配可能失败，返回值要检查。
4. 每条提前返回的路径也要释放内存，泄漏通常就出在这种分支上。

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

处理错误有四条约定：
1. 所有可能失败的调用都要检查返回值。
2. 错误统一用 uvhttp_error_t 表达。
3. 错误信息要能说明是哪个参数或哪一步出的问题。
4. 调用方要能从错误里恢复，而不是只能退出。

## 性能优化

### 1. 零拷贝优化

收发的数据尽量留在 libuv 缓冲区里，不做中间拷贝。发送文件走 sendfile，数据不必先搬进用户态。

### 2. 缓存策略

静态文件用 LRU 缓存，路由表也有缓存，连接本身通过 Keep-Alive 复用。这三处缓存对付的是不同层面的重复工作：磁盘读取、路由匹配和 TCP 握手。

### 3. 内存优化

内联函数和编译期优化去掉了抽象开销，需要时还可以挂内存池。

### 4. I/O 优化

I/O 全部异步非阻塞。能在一次事件循环里处理完的请求就批量处理，传输过程尽量零拷贝。

## 安全特性

### 1. 输入验证

进入库的参数先检查类型和取值范围，字符串操作检查边界。解析外部报文时这些检查尤其重要，长度字段不能直接信。

### 2. 内存安全

缓冲区操作做边界检查，同时检测双重释放和使用后释放。开启 sanitizer 构建可以更早暴露这类问题。

### 3. 网络安全

TLS 由 mbedtls 提供。配置在启动时校验，握手和读写错误都有明确返回值，不会静默吞掉。

## 扩展性

### 1. 中间件系统

中间件可以挂在请求处理前后，也可以自己写。鉴权、日志、CORS 这类横切逻辑适合放在这里，不必塞进每个 handler。

### 2. 插件系统

路由、处理器和分配器都允许替换成自己的实现。

### 3. 配置系统

配置可以在运行时改，也可以在编译时定，部分选项还能从环境变量读。

## 测试策略

### 1. 单元测试

单元测试覆盖单个函数，重点在边界条件和错误分支。正常路径容易写，真正会漏的是参数为空、长度为零、缓冲区刚好放满这些情况。

### 2. 集成测试

集成测试把模块拼起来跑完整请求，验证的是模块之间的约定，而不是某个函数内部。

### 3. 性能测试

性能测试分基准、压力和内存分析三类。基准看单点数字，压力看长时间和高并发下的稳定性，内存分析找泄漏和碎片。

## 文档结构

```
docs/
├── api/                    # 生成的 API 文档 + 手写 API 参考
│   ├── API_REFERENCE.md
│   └── generated/
├── dev/                    # 贡献者文档
│   ├── index.md            # 开发者文档入口
│   ├── ARCHITECTURE.md     # 架构设计（本文档）
│   ├── BUILD_MODES.md
│   ├── CI_CD.md
│   ├── DEPENDENCIES.md
│   └── TESTING_STANDARDS.md
├── guide/                  # 用户与贡献者指南
│   ├── DEVELOPER_GUIDE.md
│   ├── TESTING_GUIDE.md
│   └── TUTORIAL.md
└── zh/                     # 与英文相同结构的中文镜像
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