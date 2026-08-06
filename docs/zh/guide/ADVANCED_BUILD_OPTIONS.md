# 高级构建选项

本文档介绍 UVHTTP 的高级构建配置选项，通常仅高级用户或特定性能调优场景需要。

## 概述

大多数用户应使用默认值。这些高级选项允许针对特定用例、性能优化或资源受限环境微调内部参数。

## 内存分配器配置

### UVHTTP_ALLOCATOR_TYPE
- **类型**: STRING
- **默认值**: 0
- **说明**: 内存分配器类型选择
- **用法**: 选择内存分配器实现
- **选项**:
  - `0`: 系统分配器 (malloc/free)
  - `1`: mimalloc 分配器 (自动启用 `BUILD_WITH_MIMALLOC=ON`)
  - `2`: 自定义分配器 (应用层实现)
- **影响**:
  - 系统分配器: 标准性能，无依赖
  - mimalloc: 内存分配快 30-50%，减少碎片
  - 自定义: 完全控制内存管理
- **注意**:
  - 选项 `1` 自动启用 `BUILD_WITH_MIMALLOC=ON`
  - 选项 `2` 需要应用层实现自定义分配器函数

#### 自定义分配器实现

使用 `UVHTTP_ALLOCATOR_TYPE=2` 时，必须在应用中实现以下函数：

```c
#include <stddef.h>
#include <stdlib.h>

// 自定义分配器实现
void* uvhttp_custom_alloc(size_t size) {
    // 实现自定义分配逻辑
    return malloc(size);
}

void uvhttp_custom_free(void* ptr) {
    // 实现自定义释放逻辑
    free(ptr);
}

void* uvhttp_custom_realloc(void* ptr, size_t size) {
    // 实现自定义 realloc 逻辑
    return realloc(ptr, size);
}

void* uvhttp_custom_calloc(size_t nmemb, size_t size) {
    // 实现自定义 calloc 逻辑
    return calloc(nmemb, size);
}
```

**示例用例**:
- 嵌入式系统的内存池分配
- 调试时跟踪内存使用
- 实现自定义分配策略
- 集成应用特定内存管理器

**重要说明**:
- 这些函数必须在包含任何 UVHTTP 头文件之前实现
- 线程安全由实现者负责
- 所有分配函数必须正确处理 NULL 返回
- free 必须安全处理 NULL 指针

## HTTP 协议配置

### UVHTTP_MAX_HEADER_NAME_SIZE
- **类型**: STRING
- **默认值**: 256
- **说明**: HTTP 头部名称最大长度（字节）
- **用法**: 如需处理很长的头部名称则调整
- **影响**: 减小此值节省内存，但可能拒绝有效的长头部名称请求

### UVHTTP_MAX_HEADER_VALUE_SIZE
- **类型**: STRING
- **默认值**: 4096
- **说明**: HTTP 头部值最大长度（字节）
- **用法**: 如需处理很长的头部值则调整
- **影响**: 减小此值节省内存，但可能拒绝有效的长头部值请求

### UVHTTP_MAX_HEADERS
- **类型**: STRING
- **默认值**: 64
- **说明**: 每个请求的最大 HTTP 头部数量
- **用法**: 如需大量头部则调整
- **影响**: 减小此值节省内存，但可能拒绝有效的多头部请求

### UVHTTP_INLINE_HEADERS_CAPACITY
- **类型**: STRING
- **默认值**: 32
- **说明**: 内联头部容量优化
- **用法**: 根据请求中典型头部数量调整
- **影响**: 更高值使用更多栈内存，但可能提升多头部请求性能

### UVHTTP_MAX_URL_SIZE
- **类型**: STRING
- **默认值**: 2048
- **说明**: URL 最大长度（字节）
- **用法**: 如需处理很长的 URL 则调整
- **影响**: 减小此值节省内存，但可能拒绝有效的长 URL 请求

### UVHTTP_MAX_PATH_SIZE
- **类型**: STRING
- **默认值**: 1024
- **说明**: 路径最大长度（字节）
- **用法**: 如需处理很长的路径则调整
- **影响**: 减小此值节省内存，但可能拒绝有效的长路径请求

### UVHTTP_MAX_METHOD_SIZE
- **类型**: STRING
- **默认值**: 16
- **说明**: HTTP 方法最大长度（字节）
- **用法**: 很少需要调整
- **影响**: 内存影响极小

## 连接管理

### UVHTTP_MAX_CONNECTIONS_DEFAULT
- **类型**: STRING
- **默认值**: 2048
- **说明**: 默认最大并发连接数
- **用法**: 根据预期并发连接数调整
- **影响**: 更高值使用更多内存，但允许更多并发连接

### UVHTTP_MAX_CONNECTIONS_MAX
- **类型**: STRING
- **默认值**: 10000
- **说明**: 推荐的最大并发连接数
- **用法**: 设置为服务器容量上限
- **影响**: 更高值使用更多内存，但允许更多并发连接

### UVHTTP_BACKLOG
- **类型**: STRING
- **默认值**: 8192
- **说明**: TCP backlog 大小
- **用法**: 根据预期连接突发速率调整
- **影响**: 更高值使用更多内核内存，但更好处理突发连接

### UVHTTP_CONNECTION_TIMEOUT_DEFAULT
- **类型**: STRING
- **默认值**: 60
- **说明**: 默认连接超时（秒）
- **用法**: 根据应用需求调整
- **影响**: 更长超时使空闲连接保持更久，消耗更多资源

## 缓冲区配置

### UVHTTP_INITIAL_BUFFER_SIZE
- **类型**: STRING
- **默认值**: 8192
- **说明**: 初始缓冲区大小（字节）
- **用法**: 根据典型请求/响应大小调整
- **影响**: 更大值使用更多内存，但可能减少重新分配

### UVHTTP_MAX_BODY_SIZE
- **类型**: STRING
- **默认值**: 1048576 (1MB)
- **说明**: 最大请求体大小（字节）
- **用法**: 根据应用需求调整
- **影响**: 减小此值节省内存，但可能拒绝有效的大请求

### UVHTTP_READ_BUFFER_SIZE
- **类型**: STRING
- **默认值**: 16384 (16KB)
- **说明**: 读取缓冲区大小（字节）
- **用法**: 根据网络条件和典型数据大小调整
- **影响**: 更大值使用更多内存，但可能提升吞吐量

## 异步文件操作

### UVHTTP_ASYNC_FILE_BUFFER_SIZE
- **类型**: STRING
- **默认值**: 65536 (64KB)
- **说明**: 异步文件缓冲区大小（字节）
- **用法**: 根据文件大小和 I/O 模式调整
- **影响**: 更大值使用更多内存，但可能提升文件 I/O 性能

### UVHTTP_ASYNC_FILE_MAX_CONCURRENT
- **类型**: STRING
- **默认值**: 64
- **说明**: 最大并发文件读取数
- **用法**: 根据磁盘 I/O 容量和并发需求调整
- **影响**: 更高值使用更多内存，但允许更多并发文件操作

### UVHTTP_ASYNC_FILE_MAX_SIZE
- **类型**: STRING
- **默认值**: 10485760 (10MB)
- **说明**: 异步操作的最大文件大小（字节）
- **用法**: 大于此值的文件使用同步操作
- **影响**: 更大值使用更多内存，但允许更大文件使用异步操作

## 静态文件服务

### UVHTTP_STATIC_MAX_CACHE_SIZE
- **类型**: STRING
- **默认值**: 1048576 (1MB)
- **说明**: 静态文件缓存最大大小（字节）
- **用法**: 根据可用内存和文件访问模式调整
- **影响**: 更大值使用更多内存，但提升缓存命中率

### UVHTTP_STATIC_MAX_PATH_SIZE
- **类型**: STRING
- **默认值**: 1024
- **说明**: 静态文件路径最大长度（字节）
- **用法**: 如需处理很长的文件路径则调整
- **影响**: 减小此值节省内存，但可能拒绝有效的文件路径

### UVHTTP_STATIC_MAX_CONTENT_LENGTH
- **类型**: STRING
- **默认值**: 32
- **说明**: 静态文件 Content-Length 最大长度
- **用法**: 很少需要调整
- **影响**: 内存影响极小

### UVHTTP_STATIC_MAX_FILE_SIZE
- **类型**: STRING
- **默认值**: 1073741824 (1GB)
- **说明**: 静态文件最大大小（字节）
- **用法**: 根据应用需求调整
- **影响**: 减小此值阻止提供非常大的文件

### UVHTTP_STATIC_SMALL_FILE_THRESHOLD
- **类型**: STRING
- **默认值**: 4096 (4KB)
- **说明**: 小文件阈值（字节）
- **用法**: 小于此值的文件采用不同处理方式
- **影响**: 调整此值影响小文件性能特征

## WebSocket 配置

### UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
- **类型**: STRING
- **默认值**: 16777216 (16MB)
- **说明**: WebSocket 默认最大帧大小（字节）
- **用法**: 根据应用消息大小需求调整
- **影响**: 减小此值节省内存，但可能拒绝有效的大帧

### UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE
- **类型**: STRING
- **默认值**: 67108864 (64MB)
- **说明**: WebSocket 默认最大消息大小（字节）
- **用法**: 根据应用消息大小需求调整
- **影响**: 减小此值节省内存，但可能拒绝有效的大消息

### UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE
- **类型**: STRING
- **默认值**: 65536 (64KB)
- **说明**: WebSocket 默认接收缓冲区大小（字节）
- **用法**: 根据典型消息大小调整
- **影响**: 更大值使用更多内存，但可能提升大消息性能

### UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL
- **类型**: STRING
- **默认值**: 30
- **说明**: WebSocket 默认 ping 间隔（秒）
- **用法**: 根据网络条件和应用需求调整
- **影响**: 更短间隔更快检测死连接，但消耗更多带宽

### UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT
- **类型**: STRING
- **默认值**: 10
- **说明**: WebSocket 默认 ping 超时（秒）
- **用法**: 根据网络条件和应用需求调整
- **影响**: 更短超时更快检测死连接，但可能导致误报

## TCP 配置

### UVHTTP_TCP_KEEPALIVE_TIMEOUT
- **类型**: STRING
- **默认值**: 60
- **说明**: TCP keepalive 超时（秒）
- **用法**: 根据网络条件和应用需求调整
- **影响**: 更短超时更快检测死连接，但可能导致误报

### UVHTTP_CLIENT_IP_BUFFER_SIZE
- **类型**: STRING
- **默认值**: 64
- **说明**: 客户端 IP 缓冲区大小（字节）
- **用法**: 如需存储很长的客户端 IP 地址则调整
- **影响**: 内存影响极小

## Sendfile 配置

### UVHTTP_SENDFILE_TIMEOUT_MS
- **类型**: STRING
- **默认值**: 30000
- **说明**: Sendfile 超时（毫秒）
- **用法**: 根据网络条件和文件大小调整
- **影响**: 更长超时使慢传输的连接保持更久

### UVHTTP_SENDFILE_MAX_RETRY
- **类型**: STRING
- **默认值**: 2
- **说明**: Sendfile 最大重试次数
- **用法**: 根据网络可靠性调整
- **影响**: 更高值可能提升可靠性，但可能延迟错误检测

### UVHTTP_SENDFILE_CHUNK_SIZE
- **类型**: STRING
- **默认值**: 262144 (256KB)
- **说明**: Sendfile 块大小（字节）
- **用法**: 根据网络条件和文件大小调整
- **影响**: 更大值使用更多内存，但可能提升吞吐量

### UVHTTP_SENDFILE_MIN_FILE_SIZE
- **类型**: STRING
- **默认值**: 65536 (64KB)
- **说明**: Sendfile 最小文件大小（字节）
- **用法**: 小于此值的文件使用常规文件操作
- **影响**: 调整此值影响小文件性能特征

## 文件大小阈值

### UVHTTP_FILE_SIZE_SMALL
- **类型**: STRING
- **默认值**: 1048576 (1MB)
- **说明**: 小文件阈值（字节）
- **用法**: 用于性能优化决策
- **影响**: 调整此值影响不同文件大小的性能特征

### UVHTTP_FILE_SIZE_MEDIUM
- **类型**: STRING
- **默认值**: 10485760 (10MB)
- **说明**: 中等文件阈值（字节）
- **用法**: 用于性能优化决策
- **影响**: 调整此值影响不同文件大小的性能特征

### UVHTTP_FILE_SIZE_LARGE
- **类型**: STRING
- **默认值**: 104857600 (100MB)
- **说明**: 大文件阈值（字节）
- **用法**: 用于性能优化决策
- **影响**: 调整此值影响不同文件大小的性能特征

## 块大小配置

### UVHTTP_CHUNK_SIZE_SMALL
- **类型**: STRING
- **默认值**: 65536 (64KB)
- **说明**: 小块大小（字节）
- **用法**: 用于性能优化决策
- **影响**: 调整此值影响不同块大小的性能特征

### UVHTTP_CHUNK_SIZE_MEDIUM
- **类型**: STRING
- **默认值**: 262144 (256KB)
- **说明**: 中等块大小（字节）
- **用法**: 用于性能优化决策
- **影响**: 调整此值影响不同块大小的性能特征

### UVHTTP_CHUNK_SIZE_LARGE
- **类型**: STRING
- **默认值**: 1048576 (1MB)
- **说明**: 大块大小（字节）
- **用法**: 用于性能优化决策
- **影响**: 调整此值影响不同块大小的性能特征

## 缓存配置

### UVHTTP_CACHE_DEFAULT_MAX_ENTRIES
- **类型**: STRING
- **默认值**: 1000
- **说明**: 缓存默认最大条目数
- **用法**: 根据可用内存和缓存访问模式调整
- **影响**: 更高值使用更多内存，但提升缓存命中率

### UVHTTP_CACHE_DEFAULT_TTL
- **类型**: STRING
- **默认值**: 3600
- **说明**: 缓存默认 TTL（秒）
- **用法**: 根据数据新鲜度需求调整
- **影响**: 更长 TTL 可能提供过期数据，但提升缓存命中率

### UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE
- **类型**: STRING
- **默认值**: 10
- **说明**: LRU 缓存批量驱逐大小
- **用法**: 根据缓存访问模式调整
- **影响**: 更大值在驱逐时使用更多 CPU，但可能提升整体缓存性能

## Socket 配置

### UVHTTP_SOCKET_SEND_BUF_SIZE
- **类型**: STRING
- **默认值**: 262144 (256KB)
- **说明**: Socket 发送缓冲区大小（字节）
- **用法**: 根据网络条件和吞吐量需求调整
- **影响**: 更大值使用更多内存，但可能提升吞吐量

### UVHTTP_SOCKET_RECV_BUF_SIZE
- **类型**: STRING
- **默认值**: 262144 (256KB)
- **说明**: Socket 接收缓冲区大小（字节）
- **用法**: 根据网络条件和吞吐量需求调整
- **影响**: 更大值使用更多内存，但可能提升吞吐量

## 系统配置

### UVHTTP_PAGE_SIZE
- **类型**: STRING
- **默认值**: 4096
- **说明**: 内存页大小（字节）
- **用法**: 应匹配系统页大小
- **影响**: 不正确的值可能导致性能问题

### UVHTTP_IP_OCTET_MAX_VALUE
- **类型**: STRING
- **默认值**: 255
- **说明**: IP 八位组最大值
- **用法**: 不应更改
- **影响**: 更改此值可能导致 IP 地址解析错误

## 限流配置

### UVHTTP_RATE_LIMIT_MAX_REQUESTS
- **类型**: STRING
- **默认值**: 1000000
- **说明**: 限流最大请求数
- **用法**: 根据应用限流需求调整
- **影响**: 更高值允许更多请求，但可能增加服务器负载

### UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS
- **类型**: STRING
- **默认值**: 86400
- **说明**: 限流最大时间窗口（秒）
- **用法**: 根据应用限流需求调整
- **影响**: 更长窗口提供更多灵活性，但可能降低限流效果

### UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS
- **类型**: STRING
- **默认值**: 10
- **说明**: 限流最小超时（秒）
- **用法**: 根据应用限流需求调整
- **影响**: 更短超时可能导致更多误报

## 如何使用高级选项

配置高级选项时使用 CMake 的 `-D` 标志：

```bash
cmake -DUVHTTP_MAX_HEADER_NAME_SIZE=512 \
      -DUVHTTP_MAX_BODY_SIZE=5242880 \
      -DUVHTTP_CACHE_DEFAULT_MAX_ENTRIES=5000 \
      ..
```

## 性能调优指南

### 内存受限环境
内存受限环境中考虑减小：
- `UVHTTP_MAX_CONNECTIONS_DEFAULT`
- `UVHTTP_MAX_HEADERS`
- `UVHTTP_MAX_BODY_SIZE`
- `UVHTTP_CACHE_DEFAULT_MAX_ENTRIES`
- `UVHTTP_READ_BUFFER_SIZE`

### 高吞吐环境
高吞吐环境中考虑增大：
- `UVHTTP_MAX_CONNECTIONS_DEFAULT`
- `UVHTTP_READ_BUFFER_SIZE`
- `UVHTTP_SOCKET_SEND_BUF_SIZE`
- `UVHTTP_SOCKET_RECV_BUF_SIZE`
- `UVHTTP_CACHE_DEFAULT_MAX_ENTRIES`

### 低延迟环境
低延迟环境中考虑：
- 减小 `UVHTTP_CONNECTION_TIMEOUT_DEFAULT`
- 减小 `UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL`
- 增大 `UVHTTP_READ_BUFFER_SIZE`
- 增大 `UVHTTP_SOCKET_SEND_BUF_SIZE`

## 相关文档

- [构建配置矩阵](BUILD_CONFIGURATION_MATRIX.md)
- [开发者指南](/zh/guide/DEVELOPER_GUIDE.md)
- [性能基准](/zh/dev/PERFORMANCE_BENCHMARK.md)
