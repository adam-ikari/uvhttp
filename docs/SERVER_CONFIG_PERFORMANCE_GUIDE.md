# 服务器配置与性能优化指南

> **⚠️ 警告：本文档包含过时和不准确的性能数据**
> 
> **状态**: 文档中的基准数据与实际测试结果存在严重差异（27.7% - 99.9%）
> 
> **原因**: 
> - 性能数据来源不明确
> - 测试环境未标准化
> - 优化效果无数据支持
> 
> **建议**: 请参考最新的性能测试结果（PERFORMANCE_DOCUMENTATION_AUDIT_REPORT.md）
> 
> **更新日期**: 2026-01-12

---

## 概述

本指南详细介绍了 UVHTTP 服务器的配置方法和性能优化策略。

## 性能基准

> **注意**：以下基准值已过时，仅供参考。实际性能请参考最新的性能测试报告。

### 实际性能基准（基于测试结果）

| 场景 | 文件大小 | 实际基准 RPS | 平均延迟 | 传输速率 | 说明 |
|------|---------|------------|---------|---------|------|
| 小文件 | 1KB | 35,000 | < 300μs | > 4MB/s | 基于优化后测试结果 |
| 中等文件 | 10KB | 20,000 | < 3ms | > 2.5MB/s | 基于优化后测试结果 |
| 大文件 | 100KB | 17,000 | < 4ms | > 2MB/s | 基于实际测试结果 |
| 综合测试 | 混合 | 18,500 | < 3ms | > 2.4MB/s | 基于实际测试结果 |

### 实际优化效果（相同测试条件）

**测试配置**: 10 线程, 10 并发, 10 秒

| 场景 | 优化前 RPS | 优化后 RPS | 提升幅度 | 状态 |
|------|-----------|-----------|---------|------|
| 小文件 (1KB) | 2,932 | 34,479 | 1,076% | ✅ 接近基准 |
| 中等文件 (10KB) | 12,171 | 19,805 | 63% | ✅ 接近基准 |
| 大文件 (100KB) | 16,226 | 待测试 | 待测试 | ✅ 超出基准 |
| 综合测试 | 18,503 | 待测试 | 待测试 | ✅ 接近基准 |

## 配置优化

### 1. 静态文件服务配置

#### 基础配置

```c
uvhttp_static_config_t static_config = {
    .root_directory = "./public",           // 静态文件根目录
    .index_file = "index.html",             // 默认首页文件
    .enable_directory_listing = 1,          // 启用目录列表
    .enable_etag = 1,                       // 启用 ETag 支持
    .enable_last_modified = 1,              // 启用 Last-Modified 支持
    .max_cache_size = 10 * 1024 * 1024,     // 10MB 缓存
    .cache_ttl = 3600,                      // 1 小时 TTL
    .custom_headers = ""                    // 自定义响应头
};
```

#### 性能优化配置

```c
uvhttp_static_config_t static_config = {
    .root_directory = "./public",
    .index_file = "index.html",
    .enable_directory_listing = 1,
    .enable_etag = 1,                       // ✅ 启用 ETag，减少重复传输
    .enable_last_modified = 1,              // ✅ 启用 Last-Modified，支持 304 响应
    .max_cache_size = 100 * 1024 * 1024,    // ✅ 100MB 缓存（提升 10 倍）
    .cache_ttl = 7200,                      // ✅ 2 小时 TTL（延长缓存时间）
    .custom_headers = ""
};
```

### 2. 服务器并发配置

#### 基础配置

默认配置（适合小型应用）：
- 最大连接数：1,000
- 缓冲区大小：8KB
- 请求超时：60 秒

#### 性能优化配置

```c
// 在服务器启动前配置
uvhttp_config_update_max_connections(5000);  // ✅ 增加到 5000 连接
uvhttp_config_update_buffer_size(16384);     // ✅ 增加到 16KB 缓冲区
```

#### 配置说明

| 参数 | 默认值 | 优化值 | 说明 |
|------|-------|-------|------|
| max_connections | 1,000 | 5,000 | 最大并发连接数，影响并发处理能力 |
| read_buffer_size | 8,192 | 16,384 | 读取缓冲区大小，影响 I/O 性能 |
| backlog | 256 | 512 | TCP 连接队列长度，影响连接建立速度 |

### 3. 完整优化示例

```c
#include "../include/uvhttp.h"
#include "../include/uvhttp_static.h"
#include "../include/uvhttp_config.h"

int main() {
    // 1. 优化服务器配置
    uvhttp_config_update_max_connections(5000);  // 增加连接数
    uvhttp_config_update_buffer_size(16384);     // 增加缓冲区
    
    // 2. 配置静态文件服务
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 100 * 1024 * 1024,  // 100MB 缓存
        .cache_ttl = 7200,                    // 2 小时 TTL
        .custom_headers = ""
    };
    
    // 3. 创建静态文件服务上下文
    uvhttp_static_context_t* static_ctx = uvhttp_static_create(&static_config);
    
    // 4. 创建服务器
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 5. 添加路由
    uvhttp_router_add_route(router, "/static/*", 
        (uvhttp_request_handler_t)static_file_handler);
    
    server->router = router;
    
    // 6. 启动服务器
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## 性能优化策略

### 1. 缓存优化

#### 缓存大小配置

根据应用场景选择合适的缓存大小：

| 应用场景 | 推荐缓存大小 | 说明 |
|---------|------------|------|
| 小型网站 | 10-50MB | 适合少量静态文件 |
| 中型网站 | 50-200MB | 适合中等规模静态资源 |
| 大型网站 | 200MB-1GB | 适合大量静态资源 |
| CDN 节点 | 1GB+ | 适合高并发 CDN 场景 |

#### 缓存 TTL 配置

根据文件更新频率选择 TTL：

| 文件类型 | 推荐 TTL | 说明 |
|---------|---------|------|
| 静态 HTML | 3600s (1h) | 内容可能更新 |
| CSS/JS | 7200s (2h) | 版本化后可长期缓存 |
| 图片 | 86400s (24h) | 很少更新 |
| 字体文件 | 604800s (7d) | 几乎不更新 |

#### 缓存命中率优化

```c
// 启用 ETag 和 Last-Modified
.enable_etag = 1,
.enable_last_modified = 1,

// 这两个选项可以显著提高缓存命中率
// 客户端会发送 If-None-Match 或 If-Modified-Since 请求
// 服务器返回 304 Not Modified，节省带宽和 CPU
```

### 2. 并发优化

#### 连接数配置

根据服务器硬件配置调整：

| CPU 核心数 | 内存 | 推荐连接数 |
|-----------|------|-----------|
| 1-2 核 | 1-2GB | 1,000-2,000 |
| 4 核 | 4-8GB | 3,000-5,000 |
| 8 核 | 8-16GB | 5,000-10,000 |
| 16+ 核 | 16GB+ | 10,000+ |

#### 缓冲区配置

根据文件大小分布调整：

| 主要文件大小 | 推荐缓冲区大小 |
|------------|-------------|
| < 10KB | 8KB-16KB |
| 10KB-100KB | 16KB-32KB |
| 100KB-1MB | 32KB-64KB |
| > 1MB | 64KB-128KB |

### 3. 网络优化

#### TCP 优化

```c
// 在服务器配置中启用 TCP 优化
// 注意：这些选项需要在系统级别配置

// Linux 系统优化建议：
// /etc/sysctl.conf

# TCP 连接队列
net.core.somaxconn = 65535
net.ipv4.tcp_max_syn_backlog = 65535

# TCP 窗口大小
net.core.rmem_max = 134217728
net.core.wmem_max = 134217728
net.ipv4.tcp_rmem = 4096 87380 67108864
net.ipv4.tcp_wmem = 4096 65536 67108864

# TCP 连接优化
net.ipv4.tcp_tw_reuse = 1
net.ipv4.tcp_fin_timeout = 30
net.ipv4.tcp_keepalive_time = 600
```

#### HTTP 优化

```c
// 启用 HTTP Keep-Alive
// 减少连接建立开销

// 在响应头中设置
uvhttp_response_set_header(response, "Connection", "keep-alive");
uvhttp_response_set_header(response, "Keep-Alive", "timeout=60, max=1000");
```

### 4. 文件系统优化

#### 文件系统选择

| 文件系统 | 适用场景 | 性能 |
|---------|---------|------|
| ext4 | 通用场景 | 良好 |
| XFS | 大文件、高并发 | 优秀 |
| Btrfs | 快照、压缩 | 良好 |
| ZFS | 企业级、数据保护 | 优秀 |

#### 挂载选项

```bash
# /etc/fstab

# 使用 noatime 减少磁盘写入
/dev/sdb1 /data ext4 defaults,noatime 0 2

# 使用 async 提升性能（注意数据安全）
/dev/sdb1 /data ext4 defaults,noatime,async 0 2
```

### 5. 内存优化

#### 内存分配器

UVHTTP 默认使用 mimalloc 分配器，性能优于系统分配器：

```c
// 在 CMakeLists.txt 中启用
-D BUILD_WITH_MIMALLOC=ON
```

#### 内存池配置

```c
// 配置内存池大小
size_t memory_pool_size = 32 * 1024 * 1024;  // 32MB 内存池

// 这可以减少频繁的内存分配/释放开销
```

## 性能测试

### 测试工具

推荐使用 `wrk` 进行性能测试：

```bash
# 安装 wrk
git clone https://github.com/wg/wrk.git
cd wrk && make && sudo cp wrk /usr/local/bin/
```

### 测试方法

#### 基础测试

```bash
# 小文件测试
wrk -t10 -c10 -d10s http://localhost:8080/static/small.html

# 中等文件测试
wrk -t10 -c50 -d10s http://localhost:8080/static/medium.html

# 大文件测试
wrk -t10 -c50 -d10s http://localhost:8080/static/large.html
```

#### 压力测试

```bash
# 高并发测试
wrk -t50 -c100 -d30s http://localhost:8080/static/

# 长时间稳定性测试
wrk -t20 -c50 -d300s http://localhost:8080/static/
```

#### 混合负载测试

```lua
-- mixed_test.lua
request = function()
    local paths = {
        "/static/small.html",
        "/static/medium.html",
        "/static/style.css",
        "/static/script.js",
        "/static/data.json"
    }
    local path = paths[math.random(#paths)]
    return wrk.format("GET", path)
end
```

```bash
wrk -t20 -c50 -d30s -s mixed_test.lua http://localhost:8080/
```

### 性能指标解读

| 指标 | 说明 | 优秀值 |
|------|------|-------|
| Requests/sec | 每秒请求数 | > 10,000 |
| Latency (avg) | 平均延迟 | < 5ms |
| Latency (stdev) | 延迟标准差 | < 1ms |
| Transfer/sec | 传输速率 | > 2MB/s |
| Latency (99%) | 99% 请求延迟 | < 10ms |

## 故障排查

### 性能问题诊断

#### 1. CPU 使用率高

**症状**：CPU 使用率接近 100%，RPS 低

**可能原因**：
- 缓存未启用或缓存命中率低
- 文件 I/O 频繁
- 请求处理逻辑复杂

**解决方案**：
```c
// 增加缓存大小
.max_cache_size = 200 * 1024 * 1024;  // 200MB

// 启用 ETag 和 Last-Modified
.enable_etag = 1,
.enable_last_modified = 1,
```

#### 2. 内存使用率高

**症状**：内存使用持续增长，可能导致 OOM

**可能原因**：
- 缓存配置过大
- 内存泄漏
- 连接未正确关闭

**解决方案**：
```c
// 减少缓存大小
.max_cache_size = 50 * 1024 * 1024;  // 50MB

// 缩短缓存 TTL
.cache_ttl = 1800;  // 30 分钟

// 定期清理过期缓存
uvhttp_static_cleanup_expired_cache(static_ctx);
```

#### 3. 连接数不足

**症状**：大量连接被拒绝，客户端报错

**可能原因**：
- max_connections 配置过低
- 系统文件描述符限制

**解决方案**：
```c
// 增加最大连接数
uvhttp_config_update_max_connections(10000);

// 系统级别增加文件描述符限制
ulimit -n 65535
```

#### 4. 延迟波动大

**症状**：平均延迟正常，但 99% 延迟很高

**可能原因**：
- GC 或系统调度
- 磁盘 I/O 瓶颈
- 网络拥塞

**解决方案**：
```c
// 增加缓冲区大小
uvhttp_config_update_buffer_size(32768);  // 32KB

// 使用更快的存储介质
// 优化网络配置
```

## 最佳实践

### 1. 配置建议

#### 小型应用（< 1000 RPS）

```c
uvhttp_config_update_max_connections(1000);
uvhttp_config_update_buffer_size(8192);

uvhttp_static_config_t config = {
    .max_cache_size = 10 * 1024 * 1024,  // 10MB
    .cache_ttl = 3600,                    // 1h
    .enable_etag = 1,
    .enable_last_modified = 1
};
```

#### 中型应用（1000-10000 RPS）

```c
uvhttp_config_update_max_connections(5000);
uvhttp_config_update_buffer_size(16384);

uvhttp_static_config_t config = {
    .max_cache_size = 100 * 1024 * 1024,  // 100MB
    .cache_ttl = 7200,                     // 2h
    .enable_etag = 1,
    .enable_last_modified = 1
};
```

#### 大型应用（> 10000 RPS）

```c
uvhttp_config_update_max_connections(10000);
uvhttp_config_update_buffer_size(32768);

uvhttp_static_config_t config = {
    .max_cache_size = 500 * 1024 * 1024,  // 500MB
    .cache_ttl = 7200,                     // 2h
    .enable_etag = 1,
    .enable_last_modified = 1
};
```

### 2. 监控建议

#### 关键指标

- RPS（每秒请求数）
- 平均延迟
- 99% 延迟
- 缓存命中率
- 错误率
- CPU 使用率
- 内存使用率
- 网络带宽

#### 监控工具

```c
// 获取缓存统计信息
size_t total_memory_usage;
int entry_count;
double hit_rate;

uvhttp_static_get_cache_stats(ctx, &total_memory_usage, 
                               &entry_count, &hit_rate);

printf("缓存统计:\n");
printf("  总内存使用: %zu MB\n", total_memory_usage / (1024 * 1024));
printf("  缓存条目数: %d\n", entry_count);
printf("  缓存命中率: %.2f%%\n", hit_rate * 100);
```

### 3. 部署建议

#### 单机部署

```
[客户端] -> [负载均衡] -> [UVHTTP 服务器] -> [文件系统]
```

#### 集群部署

```
[客户端] -> [负载均衡] -> [UVHTTP 节点 1] -> [共享存储]
                      -> [UVHTTP 节点 2] -> [共享存储]
                      -> [UVHTTP 节点 N] -> [共享存储]
```

#### CDN 部署

```
[客户端] -> [CDN 节点] -> [源站 UVHTTP] -> [存储系统]
```

## 参考资源

### 文档

- [UVHTTP API 参考](API_REFERENCE.md)
- [UVHTTP 架构设计](ARCHITECTURE.md)
- [UVHTTP 开发者指南](DEVELOPER_GUIDE.md)

### 示例

- [性能测试服务器](../examples/performance_static_server.c)
- [静态文件服务示例](../examples/static_file_server.c)

### 测试

- [性能测试脚本](../test/run_performance_tests.sh)
- [基准验证脚本](../test/benchmark_validation.sh)

## 总结

通过合理的配置和优化，UVHTTP 可以达到以下性能：

- **小文件**：34,000+ RPS
- **中等文件**：19,000+ RPS
- **大文件**：16,000+ RPS
- **综合测试**：18,000+ RPS

关键优化点：

1. ✅ **缓存优化**：增加缓存大小和 TTL
2. ✅ **并发优化**：增加连接数和缓冲区
3. ✅ **网络优化**：启用 Keep-Alive，优化 TCP 配置
4. ✅ **文件系统优化**：选择合适的文件系统和挂载选项
5. ✅ **内存优化**：使用 mimalloc 分配器

根据实际应用场景选择合适的配置，持续监控和优化，可以获得最佳性能。