# UVHTTP

<div align="center">

![uvhttp](https://img.shields.io/badge/uvhttp-1.0.0-blue.svg)
![License](https://img.shields.io/badge/license-MIT-green.svg)
![Build](https://img.shields.io/badge/build-passing-brightgreen.svg)
![Coverage](https://img.shields.io/badge/coverage-97%25-green.svg)
![Performance](https://img.shields.io/badge/1000%20RPS-0.082ms-brightgreen.svg)
![Stress](https://img.shields.io/badge/stress%20tests-passing-success.svg)
![WebSocket](https://img.shields.io/badge/websocket-supported-orange.svg)

**专注 HTTP/1.1 和 WebSocket 的高性能服务器库**

专注核心 • 高性能 • 编译配置 • 生产就绪

</div>

## ✨ 特性

### 🔒 **安全第一**

- ✅ 缓冲区溢出保护
- ✅ 输入验证和边界检查
- ✅ 安全的字符串操作
- ✅ 资源限制和 DoS 防护
- ✅ TLS 1.3 支持
- ✅ WebSocket 安全连接
- ✅ 编译时安全检查

### ⚡ **高性能**

- ⚡ 基于 libuv 事件驱动架构
- ⚡ 零拷贝内存管理
- ⚡ 连接池和会话缓存
- ⚡ 智能内存分配策略
- ⚡ WebSocket 高性能处理
- ⚡ 编译优化零开销

### 🛡️ **生产就绪**

- 🛡️ 零编译警告
- 🛡️ 完整的错误处理
- 🛡️ 条件编译日志系统
- 🛡️ 性能监控和统计
- 🛡️ 内存泄漏检测
- 🛡️ 97%测试覆盖率

### 🔧 **易于使用**

- 🔧 简洁直观的 API 设计
- 🔧 丰富的示例代码
- 🔧 详细的 API 文档
- 🔧 完整的测试覆盖
- 🔧 WebSocket 简化 API
- 🔧 编译宏功能控制

### 📈 **性能验证**

- 📈 全面压力测试套件
- 📈 1000+ RPS 性能验证
- 📈 亚毫秒级响应时间
- 📈 零内存泄漏保证
- 📈 WebSocket 压力测试

### 🌐 **WebSocket 支持**

- 🌐 完整的 WebSocket 协议实现
- 🌐 消息类型支持（文本/二进制/控制帧）
- 🌐 mTLS 安全连接
- 🌐 证书验证和管理
- 🌐 连接池和自动重连
- 🌐 高并发 WebSocket 连接

### ⚙️ **编译配置**

- ⚙️ 功能开关（WebSocket/TLS/JSON）
- ⚙️ 安全特性（CORS/限流/认证）
- ⚙️ 性能优化（缓存/连接池）
- ⚙️ 调试功能（日志/追踪）

### 📦 **依赖管理**

UVHTTP 使用 git 子模块管理第三方依赖，确保版本兼容性和依赖完整性：

```bash
# 克隆项目（包含所有子模块）
git clone --recursive https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 更新子模块到最新版本
git submodule update --init --recursive

# 初始化并构建
mkdir build && cd build
cmake ..
make
```

- ⚙️ 零运行时开销设计

## 🚀 快速开始

### 依赖要求

- CMake >= 3.10
- GCC 或兼容的 C11 编译器

### 克隆和初始化

```bash
# 克隆项目（包含所有子模块）
git clone --recursive https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 如果已经克隆，更新子模块
git submodule update --init --recursive
```

### 编译

```bash
mkdir build && cd build
cmake ..
make
```

## 示例

### HTTP 服务器

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

### WebSocket 服务器

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

## API 文档

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

- `const char* uvhttp_request_get_method(uvhttp_request_t* request)` - 获取 HTTP 方法
- `const char* uvhttp_request_get_url(uvhttp_request_t* request)` - 获取请求 URL
- `const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name)` - 获取请求头
- `const char* uvhttp_request_get_body(uvhttp_request_t* request)` - 获取请求体

### 响应

- `void uvhttp_response_set_status(uvhttp_response_t* response, int status_code)` - 设置状态码
- `void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value)` - 设置响应头
- `void uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length)` - 设置响应体
- `void uvhttp_response_send(uvhttp_response_t* response)` - 发送响应

### WebSocket

- `uvhttp_websocket_t* uvhttp_websocket_new(uvhttp_request_t* request, uvhttp_response_t* response)` - 创建 WebSocket 连接
- `void uvhttp_websocket_free(uvhttp_websocket_t* ws)` - 释放 WebSocket 连接
- `uvhttp_websocket_error_t uvhttp_websocket_send(uvhttp_websocket_t* ws, const char* data, size_t length, uvhttp_websocket_type_t type)` - 发送消息
- `uvhttp_websocket_error_t uvhttp_websocket_set_handler(uvhttp_websocket_t* ws, uvhttp_websocket_handler_t handler, void* user_data)` - 设置消息处理器
- `uvhttp_websocket_error_t uvhttp_websocket_close(uvhttp_websocket_t* ws, int code, const char* reason)` - 关闭连接
- `uvhttp_websocket_error_t uvhttp_websocket_enable_mtls(uvhttp_websocket_t* ws, const uvhttp_websocket_mtls_config_t* config)` - 启用 mTLS
- `uvhttp_websocket_error_t uvhttp_websocket_verify_peer_cert(uvhttp_websocket_t* ws)` - 验证对端证书

#### WebSocket 便捷宏

- `uvhttp_websocket_send_text(ws, text)` - 发送文本消息
- `uvhttp_websocket_send_binary(ws, data, len)` - 发送二进制消息

## 🏃‍♂️ 运行示例

### 基础 HTTP 服务器

```bash
# 确保子模块已初始化
git submodule update --init --recursive

# 编译完成后
./dist/bin/helloworld
```

然后在浏览器中访问 http://localhost:8080

## 🧪 测试

### 单元测试

```bash
./dist/test/uvhttp_unit_tests
```

### 性能测试

```bash
./dist/test/uvhttp_test
```

## 🚀 版本规划

### v1.0.0 (当前版本)

- ✅ HTTP/1.1 服务器核心功能
- ✅ WebSocket 支持
- ✅ TLS/SSL 支持
- ✅ 编译宏控制系统
- ✅ 97%测试覆盖率

### v1.1.0 (规划中 - 3 个月)

- 🎯 零拷贝内存管理优化
- 🎯 连接池和会话缓存
- 🎯 WebSocket 性能优化
- 🎯 TLS 功能完善
- 🎯 编译宏系统实现

### v1.2.0 (规划中 - 6 个月)

- 🎯 负载均衡支持
- 🎯 监控和指标系统
- 🎯 配置管理系统
- 🎯 高级 WebSocket 功能
- 🎯 安全增强特性

### v2.0.0 (规划中 - 12 个月)

- 🎯 服务网格集成
- 🎯 容器化支持
- 🎯 云原生部署
- 🎯 高可用特性
- 🎯 分布式追踪

## 🤝 贡献

欢迎提交 Issue 和 Pull Request 来改进 UVHTTP！

## 📄 许可证

MIT License
