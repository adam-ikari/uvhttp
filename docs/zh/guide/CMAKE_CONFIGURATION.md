# CMake 配置指南

## 概述

UVHTTP 支持通过 CMake 配置各种编译时常量，允许用户根据实际需求调整库的行为和性能。

## 配置方法

### 1. 基本配置

在运行 `cmake` 时，可以通过 `-D` 参数指定常量的值：

```bash
mkdir build && cd build
cmake -DUVHTTP_MAX_HEADER_NAME_SIZE=512 -DUVHTTP_MAX_HEADER_VALUE_SIZE=8192 ..
make -j$(nproc)
```

### 2. 交互式配置

使用 `ccmake` 进行交互式配置：

```bash
mkdir build && cd build
ccmake ..
```

### 3. 配置文件

在 `CMakeLists.txt` 中预设配置：

```cmake
set(UVHTTP_MAX_HEADER_NAME_SIZE 512 CACHE STRING "Max HTTP header name size")
set(UVHTTP_MAX_HEADER_VALUE_SIZE 8192 CACHE STRING "Max HTTP header value size")
```

## 可配置常量

### HTTP 相关

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_MAX_HEADER_NAME_SIZE` | 256 | HTTP 头部名称最大长度 | 256-512 |
| `UVHTTP_MAX_HEADER_VALUE_SIZE` | 4096 | HTTP 头部值最大长度 | 4096-8192 |
| `UVHTTP_MAX_HEADERS` | 64 | HTTP 头部最大数量 | 32-128 |
| `UVHTTP_MAX_URL_SIZE` | 2048 | URL 最大长度 | 2048-4096 |
| `UVHTTP_MAX_PATH_SIZE` | 1024 | 路径最大长度 | 512-2048 |
| `UVHTTP_MAX_METHOD_SIZE` | 16 | HTTP 方法最大长度 | 16 |
| `UVHTTP_MAX_BODY_SIZE` | 1048576 | 请求体最大大小（字节） | 根据需求调整 |

### 连接管理

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_MAX_CONNECTIONS_DEFAULT` | 2048 | 默认最大连接数 | 1024-4096 |
| `UVHTTP_MAX_CONNECTIONS_MAX` | 10000 | 最大推荐连接数 | 10000-100000 |
| `UVHTTP_BACKLOG` | 8192 | TCP backlog 大小 | 1024-8192 |
| `UVHTTP_CONNECTION_TIMEOUT_DEFAULT` | 60 | 连接超时时间（秒） | 30-120 |
| `UVHTTP_TCP_KEEPALIVE_TIMEOUT` | 60 | TCP keepalive 超时（秒） | 30-120 |

### 缓冲区管理

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_INLINE_HEADERS_CAPACITY` | 32 | 内联头部容量 | 16-64 |
| `UVHTTP_INITIAL_BUFFER_SIZE` | 8192 | 初始缓冲区大小（字节） | 8192-16384 |
| `UVHTTP_READ_BUFFER_SIZE` | 16384 | 读取缓冲区大小（字节） | 16384-65536 |

### 静态文件服务

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_STATIC_MAX_CACHE_SIZE` | 1048576 | 静态文件缓存大小（字节） | 根据内存调整 |
| `UVHTTP_STATIC_MAX_PATH_SIZE` | 1024 | 静态文件路径最大长度 | 512-2048 |
| `UVHTTP_STATIC_MAX_FILE_SIZE` | 10485760 | 静态文件最大大小（字节） | 根据需求调整 |
| `UVHTTP_STATIC_SMALL_FILE_THRESHOLD` | 4096 | 小文件阈值（字节） | 4096-8192 |

### WebSocket

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE` | 16777216 | WebSocket 最大帧大小（字节） | 根据需求调整 |
| `UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE` | 67108864 | WebSocket 最大消息大小（字节） | 根据需求调整 |
| `UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE` | 65536 | WebSocket 接收缓冲区大小（字节） | 32768-131072 |
| `UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL` | 30 | WebSocket ping 间隔（秒） | 10-60 |
| `UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT` | 10 | WebSocket ping 超时（秒） | 5-30 |

### 异步文件操作

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_ASYNC_FILE_BUFFER_SIZE` | 65536 | 异步文件缓冲区大小（字节） | 32768-131072 |
| `UVHTTP_ASYNC_FILE_MAX_CONCURRENT` | 64 | 最大并发文件读取数 | 32-128 |
| `UVHTTP_ASYNC_FILE_MAX_SIZE` | 10485760 | 异步文件最大大小（字节） | 根据需求调整 |

### 性能优化

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_SENDFILE_CHUNK_SIZE` | 65536 | sendfile 块大小（字节） | 32768-131072 |
| `UVHTTP_SENDFILE_TIMEOUT_MS` | 30000 | sendfile 超时（毫秒） | 10000-60000 |
| `UVHTTP_STATIC_MAX_CACHE_SIZE` | 10485760 | 静态文件缓存大小（字节，10MB） | 5242880-52428800 |
| `UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE` | 2 | LRU 缓存批量驱逐大小 | 1-10 |

### 限流

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_RATE_LIMIT_MAX_REQUESTS` | 1000000 | 限流最大请求数 | 根据需求调整 |
| `UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS` | 86400 | 限流最大时间窗口（秒） | 根据需求调整 |
| `UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS` | 10 | 限流最小超时（秒） | 5-30 |

### 其他

| 常量名 | 默认值 | 说明 | 推荐配置 |
|--------|--------|------|----------|
| `UVHTTP_CLIENT_IP_BUFFER_SIZE` | 64 | 客户端 IP 缓冲区大小 | 64 |
| `UVHTTP_IP_OCTET_MAX_VALUE` | 255 | IP 八位组最大值 | 255 |

## 配置示例

### 示例 1：高并发场景

```bash
cmake \
  -DUVHTTP_MAX_CONNECTIONS_DEFAULT=4096 \
  -DUVHTTP_MAX_CONNECTIONS_MAX=20000 \
  -DUVHTTP_BACKLOG=16384 \
  -DUVHTTP_READ_BUFFER_SIZE=65536 \
  ..
```

### 示例 2：大文件传输

```bash
cmake \
  -DUVHTTP_MAX_BODY_SIZE=104857600 \
  -DUVHTTP_STATIC_MAX_FILE_SIZE=104857600 \
  -DUVHTTP_SENDFILE_CHUNK_SIZE=131072 \
  ..
```

### 示例 3：内存受限环境

```bash
cmake \
  -DUVHTTP_MAX_CONNECTIONS_DEFAULT=512 \
  -DUVHTTP_STATIC_MAX_CACHE_SIZE=524288 \
  -DUVHTTP_INITIAL_BUFFER_SIZE=4096 \
  ..
```

### 示例 4：WebSocket 优化

```bash
cmake \
  -DUVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE=33554432 \
  -DUVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE=134217728 \
  -DUVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE=131072 \
  ..
```

## 注意事项

1. **内存影响**：增大缓冲区和缓存大小会增加内存使用量
2. **性能权衡**：较大的缓冲区可能提高性能，但会增加内存占用
3. **平台限制**：某些值受操作系统限制（如文件描述符数量）
4. **测试验证**：修改配置后应进行充分的性能测试
5. **文档更新**：如果修改了默认值，请更新相关文档

## 验证配置

编译后，可以通过以下方式验证配置：

```bash
# 查看编译命令中的宏定义
make VERBOSE=1 2>&1 | grep UVHTTP_MAX_HEADER_NAME_SIZE

# 运行测试验证
./dist/bin/uvhttp_unit_tests
```

## 相关文档

- [API 参考](../api/API_REFERENCE.md)
- [性能基准](../dev/PERFORMANCE_BENCHMARK.md)
- [贡献者指南](DEVELOPER_GUIDE.md)