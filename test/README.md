# 测试目录

本目录包含 UVHTTP 项目的所有测试文件和测试脚本。

## 目录结构

```
test/
├── certs/                          # TLS 测试证书
├── cmake_build/                    # CMake 构建测试
├── config/                         # 测试配置文件和文档
│   ├── config_invalid.conf         # 无效配置示例
│   ├── config_test.conf            # 测试配置
│   ├── config_valid.conf           # 有效配置示例
│   ├── PERFORMANCE_COMPARISON_REPORT.md # 性能对比综合报告
│   └── README.md                   # 测试说明文档
├── integration/                    # 集成测试
│   ├── test_include.c              # 包含测试
│   ├── test_no_router.c            # 无路由测试
│   ├── test_route.c                # 路由测试
│   ├── test_simple.c               # 简单测试
│   ├── test_static/                # 静态文件测试目录
│   ├── test_static_middleware.c    # 静态文件中间件测试
│   ├── test_websocket_callback.c   # WebSocket 回调测试
│   ├── test_websocket_integration.c # WebSocket 集成测试
│   └── websocket_test.html         # WebSocket 测试页面
├── performance/                    # 性能测试
│   ├── performance_allocator.c     # 分配器性能测试
│   ├── performance_allocator_compare.c # 分配器性能对比
│   ├── performance_hierarchical_allocator.c # 层次化分配器测试
│   └── test_bitfield.c             # 位域测试
├── results/                        # 测试结果
│   ├── nginx_single_thread_results/ # Nginx 单线程性能测试结果
│   ├── uvhttp_performance_results/  # UVHTTP 性能测试结果
│   └── uvhttp_single_thread_results/ # UVHTTP 单线程性能测试结果
├── scripts/                        # 测试脚本
│   ├── generate_test_certs.sh      # 生成测试证书脚本
│   ├── run_performance_tests.sh    # 性能测试脚本
│   ├── run_rate_limit_tests.sh     # 限流测试脚本
│   └── run_uvhttp_performance_local.sh # UVHTTP 本地性能测试脚本
└── unit/                           # 单元测试
    ├── simple_test.cpp             # Google Test 示例测试
    ├── test_*.c                    # 其他单元测试文件
    └── uvhttp_test_framework.h     # 测试框架头文件
```

## 单元测试

### 运行单元测试

```bash
# 运行所有单元测试
./run_tests.sh

# 运行特定单元测试
cd build
./uvhttp_unit_tests

# 使用 ctest 运行测试
cd build
ctest

# 运行特定测试
cd build
ctest -R gtest_example
```

### Google Test 框架

项目使用 Google Test 框架进行单元测试。测试文件使用 `.cpp` 扩展名，使用 gtest 的断言宏。

```cpp
TEST(TestSuiteName, TestName) {
    EXPECT_EQ(expected, actual);
    EXPECT_STREQ(expected, actual);
    // 更多断言...
}
```

## 集成测试

集成测试位于 `test/integration/` 目录，测试多个组件之间的交互。

### 运行集成测试

```bash
# 编译集成测试
cd build
make test_simple

# 运行集成测试
./dist/bin/test_simple
```

## 性能测试

### 性能对比报告

完整的性能对比测试报告请查看: [config/PERFORMANCE_COMPARISON_REPORT.md](config/PERFORMANCE_COMPARISON_REPORT.md)

该报告包含以下对比测试：
- Nginx (多线程) vs UVHTTP
- Nginx (单线程) vs UVHTTP
- UVHTTP vs Python HTTP Server

### 运行性能测试

```bash
# 运行 UVHTTP 性能测试
bash test/scripts/run_uvhttp_performance_local.sh

# 运行性能测试（包含所有测试）
bash test/scripts/run_performance_tests.sh

# 运行分配器性能测试
cd build
make performance_allocator
./dist/bin/performance_allocator
```

## 限流测试

```bash
# 运行限流功能测试
bash test/scripts/run_rate_limit_tests.sh
```

## 配置测试

测试配置文件位于 `test/config/` 目录：
- `config_valid.conf` - 有效的配置示例
- `config_invalid.conf` - 无效的配置示例
- `config_test.conf` - 测试配置

## WebSocket 测试

WebSocket 测试页面: `test/integration/websocket_test.html`

启动 WebSocket 服务器后，在浏览器中打开该页面进行测试。

## TLS 测试

生成测试证书：

```bash
bash test/scripts/generate_test_certs.sh
```

测试证书将保存在 `test/certs/` 目录中。

## 测试结果

性能测试结果保存在 `test/results/` 目录：
- `nginx_single_thread_results/` - Nginx 单线程测试结果
- `uvhttp_performance_results/` - UVHTTP 性能测试结果
- `uvhttp_single_thread_results/` - UVHTTP 单线程测试结果

## 清理测试文件

```bash
# 清理构建文件
./run_tests.sh -c

# 清理测试结果
rm -rf test/results/*

# 清理构建目录
cd build
make clean
```

## 注意事项

- 所有性能测试都在本地环境运行
- 测试端口：Nginx (8888), UVHTTP (8080)
- 测试工具：wrk
- 测试配置：4线程, 100并发连接, 30秒持续时间
- 单元测试使用 Google Test 框架
- 集成测试和性能测试使用自定义框架

## 测试覆盖率

项目目标覆盖率：80%

生成覆盖率报告：

```bash
# 启用覆盖率编译
cd build
cmake -DENABLE_COVERAGE=ON ..
make

# 运行测试
ctest

# 生成覆盖率报告
lcov --capture --directory . --output-file coverage.info
genhtml coverage.info --output-directory coverage_html
```

查看覆盖率报告：
```bash
open coverage_html/index.html
```