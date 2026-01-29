# UVHTTP 基准性能测试流程

本指南说明如何使用基准性能测试程序进行性能测试和分析。

## 快速开始

### 1. 自动化测试（推荐）

使用自动化脚本运行所有基准测试：

```bash
cd benchmark
./run_benchmarks.sh
```

这个脚本会：
- 检查依赖（wrk、ab）
- 编译所有基准测试程序
- 运行所有基准测试
- 收集测试结果
- 生成测试报告

### 2. 手动测试

如果需要手动运行特定测试：

```bash
# 编译
cd build
make benchmark_rps benchmark_latency benchmark_connection benchmark_memory benchmark_comprehensive

# 运行单个测试
./dist/bin/benchmark_rps &
wrk -t4 -c100 -d10s http://127.0.0.1:18081/

# 分析结果
cd ../benchmark
python3 analyze_results.py
```

## 测试流程

### 阶段 1: 准备

#### 1.1 环境准备

确保系统满足以下要求：
- Linux 操作系统
- GCC 编译器
- Python 3.6+
- wrk（可选，用于 HTTP 性能测试）
- ab（可选，用于 HTTP 性能测试）

#### 1.2 编译项目

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

#### 1.3 编译基准测试

```bash
make benchmark_rps benchmark_latency benchmark_connection benchmark_memory benchmark_comprehensive
```

### 阶段 2: 运行测试

#### 2.1 运行 RPS 测试

```bash
./dist/bin/benchmark_rps &
wrk -t2 -c10 -d10s http://127.0.0.1:18081/  # 低并发
wrk -t4 -c50 -d10s http://127.0.0.1:18081/  # 中等并发
wrk -t8 -c200 -d10s http://127.0.0.1:18081/ # 高并发
```

#### 2.2 运行延迟测试

```bash
./dist/bin/benchmark_latency &
wrk -t4 -c100 -d10s http://127.0.0.1:18081/
```

#### 2.3 运行连接测试

```bash
./dist/bin/benchmark_connection &
ab -n 100000 -c 100 -k http://127.0.0.1:18082/
```

#### 2.4 运行内存测试

```bash
./dist/bin/benchmark_memory &
wrk -t4 -c100 -d10s http://127.0.0.1:18083/
```

#### 2.5 运行综合测试

```bash
./dist/bin/benchmark_comprehensive &
wrk -t4 -c100 -d10s http://127.0.0.1:18084/
```

### 阶段 3: 分析结果

#### 3.1 使用分析脚本

```bash
cd benchmark
python3 analyze_results.py
```

这个脚本会：
- 加载所有测试结果
- 分析性能指标
- 生成分析报告
- 提供改进建议

#### 3.2 查看结果

测试结果保存在 `benchmark/results/run_<timestamp>/` 目录下：
- `summary_report.md` - 测试摘要报告
- `analysis_report.md` - 性能分析报告
- `*.txt` - 详细测试日志
- `*.csv` - RPS 数据

## 测试端口

| 测试程序 | 端口 |
|---------|------|
| benchmark_rps | 18081 |
| benchmark_latency | 18081 |
| benchmark_connection | 18082 |
| benchmark_memory | 18083 |
| benchmark_comprehensive | 18084 |

## 性能目标

根据性能基准文档，UVHTTP 应该达到以下性能目标：

### RPS 目标
- **低并发**（2 线程 / 10 连接）: ≥ 17,000 RPS
- **中等并发**（4 纥程 / 50 连接）: ≥ 17,000 RPS
- **高并发**（8 线程 / 200 连接）: ≥ 16,000 RPS

### 延迟目标
- **平均延迟**: < 15ms
- **P99 延迟**: < 50ms

### 内存目标
- **每请求平均内存**: < 10KB
- **内存泄漏**: 0

### 错误率目标
- **错误率**: < 0.1%

## 测试场景

### 1. 基础性能测试

测试基本的 HTTP 请求处理性能：

```bash
./run_benchmarks.sh
```

### 2. 压力测试

测试高负载下的性能表现：

```bash
# 启动服务器
./dist/bin/benchmark_comprehensive &

# 高并发压力测试
wrk -t16 -c500 -d30s http://127.0.0.1:18084/

# 持续负载测试
wrk -t8 -c200 -d300s http://127.0.0.1:18084/
```

### 3. 内存泄漏测试

长时间运行测试，检查内存泄漏：

```bash
# 启动服务器
./dist/bin/benchmark_memory &

# 发送大量请求
wrk -t4 -c100 -d600s http://127.0.0.1:18083/

# 检查内存使用
top -p $(pgrep benchmark_memory)
```

### 4. 回归测试

对比不同版本的性能：

```bash
# 测试当前版本
./run_benchmarks.sh
mv results/run_* results/current/

# 切换到旧版本
git checkout <old-version>
make benchmark_*
./run_benchmarks.sh
mv results/run_* results/old/

# 对比结果
python3 analyze_results.py results/current/
python3 analyze_results.py results/old/
```

## 结果分析

### 1. 查看 RPS 结果

RPS 结果保存在 CSV 文件中：

```bash
cat results/run_*/benchmark_rps.rps.csv
```

格式：
```
test_name,rps
benchmark_rps_2t_10c,17500.5
benchmark_rps_4t_50c,17200.3
benchmark_rps_8t_200c,16600.8
```

### 2. 查看延迟统计

延迟统计包含在服务器日志中：

```bash
cat results/run_*/benchmark_latency.server.log | grep "延迟统计"
```

### 3. 查看综合评估

综合评估在分析报告中：

```bash
cat results/run_*/analysis_report.md
```

## 常见问题

### 1. 端口被占用

如果端口被占用，修改基准测试程序中的端口号：

```c
#define PORT 18085  // 修改为其他端口
```

### 2. wrk 或 ab 未安装

安装性能测试工具：

```bash
# Ubuntu/Debian
sudo apt-get install wrk apache2-utils

# macOS
brew install wrk
```

### 3. 测试结果不稳定

多次运行测试取平均值：

```bash
for i in {1..5}; do
    ./run_benchmarks.sh
done
```

### 4. 内存泄漏

使用 Valgrind 检查内存泄漏：

```bash
valgrind --leak-check=full ./dist/bin/benchmark_memory
```

## CI/CD 集成

基准测试可以集成到 CI/CD 流程中：

```yaml
# .github/workflows/benchmark.yml
name: Benchmark Tests

on:
  push:
    branches: [develop, main]
  schedule:
    - cron: '0 2 * * *'  # 每天凌晨 2 点运行

jobs:
  benchmark:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Setup
        run: |
          sudo apt-get update
          sudo apt-get install -y wrk apache2-utils python3
      
      - name: Build
        run: |
          mkdir build && cd build
          cmake ..
          make -j$(nproc) benchmark_*
      
      - name: Run Benchmarks
        run: |
          cd benchmark
          ./run_benchmarks.sh
      
      - name: Analyze Results
        run: |
          cd benchmark
          python3 analyze_results.py
      
      - name: Upload Results
        uses: actions/upload-artifact@v2
        with:
          name: benchmark-results
          path: benchmark/results/
```

## 最佳实践

### 1. 测试环境一致性

- 使用相同的硬件配置
- 关闭其他高负载程序
- 确保系统资源充足

### 2. 多次运行取平均

每个测试运行多次，取平均值以获得稳定结果：

```bash
for i in {1..5}; do
    echo "Run $i:"
    ./run_benchmarks.sh
done
```

### 3. 性能回归检测

定期运行基准测试，检测性能回归：

```bash
# 创建基线
./run_benchmarks.sh
cp results/run_* results/baseline/

# 后续测试
./run_benchmarks.sh
python3 compare_results.py results/baseline/ results/run_*/
```

### 4. 性能优化验证

在优化前后运行基准测试，验证优化效果：

```bash
# 优化前
git checkout before-optimization
./run_benchmarks.sh

# 优化后
git checkout after-optimization
./run_benchmarks.sh

# 对比结果
```

## 相关文档

- [性能基准文档](../docs/dev/PERFORMANCE_BENCHMARK.md)
- [性能测试标准](../docs/dev/PERFORMANCE_TESTING_STANDARD.md)
- [性能优化指南](../docs/guide/performance.md)
- [README.md](README.md)