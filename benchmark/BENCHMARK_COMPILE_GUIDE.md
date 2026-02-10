# Benchmark 编译说明

## 重要提醒

⚠️ **性能测试必须使用 Release 模式编译！**

## 正确的编译方式

### 方式 1：编译时指定 Release 模式（推荐）

```bash
# 清理旧的构建文件
rm -rf build

# 使用 Release 模式配置
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=OFF .

# 编译性能测试程序
make benchmark_unified

# 运行性能测试
./dist/bin/benchmark_unified 18081

# 在另一个终端运行 wrk 测试
wrk -t4 -c100 -d30s http://127.0.0.1:18081/
```

### 方式 2：使用 ccmake 交互式配置

```bash
# 清理旧的构建文件
rm -rf build

# 启动 ccmake 配置界面
ccmake .

# 按以下步骤操作：
# 1. 按 'c' 键配置选项
# 2. 将 CMAKE_BUILD_TYPE 设置为 Release
# 3. 将 ENABLE_COVERAGE 设置为 OFF
# 4. 按 'c' 键生成配置
# 5. 按 'g' 生成 Makefile 并退出
# 6. 编译：make benchmark_unified
```

## 错误的编译方式（❌ 不要使用）

```bash
# ❌ 错误 1：使用 Debug 模式
cmake -DCMAKE_BUILD_TYPE=Debug .
# 问题：Debug 模式禁用编译器优化，性能会低 50-70%

# ❌ 错误 2：启用代码覆盖率
cmake -DENABLE_COVERAGE=ON .
# 问题：覆盖率检测会显著降低性能

# ❌ 错误：混合使用 Debug 和 Release 模式
# 这会导致不一致的性能数据
```

## 编译模式对性能的影响

| 编译模式 | 优化级别 | 性能影响 | 适用场景 |
|---------|---------|----------|----------|
| **Release** | `-O2` | 100% (基准) | ✅ 性能测试、生产部署 |
| Debug | `-g -O0` | 30-50% | ❌ 性能测试、开发调试 |
| Debug + Coverage | `-g -O0` + 覆盖率检测 | 20-30% | ❌ 性能测试 |
| Release + ASan | `-O2` + 地址消毒 | 50-70% | ❌ 性能测试 |

## 验证编译模式

编译完成后，可以使用以下命令验证：

```bash
# 检查编译的二进制文件
file dist/bin/benchmark_unified

# Release 模式输出示例：
# benchmark_unified: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, ...

# Debug 模式输出示例（注意包含 "stripped"）：
# benchmark_unified: ELF 64-bit LSB executable, x86-64, version 1 (SYSV), dynamically linked, stripped, ...

# 检查 CMake 配置
grep CMAKE_BUILD_TYPE CMakeCache.txt
```

## 性能测试脚本

### 快速测试脚本

创建 `benchmark.sh`：

```bash
#!/bin/bash
set -e

echo "=== UVHTTP 性能测试 ==="
echo "编译模式：Release（必须使用 Release 模式）"

# 清理旧的构建
rm -rf build

# Release 模式配置
cmake -DCMAKE_BUILD_TYPE=Release -DENABLE_COVERAGE=OFF .

# 编译性能测试
make benchmark_unified

echo ""
echo "=== 启动性能测试服务器 ==="
./dist/bin/benchmark_unified 18081 &
SERVER_PID=$!

sleep 2

echo ""
echo "=== 运行 wrk 性能测试 ==="
wrk -t4 -c100 -d30s http://127.0.0.1:18081/

# 停止服务器
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true

echo ""
echo "=== 性能测试完成 ==="
```

使用方法：

```bash
chmod +x benchmark.sh
./benchmark.sh
```

## 常见问题

### Q: 为什么 Debug 模式下性能很低？

A: Debug 模式禁用了编译器优化（`-O0`），导致性能严重下降。性能测试必须使用 Release 模式。

### Q: 什么时候应该使用 Debug 模式？

A: Debug 模式仅用于：
- 开发调试
- 单元测试
- 代码覆盖率测试

### Q: 如何确保性能测试使用 Release 模式？

A: 编译前检查 `CMAKE_BUILD_TYPE` 配置：

```bash
grep CMAKE_BUILD_TYPE CMakeCache.txt
# 应该输出: CMAKE_BUILD_TYPE:STRING=Release
```

### Q: 可以在同一个构建中切换模式吗？

A: 不建议。每次切换模式都应该清理构建目录：

```bash
# 切换到 Release 模式
rm -rf build
cmake -DCMAKE_BUILD_TYPE=Release .
make benchmark_unified

# 切换到 Debug 模式
rm -rf build
cmake -DCMAKE_BUILD_TYPE=Debug .
make benchmark_unified
```

## 性能基准参考

在 Release 模式下的预期性能：

| 测试场景 | 预期 RPS | 平均延迟 |
|---------|---------|---------|
| 高并发（4 线程 / 100 连接） | 25,000 - 35,000 | 2-5ms |
| 中等并发（4 纓程 / 50 连接） | 20,000 - 25,000 | 2-5ms |
| 低并发（2 纯程 / 10 连接） | 15,000 - 20,000 | 1-3ms |

如果性能显著低于预期，请检查：
1. 是否使用了 Release 模式
2. 是否启用了代码覆盖率
3. 是否启用了 ASan/TSan 等消毒器
4. 系统负载是否过高