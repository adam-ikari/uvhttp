# UVHTTP 压力测试套件

本文档描述了UVHTTP服务器的全面压力测试套件，用于评估服务器在各种负载条件下的性能和稳定性。

## 测试组件

### 1. 并发连接压力测试 (`test_stress_concurrent.c`)

**目的**: 测试服务器在处理大量并发连接时的性能表现

**测试内容**:
- 渐进式连接增长测试 (100 → 1000 连接)
- 极限负载测试 (2000+ 并发连接)
- 连接建立和关闭的延迟统计
- 内存使用监控
- 错误率统计

**关键指标**:
- 最大并发连接数
- 连接建立延迟 (P50, P95, P99)
- 内存使用峰值
- 连接成功率

### 2. 请求吞吐量测试 (`test_throughput_rps.c`)

**目的**: 测量服务器在不同负载下的每秒请求数(RPS)

**测试内容**:
- 目标RPS达成测试 (1K, 5K, 10K, 20K RPS)
- 负载大小影响测试 (1B, 1KB, 10KB, 100KB)
- 并发度影响测试 (1, 10, 50, 100 并发)
- 吞吐量稳定性测试

**关键指标**:
- 实际RPS vs 目标RPS
- 响应时间分布
- 错误率
- 资源利用率

### 3. 内存泄漏压力测试 (`test_memory_leak.c`)

**目的**: 检测服务器在长时间运行下的内存泄漏问题

**测试内容**:
- 长时间运行测试 (1小时+)
- 周期性内存使用监控
- 垃圾回收模拟
- 内存增长趋势分析
- 资源清理验证

**关键指标**:
- 内存使用增长率
- 垃圾回收效果
- 长期稳定性
- 内存泄漏检测

### 4. 边界条件测试 (`test_boundary_conditions.c`)

**目的**: 测试服务器在极限条件下的行为和稳定性

**测试内容**:
- 最大连接数限制测试
- 超大请求处理测试
- 超大响应生成测试
- 系统资源耗尽场景
- 异常恢复能力测试

**关键指标**:
- 系统极限边界
- 错误处理能力
- 恢复时间
- 资源保护机制

### 5. 性能基准测试 (`test_performance_benchmark.c`)

**目的**: 建立系统基础性能基准，用于性能回归检测

**测试内容**:
- 系统调用延迟基准
- 内存分配性能基准
- 字符串处理性能
- 线程创建开销
- 文件I/O性能
- 网络连接开销
- 内存带宽测试
- CPU计算性能

**关键指标**:
- 各项操作的平均延迟
- 延迟分布 (P50, P95, P99)
- 性能标准差
- 与历史基准对比

## 使用方法

### 快速开始

```bash
# 运行完整压力测试套件
./run_stress_tests.sh
```

### 单独运行测试

```bash
# 编译测试
gcc -o test_stress_concurrent test_stress_concurrent.c -lpthread
gcc -o test_throughput_rps test_throughput_rps.c -lpthread
gcc -o test_memory_leak test_memory_leak.c -lpthread
gcc -o test_boundary_conditions test_boundary_conditions.c -lpthread
gcc -o test_performance_benchmark test_performance_benchmark.c -lm -lpthread

# 运行特定测试
./test_stress_concurrent
./test_throughput_rps
./test_memory_leak
./test_boundary_conditions
./test_performance_benchmark
```

## 测试结果解读

### 性能指标说明

- **P50/P95/P99**: 50%/95%/99%的请求/操作在此时间内完成
- **RPS**: 每秒请求数 (Requests Per Second)
- **内存增长率**: 每小时内存增长的KB数
- **错误率**: 失败请求占总请求的百分比
- **标准差**: 性能指标的稳定性度量

### 性能基准

基于测试环境建立的基准值：

| 测试项目 | 基准值 | 单位 | 说明 |
|---------|--------|------|------|
| 系统调用延迟 | 0.01 | ms | getpid()调用 |
| 内存分配延迟 | 0.05 | ms | 1KB分配 |
| 字符串处理 | 0.1 | ms | 基础字符串操作 |
| 线程创建 | 1.0 | ms | pthread创建 |
| 文件I/O | 0.5 | ms | 小文件读写 |
| 网络连接 | 2.0 | ms | TCP连接建立 |
| 内存带宽 | 1000.0 | MB/s | 内存复制带宽 |
| CPU计算 | 1000.0 | 质数/秒 | 质数计算性能 |

### 性能评估标准

- **优秀**: 实际性能 > 基准值 × 1.5
- **良好**: 基准值 × 1.0 < 实际性能 ≤ 基准值 × 1.5  
- **一般**: 基准值 × 0.8 < 实际性能 ≤ 基准值 × 1.0
- **需要优化**: 实际性能 ≤ 基准值 × 0.8

## 环境要求

### 系统要求
- Linux 操作系统
- 至少 2GB 可用内存
- 多核CPU推荐
- 支持pthread的C编译器

### 编译依赖
- GCC 或兼容的C编译器
- pthread库
- math库 (-lm)

### 运行环境
- 足够的系统资源 (内存、CPU、文件描述符)
- 适当的系统限制 (ulimit设置)
- 网络访问权限 (用于网络测试)

## 故障排除

### 常见问题

1. **编译错误**: 检查是否安装了必要的开发工具和库
2. **内存不足**: 增加系统内存或调整测试参数
3. **文件描述符限制**: 使用 `ulimit -n` 增加限制
4. **测试超时**: 调整测试参数或增加超时时间

### 调试建议

1. 使用 `strace` 跟踪系统调用
2. 使用 `valgrind` 检测内存问题
3. 监控系统资源使用情况
4. 检查系统日志

## 持续集成

建议将压力测试集成到CI/CD流程中：

```yaml
# 示例 CI 配置
stress_test:
  stage: test
  script:
    - ./run_stress_tests.sh
  artifacts:
    reports:
      junit: stress_test_results_*/summary.xml
    paths:
      - stress_test_results_*/
  only:
    - master
    - develop
    - merge_requests
```

## 贡献指南

在添加新的压力测试时，请：

1. 遵循现有代码风格和命名约定
2. 添加适当的错误处理和资源清理
3. 包含详细的性能统计输出
4. 更新本文档
5. 添加相应的测试用例

## 许可证

本压力测试套件遵循与UVHTTP项目相同的许可证。

---

**注意**: 压力测试可能会消耗大量系统资源，请在测试环境中运行，避免在生产环境执行。