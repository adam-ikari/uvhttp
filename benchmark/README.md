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
```

## 运行所有基准测试

```bash
cd build/dist/bin
./performance_allocator
./performance_allocator_compare
./test_bitfield
```

## 性能指标

当前基准测试主要关注：
- **内存分配性能**：小对象、中对象、大对象的分配/释放速度
- **零开销抽象**：验证编译期优化是否有效
- **位字段操作**：验证位操作的正确性和性能

## 结果文件

基准测试结果保存在 `results/` 目录下，包含：
- 历史性能数据
- 性能趋势分析
- 回归检测报告

## 性能目标

- **内存分配**：与系统分配器相当或更好
- **零开销抽象**：Release 模式下完全零开销
- **编译期优化**：所有抽象都应该被优化掉

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