# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-1.1.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Coverage](https://img.shields.io/badge/coverage-100%25-green.svg)
![Performance](https://img.shields.io/badge/1000%20RPS-0.082ms-brightgreen.svg)
![Stress](https://img.shields.io/badge/stress%20tests-passing-success.svg)
![WebSocket](https://img.shields.io/badge/websocket-supported-orange.svg)

**基于libuv的安全HTTP/WebSocket服务器库**

高性能 • 内存安全 • WebSocket支持 • 生产就绪

</div>

## ✨ 特性

### 🔒 **安全第一**
- ✅ 缓冲区溢出保护
- ✅ 输入验证和边界检查
- ✅ 安全的字符串操作
- ✅ 资源限制和DoS防护
- ✅ TLS 1.3支持
- ✅ WebSocket安全连接

### ⚡ **高性能**
- ⚡ 基于libuv事件驱动架构
- ⚡ 零拷贝内存管理
- ⚡ 连接池和会话缓存
- ⚡ 智能内存分配策略
- ⚡ WebSocket高性能处理

### 🛡️ **生产就绪**
- 🛡️ 零编译警告
- 🛡️ 完整的错误处理
- 🛡️ 结构化日志记录
- 🛡️ 性能监控和统计
- 🛡️ 内存泄漏检测
- 🛡️ 100%测试覆盖率

### 🔧 **易于使用**
- 🔧 简洁直观的API设计
- 🔧 丰富的示例代码
- 🔧 详细的API文档
- 🔧 完整的测试覆盖
- 🔧 WebSocket简化API

### 📈 **性能验证**
- 📈 全面压力测试套件
- 📈 1000+ RPS性能验证
- 📈 亚毫秒级响应时间
- 📈 零内存泄漏保证
- 📈 WebSocket压力测试

### 🌐 **WebSocket支持**
- 🌐 完整的WebSocket协议实现
- 🌐 消息类型支持（文本/二进制/控制帧）
- 🌐 mTLS安全连接
- 🌐 证书验证和管理
- 🌐 连接池和自动重连

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

### HTTP服务器

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

### WebSocket服务器

```c
#include "uvhttp.h"
#include <stdio.h>

void websocket_handler(uvhttp_websocket_t* ws, 
                       const uvhttp_websocket_message_t* msg, 
                       void* user_data) {
    if (msg->type == UVHTTP_WEBSOCKET_TEXT) {
        printf("收到消息: %.*s\n", (int)msg->length, msg->data);
        // 回复消息
        uvhttp_websocket_send_text(ws, "消息已收到!");
    }
}

void websocket_upgrade_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // 升级到WebSocket连接
    uvhttp_websocket_t* ws = uvhttp_websocket_new(request, response);
    if (ws) {
        uvhttp_websocket_set_handler(ws, websocket_handler, NULL);
        printf("WebSocket连接已建立\n");
    }
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/ws", websocket_upgrade_handler);
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("WebSocket服务器运行在 ws://localhost:8080/ws\n");
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

### WebSocket

- `uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, uvhttp_response_t* response)` - 创建WebSocket连接
- `void uvhttp_websocket_free(uvhttp_websocket_t* ws)` - 释放WebSocket连接
- `uvhttp_websocket_error_t uvhttp_websocket_send(uvhttp_websocket_t* ws, const char* data, size_t length, uvhttp_websocket_type_t type)` - 发送消息
- `uvhttp_websocket_error_t uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, uvhttp_websocket_handler_t handler, void* user_data)` - 设置消息处理器
- `uvhttp_websocket_error_t uvhttp_websocket_close(uvhttp_websocket_t* ws, int code, const char* reason)` - 关闭连接
- `uvhttp_websocket_error_t uvhttp_websocket_enable_mtls(uvhttp_websocket_t* ws, const uvhttp_websocket_mtls_config_t* config)` - 启用mTLS
- `uvhttp_websocket_error_t uvhttp_websocket_verify_peer_cert(uvhttp_websocket_t* ws)` - 验证对端证书

#### WebSocket便捷宏

- `uvhttp_websocket_send_text(ws, text)` - 发送文本消息
- `uvhttp_websocket_send_binary(ws, data, len)` - 发送二进制消息

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
- [WebSocket实现文档](WEBSOCKET_IMPLEMENTATION.md) - WebSocket实现细节
- [压力测试指南](STRESS_TESTING.md) - 全面的压力测试文档
- [WebSocket压力测试报告](WEBSOCKET_STRESS_TEST_REPORT.md) - WebSocket性能测试
- [开发规范](DEVELOPMENT_GUIDELINES.md) - 工程开发规范
- [示例代码](examples/) - 实用的使用示例
- [编译指南](#编译) - 详细的编译说明
- [路线图](ROADMAP.md) - 项目发展规划

## 🤝 贡献

欢迎提交Issue和Pull Request来改进UVHTTP！

## 📄 许可证

MIT License