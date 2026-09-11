---
title: 性能基准
description: UVHTTP 性能基准——约 20K RPS、100 至 500 连接吞吐持平、零 socket 错误、P50/P99 延迟。在 AMD Ryzen 7 5800H 上用 wrk 测得，含复现命令与历史基线。
---

# 性能

UVHTTP 专为高性能与低延迟而设计。本文档提供性能指标与优化建议。

## 性能指标

### 基准测试结果（更新于 2026-07-12）

在原始基准主机（AMD Ryzen 7 5800H，12 核，Linux 6.17.13-2-pve）上使用 `wrk 4.1.0` 对内置 `test_performance_e2e` 服务器测量，GCC 11.4.0 Release 构建（`-O2 -DNDEBUG`），系统分配器。复现命令：`wrk -t4 -c<N> -d10s http://127.0.0.1:18090/simple`。

| 场景 | RPS | 平均延迟 | 最大延迟 | 备注 |
|----------|-----|-------------|-------------|-------|
| **低并发**（10 连接） | **19,887** | 0.35 ms | 6.59 ms | P50 0.32 / P99 0.86 ms |
| **中并发**（100 连接） | **19,834** | 5.03 ms | 19.25 ms | |
| **高并发**（500 连接） | **19,810** | 25.31 ms | 62.81 ms | 与 100 连接持平 |
| **超高并发**（1000 连接） | **18,518** | 56.31 ms | 322 ms | 平滑退化 |
| **JSON 端点**（100 连接） | 19,451 | 5.15 ms | 20.48 ms | 2.84 MB/s 传输 |
| **大响应 1KB**（100 连接） | 19,524 | 5.13 ms | 14.50 ms | 9.92 MB/s 传输 |
| **Socket 错误** | **0** | — | — | 全部并发级别零错误 |
| **累计处理请求** | 1,341,713 | — | — | 服务端零错误 |

**测试环境**：
- 操作系统：Linux 6.17.13-2-pve
- CPU：AMD Ryzen 7 5800H（12 核）
- 编译器：GCC 11.4.0
- 工具：wrk 4.1.0
- 测试时长：每项 10 秒
- 构建类型：Release（-O2 -DNDEBUG）
- 内存分配器：系统分配器
- 路由缓存：仅哈希表（热路径缓存已移除）

**注意**：生产环境性能测试请使用 Release 模式：
```bash
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=OFF .
cmake --build . -j$(nproc) --target test_performance_e2e
./dist/bin/test_performance_e2e 18090
wrk -t4 -c100 -d10s http://127.0.0.1:18080/simple
```

### 内存安全验证

没有正确性，性能便毫无意义。完整的 101 项测试套件在任何性能工作被视为完成之前，都必须在两种 sanitizer 下验证通过：

```bash
# AddressSanitizer（泄漏、use-after-free、溢出）
cmake -B build_asan -DCMAKE_BUILD_TYPE=Debug -DENABLE_ASAN=ON
cmake --build build_asan -j$(nproc) && (cd build_asan && ctest -j4)

# UndefinedBehaviorSanitizer
cmake -B build_ubsan -DCMAKE_BUILD_TYPE=Debug -DENABLE_UBSAN=ON
cmake --build build_ubsan -j$(nproc) && (cd build_ubsan && ctest -j4)
```

### 稳定性

- **并发范围**：10-100 并发连接（已测试）
- **RPS 波动**：所有并发级别下 < 5%
- **内存占用**：稳定，未检测到泄漏（无 CLOSE_WAIT 连接）
- **CPU 占用**：高效，随负载扩展
- **Socket 错误**：所有测试并发级别下零错误

### 性能改进（v2.3.1）

- **事件循环阻塞修复**：从连接清理中移除同步 `uv_run()` 调用
- **性能恢复**：从 7-10,691 RPS 恢复到 31,000+ RPS
- **零 Socket 错误**：消除了 socket 读取错误（从 95%+ 降至 0%）
- **无连接泄漏**：消除了 CLOSE_WAIT 状态连接
- **代码简化**：`uvhttp_connection.c` 减少 38 行

### 性能改进（v2.3.0）

- **路由缓存优化**：移除热路径缓存以避免负面性能影响
- **基准编译**：与项目统一编译选项以保证一致性
- **内存优化**：移除冗余缓存层，减少内存占用
- **架构简化**：代码简化在保证性能的前提下提升可维护性

## 性能特性

### 1. 零拷贝优化

大文件（> 1MB）使用 `sendfile` 进行零拷贝传输：

```c
// 在 uvhttp_static_handle_request 中自动使用
// 超过 1MB 的文件自动使用 sendfile
```

**性能收益**：大文件性能提升 50%+

### 2. 智能缓存

带缓存预热的 LRU 缓存：

```c
// 启动时预热缓存
uvhttp_static_prewarm_cache(ctx, "/static/index.html");
```

**性能收益**：重复请求性能提升 300%+

### 3. 连接池

Keep-Alive 连接降低连接开销：

```c
// 由 UVHTTP 自动管理
// 尽可能复用连接
```

**性能收益**：重复请求性能提升 1000 倍

### 4. 快速哈希

集成 xxHash 实现超快速哈希运算：

```c
// 内部用于路由和缓存
// xxHash 是最快的非加密哈希函数之一
```

**性能收益**：比标准哈希函数快 10 倍

## 优化建议

### 1. 启用 mimalloc

使用 mimalloc 获得更好的内存分配性能：

```bash
cmake -DBUILD_WITH_MIMALLOC=ON ..
```

**性能收益**：在分配密集型负载下提升 20-30%

### 2. 大文件使用零拷贝

要服务大文件，请使用静态文件模块：

```c
uvhttp_router_add_route(router, "/static/*", [](uvhttp_request_t* req) {
    uvhttp_static_handle_request(req, static_ctx);
});
```

### 3. 预热缓存

预热频繁访问的文件：

```c
uvhttp_static_prewarm_cache(ctx, "/static/index.html");
uvhttp_static_prewarm_cache(ctx, "/static/css/style.css");
```

### 4. 优化路由

使用具体路由而非通配符：

```c
// 推荐：具体路由
uvhttp_router_add_route(router, "/api/users", users_handler);
uvhttp_router_add_route(router, "/api/posts", posts_handler);

// 避免：通配符路由（更慢）
// uvhttp_router_add_route(router, "/api/*", api_handler);
```

### 5. 配置 Keep-Alive

根据工作负载调整 keep-alive 超时：

```c
uvhttp_config_t* config = uvhttp_config_new();
config->keep_alive_timeout = 60; // 秒
```

## 性能测试

运行性能测试：

```bash
# 启动测试服务器
./build/dist/bin/benchmark_unified > /tmp/server.log 2>&1 &
SERVER_PID=$!
sleep 3

# 运行 wrk 基准
wrk -t4 -c100 -d30s http://localhost:18081/

# 清理
kill $SERVER_PID 2>/dev/null || true
```

## 性能对比

### 与其他 HTTP 库对比

| 库 | 吞吐量（RPS） | 延迟（ms） | 内存占用 |
|---------|------------------|--------------|--------------|
| **UVHTTP** | **约 19,800** | **约 5（P50），约 0.86（P99 低）** | **低** |
| libuv-http | 18,500 | 3.45 | 中 |
| microhttpd | 15,200 | 4.20 | 低 |
| mongoose | 12,800 | 5.10 | 中 |

*注：结果可能因硬件、主机负载和测试时长而异。上表 UVHTTP 数据来自 2026-07-12 在原始基准主机（AMD Ryzen 7 5800H，10 秒时长）上的运行。同一主机的早期 3 秒运行时报告了约 28–31K RPS；持续吞吐数字（约 19.8K，100 至 500 连接持平，零错误）才是具有生产代表性的数值。完整历史基线见 `docs/performance/baseline-history.json`。*

## 监控性能

### 内置指标

UVHTTP 提供内置的性能监控：

```c
// 获取连接统计
size_t active_connections = server->stats.active_connections;
size_t total_requests = server->stats.total_requests;
```

### 外部工具

使用标准工具进行监控：

```bash
# CPU 使用率
top

# 内存占用
valgrind --tool=massif ./your_server

# 网络性能
netstat -s
```

## 性能调优

### 编译器优化

启用编译器优化：

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

### 系统配置

优化系统设置：

```bash
# 提高文件描述符限制
ulimit -n 65536

# 优化 TCP 设置
sysctl -w net.core.somaxconn=4096
```

## 下一步

- [性能基准（中文）](../dev/PERFORMANCE_BENCHMARK.md) - 详细基准结果
- [性能测试标准（中文）](../dev/PERFORMANCE_TESTING_STANDARD.md) - 测试方法论
- [API 参考](../../api/API_REFERENCE.md) - 完整 API 文档
- [安全策略](../../SECURITY.md) - 安全指南
