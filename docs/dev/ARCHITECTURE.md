# UVHTTP 架构设计文档

> 版本：2.2.0  
> 更新日期：2026-02-01  
> 状态：正式发布

## 目录

1. [概述](#概述)
2. [整体架构](#整体架构)
3. [核心模块](#核心模块)
4. [路由缓存优化](#路由缓存优化)
5. [性能优化](#性能优化)
6. [内存管理](#内存管理)
7. [错误处理](#错误处理)
8. [线程模型](#线程模型)
9. [依赖管理](#依赖管理)
10. [扩展性设计](#扩展性设计)

---

## 概述

UVHTTP 是一个基于 libuv 的高性能、轻量级 HTTP/1.1 和 WebSocket 服务器库，采用 C11 标准编写。

### 设计原则

1. **专注核心**：只实现 HTTP 协议处理，不内置业务逻辑
2. **零开销**：生产环境无任何抽象层成本
3. **极简工程**：移除所有不必要的复杂度
4. **测试分离**：测试代码与生产代码完全分离
5. **零全局变量**：支持多实例和单元测试
6. **生产就绪**：完整的错误处理和资源管理

### 核心特性

- **高性能**：峰值吞吐量 31,883 RPS（Debug 模式）
- **零拷贝**：sendfile 大文件传输，性能提升 50%+
- **智能缓存**：LRU 缓存 + 缓存预热机制
- **路由缓存**：分层缓存策略，利用 CPU 缓存局部性
- **安全**：缓冲区溢出保护、输入验证、TLS 1.3 支持
- **模块化**：通过编译宏控制功能模块

---

## 整体架构

### 架构图

```
┌─────────────────────────────────────────────────────────────┐
│                      应用层（Application）                    │
│  用户代码、业务逻辑、中间件、路由处理器、WebSocket 处理器     │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      UVHTTP 核心层（Core）                     │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │  Server  │  │  Router  │  │  Request │  │ Response │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │Connection│  │  Static  │  │ WebSocket│  │   TLS    │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │  Config  │  │  Context │  │  Error   │  │  Utils   │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                    依赖层（Dependencies）                       │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐  ┌─────────┐       │
│  │  libuv  │  │ llhttp  │  │ mbedtls │  │ mimalloc │       │
│  └─────────┘  └─────────┘  └─────────┘  └─────────┘       │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐                     │
│  │  xxhash │  │  uthash │  │  cjson  │                     │
│  └─────────┘  └─────────┘  └─────────┘                     │
└─────────────────────────────────────────────────────────────┘
                              ↓
┌─────────────────────────────────────────────────────────────┐
│                      操作系统层（OS）                          │
│  Linux / macOS / Windows（计划中）                          │
└─────────────────────────────────────────────────────────────┘
```

### 模块依赖关系

```
uvhttp_server
    ├── uvhttp_router
    │   ├── uvhttp_router_cache (路由缓存)
    │   └── uvhttp_hash
    ├── uvhttp_connection
    │   ├── uvhttp_request
    │   ├── uvhttp_response
    │   ├── uvhttp_tls
    │   └── uvhttp_websocket
    ├── uvhttp_static
    │   └── uvhttp_lru_cache
    ├── uvhttp_config
    ├── uvhttp_context
    └── uvhttp_error
```

---

## 核心模块

### 1. 服务器模块（uvhttp_server）

**职责**：
- 服务器生命周期管理（创建、启动、停止）
- 连接管理（接受新连接、维护连接池）
- 路由管理（路由注册、匹配）
- 统计信息（请求数、活动连接数）

**关键函数**：
```c
uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server);
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port);
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
```

**性能优化**：
- 使用 libuv 事件循环，零阻塞
- 连接池复用，减少 TCP 握手开销
- 统计信息使用原子操作，避免锁

### 2. 路由模块（uvhttp_router）

**职责**：
- 路由注册和管理
- 路由匹配（支持通配符、参数）
- 路由缓存优化

**关键函数**：
```c
uvhttp_router_t* uvhttp_router_new(void);
void uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_route_handler_t handler);
uvhttp_route_handler_t uvhttp_router_match(uvhttp_router_t* router, const char* path);
```

**路由缓存优化**：
- 分层缓存策略（热路径 + 哈希表）
- 利用 CPU 缓存局部性
- O(1) 快速前缀匹配

### 3. 连接模块（uvhttp_connection）

**职责**：
- 连接生命周期管理
- HTTP 请求解析
- TLS 握手
- WebSocket 升级

**关键函数**：
```c
uvhttp_error_t uvhttp_connection_new(uvhttp_server_t* server, uvhttp_connection_t** conn);
uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t* conn);
void uvhttp_connection_close(uvhttp_connection_t* conn);
```

**内存布局优化**：
- 缓存行对齐（64 字节）
- 热路径字段放在缓存行开头
- 大块缓冲区放在结构体末尾

### 4. 请求模块（uvhttp_request）

**职责**：
- HTTP 请求解析
- 请求头管理
- 查询参数解析
- 请求体处理

**关键函数**：
```c
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_path(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name);
```

**性能优化**：
- 使用 llhttp 解析器，高性能 HTTP 解析
- 内联头部存储（避免动态分配）
- 零拷贝字符串处理

### 5. 响应模块（uvhttp_response）

**职责**：
- HTTP 响应构建
- 响应头管理
- 响应体发送
- 分块传输编码

**关键函数**：
```c
void uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
void uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t len);
void uvhttp_response_send(uvhttp_response_t* response);
```

**性能优化**：
- 零拷贝大文件传输（sendfile）
- 分块传输编码支持
- 响应缓存

### 6. 静态文件模块（uvhttp_static）

**职责**：
- 静态文件服务
- 文件类型检测
- 缓存控制
- 零拷贝传输

**关键函数**：
```c
uvhttp_static_context_t* uvhttp_static_create(const char* root_dir);
void uvhttp_static_handle_request(uvhttp_static_context_t* ctx, uvhttp_request_t* request, uvhttp_response_t* response);
```

**性能优化**：
- LRU 缓存 + 缓存预热
- sendfile 零拷贝传输
- 文件类型检测优化

---

## 路由缓存优化

### 设计目标

- **减少路由匹配时间**：从 O(n) 优化到 O(1)
- **利用 CPU 缓存局部性**：减少缓存未命中
- **支持动态路由**：支持参数化路由和通配符

### 架构设计

#### 分层缓存策略

```
┌─────────────────────────────────────────────────────────────┐
│                    热路径缓存（Hot Path Cache）                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Slot 0   │  │ Slot 1   │  │ Slot 2   │  │ Slot 3   │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Slot 4   │  │ Slot 5   │  │ Slot 6   │  │ Slot 7   │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
                              ↓ 命中率 > 90%
┌─────────────────────────────────────────────────────────────┐
│                    哈希表缓存（Hash Table Cache）              │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Bucket 0 │  │ Bucket 1 │  │ Bucket 2 │  │ Bucket 3 │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │ Bucket 4 │  │ Bucket 5 │  │ Bucket 6 │  │ Bucket 7 │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
                              ↓ 命中率 > 80%
┌─────────────────────────────────────────────────────────────┐
│                    线性回退（Linear Fallback）                │
│  顺序遍历路由表，进行完整匹配                                 │
└─────────────────────────────────────────────────────────────┘
```

### 实现细节

#### 热路径缓存（Hot Path Cache）

```c
#define HOT_PATH_CACHE_SIZE 8

typedef struct {
    const char* path;
    uvhttp_route_handler_t handler;
    uint64_t timestamp;
} hot_path_cache_entry_t;

typedef struct {
    hot_path_cache_entry_t entries[HOT_PATH_CACHE_SIZE];
    size_t index;
} hot_path_cache_t;
```

**特性**：
- 固定大小（8 个槽位）
- LRU 替换策略
- 无锁设计（单线程事件循环）

#### 哈希表缓存（Hash Table Cache）

```c
#define HASH_TABLE_SIZE 64

typedef struct {
    const char* path;
    uvhttp_route_handler_t handler;
    struct hash_table_entry* next;
} hash_table_entry_t;

typedef struct {
    hash_table_entry_t* buckets[HASH_TABLE_SIZE];
} hash_table_cache_t;
```

**特性**：
- 固定大小（64 个桶）
- 链表解决冲突
- xxHash 快速哈希

#### 路由匹配流程

```c
uvhttp_route_handler_t uvhttp_router_match_cached(uvhttp_router_t* router, const char* path) {
    // 1. 检查热路径缓存（90%+ 命中率）
    uvhttp_route_handler_t handler = hot_path_cache_lookup(&router->cache->hot_path, path);
    if (handler) {
        return handler;
    }
    
    // 2. 检查哈希表缓存（80%+ 命中率）
    handler = hash_table_cache_lookup(&router->cache->hash_table, path);
    if (handler) {
        // 提升到热路径缓存
        hot_path_cache_insert(&router->cache->hot_path, path, handler);
        return handler;
    }
    
    // 3. 线性回退：遍历路由表
    handler = uvhttp_router_match_linear(router, path);
    if (handler) {
        // 插入哈希表缓存
        hash_table_cache_insert(&router->cache->hash_table, path, handler);
        // 提升到热路径缓存
        hot_path_cache_insert(&router->cache->hot_path, path, handler);
    }
    
    return handler;
}
```

### 性能指标

- **缓存命中率**：> 95%（热路径缓存 + 哈希表缓存）
- **路由匹配时间**：< 1 μs
- **内存开销**：< 8 KB（固定大小缓存）

---

## 性能优化

### 1. 零拷贝优化

#### sendfile 大文件传输

```c
uvhttp_error_t uvhttp_static_sendfile(const char* filepath, uvhttp_response_t* response) {
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        return UVHTTP_ERROR_FILE_NOT_FOUND;
    }
    
    struct stat st;
    fstat(fd, &st);
    
    // 使用 sendfile 零拷贝传输
    uv_fs_sendfile(loop, &req, response->stream, fd, 0, st.st_size, NULL);
    
    close(fd);
    return UVHTTP_OK;
}
```

**性能提升**：50%+

### 2. 智能缓存

#### LRU 缓存

```c
typedef struct {
    char* key;
    void* value;
    size_t size;
    uint64_t timestamp;
    struct lru_entry* prev;
    struct lru_entry* next;
} lru_entry_t;

typedef struct {
    lru_entry_t* head;
    lru_entry_t* tail;
    size_t capacity;
    size_t size;
} lru_cache_t;
```

**性能提升**：30%+

#### 缓存预热

```c
void uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx, const char* filepath) {
    // 预加载文件到缓存
    char* content = read_file(filepath);
    lru_cache_insert(ctx->cache, filepath, content, strlen(content));
}
```

### 3. 内存优化

#### mimalloc 分配器

```c
// 编译期选择分配器
#define UVHTTP_ALLOCATOR_TYPE 1  // 1 = mimalloc, 0 = system allocator

#if UVHTTP_ALLOCATOR_TYPE == 1
    #include <mimalloc.h>
    #define uvhttp_alloc(size) mi_malloc(size)
    #define uvhttp_free(ptr) mi_free(ptr)
#else
    #define uvhttp_alloc(size) malloc(size)
    #define uvhttp_free(ptr) free(ptr)
#endif
```

**性能提升**：30%+

### 4. TCP 优化

```c
// TCP_NODELAY：禁用 Nagle 算法
int flag = 1;
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));

// TCP_KEEPALIVE：启用保活机制
int keepalive = 1;
setsockopt(fd, SOL_SOCKET, SO_KEEPALIVE, &keepalive, sizeof(keepalive));
```

### 5. 路由匹配优化

#### O(1) 快速前缀匹配

```c
uvhttp_route_handler_t uvhttp_router_match_fast(uvhttp_router_t* router, const char* path) {
    // 使用哈希表快速查找
    uint64_t hash = xxhash64(path, strlen(path));
    hash_table_entry_t* entry = router->hash_table[hash % HASH_TABLE_SIZE];
    
    while (entry) {
        if (strcmp(entry->path, path) == 0) {
            return entry->handler;
        }
        entry = entry->next;
    }
    
    return NULL;
}
```

### 性能指标

- **峰值吞吐量**：31,883 RPS（Debug 模式，100 并发）
- **高并发稳定性**：10-500 并发，RPS 波动仅 5%
- **最小延迟**：352 μs（低并发）

---

## 内存管理

### 统一分配器

#### 分配器接口

```c
// 内存分配
static inline void* uvhttp_alloc(size_t size) {
#if UVHTTP_ALLOCATOR_TYPE == 1
    return mi_malloc(size);
#else
    return malloc(size);
#endif
}

// 内存释放
static inline void uvhttp_free(void* ptr) {
#if UVHTTP_ALLOCATOR_TYPE == 1
    mi_free(ptr);
#else
    free(ptr);
#endif
}

// 内存重新分配
static inline void* uvhttp_realloc(void* ptr, size_t new_size) {
#if UVHTTP_ALLOCATOR_TYPE == 1
    return mi_realloc(ptr, new_size);
#else
    return realloc(ptr, new_size);
#endif
}

// 内存清零分配
static inline void* uvhttp_calloc(size_t num, size_t size) {
#if UVHTTP_ALLOCATOR_TYPE == 1
    return mi_calloc(num, size);
#else
    return calloc(num, size);
#endif
}
```

### 内存布局优化

#### 缓存行对齐

```c
struct uvhttp_connection {
    /* 缓存行1（0-63字节）：热路径字段 */
    uvhttp_connection_state_t state;  /* 4 字节 */
    int parsing_complete;              /* 4 字节 */
    int keepalive;                     /* 4 字节 */
    int chunked_encoding;              /* 4 字节 */
    int close_pending;                 /* 4 字节 */
    int need_restart_read;             /* 4 字节 */
    int tls_enabled;                   /* 4 字节 */
    int _padding1;                     /* 4 字节 */
    size_t body_received;              /* 8 字节 */
    size_t content_length;             /* 8 字节 */
    size_t read_buffer_used;           /* 8 字节 */
    size_t read_buffer_size;           /* 8 字节 */
    
    /* 缓存行2（64-127字节）：指针字段 */
    struct uvhttp_server* server;     /* 8 字节 */
    uvhttp_request_t* request;        /* 8 字节 */
    uvhttp_response_t* response;      /* 8 字节 */
    void* ssl;                        /* 8 字节 */
    char* read_buffer;                /* 8 字节 */
    /* ... */
};
```

### 内存安全

#### 输入验证

```c
uvhttp_error_t uvhttp_connection_new(uvhttp_server_t* server, uvhttp_connection_t** conn) {
    // NULL 指针检查
    if (!server || !conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 分配内存
    uvhttp_connection_t* c = uvhttp_alloc(sizeof(uvhttp_connection_t));
    if (!c) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    // 清零内存
    memset(c, 0, sizeof(uvhttp_connection_t));
    
    *conn = c;
    return UVHTTP_OK;
}
```

#### 缓冲区溢出保护

```c
void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    
    // 检查缓冲区边界
    if (conn->read_buffer_used + (size_t)nread > conn->read_buffer_size) {
        UVHTTP_LOG_ERROR("Read buffer overflow: %zu + %zd > %zu\n",
                         conn->read_buffer_used, nread, conn->read_buffer_size);
        uvhttp_connection_close(conn);
        return;
    }
    
    // 更新已使用的缓冲区大小
    conn->read_buffer_used += nread;
}
```

---

## 错误处理

### 错误码体系

#### 错误码分类

```c
// 服务器错误（-1000 到 -1099）
#define UVHTTP_ERROR_SERVER_CREATE_FAILED      -1000
#define UVHTTP_ERROR_SERVER_LISTEN_FAILED      -1001
#define UVHTTP_ERROR_SERVER_CLOSE_FAILED       -1002

// 连接错误（-2000 到 -2099）
#define UVHTTP_ERROR_CONNECTION_CREATE_FAILED -2000
#define UVHTTP_ERROR_CONNECTION_START_FAILED  -2001
#define UVHTTP_ERROR_CONNECTION_CLOSE_FAILED  -2002

// 请求错误（-3000 到 -3099）
#define UVHTTP_ERROR_REQUEST_PARSE_FAILED     -3000
#define UVHTTP_ERROR_REQUEST_TOO_LARGE        -3001

// 响应错误（-4000 到 -4099）
#define UVHTTP_ERROR_RESPONSE_SEND_FAILED     -4000
#define UVHTTP_ERROR_RESPONSE_TOO_LARGE       -4001

// 路由错误（-5000 到 -5099）
#define UVHTTP_ERROR_ROUTE_NOT_FOUND          -5000
#define UVHTTP_ERROR_ROUTE_INVALID            -5001

// TLS 错误（-6000 到 -6099）
#define UVHTTP_ERROR_TLS_INIT_FAILED          -6000
#define UVHTTP_ERROR_TLS_HANDSHAKE_FAILED     -6001

// WebSocket 错误（-7000 到 -7099）
#define UVHTTP_ERROR_WEBSOCKET_HANDSHAKE_FAILED -7000
#define UVHTTP_ERROR_WEBSOCKET_CLOSE_FAILED    -7001

// 通用错误（-9000 到 -9099）
#define UVHTTP_ERROR_INVALID_PARAM            -9000
#define UVHTTP_ERROR_OUT_OF_MEMORY            -9001
#define UVHTTP_ERROR_IO_ERROR                 -9002
#define UVHTTP_ERROR_NOT_SUPPORTED            -9003
```

### 错误处理 API

```c
// 获取错误名称
const char* uvhttp_error_string(uvhttp_error_t error);

// 获取错误分类
const char* uvhttp_error_category_string(uvhttp_error_t error);

// 获取错误描述
const char* uvhttp_error_description(uvhttp_error_t error);

// 获取修复建议
const char* uvhttp_error_suggestion(uvhttp_error_t error);

// 检查是否可恢复
int uvhttp_error_is_recoverable(uvhttp_error_t error);
```

### 错误处理示例

```c
uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %s\n", uvhttp_error_string(result));
    fprintf(stderr, "描述: %s\n", uvhttp_error_description(result));
    fprintf(stderr, "建议: %s\n", uvhttp_error_suggestion(result));
    
    if (uvhttp_error_is_recoverable(result)) {
        // 尝试恢复
        result = uvhttp_server_listen(server, "0.0.0.0", 8081);
    } else {
        // 无法恢复，退出
        return 1;
    }
}
```

---

## 线程模型

### 单线程事件循环

UVHTTP 采用单线程事件循环模型，所有操作都在事件循环线程中执行。

```
┌─────────────────────────────────────────────────────────────┐
│                    单线程事件循环（Event Loop）                │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐   │
│  │  读取    │  │  解析    │  │  处理    │  │  写入    │   │
│  │  事件    │  │  请求    │  │  请求    │  │  响应    │   │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘   │
└─────────────────────────────────────────────────────────────┘
```

### 优势

- **零锁开销**：无需锁，避免锁竞争
- **数据访问安全**：单线程保证数据一致性
- **执行流可预测**：事件驱动，执行顺序明确
- **性能优化**：CPU 缓存友好

### 限制

- **单核利用**：无法充分利用多核 CPU
- **阻塞风险**：长时间阻塞操作会影响性能

### 最佳实践

- **避免阻塞操作**：使用异步 I/O
- **快速处理请求**：每个请求处理时间 < 10ms
- **使用连接池**：复用连接，减少开销

### 事件循环模式选择

libuv 提供三种事件循环运行模式，选择正确的模式对性能至关重要：

#### UV_RUN_DEFAULT（推荐用于生产）

```c
uv_run(loop, UV_RUN_DEFAULT);
```

**特点**：
- 阻塞模式，直到没有活动句柄
- 最高性能，无额外延迟
- 适合长期运行的服务器

**使用场景**：
- 生产环境 HTTP 服务器
- 需要最大吞吐量的场景
- 长期运行的服务

**性能**：
- RPS: 30,000+
- 延迟: < 5ms
- 适合高并发场景

**优雅关闭**：
需要配合 `uv_signal_t` 和 `uv_stop()` 实现：

```c
static void on_signal(uv_signal_t* handle, int signum) {
    (void)signum;
    (void)handle;
    
    if (g_ctx && g_ctx->loop && uv_loop_alive(g_ctx->loop)) {
        uv_stop(g_ctx->loop);  /* 停止事件循环 */
    }
}

/* 在 main 中 */
uv_signal_t sigint;
uv_signal_init(loop, &sigint);
uv_signal_start(&sigint, on_signal, SIGINT);

uv_run(loop, UV_RUN_DEFAULT);

/* 清理 */
uv_signal_stop(&sigint);
uv_close((uv_handle_t*)&sigint, on_signal_close);
uv_run(loop, UV_RUN_ONCE);  /* 处理关闭回调 */
```

#### UV_RUN_ONCE（不推荐用于生产）

```c
while (running) {
    uv_run(loop, UV_RUN_ONCE);
}
```

**特点**：
- 执行一个事件迭代
- 如果有活动句柄，会阻塞等待
- 无法优雅关闭（死循环）

**问题**：
```c
/* 问题代码 */
while (ctx->running) {
    uv_run(loop, UV_RUN_ONCE);  /* 如果有活动句柄，永远不会返回 */
}
```

**原因**：
- TCP 监听句柄是活动句柄
- `UV_RUN_ONCE` 会一直等待新事件
- `ctx->running` 永远不会被检查

**性能**：
- RPS: ~200（受限于事件检查频率）
- 延迟: 460ms+
- 不适合生产环境

#### UV_RUN_NOWAIT（仅用于测试）

```c
while (running) {
    uv_run(loop, UV_RUN_NOWAIT);
    usleep(10000);  /* 10ms 睡眠 */
}
```

**特点**：
- 非阻塞模式
- 需要手动添加睡眠
- 可以优雅关闭

**性能问题**：
```c
/* 性能瓶颈 */
while (ctx->running) {
    uv_run(loop, UV_RUN_NOWAIT);
    usleep(10000);  /* 10ms 睡眠限制吞吐量 */
}
```

**性能影响**：
- 每秒最多 100 次事件循环迭代（1000ms / 10ms）
- RPS 限制在 ~200
- 延迟高达 460ms（请求在队列中等待）

**性能对比**：

| 模式 | RPS | 延迟 | 吞吐量 | 适用场景 |
|------|-----|------|--------|----------|
| UV_RUN_DEFAULT | 30,000+ | < 5ms | 高 | 生产环境 |
| UV_RUN_ONCE | ~200 | 460ms+ | 低 | 不推荐 |
| UV_RUN_NOWAIT + 10ms | ~195 | 460ms+ | 低 | 测试/调试 |

**性能提升案例**：

benchmark_unified.c 优化前后对比：

```c
/* 优化前：UV_RUN_NOWAIT + usleep(10000) */
while (ctx->running) {
    uv_run(loop, UV_RUN_NOWAIT);
    usleep(10000);  /* 10ms 睡眠 */
}
/* 性能：195 RPS, 460ms 延迟 */

/* 优化后：UV_RUN_DEFAULT */
uv_run(loop, UV_RUN_DEFAULT);
/* 性能：31,805 RPS, 3.02ms 延迟 */
/* 提升：157x RPS, 144x 延迟降低 */
```

### 事件循环优化建议

1. **生产环境使用 UV_RUN_DEFAULT**
   - 最高性能
   - 配合 `uv_signal_t` 实现优雅关闭
   - 使用 `uv_stop()` 停止循环

2. **避免使用 UV_RUN_NOWAIT + usleep**
   - 性能严重受限
   - 仅用于测试和调试
   - 不适合生产环境

3. **信号处理使用 libuv 信号**
   ```c
   /* 正确：使用 uv_signal_t */
   uv_signal_t sigint;
   uv_signal_init(loop, &sigint);
   uv_signal_start(&sigint, on_signal, SIGINT);
   
   /* 错误：使用 POSIX 信号 */
   signal(SIGINT, posix_signal_handler);  /* 不安全 */
   ```

4. **正确清理信号句柄**
   ```c
   /* 停止信号 */
   uv_signal_stop(&sigint);
   
   /* 关闭句柄 */
   uv_close((uv_handle_t*)&sigint, on_signal_close);
   
   /* 处理关闭回调 */
   uv_run(loop, UV_RUN_ONCE);
   ```

5. **避免阻塞操作**
   - 使用异步 I/O
   - 使用 `uv_timer_t` 模拟延迟
   - 不要在事件循环中使用 `sleep()` 或 `usleep()`

---

## 依赖管理

### 核心依赖

| 依赖 | 版本 | 用途 | 许可证 |
|------|------|------|--------|
| libuv | 1.x | 异步 I/O | MIT |
| llhttp | 8.x | HTTP 解析 | MIT |
| mbedtls | 3.x | TLS 支持 | Apache 2.0 |
| mimalloc | 2.x | 内存分配器 | MIT |
| xxhash | 0.8.x | 快速哈希 | BSD |
| uthash | 2.x | 哈希表 | BSD |
| cjson | 1.7.x | JSON 解析 | MIT |

### 可选依赖

| 依赖 | 版本 | 用途 | 许可证 |
|------|------|------|--------|
| googletest | 1.14.x | 单元测试 | BSD |
| cmocka | 1.x | Mock 框架 | Apache 2.0 |

### 依赖管理方式

使用 Git 子模块管理依赖：

```bash
# 初始化子模块
git submodule update --init --recursive

# 更新子模块
git submodule update --remote
```

### 编译选项

```cmake
# WebSocket 支持
option(BUILD_WITH_WEBSOCKET "Build with WebSocket support" ON)

# TLS 支持
option(BUILD_WITH_HTTPS "Build with TLS support" ON)

# mimalloc 分配器
option(BUILD_WITH_MIMALLOC "Build with mimalloc allocator" ON)

# 代码覆盖率
option(ENABLE_COVERAGE "Enable code coverage" OFF)

# 示例程序
option(BUILD_EXAMPLES "Build example programs" ON)
```

---

## 扩展性设计

### 功能模块

通过编译宏控制功能模块：

```c
// WebSocket 支持
#if UVHTTP_FEATURE_WEBSOCKET
    #include "uvhttp_websocket.h"
#endif

// TLS 支持
#if UVHTTP_FEATURE_TLS
    #include "uvhttp_tls.h"
#endif

// 静态文件服务
#if UVHTTP_FEATURE_STATIC_FILES
    #include "uvhttp_static.h"
#endif

// 限流功能
#if UVHTTP_FEATURE_RATE_LIMIT
    #include "uvhttp_rate_limit.h"
#endif
```

### 中间件支持

```c
typedef struct {
    uvhttp_middleware_func_t func;
    void* user_data;
} uvhttp_middleware_t;

typedef int (*uvhttp_middleware_func_t)(uvhttp_request_t* request, 
                                         uvhttp_response_t* response,
                                         void* user_data);

// 注册中间件
void uvhttp_server_use_middleware(uvhttp_server_t* server, 
                                   uvhttp_middleware_func_t func,
                                   void* user_data);
```

### 插件系统

```c
typedef struct {
    const char* name;
    int (*init)(void);
    void (*cleanup)(void);
    int (*handle_request)(uvhttp_request_t* request, uvhttp_response_t* response);
} uvhttp_plugin_t;

// 注册插件
void uvhttp_server_register_plugin(uvhttp_server_t* server, uvhttp_plugin_t* plugin);
```

---

## 总结

UVHTTP 采用了先进的架构设计，包括：

1. **分层缓存策略**：热路径缓存 + 哈希表缓存，路由匹配时间 < 1 μs
2. **零拷贝优化**：sendfile 大文件传输，性能提升 50%+
3. **智能缓存**：LRU 缓存 + 缓存预热，性能提升 30%+
4. **内存优化**：mimalloc 分配器，性能提升 30%+
5. **缓存行对齐**：利用 CPU 缓存局部性
6. **单线程事件循环**：零锁开销，数据访问安全
7. **模块化设计**：通过编译宏控制功能模块
8. **完善的错误处理**：统一的错误码体系

这些设计使得 UVHTTP 成为高性能、轻量级、生产就绪的 HTTP 服务器库。

---

**文档版本**：1.0  
**最后更新**：2026-02-01  
**维护者**：UVHTTP 开发团队