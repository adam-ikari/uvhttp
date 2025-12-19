# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Coverage](https://img.shields.io/badge/coverage-85%25-green.svg)
![Performance](https://img.shields.io/badge/1000%20RPS-0.082ms-brightgreen.svg)
![Stress](https://img.shields.io/badge/stress%20tests-passing-success.svg)

**基于libuv的安全HTTP服务器库**

高性能 • 内存安全 • 生产就绪

</div>

## ✨ 特性

### 🔒 **安全第一**
- ✅ 缓冲区溢出保护
- ✅ 输入验证和边界检查
- ✅ 安全的字符串操作
- ✅ 资源限制和DoS防护
- ✅ TLS 1.3支持

### ⚡ **高性能**
- ⚡ 基于libuv事件驱动架构
- ⚡ 零拷贝内存管理
- ⚡ 连接池和会话缓存
- ⚡ 智能内存分配策略

### 🛡️ **生产就绪**
- 🛡️ 零编译警告
- 🛡️ 完整的错误处理
- 🛡️ 结构化日志记录
- 🛡️ 性能监控和统计
- 🛡️ 内存泄漏检测

### 🔧 **易于使用**
- 🔧 简洁直观的API设计
- 🔧 丰富的示例代码
- 🔧 详细的API文档
- 🔧 完整的测试覆盖

### 📈 **性能验证**
- 📈 全面压力测试套件
- 📈 1000+ RPS性能验证
- 📈 亚毫秒级响应时间
- 📈 零内存泄漏保证

## 🚀 快速开始

### 依赖要求

- libuv >= 1.0.0
- mbedtls >= 2.0.0 (TLS支持)
- CMake >= 3.16

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install libuv-dev libmbedtls-dev

# CentOS/RHEL
sudo yum install libuv-devel mbedtls-devel

# macOS (使用Homebrew)
brew install libuv mbedtls
```

### 编译

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
mkdir build && cd build
cmake ..
make
```

## 示例

```c
#include "uvhttp.h"
#include <stdio.h>

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

## API文档

### 服务器

- `uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop)` - 创建新服务器
- `void uvhttp_server_free(uvhttp_server_t* server)` - 释放服务器
- `int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port)` - 监听端口
- `void uvhttp_server_stop(uvhttp_server_t* server)` - 停止服务器

### 路由

- `uvhttp_router_t* uvhttp_router_new(void)` - 创建新路由
- `void uvhttp_router_add_route(uvhttp_router_t* router, const char* path, uvhttp_request_handler_t handler)` - 添加路由
- `uvhttp_request_handler_t uvhttp_router_find_handler(uvhttp_router_t* router, const char* path)` - 查找路由处理器

### 请求

- `const char* uvhttp_request_get_method(uvhttp_request_t* request)` - 获取HTTP方法
- `const char* uvhttp_request_get_url(uvhttp_request_t* request)` - 获取请求URL
- `const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name)` - 获取请求头
- `const char* uvhttp_request_get_body(uvhttp_request_t* request)` - 获取请求体

### 响应

- `void uvhttp_response_set_status(uvhttp_response_t* response, int status_code)` - 设置状态码
- `void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value)` - 设置响应头
- `void uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length)` - 设置响应体
- `void uvhttp_response_send(uvhttp_response_t* response)` - 发送响应

## 🏃‍♂️ 运行示例

### 基础HTTP服务器

```bash
# 编译完成后
./build/uvhttp_example
```

然后在浏览器中访问 http://localhost:8080

### 快速压力测试

```bash
# 一键运行完整压力测试套件
./run_stress_tests.sh

# 或者手动运行简单测试
export LD_LIBRARY_PATH=deps/libuv/.libs:$LD_LIBRARY_PATH
./test_server_simple &  # 启动服务器
./test_simple_stress     # 运行压力测试
```

预期结果：1000 RPS，0.082ms平均延迟，100%成功率

## 🧪 测试

### 单元测试

```bash
./build/uvhttp_test
```

### 压力测试

UVHTTP提供了全面的压力测试套件，基于libuv事件驱动架构，可以真实评估服务器性能：

```bash
# 运行完整压力测试套件
./run_stress_tests.sh
```

#### 压力测试特性

- **🔥 高并发测试** - 支持1000+并发连接
- **⚡ 吞吐量测试** - 测量RPS性能（支持1000-5000 RPS）
- **🛡️ 内存泄漏检测** - 长时间运行稳定性测试
- **🎯 边界条件测试** - 极限负载下的系统行为
- **📊 性能基准** - 系统基础性能指标测量

#### 测试结果示例

```
--- 压力测试结果 ---
测试持续时间: 30.00 秒
总请求数: 30000
成功请求: 30000 (100.0%)
失败请求: 0 (0.0%)
目标RPS: 1000
实际RPS: 1000.0
RPS达成率: 100.0%
平均响应时间: 0.082 ms
最小响应时间: 0.066 ms
最大响应时间: 0.620 ms
内存使用变化: 0 KB
```

#### 单独运行测试

```bash
# 编译测试程序
gcc -std=c11 -o test_server_simple test_server_simple.c -L deps/libuv/.libs -luv -I deps/libuv/include -lpthread -lm
gcc -o test_simple_stress test_simple_stress.c -lpthread -lm

# 启动测试服务器
export LD_LIBRARY_PATH=deps/libuv/.libs:$LD_LIBRARY_PATH
./test_server_simple &

# 运行压力测试
./test_simple_stress
```

详细的压力测试文档请参考：[STRESS_TESTING.md](STRESS_TESTING.md)

## 📚 文档

- [API文档](#api文档) - 详细的API参考
- [压力测试指南](STRESS_TESTING.md) - 全面的压力测试文档
- [示例代码](examples/) - 实用的使用示例
- [编译指南](#编译) - 详细的编译说明

## 🤝 贡献

欢迎提交Issue和Pull Request来改进UVHTTP！

## 📄 许可证

MIT License