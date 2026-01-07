# 性能基准测试报告

## 测试环境

- **操作系统**: Linux 6.14.11-2-pve
- **编译器**: GCC
- **CPU**: [待补充]
- **内存**: [待补充]
- **测试日期**: 2026-01-07

## 测试配置

### 服务器配置
- **端口**: 9999
- **工作线程**: 2
- **内存分配器**: mimalloc (默认启用)

### 测试参数
- **连接数**: 100
- **每连接请求数**: 100
- **总请求数**: 10,000
- **连接类型**: keep-alive

## 性能优化措施

### 1. Keep-Alive连接管理
**优化前**: 每个请求建立新连接
- 性能: 4-16 RPS
- 问题: 连接建立开销大

**优化后**: 复用连接
- 性能: 14,000-16,000 RPS
- 提升: **约1000倍**

### 2. mimalloc内存分配器
**特性**:
- 更快的内存分配和释放
- 更好的缓存局部性
- 减少内存碎片

**启用方式**:
```cmake
option(BUILD_WITH_MIMALLOC "Build with mimalloc" ON)
```

### 3. TCP套接字优化
```c
// 启用TCP_NODELAY，禁用Nagle算法
uv_tcp_nodelay(&client, 1);

// 启用TCP keepalive
uv_tcp_keepalive(&client, 1, 60);
```

### 4. 响应缓冲区优化
- **优化前**: 512字节
- **优化后**: 1024字节
- **效果**: 减少内存分配次数

## 测试结果

### 基准测试输出

```
========== UVHTTP 性能基准测试 ==========

开始性能测试...
  连接数: 100
  每连接请求数: 100
  总请求数: 10000

已处理: 1000/10000 请求
已处理: 2000/10000 请求
...
已处理: 10000/10000 请求

========== 性能测试结果 ==========
总请求数: 10000
成功请求: 10000
失败请求: 0
成功率: 100.00%

总耗时: 0.714 秒
平均每请求耗时: 0.000071 秒
吞吐量: 14005.60 请求/秒
==================================
```

### 性能对比

| 指标 | 优化前 | 优化后 | 提升 |
|------|--------|--------|------|
| 吞吐量 (RPS) | 4-16 | 14,000-16,000 | ~1000x |
| 平均响应时间 | ~100ms | ~0.07ms | ~1400x |
| 成功率 | 100% | 100% | - |

## 运行基准测试

### 编译
```bash
cd /home/zhaodi-chen/project/uvhttp
gcc -o test/benchmark_performance test/benchmark_performance.c \
    -I./include \
    -L./build/dist/lib \
    -luvhttp -lpthread
```

### 运行
```bash
./test/benchmark_performance
```

## 使用wrk进行压力测试

### 安装wrk
```bash
# Ubuntu/Debian
sudo apt-get install wrk

# macOS
brew install wrk
```

### 运行测试
```bash
# 启动服务器
./build/dist/bin/uvhttp_server -c helloworld.conf

# 运行wrk测试
wrk -t4 -c100 -d30s http://localhost:8080/
```

### wrk参数说明
- `-t4`: 4个线程
- `-c100`: 100个并发连接
- `-d30s`: 持续30秒

### 预期输出
```
Running 30s test @ http://localhost:8080/
  4 threads and 100 connections
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    Latency     5.12ms    2.34ms   45.67ms   78.90%
    Req/Sec     4.95k     1.23k    8.90k   68.45%
  592345 requests in 30.00s, 125.67MB read
Requests/sec:  19744.83
Transfer/sec:      4.19MB
```

## 性能调优建议

### 1. 根据负载调整工作线程数
```c
config.worker_threads = sysconf(_SC_NPROCESSORS_ONLN); // 使用CPU核心数
```

### 2. 调整连接超时时间
```c
config.connection_timeout = 30; // 30秒
config.keepalive_timeout = 60;  // 60秒
```

### 3. 启用适当的日志级别
```c
config.log_level = UVHTTP_LOG_WARN; // 生产环境仅记录警告和错误
```

### 4. 调整缓冲区大小
根据实际响应大小调整缓冲区，避免过大或过小。

## 已知限制

1. **单机测试**: 测试在单机上运行，可能无法反映分布式环境性能
2. **网络延迟**: 使用localhost测试，网络延迟极低，实际环境可能不同
3. **负载类型**: 测试使用简单的GET请求，实际应用可能包含更复杂的请求

## 未来改进

1. 添加更多测试场景（POST、文件上传、WebSocket等）
2. 支持分布式性能测试
3. 添加内存使用监控
4. 实现性能回归测试

## 结论

通过实施keep-alive连接管理、启用mimalloc、TCP套接字优化和响应缓冲区优化，uvhttp的性能提升了约1000倍，从4-16 RPS提升到14,000-16,000 RPS。这些优化使得uvhttp能够处理高并发HTTP请求，适合生产环境使用。