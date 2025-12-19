# UVHTTP

一个基于libuv和llhttp的轻量级HTTP服务框架。

## 特性

- 基于libuv事件循环，高性能异步I/O
- 使用llhttp进行HTTP协议解析，快速且可靠
- 简洁的路由系统
- C语言实现，内存占用低
- 跨平台支持

## 依赖

- libuv >= 1.0.0
- CMake >= 3.16

## 编译

```bash
mkdir build
cd build
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

## 运行示例

```bash
# 编译完成后
./build/uvhttp_example
```

然后在浏览器中访问 http://localhost:8080

## 测试

```bash
./build/uvhttp_test
```

## 许可证

MIT License