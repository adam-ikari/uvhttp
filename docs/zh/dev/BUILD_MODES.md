# UVHTTP 构建模式规范

## 概述

UVHTTP 有三种构建模式。Release 模式用于生产环境和基准测试，Debug 模式用于开发，Coverage 模式用于覆盖率分析。本页说明每种模式改变了什么，以及何时使用。

## 构建模式分类

### 1. Release 模式（生产环境）

Release 模式用于生产部署、性能基准测试和发布构建。所有性能测试程序和所有示例程序都以 Release 模式构建。

必须使用 Release 模式的程序：

- `benchmark_unified`：完整的性能测试服务器，包含所有测试场景。
- `performance_allocator`：测量分配器。
- `performance_allocator_compare`：对比分配器。
- `test_bitfield`：测量位域访问。
- examples/ 下的所有示例程序。

构建选项：

```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

默认优化级别为 `-O2`。构建针对代码大小和速度优化，不保留调试符号，并定义 `NDEBUG`。

Release 模式提供最高吞吐量、最小内存占用和最小二进制体积。

### 2. Debug 模式（开发阶段）

Debug 模式用于单元测试、集成测试、覆盖率分析、调试和日常开发。

可以使用 Debug 模式的程序：

- `uvhttp_unit_tests`：单元测试二进制。
- test/ 下的所有测试程序。

构建选项：

```bash
cmake -DENABLE_DEBUG=ON ..
```

或

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

优化级别为 `-O0`。优化关闭，调试符号完整，`NDEBUG` 未定义，断言保持生效。

Debug 模式提供完整的调试信息，可用于 GDB/LLDB、Valgrind 和 AddressSanitizer。

### 3. Coverage 模式（代码覆盖率）

Coverage 模式用于覆盖率分析和测试质量评估。

构建选项：

```bash
cmake -DENABLE_COVERAGE=ON ..
```

优化级别为 `-O0`。优化关闭，gcov 收集开启，构建生成 `.gcda` 和 `.gcno` 文件。

Coverage 模式报告行覆盖率、分支覆盖率和函数覆盖率。

## 构建模式对比

| 特性 | Release | Debug | Coverage |
|------|---------|-------|----------|
| 优化级别 | `-O2` | `-O0` | `-O0` |
| 调试符号 | 无 | 完整 | 完整 |
| NDEBUG | 启用 | 禁用 | 禁用 |
| 断言 | 禁用 | 启用 | 启用 |
| 性能 | 最高 | 最低 | 最低 |
| 调试能力 | 有限 | 完整 | 完整 |
| 二进制大小 | 最小 | 最大 | 最大 |
| 适用场景 | 生产、性能测试 | 开发、调试 | 覆盖率分析 |

## 编译选项详解

### Release 模式选项

```cmake
# 在 CMakeLists.txt 中定义
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
```

`-O2` 开启能收回成本、又不会明显拖慢编译的优化。`-DNDEBUG` 移除断言。`-ffunction-sections -fdata-sections` 将每个函数和数据项放入独立段，`-Wl,--gc-sections -s` 在链接时丢弃未使用的段并去除符号表。

### Debug 模式选项

```cmake
# 在 CMakeLists.txt 中定义
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
endif()
```

`-g` 生成调试信息，`-O0` 关闭优化。断言和调试代码保留在二进制中。

### Coverage 模式选项

```cmake
# 在 CMakeLists.txt 中定义
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
```

`--coverage` 开启覆盖率收集，`-fprofile-arcs` 记录程序流程图，`-ftest-coverage` 写入覆盖率数据。

## 构建最佳实践

### 1. 性能测试必须使用 Release 模式

Debug 模式的数据会误导。优化关闭时吞吐量可能下降 10-100 倍，因此基准测试必须运行生产配置。

下面的示例用两种方式运行同一测试：

```bash
# 错误：用 Debug 模式运行性能测试
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make benchmark_rps
./dist/bin/benchmark_rps 8080
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
# 结果：~10,000 RPS（不准确）

# 正确：用 Release 模式运行性能测试
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make benchmark_unified
./dist/bin/benchmark_unified 8080
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
# 结果：~20,000+ RPS（准确）
```

### 2. 开发和调试使用 Debug 模式

Debug 模式保留完整的调试信息并保持断言开启，异常状态会在产生它的位置附近暴露。GDB 和 LLDB 都可使用。

```bash
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
```

### 3. 代码覆盖率使用 Coverage 模式

Coverage 模式产生准确的覆盖率数据，并能生成详细的覆盖率报告。

```bash
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
gcovr --html --html-details -o coverage.html
```

### 4. 分离构建目录

每种模式保留一个构建目录：

```bash
# Debug 构建
mkdir build-debug && cd build-debug
cmake -DENABLE_DEBUG=ON ..
make

# Release 构建
mkdir build-release && cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make

# Coverage 构建
mkdir build-coverage && cd build-coverage
cmake -DENABLE_COVERAGE=ON ..
make
```

## CMake 配置规范

### 性能测试程序配置

基准测试目标位于 `benchmark/benchmark.cmake`，必须保持 Release 优化。

```cmake
# benchmark/benchmark.cmake

# 性能测试可执行文件
add_executable(benchmark_rps
    ${CMAKE_SOURCE_DIR}/benchmark/benchmark_rps.c
)
target_link_libraries(benchmark_rps
    uvhttp
    ${UVHTTP_CORE_DEPS}
)

# 确保使用 Release 优化
set_target_properties(benchmark_unified PROPERTIES
    CMAKE_BUILD_TYPE Release
)
```

### 单元测试程序配置

单元测试目标位于 `test/CMakeLists.txt`，可以用 Debug 模式构建。

```cmake
# test/CMakeLists.txt

# 单元测试
add_executable(uvhttp_unit_tests
    ${TEST_SOURCES}
)
target_link_libraries(uvhttp_unit_tests
    uvhttp
    ${UVHTTP_TEST_DEPS}
)

# 可以使用 Debug 模式
if(ENABLE_DEBUG)
    set_target_properties(uvhttp_unit_tests PROPERTIES
        CMAKE_BUILD_TYPE Debug
    )
endif()
```

## 验证构建模式

### 检查当前构建模式

```bash
# 方法 1：查看 CMake 缓存
cat build/CMakeCache.txt | grep CMAKE_BUILD_TYPE

# 方法 2：使用 cmake 命令
cd build
cmake -L | grep CMAKE_BUILD_TYPE

# 方法 3：检查二进制文件
file build/dist/bin/benchmark_rps
```

### 验证优化级别

构建日志显示编译器实际收到的优化标志：

```bash
# 检查编译选项
make VERBOSE=1 | grep -E "\-O[0-3]"

# 检查符号表
nm build/dist/bin/benchmark_unified | wc -l
# Release 模式：符号表很小
# Debug 模式：符号表很大
```

## 常见问题

### Q1：为什么性能测试结果不稳定？

大多数情况由四个原因造成：

1. 二进制以 Debug 模式编译。
2. 机器负载过高。
3. 网络是瓶颈。
4. 端口已被占用。

**解决方案**：

```bash
# 确保使用 Release 模式
cmake -DCMAKE_BUILD_TYPE=Release ..
make clean && make

# 使用空闲端口
./dist/bin/benchmark_unified 18082

# 多次测试取平均值
for i in {1..5}; do
    wrk -t4 -c100 -d10s http://127.0.0.1:18082/
done
```

### Q2：如何在 Release 模式下调试？

Release 模式可以携带调试信息：

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      -DCMAKE_CXX_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      ..
```

### Q3：什么时候使用 `-O3` 优化？

`-O3` 通常是一笔不划算的交易：

1. 它可能增大代码体积。
2. 它可能引入不稳定的优化。
3. `-O2` 已经很好地平衡了速度和体积。

如果需要最后一点性能，可以尝试：

```bash
cmake -DCMAKE_C_FLAGS_RELEASE="-O3 -DNDEBUG" \
      -DCMAKE_CXX_FLAGS_RELEASE="-O3 -DNDEBUG" \
      ..
```

## 性能基准

### 预期性能指标（Release 模式）

| 测试类型 | 预期 RPS | 预期延迟 |
|----------|----------|----------|
| 简单 GET | 20,000+ | < 5ms |
| JSON 响应 | 15,000+ | < 10ms |
| 静态文件 | 10,000+ | < 20ms |
| 路由查找 | 600,000+ | < 2μs |

### Debug 模式性能下降

Debug 模式通常比 Release 模式慢 10-100 倍。因此：

- 不要把 Debug 模式的性能数据当作基准。
- 不要基于 Debug 模式的性能做优化决策。
- 在 Release 模式下运行性能测试。
- 只在同一构建模式内比较数字。

## 总结

**关键原则**：

1. **性能测试必须使用 Release 模式**——否则数据不准确。
2. **开发和调试使用 Debug 模式**——便于排查问题。
3. **覆盖率分析使用 Coverage 模式**——数据准确。
4. **分离构建目录**——避免构建模式混淆。
5. **验证构建模式**——确保使用正确配置。

**命令速查**：

```bash
# Release 构建（性能测试）
cmake -DCMAKE_BUILD_TYPE=Release ..

# Debug 构建（开发调试）
cmake -DENABLE_DEBUG=ON ..

# Coverage 构建（覆盖率分析）
cmake -DENABLE_COVERAGE=ON ..

# 检查构建模式
cat build/CMakeCache.txt | grep CMAKE_BUILD_TYPE
```

## 参考资料

- [CMake 构建模式文档](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)
- [GCC 优化选项](https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html)
- [贡献者指南](../guide/DEVELOPER_GUIDE.md)
- [性能测试标准](PERFORMANCE_TESTING_STANDARD.md)
