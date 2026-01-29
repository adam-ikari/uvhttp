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

**可用端点：**
- `GET  /` - 简单文本响应（13 bytes）
- `GET  /json` - JSON 响应（50 bytes）
- `POST /api` - POST 请求处理（23 bytes）
- `GET  /small` - 小响应（1KB）
- `GET  /medium` - 中等响应（10KB）
- `GET  /large` - 大响应（100KB）
- `GET  /health` - 健康检查（22 bytes）

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

### 2. 不同响应大小测试

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

### 持续性能监控

集成到 CI/CD 流程中：

```yaml
# .github/workflows/performance.yml
- name: Run performance tests
  run: |
    cd benchmark
    ./run_benchmarks.sh
    
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