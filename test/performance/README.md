# UVHTTP 性能测试指南

本目录包含 UVHTTP 的标准化性能测试工具和配置。

## 目录结构

```
test/performance/
├── performance_test_config.sh          # 性能测试配置文件
├── run_standard_performance_test.sh    # 标准性能测试脚本
├── check_performance_regression.sh     # 性能回归检查脚本
└── README.md                           # 本文档
```

## 快速开始

### 1. 编译项目

```bash
cd /home/zhaodi-chen/project/uvhttp
mkdir build && cd build
cmake ..
make -j$(nproc)
```

### 2. 运行标准性能测试

```bash
cd /home/zhaodi-chen/project/uvhttp
chmod +x test/performance/run_standard_performance_test.sh
./test/performance/run_standard_performance_test.sh
```

### 3. 检查性能回归

```bash
cd /home/zhaodi-chen/project/uvhttp
chmod +x test/performance/check_performance_regression.sh
./test/performance/check_performance_regression.sh
```

## 测试配置

### 标准测试配置

- **线程数**: 4
- **并发连接**: 100
- **测试时长**: 30 秒

### 测试场景

| 场景 | 线程数 | 连接数 | 时长 |
|------|--------|--------|------|
| 低并发 | 2 | 10 | 30 秒 |
| 中等并发 | 4 | 50 | 30 秒 |
| 高并发 | 4 | 100 | 30 秒 |
| 超高并发 | 8 | 200 | 30 秒 |

### 测试文件

| 文件 | 路径 | 大小 |
|------|------|------|
| 主页 | `/` | HTML |
| 小文件 | `/static/index.html` | 12B |
| 中等文件 | `/static/medium.html` | 10KB |
| 大文件 | `/static/large.html` | 100KB |

## 性能基准值

以下是基于实际测试验证的性能基准值（中等并发场景）：

### 主页性能

- **RPS 目标**: 9,500
- **RPS 最小**: 8,000
- **平均延迟最大**: 15ms
- **P99 延迟最大**: 30ms

### 小文件性能

- **RPS 目标**: 7,000
- **RPS 最小**: 6,000
- **平均延迟最大**: 20ms
- **P99 延迟最大**: 50ms

### 中等文件性能

- **RPS 目标**: 4,500
- **RPS 最小**: 4,000
- **平均延迟最大**: 50ms
- **P99 延迟最大**: 100ms

### 大文件性能

- **RPS 目标**: 4,600
- **RPS 最小**: 4,000
- **平均延迟最大**: 30ms
- **P99 延迟最大**: 60ms

## 性能回归阈值

### 吞吐量回归

- **警告**: 下降 5%
- **失败**: 下降 10%

### 延迟回归

- **警告**: 增加 10%
- **失败**: 增加 20%

## 测试结果

### 结果目录

测试结果保存在 `test/performance/results/run_<timestamp>/` 目录中。

### 结果文件

每个测试生成两个文件：

1. **文本格式**: `<test_name>.txt` - 完整的 wrk 输出
2. **JSON 格式**: `<test_name>.json` - 结构化的性能指标

### 汇总报告

测试完成后会生成 `summary_report.txt`，包含所有测试结果的汇总。

## 使用示例

### 运行完整测试流程

```bash
# 1. 编译项目
cd build && make -j$(nproc)

# 2. 运行性能测试
cd ..
./test/performance/run_standard_performance_test.sh

# 3. 检查性能回归
./test/performance/check_performance_regression.sh
```

### 自定义测试配置

```bash
# 自定义端口
export UVHTTP_PORT=8081

# 自定义测试时长
export STANDARD_DURATION=60

# 运行测试
./test/performance/run_standard_performance_test.sh
```

## CI/CD 集成

### 在 CI/CD 中运行性能测试

```yaml
# .github/workflows/nightly.yml
- name: Run performance tests
  run: |
    cd build
    cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_EXAMPLES=ON ..
    make -j$(nproc)
    cd ..
    ./test/performance/run_standard_performance_test.sh

- name: Check performance regression
  run: |
    ./test/performance/check_performance_regression.sh
```

## 性能优化建议

1. **减少延迟波动**: 优化事件循环处理
2. **提高小文件 RPS**: 优化小文件处理路径
3. **处理超时错误**: 调查中等文件传输超时问题
4. **缓存优化**: 为频繁访问的小文件启用内容缓存

## 注意事项

1. **测试环境**: 性能测试结果受系统负载、网络状况等影响
2. **测试工具**: 必须使用 CMake 编译的程序
3. **测试文件**: 确保测试文件存在且可访问
4. **测试时长**: 每个测试至少运行 30 秒以获得稳定结果
5. **预热**: 测试前会自动预热服务器，确保稳定性能

## 故障排除

### 端口被占用

```bash
# 检查端口占用
lsof -i :8080

# 停止占用进程
pkill -9 -f performance_static_server
```

### wrk 未安装

```bash
# Ubuntu/Debian
sudo apt-get install wrk

# macOS
brew install wrk
```

### 测试文件不存在

```bash
# 创建测试文件
mkdir -p public/static
echo "<html><body>Test</body></html>" > public/static/index.html
dd if=/dev/zero of=public/static/medium.html bs=1024 count=10
dd if=/dev/zero of=public/static/large.html bs=1024 count=100
```

## 参考文档

- [性能基准测试](../../docs/PERFORMANCE_BENCHMARK.md)
- [性能测试标准](../../docs/PERFORMANCE_TESTING_STANDARD.md)
- [静态文件服务](../../docs/STATIC_FILE_SERVER.md)

---

**最后更新**: 2026-01-22
**维护者**: UVHTTP Team