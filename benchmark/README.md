# UVHTTP 性能基准测试

本目录包含 UVHTTP 服务器的性能基准测试工具和配置。

## 概述

UVHTTP 性能基准测试使用标准的 HTTP 性能测试工具（wrk、ab）对服务器进行全面测试。测试包括多种场景、并发级别和响应大小，以评估服务器的性能表现。

## 快速开始

### 1. 编译基准测试服务器

```bash
cd /home/zhaodi-chen/project/uvhttp/build
make benchmark_rps
```

### 2. 运行完整测试套件

```bash
cd /home/zhaodi-chen/project/uvhttp/benchmark
./run_benchmarks.sh
```

### 3. 手动运行测试

```bash
# 启动服务器
./build/dist/bin/benchmark_rps 18081

# 在另一个终端运行性能测试
wrk -t4 -c100 -d30s http://127.0.0.1:18081/
```

## 测试工具

### benchmark_rps

HTTP 性能基准测试服务器，支持多种测试场景：

**启动服务器：**
```bash
./build/dist/bin/benchmark_rps [端口]
```

**默认端口：** 18081

**路由数量：** 7 个路由（数组模式）

**可用端点：**
- `GET  /` - 简单文本响应（13 bytes）
- `GET  /json` - JSON 响应（50 bytes）
- `POST /api` - POST 请求处理（23 bytes）
- `GET  /small` - 小响应（1KB）
- `GET  /medium` - 中等响应（10KB）
- `GET  /large` - 大响应（100KB）
- `GET  /health` - 健康检查（22 bytes）

### benchmark_rps_150_routes

HTTP 性能基准测试服务器，支持 150 路由场景（Trie 模式）：

**启动服务器：**
```bash
./build/dist/bin/benchmark_rps_150_routes [端口]
```

**默认端口：** 18095

**路由数量：** 150 个路由（超过 HYBRID_THRESHOLD = 100，自动切换到 Trie 模式）

**可用端点：**
- `GET  /api/resource1` 到 `/api/resource150` - 150 个路由端点

**用途：**
- 测试 Trie 模式下的路由性能
- 验证大量路由场景下的性能表现
- 对比数组模式和 Trie 模式的性能差异

**性能测试：**
```bash
# 启动服务器
./build/dist/bin/benchmark_rps_150_routes 18095

# 在另一个终端运行性能测试
wrk -t4 -c100 -d30s http://127.0.0.1:18095/api/resource1
wrk -t4 -c100 -d30s http://127.0.0.1:18095/api/resource50
wrk -t4 -c100 -d30s http://127.0.0.1:18095/api/resource100
```

### benchmark_file_transfer

文件传输性能基准测试服务器，专门用于测试静态文件传输性能，包括 sendfile 零拷贝优化：

**启动服务器：**
```bash
./build/dist/bin/benchmark_file_transfer [端口]
```

**默认端口：** 18082

**可用端点：**
- `GET /file/small` - 小文件（1KB）
- `GET /file/medium` - 中等文件（64KB）
- `GET /file/large` - 大文件（1MB）
- `GET /file/xlarge` - 超大文件（10MB）
- `GET /file/xxlarge` - 超超大文件（100MB）
- `GET /health` - 健康检查

**特性：**
- 静态文件服务，使用 `uvhttp_router_add_static_route`
- sendfile 零拷贝优化（> 64KB 文件）
- LRU 缓存（100MB）
- ETag 和 Last-Modified 支持
- 无文件大小限制

**性能测试：**
```bash
# 启动服务器
./build/dist/bin/benchmark_file_transfer 18082

# 小文件测试
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/small

# 大文件测试
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/large

# 超大文件测试
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/xxlarge

# 使用 ab 测试（适合小文件）
ab -n 1000 -c 10 http://127.0.0.1:18082/file/small

# 并发测试
wrk -t8 -c200 -d60s http://127.0.0.1:18082/file/medium

# 长时间稳定性测试
wrk -t4 -c100 -d300s http://127.0.0.1:18082/file/large
```

**性能指标：**
- 1KB 文件：~0.002s，1.0 MB/s
- 64KB 文件：~0.001s，65.5 MB/s
- 1MB 文件：~0.001s，1.0 GB/s
- 10MB 文件：~0.007s，1.7 GB/s
- 100MB 文件：~0.060s，1.8 GB/s

**并发性能：**
- 10 个并发 1MB 文件：~0.018s
- 5 个并发 50MB 文件：~0.057s

### run_benchmarks.sh

自动化测试脚本，运行完整的性能测试套件：

```bash
./run_benchmarks.sh
```

**功能：**
- 自动编译基准测试服务器
- 启动服务器并运行测试
- 支持多种并发级别
- 生成详细的测试报告
- 支持 wrk 和 ab 工具

## 测试场景

### 1. 基本性能测试

测试服务器在不同并发级别下的 RPS 性能：

```bash
# 低并发
wrk -t2 -c10 -d30s http://127.0.0.1:18081/

# 中等并发
wrk -t4 -c50 -d30s http://127.0.0.1:18081/

# 高并发
wrk -t8 -c200 -d30s http://127.0.0.1:18081/

# 极高并发
wrk -t16 -c500 -d30s http://127.0.0.1:18081/
```

### 2. 路由模式性能测试

UVHTTP 支持两种路由模式：

- **数组模式**：路由数量 < 100 时使用，O(n) 查找
- **Trie 模式**：路由数量 ≥ 100 时自动切换，O(m) 查找（m 为路径深度）

**benchmark_rps（7 路由，数组模式）：**
```bash
./build/dist/bin/benchmark_rps 18081
wrk -t4 -c100 -d30s http://127.0.0.1:18081/
```

**benchmark_rps_150_routes（150 路由，Trie 模式）：**
```bash
./build/dist/bin/benchmark_rps_150_routes 18095
wrk -t4 -c100 -d30s http://127.0.0.1:18095/api/resource1
wrk -t4 -c100 -d30s http://127.0.0.1:18095/api/resource50
wrk -t4 -c100 -d30s http://127.0.0.1:18095/api/resource100
```

**性能对比：**
- 7 路由（数组模式）：约 20,845 RPS
- 150 路由（Trie 模式）：约 20,376 RPS
- 性能变化：-1.42%（在测试误差范围内）

**优化效果：**
- 节点大小从 272 字节减少到 128 字节（-53%）
- 缓存行使用从 5 行减少到 2 行（-60%）
- 内存占用减少，缓存局部性提升

### 3. 不同响应大小测试

测试服务器处理不同大小响应的能力：

```bash
# 简单文本（13 bytes）
wrk -t4 -c100 -d30s http://127.0.0.1:18081/

# JSON 响应（50 bytes）
wrk -t4 -c100 -d30s http://127.0.0.1:18081/json

# 小响应（1KB）
wrk -t4 -c100 -d30s http://127.0.0.1:18081/small

# 中等响应（10KB）
wrk -t4 -c100 -d30s http://127.0.0.1:18081/medium

# 大响应（100KB）
wrk -t4 -c100 -d30s http://127.0.0.1:18081/large
```

### 4. 文件传输性能测试

测试静态文件传输性能，包括 sendfile 零拷贝优化：

```bash
# 启动文件传输测试服务器
./build/dist/bin/benchmark_file_transfer 18082

# 小文件传输（1KB）
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/small

# 中等文件传输（64KB）
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/medium

# 大文件传输（1MB）
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/large

# 超大文件传输（10MB）
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/xlarge

# 超超大文件传输（100MB）
wrk -t4 -c100 -d30s http://127.0.0.1:18082/file/xxlarge

# 并发文件传输测试
wrk -t8 -c200 -d60s http://127.0.0.1:18082/file/large
```

**文件完整性验证：**
```bash
# 下载文件并验证 MD5
curl -o /tmp/test_file.bin http://127.0.0.1:18082/file/large
md5sum /tmp/test_file.bin /home/zhaodi-chen/project/uvhttp/build/public/file_test/large.bin
```

### 3. 使用 ab 工具

使用 Apache Bench 进行性能测试：

```bash
# 基本测试
ab -n 100000 -c 100 -k http://127.0.0.1:18081/

# JSON 响应测试
ab -n 100000 -c 100 -k http://127.0.0.1:18081/json

# 不同并发级别
ab -n 100000 -c 10 -k http://127.0.0.1:18081/
ab -n 100000 -c 50 -k http://127.0.0.1:18081/
ab -n 100000 -c 200 -k http://127.0.0.1:18081/
```

## 性能目标

根据 UVHTTP 性能基准，服务器应达到以下目标：

### HTTP 响应性能

| 并发级别 | 线程数 | 连接数 | 目标 RPS |
|---------|--------|--------|----------|
| 低并发 | 2 | 10 | ≥ 17,000 |
| 中等并发 | 4 | 50 | ≥ 17,000 |
| 高并发 | 8 | 200 | ≥ 16,000 |
| 极高并发 | 16 | 500 | ≥ 15,000 |

**延迟目标：**
- 平均延迟: < 15ms
- P50 延迟: < 10ms
- P95 延迟: < 20ms
- P99 延迟: < 30ms

**错误率目标：**
- 最大错误率: < 0.1%

### 文件传输性能

| 文件大小 | 目标传输时间 | 目标传输速度 |
|---------|-------------|-------------|
| 1KB | < 0.005s | > 200 KB/s |
| 64KB | < 0.005s | > 12 MB/s |
| 1MB | < 0.005s | > 200 MB/s |
| 10MB | < 0.010s | > 1 GB/s |
| 100MB | < 0.100s | > 1 GB/s |

**并发文件传输：**
- 10 个并发 1MB 文件：< 0.030s
- 5 个并发 50MB 文件：< 0.100s

**文件完整性：**
- 所有文件 MD5 验证必须通过
- 无数据损坏或丢失

## 测试结果

测试结果保存在 `results/` 目录下，按时间戳组织：

```
results/
└── run_20260128_120000/
    ├── report.md          # 测试报告
    ├── results.csv        # CSV 格式结果
    ├── server.log         # 服务器日志
    ├── _low_concurrent/   # 低并发测试结果
    ├── _medium_concurrent/ # 中等并发测试结果
    ├── _high_concurrent/  # 高并发测试结果
    └── _extreme_concurrent/ # 极高并发测试结果
```

## 配置文件

测试配置保存在 `benchmark_config.yml` 中，可以自定义：

- 服务器配置（主机、端口）
- 测试配置（时长、预热）
- 并发级别
- 测试端点
- 性能目标
- 工具配置

## 最佳实践

### 1. 测试环境

- 使用独立的测试机器
- 关闭不必要的后台进程
- 确保网络稳定
- 使用固定测试参数

### 2. 测试流程

1. 预热服务器（5 秒）
2. 运行测试（30 秒）
3. 冷却服务器（2 秒）
4. 重复测试 3 次取平均值

### 3. 结果分析

- 关注 RPS、延迟、错误率
- 与基准数据对比
- 分析性能瓶颈
- 记录测试环境

### 4. 性能优化

- 使用 Keep-Alive 连接
- 优化响应大小
- 减少内存分配
- 使用 mimalloc 分配器

## 故障排除

### 服务器无法启动

```bash
# 检查端口是否被占用
lsof -i :18081

# 杀死占用端口的进程
kill -9 $(lsof -ti :18081)

# 使用其他端口
./build/dist/bin/benchmark_rps 18082
```

### wrk 未安装

```bash
# Ubuntu/Debian
sudo apt-get install wrk

# macOS
brew install wrk
```

### ab 未安装

```bash
# Ubuntu/Debian
sudo apt-get install apache2-utils

# macOS
brew install apache2
```

## 高级用法

### 自定义测试脚本

创建自定义的 Lua 脚本用于 wrk：

```lua
-- post.lua
wrk.method = "POST"
wrk.body   = '{"test":"data"}'
wrk.headers["Content-Type"] = "application/json"
```

使用自定义脚本：

```bash
wrk -t4 -c100 -d30s -s post.lua http://127.0.0.1:18081/api
```

### 文件传输性能分析

使用 curl 分析文件传输性能：

```bash
# 测试不同大小文件的传输性能
curl -o /dev/null -s -w "1KB: Status=%{http_code}, Time=%{time_total}s, Speed=%{speed_download} bytes/s\n" http://127.0.0.1:18082/file/small
curl -o /dev/null -s -w "1MB: Status=%{http_code}, Time=%{time_total}s, Speed=%{speed_download} bytes/s\n" http://127.0.0.1:18082/file/large
curl -o /dev/null -s -w "100MB: Status=%{http_code}, Time=%{time_total}s, Speed=%{speed_download} bytes/s\n" http://127.0.0.1:18082/file/xxlarge

# 并发文件传输测试
for i in {1..10}; do curl -o /dev/null -s http://127.0.0.1:18082/file/large & done; wait

# 文件完整性验证
curl -o /tmp/test.bin http://127.0.0.1:18082/file/large
md5sum /tmp/test.bin /home/zhaodi-chen/project/uvhttp/build/public/file_test/large.bin
```

### 持续性能监控

集成到 CI/CD 流程中：

```yaml
# .github/workflows/performance.yml
- name: Run performance tests
  run: |
    cd benchmark
    ./run_benchmarks.sh
    
- name: Run file transfer tests
  run: |
    ./build/dist/bin/benchmark_file_transfer 18082 &
    sleep 5
    curl -o /dev/null -s -w "1MB: %{http_code}, %{time_total}s\n" http://127.0.0.1:18082/file/large
    curl -o /dev/null -s -w "100MB: %{http_code}, %{time_total}s\n" http://127.0.0.1:18082/file/xxlarge
    pkill -f benchmark_file_transfer
    
- name: Check performance regression
  run: |
    python scripts/check_performance.py
```

## 参考资源

- [wrk 文档](https://github.com/wg/wrk)
- [ab 文档](https://httpd.apache.org/docs/2.4/programs/ab.html)
- [UVHTTP 性能基准文档](../docs/dev/PERFORMANCE_BENCHMARK.md)
- [UVHTTP 性能测试标准](../docs/dev/PERFORMANCE_TESTING_STANDARD.md)

## 贡献

欢迎贡献新的测试场景、工具和配置！

## 许可证

MIT License