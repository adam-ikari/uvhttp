# UVHTTP 开发指南

## 目录
1. [快速开始](#快速开始)
2. [API 参考](#api-参考)
3. [示例程序](#示例程序)
4. [测试指南](#测试指南)
5. [性能优化](#性能优化)
6. [常见问题](#常见问题)

## 快速开始

### 安装依赖

```bash
# Ubuntu/Debian
sudo apt-get install libuv-dev libmbedtls-dev

# CentOS/RHEL
sudo yum install libuv-devel mbedtls-devel

# macOS
brew install libuv mbedtls
```

### 编译项目

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

### 运行第一个服务器

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
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## API 参考

### 服务器管理

#### `uvhttp_server_t* uvhttp_server_new(uv_loop_t* loop)`
创建新的 HTTP 服务器实例。

**参数:**
- `loop`: libuv 事件循环

**返回:**
- 成功返回服务器实例指针，失败返回 NULL

#### `int uvhttp_server_listen(uvhttp_server_t* server, const char* host, int port)`
启动服务器监听指定端口。

**参数:**
- `server`: 服务器实例
- `host`: 监听地址
- `port`: 监听端口

**返回:**
- 成功返回 0，失败返回负数

### 请求处理

#### `const char* uvhttp_request_get_method(uvhttp_request_t* request)`
获取 HTTP 请求方法。

#### `const char* uvhttp_request_get_url(uvhttp_request_t* request)`
获取请求 URL。

#### `const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name)`
获取请求头。

### 响应处理

#### `void uvhttp_response_set_status(uvhttp_response_t* response, int status_code)`
设置 HTTP 响应状态码。

#### `void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value)`
设置响应头。

#### `int uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length)`
设置响应体。

## 示例程序

### 1. 简单 HTTP 服务器

位置: `examples/simple_server.c`

功能:
- 基本路由处理
- HTML 响应
- 错误处理
- 日志记录

运行:
```bash
cd build
./simple_server
```

访问:
- http://localhost:8080/ - 主页
- http://localhost:8080/api - JSON API
- http://localhost:8080/info - 服务器信息

### 2. RESTful API 服务器

位置: `examples/restful_api_server.c`

功能:
- CRUD 操作
- JSON 处理
- CORS 支持
- 中间件

API 端点:
- `GET /tasks` - 获取所有任务
- `GET /tasks/{id}` - 获取单个任务
- `POST /tasks` - 创建任务
- `PUT /tasks/{id}` - 更新任务
- `DELETE /tasks/{id}` - 删除任务

### 3. WebSocket 服务器

位置: `examples/websocket_example.c`

功能:
- WebSocket 连接
- 消息广播
- 心跳检测

## 测试指南

### 运行测试套件

```bash
# 运行所有测试
make test

# 运行综合测试
./build/comprehensive_test_suite

# 运行 WebSocket 测试
./build/websocket_integration_test

# 运行压力测试
./run_stress_tests.sh
```

### 测试类型

1. **单元测试**
   - 请求处理测试
   - 响应处理测试
   - 路由系统测试
   - 内存管理测试

2. **集成测试**
   - 完整请求-响应流程
   - WebSocket 连接测试
   - 错误处理测试

3. **性能测试**
   - 内存分配性能
   - 字符串处理性能
   - 并发连接测试

4. **压力测试**
   - 高并发连接
   - 长时间运行
   - 内存泄漏检测

### 编写测试

测试文件命名规范: `test_*.c`

测试函数命名规范: `test_*()`

```c
void test_feature_name() {
    /* 测试代码 */
    TEST_ASSERT(condition, "测试描述");
}
```

## 性能优化

### 内存管理

UVHTTP 提供了多种内存分配器选项：

```c
// 使用系统分配器（默认）
#define UVHTTP_ALLOCATOR_TYPE 0

// 使用 mimalloc
#define UVHTTP_ALLOCATOR_TYPE 1

// 使用自定义分配器
#define UVHTTP_ALLOCATOR_TYPE 2
```

### 连接优化

```c
// 设置最大连接数
server->max_connections = 1000;

// 设置读取缓冲区大小
server->read_buffer_size = 8192;
```

### 错误恢复

```c
// 配置重试策略
uvhttp_set_error_recovery_config(
    3,      // 最大重试次数
    100,    // 基础延迟 (ms)
    5000,   // 最大延迟 (ms)
    2.0     // 退避倍数
);
```

## 常见问题

### Q: 如何处理静态文件？

A: 使用内置的静态文件中间件：

```c
void static_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_url(request);
    uvhttp_serve_static_file(response, path, "./public");
}
```

### Q: 如何启用 HTTPS？

A: 配置 TLS 上下文：

```c
uvhttp_tls_config_t tls_config = {
    .cert_file = "server.crt",
    .key_file = "server.key"
};

uvhttp_server_enable_tls(server, &tls_config);
```

### Q: 如何处理大文件上传？

A: 配置请求体大小限制：

```c
// 设置最大请求体大小 (10MB)
server->max_body_size = 10 * 1024 * 1024;
```

### Q: 如何添加中间件？

A: 在路由处理器中调用中间件函数：

```c
void auth_middleware(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* auth = uvhttp_request_get_header(request, "Authorization");
    if (!auth || !validate_token(auth)) {
        uvhttp_response_set_status(response, 401);
        uvhttp_response_send(response);
        return;
    }
    // 继续处理
}
```

### Q: 如何调试内存问题？

A: 启用内存调试模式：

```c
#define UVHTTP_ENABLE_MEMORY_DEBUG

// 获取内存统计
size_t total, current, alloc_count, free_count;
uvhttp_get_memory_stats(&total, &current, &alloc_count, &free_count);

// 检查内存泄漏
if (uvhttp_check_memory_leaks()) {
    printf("检测到内存泄漏\n");
}
```

## 贡献指南

1. Fork 项目
2. 创建功能分支
3. 编写测试
4. 提交 Pull Request

## 许可证

MIT License - 详见 LICENSE 文件