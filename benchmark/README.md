# UVHTTP 基准性能测试

本目录包含 UVHTTP 的基准性能测试程序，用于测量和验证库的性能特性。

## 测试程序

### 1. performance_allocator

**用途**：测试 UVHTTP 统一内存分配器的性能

**测试内容**：
- 小对象分配（64 字节）
- 中等对象分配（512 字节）
- 大对象分配（4096 字节）
- 混合大小分配

**运行方式**：
```bash
./performance_allocator
```

**预期结果**：
- 验证零开销抽象设计
- 性能与系统分配器相当或更好

### 2. performance_allocator_compare

**用途**：对比系统分配器和 UVHTTP 统一分配器的性能

**测试内容**：
- 系统分配器（malloc/free）性能
- UVHTTP 统一分配器性能
- 不同大小对象的分配/释放性能对比

**运行方式**：
```bash
./performance_allocator_compare
```

**预期结果**：
- UVHTTP 统一分配器使用内联函数编译期优化
- 性能与系统分配器相当，零运行时开销

### 3. test_bitfield

**用途**：测试位字段操作的效率和正确性

**测试内容**：
- 位字段设置和获取
- 位字段批量操作
- 性能基准测试

**运行方式**：
```bash
./test_bitfield
```

### 4. benchmark_rps

**用途**：测试 RPS（每秒请求数）性能

**测试内容**：
- 低并发场景（2 线程 / 10 连接）
- 中等并发场景（4 线程 / 50 连接）
- 高并发场景（8 线程 / 200 连接）
- 极高并发场景（16 线程 / 500 连接）

**运行方式**：
```bash
./benchmark_rps
```

**预期结果**：
- 低并发：≥ 17,000 RPS
- 中等并发：≥ 17,000 RPS
- 高并发：≥ 16,000 RPS

### 5. benchmark_latency

**用途**：测试延迟性能

**测试内容**：
- 平均延迟
- P50、P95、P99、P999 延迟
- 最小/最大延迟

**运行方式**：
```bash
./benchmark_latency
```

**预期结果**：
- 平均延迟 < 15ms
- P99 延迟 < 50ms

### 6. benchmark_connection

**用途**：测试连接管理性能

**测试内容**：
- 连接建立速度
- 连接保持能力
- 连接关闭效率
- Keep-Alive 连接复用

**运行方式**：
```bash
./benchmark_connection
```

**预期结果**：
- 成功率 > 99.9%
- 连接复用率 > 95%

### 7. benchmark_memory

**用途**：测试内存使用情况

**测试内容**：
- 峰值内存使用
- 每请求平均内存
- 内存分配/释放次数
- 内存泄漏检测

**运行方式**：
```bash
./benchmark_memory
```

**预期结果**：
- 每请求平均内存 < 10KB
- 无内存泄漏

### 8. benchmark_comprehensive

**用途**：综合性能测试

**测试内容**：
- RPS 性能
- 延迟性能
- 内存使用
- 错误率
- 综合性能评估

**运行方式**：
```bash
./benchmark_comprehensive
```

**预期结果**：
- RPS ≥ 17,000
- 平均延迟 < 15ms
- 错误率 < 0.1%

## 编译

```bash
# 从项目根目录编译
mkdir build && cd build
cmake ..
make

# 编译基准测试
make performance_allocator
make performance_allocator_compare
make test_bitfield
make benchmark_rps
make benchmark_latency
make benchmark_connection
make benchmark_memory
make benchmark_comprehensive
```

## 运行所有基准测试

```bash
cd build/dist/bin
./performance_allocator
./performance_allocator_compare
./test_bitfield
./benchmark_rps
./benchmark_latency
./benchmark_connection
./benchmark_memory
./benchmark_comprehensive
```

## 性能指标

当前基准测试主要关注：
- **RPS 性能**：每秒处理的请求数
- **延迟性能**：请求处理的延迟分布
- **内存使用**：内存分配和释放效率
- **连接管理**：连接建立和复用效率
- **零开销抽象**：验证编译期优化是否有效

## 性能目标

### RPS 目标
- **低并发**：≥ 17,000 RPS（2 线程 / 10 连接）
- **中等并发**：≥ 17,000 RPS（4 线程 / 50 连接）
- **高并发**：≥ 16,000 RPS（8 线程 / 200 连接）

### 延迟目标
- **平均延迟**：< 15ms
- **P99 延迟**：< 50ms

### 内存目标
- **每请求平均内存**：< 10KB
- **内存泄漏**：0

### 错误率目标
- **错误率**：< 0.1%

## 结果文件

基准测试结果保存在 `results/` 目录下，包含：
- 历史性能数据
- 性能趋势分析
- 回归检测报告

## 使用 wrk 进行性能测试

```bash
# 低并发测试
wrk -t2 -c10 -d10s http://127.0.0.1:18081/

# 中等并发测试
wrk -t4 -c50 -d10s http://127.0.0.1:18081/

# 高并发测试
wrk -t8 -c200 -d10s http://127.0.0.1:18081/

# 极高并发测试
wrk -t16 -c500 -d10s http://127.0.0.1:18081/
```

## 使用 ab 进行性能测试

```bash
# 基本 HTTP 测试
ab -n 100000 -c 100 -k http://127.0.0.1:18081/

# Keep-Alive 测试
ab -n 100000 -c 100 -k http://127.0.0.1:18081/
```

## 添加新的基准测试

1. 在 `benchmark/` 目录下创建新的测试文件
2. 在 `benchmark.cmake` 中添加编译规则
3. 更新本 README.md 文件
4. 运行测试并记录基准结果

## 注意事项

- 基准测试应该在 Release 模式下运行
- 避免在基准测试期间运行其他高负载程序
- 多次运行测试取平均值以获得稳定结果
- 使用相同的硬件配置进行对比测试
- 确保端口 18081-18084 可用

## 性能优化建议

根据基准测试结果，可以考虑以下优化：
1. **RPS 低**：检查连接管理、内存分配
2. **延迟高**：检查事件循环、I/O 操作
3. **内存高**：检查内存泄漏、缓冲区大小
4. **错误率高**：检查错误处理、资源管理

## 相关文档

- [性能基准文档](../docs/dev/PERFORMANCE_BENCHMARK.md)
- [性能测试标准](../docs/dev/PERFORMANCE_TESTING_STANDARD.md)
- [性能优化指南](../docs/guide/performance.md)