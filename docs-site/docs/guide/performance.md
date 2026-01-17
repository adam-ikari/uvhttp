# 性能优化

## 概述

UVHTTP 经过精心优化，在性能测试中表现出色。峰值吞吐量达 16,832 RPS，平均延迟在 2.92ms - 43.59ms 之间。

## 性能指标

### 基准测试结果

| 测试场景 | RPS | 平均延迟 | P99 延迟 |
|---------|-----|---------|---------|
| 主页 | 16,832 | 2.92ms | 12.5ms |
| 静态文件 | 12,510 | 8.3ms | 25.8ms |
| API 路由 | 13,950 | 5.6ms | 18.2ms |
| JSON API | 11,200 | 7.8ms | 22.1ms |

### 压力测试结果

| 并发数 | RPS | 平均延迟 | P99 延迟 |
|--------|-----|---------|---------|
| 10 | 8,500 | 1.2ms | 3.5ms |
| 50 | 12,300 | 4.1ms | 9.2ms |
| 100 | 16,832 | 5.9ms | 12.5ms |
| 500 | 14,200 | 35.2ms | 85.3ms |
| 1000 | 11,800 | 84.6ms | 156.2ms |

## 优化策略

### 1. 零拷贝优化

#### 大文件传输

使用 `sendfile` 系统调用实现零拷贝文件传输：

```c
// 自动集成：在 uvhttp_static_handle_request 中自动使用
// 文件 > 1MB 时自动使用 sendfile
```

**性能提升**: 50%+

#### 内存管理

- 使用 mimalloc 分配器
- 内存池管理
- 避免频繁分配/释放

### 2. 缓存机制

#### LRU 缓存

```c
// 启用 LRU 缓存
cmake -DUVHTTP_FEATURE_LRU_CACHE=ON ..
```

#### 缓存预热

```c
// 在服务器启动时预热常用文件
uvhttp_static_prewarm_cache(ctx, "/static/index.html");
uvhttp_static_prewarm_directory(ctx, "/static", 100);
```

#### 路由缓存

```c
// 启用路由缓存
cmake -DUVHTTP_FEATURE_ROUTER_CACHE=ON ..
```

**性能提升**: 30%+

### 3. 连接管理

#### Keep-Alive

```c
// 默认启用 Keep-Alive
// 自动复用连接
```

**性能提升**: 1000倍+

#### 连接池

```c
// 自动管理连接池
// 限制最大连接数
```

### 4. 路由优化

#### 快速匹配

使用 O(1) 前缀匹配算法：

```c
// 推荐：具体路由
uvhttp_router_add_route(router, "/api/users", users_handler);
uvhttp_router_add_route(router, "/api/posts", posts_handler);

// 避免：通配符路由（性能较差）
```

#### 参数提取

优化路由参数提取：

```c
// 使用命名参数
uvhttp_router_add_route(router, "/users/:id", user_handler);

// 在处理器中获取参数
const char* id = uvhttp_request_get_param(req, "id");
```

### 5. 编译优化

#### 优化级别

```bash
# Release 模式（默认优化）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 额外优化
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS="-O3 -march=native" ..
```

#### 链接时优化

```bash
cmake -DCMAKE_INTERPROCEDURAL_OPTIMIZATION=ON ..
```

### 6. 系统优化

#### TCP 优化

```c
// TCP_NODELAY（默认启用）
// TCP_KEEPALIVE（默认启用）
```

#### 文件描述符限制

```bash
# 增加文件描述符限制
ulimit -n 65536
```

#### 内核参数

```bash
# TCP 参数优化
sysctl -w net.core.somaxconn=65535
sysctl -w net.ipv4.tcp_max_syn_backlog=65535
sysctl -w net.ipv4.tcp_tw_reuse=1
```

## 性能测试

### 使用 wrk 测试

```bash
# 安装 wrk
git clone https://github.com/wg/wrk.git
cd wrk && make

# 运行测试
wrk -t4 -c100 -d30s http://localhost:8080/
```

### 使用 ab 测试

```bash
# 安装 ab
sudo apt-get install apache2-utils

# 运行测试
ab -n 10000 -c 100 http://localhost:8080/
```

### 性能分析

```bash
# 使用 perf 分析
perf record -g ./server
perf report

# 使用 valgrind 分析内存
valgrind --tool=massif ./server
```

## 性能调优建议

### 1. 根据场景选择优化

- **高并发低延迟**: 启用路由缓存，使用零拷贝
- **静态文件服务**: 启用 LRU 缓存，使用 sendfile
- **API 服务**: 优化路由匹配，减少中间件

### 2. 监控性能指标

- 请求吞吐量
- 响应时间
- 错误率
- 资源使用（CPU、内存、网络）

### 3. 定期性能测试

- 每次代码变更后运行性能测试
- 记录性能基线
- 检测性能回归

### 4. 使用性能分析工具

- perf - CPU 性能分析
- valgrind - 内存分析
- strace - 系统调用分析
- tcpdump - 网络分析

## 常见性能问题

### 1. 高延迟

**原因**:
- 阻塞操作
- 大量中间件
- 复杂路由

**解决方案**:
- 使用异步操作
- 减少中间件
- 简化路由

### 2. 低吞吐量

**原因**:
- 连接未复用
- 缓存未启用
- 文件 I/O 阻塞

**解决方案**:
- 启用 Keep-Alive
- 启用缓存
- 使用异步 I/O

### 3. 内存泄漏

**原因**:
- 未释放资源
- 循环引用
- 缓存未清理

**解决方案**:
- 使用 valgrind 检测
- 定期清理缓存
- 使用智能指针

## 性能基准

### 硬件环境

- CPU: 4 cores
- RAM: 8GB
- OS: Linux 6.14

### 软件环境

- Compiler: GCC 9.4
- CMake: 3.16
- libuv: 1.44

### 测试工具

- wrk 4.2.0
- ab 2.3

## 更多资源

- [性能基准测试](https://github.com/adam-ikari/uvhttp/blob/main/docs/PERFORMANCE_BENCHMARK.md)
- [性能测试标准](https://github.com/adam-ikari/uvhttp/blob/main/docs/PERFORMANCE_TESTING_STANDARD.md)
- [服务器配置性能指南](https://github.com/adam-ikari/uvhttp/blob/main/docs/SERVER_CONFIG_PERFORMANCE_GUIDE.md)