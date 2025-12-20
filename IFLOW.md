# UVHTTP 项目指南

## 项目概述

UVHTTP 是一个基于 libuv 的高性能、内存安全的 HTTP 服务器库，采用 C11 标准编写。该项目专注于提供安全、快速且生产就绪的 HTTP 服务器实现，具有完整的功能特性和全面的测试覆盖。

### 核心特性

- **🔒 安全第一**：缓冲区溢出保护、输入验证、内存安全机制
- **⚡ 高性能**：基于 libuv 事件驱动架构，零拷贝内存管理
- **🛡️ 生产就绪**：零编译警告、完整错误处理、资源限制保护
- **🔧 易于使用**：简洁直观的 API 设计，丰富的示例代码
- **📊 可观测性**：结构化日志记录、性能监控、内存使用跟踪

### 技术栈

- **核心语言**：C11 标准
- **异步框架**：libuv (事件驱动 I/O)
- **HTTP 解析**：llhttp (高性能 HTTP 解析器)
- **TLS 支持**：mbedtls (可选的安全传输层)
- **测试框架**：自定义 gtest 实现
- **构建系统**：CMake 3.10+ / Makefile

## 项目结构

```
uvhttp/
├── include/           # 公共头文件
│   ├── uvhttp.h      # 主头文件，包含所有模块
│   ├── uvhttp_*.h    # 各模块专用头文件
│   └── ...
├── src/              # 源代码实现
│   ├── uvhttp_*.c    # 核心模块实现
│   └── ...
├── examples/         # 示例代码
│   ├── simple_server.c
│   └── complete_example.c
├── test/             # 测试文件
│   ├── test_*.c      # 单元测试
│   └── test_*_stress.c  # 压力测试
├── deps/             # 依赖库
│   ├── libuv/        # 异步 I/O 库
│   ├── llhttp/       # HTTP 解析器
│   ├── mbedtls/      # TLS 实现
│   └── googletest/   # 测试框架
├── test/certs/       # TLS 证书
├── build/            # 构建输出目录
└── docs/             # 文档
```

## 构建和运行

### 依赖要求

- **编译器**：GCC 或兼容的 C11 编译器
- **CMake**：3.10 或更高版本
- **libuv**：1.0.0+ (已包含在 deps/ 目录)
- **mbedtls**：2.0.0+ (可选，用于 TLS 支持)

### 构建命令

#### 使用 CMake (推荐)

```bash
# 创建构建目录并配置
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..

# 编译项目
make -j$(nproc)

# 运行测试
./uvhttp_unit_tests
```

#### 使用 Makefile (快速开发)

```bash
# 检查依赖
make check-deps

# 编译示例
make all

# 运行简单服务器
make run-simple

# 运行完整示例
make run-complete

# 测试 API 端点
make test

# 压力测试
make stress-test

# 清理构建文件
make clean
```

### 运行示例

```bash
# 方法1：使用 Makefile
make run-complete

# 方法2：手动运行
cd build
export LD_LIBRARY_PATH=deps/libuv/.libs:$LD_LIBRARY_PATH
./examples/simple_server
```

服务器启动后，可以访问：
- http://localhost:8080/ - 主页
- http://localhost:8080/api - API 端点

## 测试

### 运行完整测试套件

```bash
# 使用测试脚本（推荐）
./run_tests.sh

# 或者手动运行
cd build
./uvhttp_unit_tests
```

### 压力测试

```bash
# 运行完整压力测试套件
./run_stress_tests.sh

# 单独运行特定测试
./test_simple_stress        # 简单压力测试
./test_performance_benchmark # 性能基准测试
./test_memory_leak         # 内存泄漏测试
```

### 代码覆盖率

```bash
# 启用覆盖率构建
cmake -DCMAKE_BUILD_TYPE=Debug -DENABLE_COVERAGE=ON ..
make

# 生成覆盖率报告
make coverage

# 查看报告
open coverage_html/index.html
```

## 开发约定

### 代码风格

- 使用 C11 标准
- 遵循 Linux 内核代码风格
- 所有公共 API 必须有完整的文档注释
- 错误处理必须完整且一致
- 内存管理必须安全，避免泄漏

### 测试约定

- 所有新功能必须包含单元测试
- 性能关键代码需要基准测试
- 安全相关代码需要边界条件测试
- 目标代码覆盖率：80%+

### 提交规范

- 使用清晰的提交信息
- 每个提交应该是一个逻辑完整的变更
- 提交前必须通过所有测试
- 不要提交构建产物或临时文件

## API 使用示例

### 基础 HTTP 服务器

```c
#include "uvhttp.h"

void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```



## 性能指标

基于标准测试环境的性能基准：

| 指标 | 基准值 | 说明 |
|------|--------|------|
| RPS | 1000+ | 每秒请求数 |
| 延迟 | 0.082ms | 平均响应时间 |
| 并发连接 | 1000+ | 同时处理连接数 |
| 内存使用 | <10MB | 基础内存占用 |
| CPU 使用率 | <5% | 空闲时 CPU 占用 |

## 故障排除

### 常见问题

1. **编译错误：找不到 libuv**
   ```bash
   cd deps/libuv && ./autogen.sh && ./configure && make -j4
   ```

2. **运行时错误：找不到 libuv.so**
   ```bash
   export LD_LIBRARY_PATH=deps/libuv/.libs:$LD_LIBRARY_PATH
   ```

3. **测试失败：端口被占用**
   ```bash
   # 检查端口占用
   lsof -i :8080
   # 或使用其他端口
   ```

### 调试工具

- **内存检查**：`valgrind --leak-check=full ./uvhttp_unit_tests`
- **性能分析**：`perf record ./uvhttp_test`
- **系统调用跟踪**：`strace -o trace.log ./uvhttp_server`

## 贡献指南

1. Fork 项目并创建功能分支
2. 遵循现有代码风格和约定
3. 添加适当的测试用例
4. 确保所有测试通过
5. 更新相关文档
6. 提交 Pull Request

## 许可证

本项目采用 MIT 许可证，详见 LICENSE 文件。

## 联系方式

- 项目主页：https://github.com/adam-ikari/uvhttp
- 问题报告：https://github.com/adam-ikari/uvhttp/issues
- 文档：详见 README.md 和 STRESS_TESTING.md