# UVHTTP 构建模式规范

## 概述

本文档定义了 UVHTTP 项目的构建模式规范，明确哪些程序必须使用 Release 模式编译，哪些程序可以使用 Debug 模式，以及相应的编译选项和最佳实践。

## 构建模式分类

### 1. Release 模式（生产环境）

**适用场景**：
- 所有性能测试程序
- 生产环境部署
- 性能基准测试
- 发布版本

**必须使用 Release 模式的程序**：
- `benchmark_rps` - RPS 性能测试服务器
- `benchmark_latency` - 延迟测试
- `benchmark_connection` - 连接性能测试
- `benchmark_memory` - 内存性能测试
- `benchmark_comprehensive` - 综合性能测试
- `benchmark_file_transfer` - 文件传输性能测试
- `benchmark_router` - 路由性能测试
- `benchmark_router_simple` - 简化路由性能测试
- `benchmark_router_comparison` - 路由优化对比测试
- `benchmark_router_simple_comparison` - 简化路由优化对比测试
- `benchmark_router_minimal` - 最小化路由性能测试
- `performance_allocator` - 分配器性能测试
- `performance_allocator_compare` - 分配器对比测试
- 所有示例程序（examples/）

**编译选项**：
```bash
cmake -DCMAKE_BUILD_TYPE=Release ..
```

**优化级别**：`-O2`（默认）
- 代码大小优化
- 性能优化
- 无调试符号
- 启用 `NDEBUG` 宏

**性能特征**：
- 最高性能
- 最小内存占用
- 最小二进制大小

### 2. Debug 模式（开发调试）

**适用场景**：
- 单元测试
- 集成测试
- 代码覆盖率分析
- 调试和问题排查
- 开发阶段

**可以使用 Debug 模式的程序**：
- `uvhttp_unit_tests` - 单元测试
- 所有测试程序（test/）

**编译选项**：
```bash
cmake -DENABLE_DEBUG=ON ..
```

或

```bash
cmake -DCMAKE_BUILD_TYPE=Debug ..
```

**优化级别**：`-O0`
- 无优化
- 完整调试符号
- 禁用 `NDEBUG` 宏
- 启用断言

**调试特征**：
- 完整的调试信息
- 支持 GDB/LLDB 调试
- 支持 Valgrind 内存检查
- 支持 AddressSanitizer

### 3. Coverage 模式（代码覆盖率）

**适用场景**：
- 代码覆盖率分析
- 测试质量评估

**编译选项**：
```bash
cmake -DENABLE_COVERAGE=ON ..
```

**优化级别**：`-O0`
- 无优化
- 启用 gcov 覆盖率收集
- 生成 `.gcda` 和 `.gcno` 文件

**覆盖率特征**：
- 支持行覆盖率
- 支持分支覆盖率
- 支持函数覆盖率

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
# CMakeLists.txt 中定义
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG" CACHE STRING "" FORCE)
```

**说明**：
- `-O2`：启用高级优化，平衡性能和编译时间
- `-DNDEBUG`：禁用断言，提升性能
- `-ffunction-sections -fdata-sections`：函数和数据分段
- `-Wl,--gc-sections -s`：链接时移除未使用的段，去除符号表

### Debug 模式选项

```cmake
# CMakeLists.txt 中定义
if(ENABLE_DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0")
endif()
```

**说明**：
- `-g`：生成调试信息
- `-O0`：禁用所有优化
- 保留所有断言和调试代码

### Coverage 模式选项

```cmake
# CMakeLists.txt 中定义
if(ENABLE_COVERAGE)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage -fprofile-arcs -ftest-coverage")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --coverage")
endif()
```

**说明**：
- `--coverage`：启用代码覆盖率收集
- `-fprofile-arcs`：生成程序流图
- `-ftest-coverage`：生成覆盖率数据

## 构建最佳实践

### 1. 性能测试必须使用 Release 模式

**原因**：
- Debug 模式的性能数据不具代表性
- Debug 模式禁用所有优化，性能可能低 10-100 倍
- 性能测试需要在生产环境配置下进行

**示例**：
```bash
# ❌ 错误：使用 Debug 模式运行性能测试
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make benchmark_rps
./dist/bin/benchmark_rps 8080
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
# 结果：~10,000 RPS（不准确）

# ✅ 正确：使用 Release 模式运行性能测试
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make benchmark_rps
./dist/bin/benchmark_rps 8080
wrk -t4 -c100 -d30s http://127.0.0.1:8080/
# 结果：~20,000+ RPS（准确）
```

### 2. 开发和调试使用 Debug 模式

**原因**：
- 完整的调试信息
- 启用断言，及早发现问题
- 便于使用 GDB/LLDB 调试

**示例**：
```bash
mkdir build && cd build
cmake -DENABLE_DEBUG=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
```

### 3. 代码覆盖率使用 Coverage 模式

**原因**：
- 准确的覆盖率数据
- 支持生成详细的覆盖率报告

**示例**：
```bash
mkdir build && cd build
cmake -DENABLE_COVERAGE=ON ..
make uvhttp_unit_tests
./dist/bin/uvhttp_unit_tests
gcovr --html --html-details -o coverage.html
```

### 4. 分离构建目录

**推荐做法**：
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

所有性能测试程序应在 `benchmark/benchmark.cmake` 中配置，并确保使用 Release 模式优化。

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
set_target_properties(benchmark_rps PROPERTIES
    CMAKE_BUILD_TYPE Release
)
```

### 单元测试程序配置

单元测试程序应在 `test/CMakeLists.txt` 中配置，可以使用 Debug 模式。

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

```bash
# 检查编译选项
make VERBOSE=1 | grep -E "\-O[0-3]"

# 检查符号表
nm build/dist/bin/benchmark_rps | wc -l
# Release 模式：符号表很小
# Debug 模式：符号表很大
```

## 常见问题

### Q1: 为什么性能测试结果不稳定？

**A**: 可能原因：
1. 使用了 Debug 模式编译
2. 系统负载过高
3. 网络瓶颈
4. 端口占用

**解决方案**：
```bash
# 确保使用 Release 模式
cmake -DCMAKE_BUILD_TYPE=Release ..
make clean && make

# 使用空闲端口
./dist/bin/benchmark_rps 18082

# 多次测试取平均值
for i in {1..5}; do
    wrk -t4 -c100 -d10s http://127.0.0.1:18082/
done
```

### Q2: 如何在 Release 模式下调试？

**A**: 可以在 Release 模式下添加调试信息：

```bash
cmake -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      -DCMAKE_CXX_FLAGS_RELEASE="-O2 -g -DNDEBUG" \
      ..
```

### Q3: 什么时候使用 `-O3` 优化？

**A**: 一般不推荐使用 `-O3`，原因：
1. 可能增加代码大小
2. 可能引入不稳定的优化
3. `-O2` 已经提供了很好的性能平衡

如果需要极致性能，可以尝试：
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

Debug 模式的性能通常比 Release 模式低 **10-100 倍**，因此：

- ❌ 不要使用 Debug 模式的性能数据作为基准
- ❌ 不要基于 Debug 模式的性能做优化决策
- ✅ 性能测试必须使用 Release 模式
- ✅ 性能对比必须在相同构建模式下进行

## 总结

**关键原则**：
1. **性能测试必须使用 Release 模式** - 否则数据不准确
2. **开发和调试使用 Debug 模式** - 便于问题排查
3. **覆盖率分析使用 Coverage 模式** - 准确的覆盖率数据
4. **分离构建目录** - 避免构建模式混淆
5. **验证构建模式** - 确保使用了正确的配置

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
- [开发者指南](../guide/DEVELOPER_GUIDE.md)
- [性能测试标准](PERFORMANCE_TESTING_STANDARD.md)