# UVHTTP 完整教程 - 从入门到精通

> 本教程循序渐进地介绍如何使用 UVHTTP 构建高性能 HTTP 服务器
> 从单线程服务到多线程架构，从简单路由到复杂配置，涵盖数据库集成和负载均衡

## 前置要求

### 必需工具
- **GCC/Clang** - C 编译器
- **CMake** - 构建系统
- **Git** - 版本控制（可选）

### 依赖说明
UVHTTP 采用自包含的依赖管理方式，所有依赖都包含在项目源码中：
- **libuv** - 异步 I/O 库（位于 `deps/libuv/`）
- **llhttp** - HTTP 解析器（位于 `deps/llhttp/`）
- **mbedtls** - TLS/SSL 支持（位于 `deps/mbedtls/`）
- **cjson** - JSON 处理（位于 `deps/cjson/`）
- **mimalloc** - 内存分配器（位于 `deps/mimalloc/`）

**无需额外安装系统依赖**，所有依赖都会在编译时自动构建。

### 快速开始
```bash
# 1. 克隆或进入项目目录
cd uvhttp

# 2. 编译项目
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)

# 3. 编译完成，库文件位于 build/ 目录
```

详见：[附录：依赖管理和编译](#附录依赖管理和编译)

## 目录

- [第一部分：入门基础](#第一部分入门基础)
  - [第1章：Hello World - 第一个HTTP服务器](#第1章hello-world----第一个http服务器)
  - [第2章：理解核心概念](#第2章理解核心概念)
  - [第3章：路由系统基础](#第3章路由系统基础)
- [第二部分：进阶开发](#第二部分进阶开发)
  - [第4章：复杂路由配置](#第4章复杂路由配置)
  - [第5章：请求处理进阶](#第5章请求处理进阶)
  - [第6章：响应处理优化](#第6章响应处理优化)
    - [6.1 静态文件中间件](#61-静态文件中间件)
    - [6.2 WebSocket 中间件](#62-websocket-中间件)
    - [6.3 统一响应处理](#63-统一响应处理)(#62-websocket-中间件)
    - [6.3 统一响应处理](#63-统一响应处理)
- [第三部分：高级架构](#第三部分高级架构)
  - [第8章：使用 libuv 数据指针](#第8章使用-libuv-数据指针)
  - [第9章：多线程服务器](#第9章多线程服务器)
  - [第10章：异步数据库集成](#第10章异步数据库集成)
  - [第11章：负载均衡](#第11章负载均衡)
- [第四部分：生产实践](#第四部分生产实践)
  - [第11章：性能优化](#第11章性能优化)
  - [第12章：安全配置](#第12章安全配置)
  - [第13章：监控和日志](#第13章监控和日志)

---

## 第一部分：入门基础

### 第1章：Hello World - 第一个HTTP服务器

#### 1.1 环境准备

**安装构建工具**：
```bash
# Ubuntu/Debian
sudo apt-get install build-essential cmake git

# CentOS/RHEL
sudo yum install gcc gcc-c++ make cmake git

# macOS
xcode-select --install
brew install cmake git
```

**获取源码**：
```bash
# 克隆仓库（包含所有依赖）
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 或者使用已存在的项目
cd /path/to/uvhttp
```

**编译 UVHTTP**（使用项目自带的依赖）：
```bash
# 创建构建目录
mkdir build && cd build

# 配置 CMake（会自动使用 deps/ 目录中的依赖）
cmake -DCMAKE_BUILD_TYPE=Release ..

# 编译
make -j$(nproc)

# 安装（可选）
sudo make install
```

**依赖说明**：
UVHTTP 项目已经包含了以下依赖，无需额外安装：
- **libuv** - 位于 `deps/libuv/` 目录
- **llhttp** - 位于 `deps/llhttp/` 目录
- **mbedtls** - 位于 `deps/mbedtls/` 目录
- **cjson** - 位于 `deps/cjson/` 目录
- **mimalloc** - 位于 `deps/mimalloc/` 目录
- **uthash** - 位于 `deps/uthash/` 目录
- **xxhash** - 位于 `deps/xxhash/` 目录

这些依赖会自动编译并链接到 UVHTTP 库中。

#### 1.2 最简单的HTTP服务器

创建 `hello_world.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

// 请求处理器函数
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 设置响应状态码
    uvhttp_response_set_status(res, 200);
    
    // 设置响应头
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    
    // 设置响应体
    const char* body = "Hello, World!";
    uvhttp_response_set_body(res, body, strlen(body));
    
    // 发送响应
    return uvhttp_response_send(res);
}

int main() {
    printf("启动 Hello World 服务器...\n");
    
    // 创建事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 创建服务器
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "服务器创建失败\n");
        return 1;
    }
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_server_set_router(server, router);
    
    // 添加路由
    uvhttp_router_add_route(router, "/", hello_handler);
    
    // 启动服务器监听
    int result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "服务器启动失败: %d\n", result);
        return 1;
    }
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("按 Ctrl+C 停止服务器\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理资源
    uvhttp_server_free(server);
    
    return 0;
}
```

**编译和运行**：
```bash
# 方法 1：使用 CMake 编译（推荐）
# 在项目根目录
mkdir -p build/examples
cd build/examples

# 创建 CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(hello_world C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../../include NO_DEFAULT_PATH)
find_library(UVHTTP_LIBRARY uvhttp PATHS ../.. NO_DEFAULT_PATH)

include_directories(${UVHTTP_INCLUDE_DIR})

add_executable(hello_world ../../examples/01_basics/01_hello_world.c)
target_link_libraries(hello_world ${UVHTTP_LIBRARY} uv pthread m)
EOF

# 编译
cmake ..
make

# 运行
./hello_world
```

**或者使用项目统一的构建系统**：
```bash
# 在项目根目录
cd build
cmake ..
make hello_world

# 运行
./examples/hello_world
```

**测试**：
```bash
curl http://localhost:8080/
```

#### 1.3 代码解析

**核心组件**：
1. **事件循环 (uv_loop_t)**：libuv 的事件循环，处理所有异步操作
2. **服务器 (uvhttp_server_t)**：HTTP 服务器实例
3. **路由器 (uvhttp_router_t)**：路由匹配和分发
4. **请求处理器**：处理 HTTP 请求的回调函数

**工作流程**：
```
客户端请求 → libuv 接收 → uvhttp 解析 → 路由匹配 → 处理器执行 → 响应发送
```

---

### 第2章：理解核心概念

#### 2.1 UVHTTP 架构

```
┌─────────────────────────────────────────┐
│         应用层 (你的代码)                │
│    ┌──────────────┐    ┌──────────────┐ │
│    │  请求处理器   │    │  业务逻辑     │ │
│    └──────────────┘    └──────────────┘ │
├─────────────────────────────────────────┤
│         API 层 (uvhttp)                 │
│    ┌──────────────┐    ┌──────────────┐ │
│    │  服务器API    │    │  路由系统     │ │
│    └──────────────┘    └──────────────┘ │
├─────────────────────────────────────────┤
│       核心层 (uvhttp_core)              │
│    ┌──────────────┐    ┌──────────────┐ │
│    │  请求解析     │    │  响应构建     │ │
│    └──────────────┘    └──────────────┘ │
├─────────────────────────────────────────┤
│      基础层 (libuv, llhttp)            │
│    ┌──────────────┐    ┌──────────────┐ │
│    │  事件驱动     │    │  HTTP解析     │ │
│    └──────────────┘    └──────────────┘ │
└─────────────────────────────────────────┘
```

#### 2.2 关键数据结构

**请求对象 (uvhttp_request_t)**：
```c
typedef struct uvhttp_request {
    uvhttp_method_t method;      // HTTP 方法 (GET, POST, etc.)
    char url[2048];              // 请求 URL
    uvhttp_header_t* headers;    // 请求头数组
    size_t header_count;         // 头部数量
    char* body;                  // 请求体
    size_t body_length;          // 请求体长度
    // ... 其他字段
} uvhttp_request_t;
```

**响应对象 (uvhttp_response_t)**：
```c
typedef struct uvhttp_response {
    uv_tcp_t* client;            // 客户端连接
    int status_code;             // HTTP 状态码
    uvhttp_header_t headers[64]; // 响应头数组
    size_t header_count;         // 头部数量
    char* body;                  // 响应体
    size_t body_length;          // 响应体长度
    // ... 其他字段
} uvhttp_response_t;
```

#### 2.3 事件驱动模型

**单线程事件循环**：
```c
// 事件循环持续运行
uv_run(loop, UV_RUN_DEFAULT);

// 运行模式
UV_RUN_DEFAULT  // 运行直到没有活动句柄
UV_RUN_ONCE     // 运行一次迭代
UV_RUN_NOWAIT   // 非阻塞运行一次
```

**异步操作**：
```c
// 所有 I/O 操作都是异步的
// 不会阻塞事件循环
uv_write(&write_req, stream, &buf, 1, on_write_complete);
uv_read_start(stream, alloc_buffer, on_read_complete);
```

---

### 第3章：路由系统基础

#### 3.1 基本路由

创建 `examples/02_routing/01_simple_routing.c`：

```c
#include "uvhttp.h"
#include <stdio.h>

// 主页处理器
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>主页</h1><p>欢迎访问 UVHTTP</p></body></html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

// 关于页面处理器
int about_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = "<html><body><h1>关于</h1><p>UVHTTP 高性能 HTTP 服务器</p></body></html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

// API 处理器
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"message\":\"API 响应\",\"status\":\"ok\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加多个路由
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/about", about_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("路由:\n");
    printf("  /        - 主页\n");
    printf("  /about   - 关于页面\n");
    printf("  /api     - API 接口\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

**编译和运行**：
```bash
# 使用 CMake 编译
cd build
cmake ..
make simple_routing

# 运行
./examples/simple_routing

# 测试
curl http://localhost:8080/
curl http://localhost:8080/about
curl http://localhost:8080/api
```

#### 3.2 路由参数

创建 `route_params.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// 用户详情处理器
int user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 从 URL 中提取用户 ID
    const char* url = uvhttp_request_get_url(req);
    
    // 简单的路径解析（实际应用中应该使用路由参数）
    char user_id[64] = {0};
    if (sscanf(url, "/user/%63s", user_id) == 1) {
        char response[512];
        snprintf(response, sizeof(response),
            "{\"user_id\":\"%s\",\"name\":\"用户 %s\",\"email\":\"user%s@example.com\"}",
            user_id, user_id, user_id);
        
        uvhttp_response_set_status(res, 200);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, response, strlen(response));
    } else {
        const char* error = "{\"error\":\"无效的用户ID\"}";
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
    }
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加带参数的路由
    uvhttp_router_add_route(router, "/user/*", user_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试: curl http://localhost:8080/user/123\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

#### 3.3 HTTP 方法路由

创建 `method_routing.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// GET 请求处理器
int get_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"method\":\"GET\",\"message\":\"获取资源\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

// POST 请求处理器
int post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    
    char response[512];
    if (body) {
        snprintf(response, sizeof(response),
            "{\"method\":\"POST\",\"message\":\"创建资源\",\"received\":\"%s\"}",
            body);
    } else {
        snprintf(response, sizeof(response),
            "{\"method\":\"POST\",\"message\":\"创建资源\",\"received\":null}");
    }
    
    uvhttp_response_set_status(res, 201);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

// PUT 请求处理器
int put_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    
    char response[512];
    if (body) {
        snprintf(response, sizeof(response),
            "{\"method\":\"PUT\",\"message\":\"更新资源\",\"received\":\"%s\"}",
            body);
    } else {
        snprintf(response, sizeof(response),
            "{\"method\":\"PUT\",\"message\":\"更新资源\",\"received\":null}");
    }
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

// DELETE 请求处理器
int delete_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"method\":\"DELETE\",\"message\":\"删除资源\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加不同 HTTP 方法的路由
    uvhttp_router_add_route(router, "/resource", get_handler);
    uvhttp_router_add_route(router, "/resource", post_handler);
    uvhttp_router_add_route(router, "/resource", put_handler);
    uvhttp_router_add_route(router, "/resource", delete_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试:\n");
    printf("  curl http://localhost:8080/resource\n");
    printf("  curl -X POST http://localhost:8080/resource -d '{\"name\":\"test\"}'\n");
    printf("  curl -X PUT http://localhost:8080/resource -d '{\"name\":\"updated\"}'\n");
    printf("  curl -X DELETE http://localhost:8080/resource\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

---

## 第二部分：进阶开发

### 第4章：复杂路由配置

#### 4.1 中间件模式

创建 `middleware.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

// 日志中间件
int logging_middleware(uvhttp_request_t* req, uvhttp_response_t* res) {
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    
    printf("[%s] %s %s\n", time_str, method, url);
    
    // 继续处理请求（这里需要实际的路由分发）
    return 0;
}

// 认证中间件
int auth_middleware(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* auth = uvhttp_request_get_header(req, "Authorization");
    
    if (!auth || strcmp(auth, "Bearer secret-token") != 0) {
        const char* error = "{\"error\":\"未授权\",\"message\":\"无效的认证令牌\"}";
        
        uvhttp_response_set_status(res, 401);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_header(res, "WWW-Authenticate", "Bearer");
        uvhttp_response_set_body(res, error, strlen(error));
        
        return uvhttp_response_send(res);
    }
    
    // 认证成功，继续处理
    return 0;
}

// 受保护的处理器
int protected_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 先通过认证中间件
    if (auth_middleware(req, res) != 0) {
        return 0; // 认证失败，已发送响应
    }
    
    const char* json = "{\"message\":\"访问成功\",\"data\":\"敏感信息\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

// 公开处理器
int public_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 先通过日志中间件
    logging_middleware(req, res);
    
    const char* json = "{\"message\":\"公开访问\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加路由
    uvhttp_router_add_route(router, "/public", public_handler);
    uvhttp_router_add_route(router, "/protected", protected_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试:\n");
    printf("  curl http://localhost:8080/public\n");
    printf("  curl http://localhost:8080/protected\n");
    printf("  curl -H 'Authorization: Bearer secret-token' http://localhost:8080/protected\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

#### 4.2 路由分组

创建 `route_groups.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// API v1 路由组
int api_v1_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v1\",\"resource\":\"users\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int api_v1_posts_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v1\",\"resource\":\"posts\"}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

// API v2 路由组
int api_v2_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v2\",\"resource\":\"users\",\"features\":[\"pagination\",\"filtering\"]}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int api_v2_posts_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* json = "{\"version\":\"v2\",\"resource\":\"posts\",\"features\":[\"pagination\",\"filtering\",\"sorting\"]}";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // API v1 路由组
    uvhttp_router_add_route(router, "/api/v1/users", api_v1_users_handler);
    uvhttp_router_add_route(router, "/api/v1/posts", api_v1_posts_handler);
    
    // API v2 路由组
    uvhttp_router_add_route(router, "/api/v2/users", api_v2_users_handler);
    uvhttp_router_add_route(router, "/api/v2/posts", api_v2_posts_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("API 路由:\n");
    printf("  /api/v1/users  - 用户列表 (v1)\n");
    printf("  /api/v1/posts  - 文章列表 (v1)\n");
    printf("  /api/v2/users  - 用户列表 (v2)\n");
    printf("  /api/v2/posts  - 文章列表 (v2)\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

---

### 第5章：请求处理进阶

#### 5.1 请求头处理

创建 `request_headers.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// 请求头信息处理器
int headers_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    char response[4096];
    int pos = 0;
    
    // 构建 JSON 响应
    pos += snprintf(response + pos, sizeof(response) - pos, "{\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"method\": \"%s\",\n", 
                   uvhttp_request_get_method(req));
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"url\": \"%s\",\n", 
                   uvhttp_request_get_url(req));
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"headers\": {\n");
    
    // 获取常见请求头
    const char* user_agent = uvhttp_request_get_header(req, "User-Agent");
    const char* accept = uvhttp_request_get_header(req, "Accept");
    const char* content_type = uvhttp_request_get_header(req, "Content-Type");
    const char* authorization = uvhttp_request_get_header(req, "Authorization");
    
    if (user_agent) {
        pos += snprintf(response + pos, sizeof(response) - pos, 
                       "    \"User-Agent\": \"%s\",\n", user_agent);
    }
    if (accept) {
        pos += snprintf(response + pos, sizeof(response) - pos, 
                       "    \"Accept\": \"%s\",\n", accept);
    }
    if (content_type) {
        pos += snprintf(response + pos, sizeof(response) - pos, 
                       "    \"Content-Type\": \"%s\",\n", content_type);
    }
    if (authorization) {
        pos += snprintf(response + pos, sizeof(response) - pos, 
                       "    \"Authorization\": \"***\"\n");
    }
    
    // 移除最后的逗号
    if (pos > 0 && response[pos - 2] == ',') {
        pos -= 2;
    }
    
    pos += snprintf(response + pos, sizeof(response) - pos, "  }\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "}\n");
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/headers", headers_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试: curl -v http://localhost:8080/headers\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

#### 5.2 请求体处理

创建 `request_body.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// JSON POST 处理器
int json_post_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    
    if (!body || strlen(body) == 0) {
        const char* error = "{\"error\":\"请求体为空\"}";
        
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        
        return uvhttp_response_send(res);
    }
    
    // 验证 Content-Type
    const char* content_type = uvhttp_request_get_header(req, "Content-Type");
    if (!content_type || strstr(content_type, "application/json") == NULL) {
        const char* error = "{\"error\":\"不支持的 Content-Type\",\"expected\":\"application/json\"}";
        
        uvhttp_response_set_status(res, 415);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        
        return uvhttp_response_send(res);
    }
    
    // 处理 JSON 数据（这里简单回显）
    char response[4096];
    snprintf(response, sizeof(response),
        "{\"status\":\"success\",\"received\":\"%s\",\"length\":%zu}",
        body, strlen(body));
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

// 文件上传处理器
int upload_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    size_t body_length = 0;
    
    if (body) {
        body_length = strlen(body);
    }
    
    char response[512];
    snprintf(response, sizeof(response),
        "{\"status\":\"received\",\"filename\":\"uploaded.dat\",\"size\":%zu}",
        body_length);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/api/json", json_post_handler);
    uvhttp_router_add_route(router, "/api/upload", upload_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试:\n");
    printf("  curl -X POST http://localhost:8080/api/json \\\n");
    printf("       -H 'Content-Type: application/json' \\\n");
    printf("       -d '{\"name\":\"test\"}'\n");
    printf("  curl -X POST http://localhost:8080/api/upload \\\n");
    printf("       -F 'file=@/path/to/file'\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

---

### 第6章：响应处理优化

#### 6.1 静态文件中间件

创建 `static_files.c`：

```c
#include "uvhttp.h"
#include "uvhttp_static.h"
#include <stdio.h>
#include <string.h>

// 静态文件服务上下文
static uvhttp_static_context_t* g_static_ctx = NULL;

/**
 * @brief 静态文件请求处理器
 */
int static_file_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    if (!g_static_ctx) {
        const char* error = "{\"error\":\"静态文件服务未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    // 处理静态文件请求
    int result = uvhttp_static_handle_request(g_static_ctx, req, res);
    if (result != 0) {
        const char* error = "{\"error\":\"文件未找到\"}";
        uvhttp_response_set_status(res, 404);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    return 0;
}

/**
 * @brief 主页处理器
 */
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP 静态文件服务</title>"
        "<meta charset='utf-8'>"
        "</head>"
        "<body>"
        "<h1>🚀 UVHTTP 静态文件服务</h1>"
        "<p>访问以下文件：</p>"
        "<ul>"
        "<li><a href='/index.html'>index.html</a></li>"
        "<li><a href='/about.html'>about.html</a></li>"
        "<li><a href='/style.css'>style.css</a></li>"
        "</ul>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

int main() {
    printf("启动静态文件服务器...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 配置静态文件服务
    uvhttp_static_config_t static_config = {
        .root_directory = "./public",
        .index_file = "index.html",
        .enable_directory_listing = 1,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 10 * 1024 * 1024,  // 10MB 缓存
        .cache_ttl = 3600,                   // 1 小时 TTL
        .custom_headers = ""
    };
    
    // 创建静态文件服务上下文
    g_static_ctx = uvhttp_static_create(&static_config);
    if (!g_static_ctx) {
        fprintf(stderr, "错误: 无法创建静态文件服务上下文\n");
        return 1;
    }
    
    printf("✓ 静态文件服务已配置\n");
    printf("  根目录: %s\n", static_config.root_directory);
    printf("  索引文件: %s\n", static_config.index_file);
    
    // 添加路由
    uvhttp_router_add_route(router, "/", home_handler);
    uvhttp_router_add_route(router, "/static/*", static_file_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("\n========================================\n");
    printf("  服务器运行在 http://localhost:8080\n");
    printf("========================================\n\n");
    
    printf("测试:\n");
    printf("  curl http://localhost:8080/\n");
    printf("  curl http://localhost:8080/static/index.html\n");
    printf("  curl http://localhost:8080/static/about.html\n\n");
    
    printf("按 Ctrl+C 停止服务器\n\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    if (g_static_ctx) {
        uvhttp_static_free(g_static_ctx);
    }
    uvhttp_server_free(server);
    
    return 0;
}
```

**创建测试文件**：
```bash
# 创建 public 目录
mkdir -p public

# 创建 index.html
cat > public/index.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>UVHTTP 静态文件服务</title>
    <link rel="stylesheet" href="/static/style.css">
</head>
<body>
    <h1>欢迎访问 UVHTTP</h1>
    <p>这是一个静态文件服务示例。</p>
    <a href="/static/about.html">关于我们</a>
</body>
</html>
EOF

# 创建 about.html
cat > public/about.html << 'EOF'
<!DOCTYPE html>
<html>
<head>
    <title>关于我们</title>
    <link rel="stylesheet" href="/static/style.css">
</head>
<body>
    <h1>关于 UVHTTP</h1>
    <p>UVHTTP 是一个高性能的 HTTP 服务器库。</p>
    <a href="/static/index.html">返回主页</a>
</body>
</html>
EOF

# 创建 style.css
cat > public/style.css << 'EOF'
body {
    font-family: Arial, sans-serif;
    margin: 40px;
    background: #f5f5f5;
}
h1 {
    color: #007bff;
}
a {
    color: #007bff;
    text-decoration: none;
}
a:hover {
    text-decoration: underline;
}
EOF
```

**编译和运行**：
```bash
cd build
cmake ..
make static_files
./examples/static_files

# 测试
curl http://localhost:8080/
curl http://localhost:8080/static/index.html
```

#### 6.2 WebSocket 中间件

创建 `websocket_server.c`：

```c
#include "uvhttp.h"
#include "uvhttp_websocket_middleware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// WebSocket 中间件实例
static uvhttp_ws_middleware_t* g_ws_middleware = NULL;

/**
 * @brief WebSocket 消息处理回调
 */
int ws_message_handler(uvhttp_ws_connection_t* ws_conn, 
                        const char* data, 
                        size_t len, 
                        int opcode,
                        void* user_data) {
    printf("收到 WebSocket 消息: %.*s\n", (int)len, data);
    
    // 回显消息
    uvhttp_ws_middleware_send(g_ws_middleware, ws_conn, data, len);
    
    return 0;
}

/**
 * @brief WebSocket 连接建立回调
 */
int ws_connect_handler(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    printf("WebSocket 连接建立\n");
    return 0;
}

/**
 * @brief WebSocket 连接关闭回调
 */
int ws_close_handler(uvhttp_ws_connection_t* ws_conn, void* user_data) {
    printf("WebSocket 连接关闭\n");
    return 0;
}

/**
 * @brief 主页处理器
 */
int home_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html = 
        "<!DOCTYPE html>"
        "<html>"
        "<head>"
        "<title>UVHTTP WebSocket 示例</title>"
        "<meta charset='utf-8'>"
        "<style>"
        "body { font-family: Arial, sans-serif; margin: 40px; }"
        "#messages { border: 1px solid #ccc; padding: 10px; height: 300px; overflow-y: scroll; }"
        "input { padding: 10px; width: 300px; }"
        "button { padding: 10px 20px; }"
        "</style>"
        "</head>"
        "<body>"
        "<h1>🚀 UVHTTP WebSocket 示例</h1>"
        "<div id='messages'></div>"
        "<br>"
        "<input type='text' id='message' placeholder='输入消息'>"
        "<button onclick='sendMessage()'>发送</button>"
        "<button onclick='disconnect()'>断开连接</button>"
        "<script>"
        "let ws = null;"
        "function connect() {"
        "  ws = new WebSocket('ws://localhost:8080/ws');"
        "  ws.onopen = function() {"
        "    addMessage('已连接到服务器');"
        "  };"
        "  ws.onmessage = function(event) {"
        "    addMessage('服务器: ' + event.data);"
        "  };"
        "  ws.onclose = function() {"
        "    addMessage('连接已关闭');"
        "  };"
        "}"
        "function sendMessage() {"
        "  const input = document.getElementById('message');"
        "  if (ws && ws.readyState === WebSocket.OPEN) {"
        "    ws.send(input.value);"
        "    addMessage('你: ' + input.value);"
        "    input.value = '';"
        "  }"
        "}"
        "function disconnect() {"
        "  if (ws) {"
        "    ws.close();"
        "  }"
        "}"
        "function addMessage(msg) {"
        "  const div = document.getElementById('messages');"
        "  div.innerHTML += '<p>' + msg + '</p>';"
        "  div.scrollTop = div.scrollHeight;"
        "}"
        "connect();"
        "</script>"
        "</body>"
        "</html>";
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html, strlen(html));
    
    return uvhttp_response_send(res);
}

int main() {
    printf("启动 WebSocket 服务器...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 注册 WebSocket 处理器
    uvhttp_ws_handler_t ws_handler;
    ws_handler.on_connect = ws_connect_handler;
    ws_handler.on_message = ws_message_handler;
    ws_handler.on_close = ws_close_handler;
    ws_handler.user_data = NULL;
    
    uvhttp_error_t result = uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 无法注册 WebSocket 处理器 (错误码: %d)\n", result);
        return 1;
    }
    
    printf("✓ WebSocket 处理器已注册\n");
    printf("  WebSocket 路径: /ws\n");
    
    // 添加 HTTP 路由
    uvhttp_router_add_route(router, "/", home_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("\n========================================\n");
    printf("  服务器运行在 http://localhost:8080\n");
    printf("========================================\n\n");
    
    printf("测试:\n");
    printf("  1. 在浏览器中打开 http://localhost:8080\n");
    printf("  2. 输入消息并点击发送\n");
    printf("  3. 查看服务器回显的消息\n\n");
    
    printf("WebSocket URL: ws://localhost:8080/ws\n\n");
    
    printf("按 Ctrl+C 停止服务器\n\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

**编译和运行**：
```bash
cd build
cmake ..
make websocket_server
./examples/websocket_server

# 在浏览器中打开 http://localhost:8080
# 或使用 wscat 测试
# wscat -c ws://localhost:8080/ws
```

#### 6.3 统一响应处理

创建 `unified_response.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// JSON 响应助手函数
void send_json_response(uvhttp_response_t* res, int status, const char* json_data) {
    uvhttp_response_set_status(res, status);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json_data, strlen(json_data));
    uvhttp_response_send(res);
}

// HTML 响应助手函数
void send_html_response(uvhttp_response_t* res, int status, const char* html_data) {
    uvhttp_response_set_status(res, status);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");
    uvhttp_response_set_body(res, html_data, strlen(html_data));
    uvhttp_response_send(res);
}

// 错误响应助手函数
void send_error_response(uvhttp_response_t* res, int status, const char* error, const char* message) {
    char response[512];
    snprintf(response, sizeof(response),
        "{\"error\":\"%s\",\"message\":\"%s\",\"status\":%d}",
        error, message, status);
    
    send_json_response(res, status, response);
}

// 使用统一响应的处理器
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* method = uvhttp_request_get_method(req);
    
    if (strcmp(method, "GET") == 0) {
        // GET 请求 - 返回数据
        const char* json = "{\"data\":[{\"id\":1,\"name\":\"Item 1\"},{\"id\":2,\"name\":\"Item 2\"}]}";
        send_json_response(res, 200, json);
    } else if (strcmp(method, "POST") == 0) {
        // POST 请求 - 创建资源
        const char* body = uvhttp_request_get_body(req);
        if (!body) {
            send_error_response(res, 400, "missing_body", "请求体缺失");
        } else {
            const char* json = "{\"status\":\"created\",\"id\":123}";
            send_json_response(res, 201, json);
        }
    } else {
        // 不支持的方法
        send_error_response(res, 405, "method_not_allowed", "不支持的 HTTP 方法");
    }
    
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/api", api_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试:\n");
    printf("  curl http://localhost:8080/api\n");
    printf("  curl -X POST http://localhost:8080/api -d '{\"name\":\"test\"}'\n");
    printf("  curl -X PUT http://localhost:8080/api -d '{\"name\":\"test\"}'\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

#### 6.2 流式响应

创建 `streaming_response.c`：

```c
#include "uvhttp.h"
#include <stdio.h>
#include <string.h>

// 流式数据处理器
int stream_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 设置流式响应头
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_header(res, "Transfer-Encoding", "chunked");
    uvhttp_response_set_header(res, "Cache-Control", "no-cache");
    
    // 发送初始响应头
    uvhttp_response_send(res);
    
    // 注意：实际的流式响应需要更复杂的实现
    // 这里只是演示概念
    
    // 在实际应用中，你可以：
    // 1. 使用 libuv 的异步写入
    // 2. 分批发送数据
    // 3. 保持连接打开，持续发送数据
    
    const char* message = "流式响应数据\n";
    uvhttp_response_set_body(res, message, strlen(message));
    
    return 0;
}

// 服务器推送事件 (SSE) 处理器
int sse_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 设置 SSE 响应头
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/event-stream");
    uvhttp_response_set_header(res, "Cache-Control", "no-cache");
    uvhttp_response_set_header(res, "Connection", "keep-alive");
    
    // 发送响应头
    uvhttp_response_send(res);
    
    // 注意：实际的 SSE 需要持续发送事件
    // 这里只是演示概念
    
    const char* event = "event: message\ndata: Hello from SSE\n\n";
    uvhttp_response_set_body(res, event, strlen(event));
    
    return 0;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/stream", stream_handler);
    uvhttp_router_add_route(router, "/sse", sse_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("测试:\n");
    printf("  curl http://localhost:8080/stream\n");
    printf("  curl -N http://localhost:8080/sse\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    uvhttp_server_free(server);
    
    return 0;
}
```

---

## 第三部分：高级架构

### 第7章：使用 libuv 数据指针

#### 7.1 为什么需要数据指针

在开发 HTTP 服务器时，我们经常需要存储应用状态，如：
- 服务器配置
- 请求计数器
- 数据库连接池
- 缓存对象

**传统方法的问题**：
```c
// ❌ 使用全局变量 - 线程不安全
static uvhttp_server_t* g_server = NULL;
static int g_request_count = 0;
```

**更好的方法**：
```c
// ✅ 使用 libuv 数据指针 - 线程安全
typedef struct {
    uvhttp_server_t* server;
    int request_count;
    // 其他应用数据...
} app_context_t;

// 将上下文存储在事件循环中
loop->data = ctx;
```

#### 7.2 创建应用上下文

```c
#include "uvhttp.h"
#include <time.h>

/**
 * @brief 应用上下文结构
 * 
 * 封装所有应用相关的数据
 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    int request_count;
    time_t start_time;
    char server_name[64];
} app_context_t;

/**
 * @brief 创建应用上下文
 */
app_context_t* app_context_create(uv_loop_t* loop, const char* name) {
    // 分配内存
    app_context_t* ctx = (app_context_t*)malloc(sizeof(app_context_t));
    if (!ctx) {
        return NULL;
    }
    
    // 初始化
    ctx->server = NULL;
    ctx->router = NULL;
    ctx->request_count = 0;
    ctx->start_time = time(NULL);
    strncpy(ctx->server_name, name, sizeof(ctx->server_name) - 1);
    
    // 创建服务器
    ctx->server = uvhttp_server_new(loop);
    if (!ctx->server) {
        free(ctx);
        return NULL;
    }
    
    // 创建路由器
    ctx->router = uvhttp_router_new();
    if (!ctx->router) {
        uvhttp_server_free(ctx->server);
        free(ctx);
        return NULL;
    }
    
    // 设置路由器
    uvhttp_server_set_router(ctx->server, ctx->router);
    
    // 将上下文设置到事件循环
    loop->data = ctx;
    
    return ctx;
}

/**
 * @brief 销毁应用上下文
 */
void app_context_destroy(app_context_t* ctx, uv_loop_t* loop) {
    if (!ctx) return;
    
    // 清理服务器
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
    }
    
    // 重置 data 指针
    loop->data = NULL;
    
    free(ctx);
}
```

#### 7.3 在处理器中访问上下文

```c
// 便捷宏
#define GET_CTX(loop) ((app_context_t*)((loop)->data))

/**
 * @brief 统计处理器
 */
int stats_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 获取事件循环
    uv_loop_t* loop = uv_default_loop();
    
    // 获取应用上下文
    app_context_t* ctx = GET_CTX(loop);
    
    // 检查上下文是否存在
    if (!ctx) {
        const char* error = "{\"error\":\"上下文未初始化\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json");
        uvhttp_response_set_body(res, error, strlen(error));
        return uvhttp_response_send(res);
    }
    
    // 使用上下文数据
    ctx->request_count++;
    
    long uptime = time(NULL) - ctx->start_time;
    
    char response[512];
    snprintf(response, sizeof(response),
        "{\n"
        "  \"server_name\": \"%s\",\n"
        "  \"request_count\": %d,\n"
        "  \"uptime_seconds\": %ld,\n"
        "  \"active_connections\": %zu\n"
        "}",
        ctx->server_name,
        ctx->request_count,
        uptime,
        ctx->server ? ctx->server->active_connections : 0);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}
```

#### 7.4 完整示例

```c
int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    
    // 创建应用上下文
    app_context_t* ctx = app_context_create(loop, "MyServer");
    if (!ctx) {
        fprintf(stderr, "错误: 无法创建应用上下文\n");
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(ctx->router, "/stats", stats_handler);
    
    // 启动服务器
    uvhttp_server_listen(ctx->server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    app_context_destroy(ctx, loop);
    
    return 0;
}
```

#### 7.5 多线程环境中的使用

```c
// 工作线程上下文
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    app_context_t* app_ctx;  // 共享的应用上下文
    pthread_mutex_t mutex;
} worker_context_t;

void* worker_thread(void* arg) {
    worker_context_t* worker = (worker_context_t*)arg;
    
    // 创建独立的事件循环
    worker->loop = uv_loop_new();
    
    // 创建线程特定的上下文
    app_context_t* thread_ctx = malloc(sizeof(app_context_t));
    thread_ctx->server = uvhttp_server_new(worker->loop);
    thread_ctx->router = uvhttp_router_new();
    
    // 设置到事件循环
    worker->loop->data = thread_ctx;
    
    // 运行事件循环
    uv_run(worker->loop, UV_RUN_DEFAULT);
    
    return NULL;
}
```

**详细教程**：参见 [libuv 数据指针完整指南](LIBUV_DATA_POINTER.md)

### 第8章：多线程服务器

#### 7.1 理解多线程架构

**单线程 vs 多线程**：

```
单线程模型：
┌─────────────────┐
│  Event Loop     │
│  (主线程)        │
└─────────────────┘
    ↓
┌─────────────────┐
│  所有请求       │
│  串行处理       │
└─────────────────┘

多线程模型：
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  Event Loop     │  │  Event Loop     │  │  Event Loop     │
│  (线程 1)        │  │  (线程 2)        │  │  (线程 3)        │
└─────────────────┘  └─────────────────┘  └─────────────────┘
       ↓                    ↓                    ↓
┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐
│  部分请求       │  │  部分请求       │  │  部分请求       │
│  并行处理       │  │  并行处理       │  │  并行处理       │
└─────────────────┘  └─────────────────┘  └─────────────────┘
```

#### 7.2 多线程服务器实现

创建 `multithreaded_server.c`：

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define THREAD_COUNT 4
#define PORT_BASE 8080

// 线程数据结构
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    uv_async_t async;
    int is_running;
} worker_thread_t;

static worker_thread_t workers[THREAD_COUNT];

// 请求处理器
int request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    char response[256];
    snprintf(response, sizeof(response),
        "{\"message\":\"Hello from thread\",\"thread_id\":%d}",
        pthread_self() % 1000);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    
    return uvhttp_response_send(res);
}

// 工作线程函数
void* worker_thread_func(void* arg) {
    worker_thread_t* worker = (worker_thread_t*)arg;
    
    printf("工作线程 %d 启动\n", worker->thread_id);
    
    // 创建事件循环
    worker->loop = uv_loop_new();
    
    // 创建服务器
    worker->server = uvhttp_server_new(worker->loop);
    if (!worker->server) {
        fprintf(stderr, "线程 %d: 服务器创建失败\n", worker->thread_id);
        return NULL;
    }
    
    // 创建路由器
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", request_handler);
    uvhttp_server_set_router(worker->server, router);
    
    // 启动服务器监听（每个线程监听不同端口）
    int port = PORT_BASE + worker->thread_id;
    int result = uvhttp_server_listen(worker->server, "0.0.0.0", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "线程 %d: 服务器启动失败: %d\n", worker->thread_id, result);
        return NULL;
    }
    
    printf("线程 %d: 服务器监听端口 %d\n", worker->thread_id, port);
    
    // 运行事件循环
    worker->is_running = 1;
    uv_run(worker->loop, UV_RUN_DEFAULT);
    
    printf("工作线程 %d 退出\n", worker->thread_id);
    
    return NULL;
}

// 优雅关闭
void shutdown_handler(uv_async_t* async) {
    worker_thread_t* worker = (worker_thread_t*)async->data;
    
    printf("关闭线程 %d\n", worker->thread_id);
    
    // 停止服务器
    if (worker->server) {
        uvhttp_server_stop(worker->server);
    }
    
    // 停止事件循环
    if (worker->loop) {
        uv_stop(worker->loop);
    }
    
    worker->is_running = 0;
}

int main() {
    pthread_t threads[THREAD_COUNT];
    
    printf("启动多线程 HTTP 服务器\n");
    printf("线程数: %d\n", THREAD_COUNT);
    printf("端口范围: %d-%d\n", PORT_BASE, PORT_BASE + THREAD_COUNT - 1);
    
    // 创建工作线程
    for (int i = 0; i < THREAD_COUNT; i++) {
        workers[i].thread_id = i;
        workers[i].is_running = 0;
        
        // 初始化异步句柄
        uv_async_init(uv_default_loop(), &workers[i].async, shutdown_handler);
        workers[i].async.data = &workers[i];
        
        // 创建线程
        int result = pthread_create(&threads[i], NULL, worker_thread_func, &workers[i]);
        if (result != 0) {
            fprintf(stderr, "创建线程 %d 失败\n", i);
            return 1;
        }
    }
    
    printf("所有线程已启动\n");
    printf("按 Ctrl+C 停止服务器\n");
    
    // 等待信号
    uv_run(uv_default_loop(), UV_RUN_DEFAULT);
    
    // 发送关闭信号到所有工作线程
    for (int i = 0; i < THREAD_COUNT; i++) {
        uv_async_send(&workers[i].async);
    }
    
    // 等待所有线程结束
    for (int i = 0; i < THREAD_COUNT; i++) {
        pthread_join(threads[i], NULL);
        
        // 清理资源
        if (workers[i].server) {
            uvhttp_server_free(workers[i].server);
        }
        if (workers[i].loop) {
            uv_loop_close(workers[i].loop);
            free(workers[i].loop);
        }
    }
    
    printf("服务器已关闭\n");
    
    return 0;
}
```

**编译和运行**：
```bash
gcc -o multithreaded_server multithreaded_server.c \
    -I../include \
    -L../build \
    -luvhttp -luv -lpthread

./multithreaded_server
```

**测试**：
```bash
# 测试不同线程
curl http://localhost:8080/
curl http://localhost:8081/
curl http://localhost:8082/
curl http://localhost:8083/
```

---

### 第9章：异步数据库集成

#### 8.1 异步数据库连接

创建 `async_database.c`：

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

// 数据库连接结构
typedef struct {
    char host[128];
    int port;
    char database[64];
    char username[64];
    char password[64];
    int is_connected;
    pthread_mutex_t mutex;
} database_connection_t;

// 数据库查询请求
typedef struct {
    uvhttp_request_t* request;
    uvhttp_response_t* response;
    char query[512];
    database_connection_t* db;
} db_query_request_t;

static database_connection_t g_db = {0};

// 初始化数据库连接
int db_init(database_connection_t* db) {
    strcpy(db->host, "localhost");
    db->port = 3306;
    strcpy(db->database, "testdb");
    strcpy(db->username, "root");
    strcpy(db->password, "password");
    db->is_connected = 0;
    pthread_mutex_init(&db->mutex, NULL);
    
    // 在实际应用中，这里应该建立真实的数据库连接
    printf("数据库连接初始化: %s@%s:%d/%s\n", 
           db->username, db->host, db->port, db->database);
    
    return 0;
}

// 异步数据库查询回调
void on_db_query_complete(uv_work_t* req, int status) {
    db_query_request_t* query_req = (db_query_request_t*)req->data;
    
    // 模拟查询结果
    char result[1024];
    snprintf(result, sizeof(result),
        "{\"status\":\"success\",\"query\":\"%s\",\"data\":[{\"id\":1,\"name\":\"Item 1\"},{\"id\":2,\"name\":\"Item 2\"}]}",
        query_req->query);
    
    // 发送响应
    uvhttp_response_set_status(query_req->response, 200);
    uvhttp_response_set_header(query_req->response, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(query_req->response, result, strlen(result));
    uvhttp_response_send(query_req->response);
    
    // 清理
    free(query_req);
    free(req);
}

// 数据库查询工作函数
void db_query_work(uv_work_t* req) {
    db_query_request_t* query_req = (db_query_request_t*)req->data;
    
    // 模拟数据库查询（实际应用中应该执行真实的数据库操作）
    printf("执行查询: %s\n", query_req->query);
    
    // 加锁保护数据库连接
    pthread_mutex_lock(&query_req->db->mutex);
    
    // 模拟查询延迟
    usleep(10000); // 10ms
    
    pthread_mutex_unlock(&query_req->db->mutex);
}

// API 处理器 - 获取用户列表
int get_users_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 创建异步查询请求
    db_query_request_t* query_req = malloc(sizeof(db_query_request_t));
    if (!query_req) {
        const char* error = "{\"error\":\"内存分配失败\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }
    
    query_req->request = req;
    query_req->response = res;
    strcpy(query_req->query, "SELECT * FROM users");
    query_req->db = &g_db;
    
    // 创建工作请求
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = query_req;
    
    // 在线程池中执行数据库查询
    uv_queue_work(uv_default_loop(), work_req, db_query_work, on_db_query_complete);
    
    return 0; // 异步处理，不立即发送响应
}

// API 处理器 - 创建用户
int create_user_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* body = uvhttp_request_get_body(req);
    
    if (!body) {
        const char* error = "{\"error\":\"请求体为空\"}";
        uvhttp_response_set_status(res, 400);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }
    
    // 创建异步查询请求
    db_query_request_t* query_req = malloc(sizeof(db_query_request_t));
    if (!query_req) {
        const char* error = "{\"error\":\"内存分配失败\"}";
        uvhttp_response_set_status(res, 500);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }
    
    query_req->request = req;
    query_req->response = res;
    snprintf(query_req->query, sizeof(query_req->query), 
             "INSERT INTO users VALUES (%s)", body);
    query_req->db = &g_db;
    
    // 创建工作请求
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = query_req;
    
    // 在线程池中执行数据库查询
    uv_queue_work(uv_default_loop(), work_req, db_query_work, on_db_query_complete);
    
    return 0;
}

int main() {
    // 初始化数据库
    db_init(&g_db);
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加 API 路由
    uvhttp_router_add_route(router, "/api/users", get_users_handler);
    uvhttp_router_add_route(router, "/api/users", create_user_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("异步数据库集成演示\n");
    printf("测试:\n");
    printf("  curl http://localhost:8080/api/users\n");
    printf("  curl -X POST http://localhost:8080/api/users -d '{\"name\":\"test\"}'\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    pthread_mutex_destroy(&g_db.mutex);
    uvhttp_server_free(server);
    
    return 0;
}
```

#### 8.2 连接池管理

创建 `connection_pool.c`：

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define MAX_CONNECTIONS 10

// 数据库连接结构
typedef struct {
    int id;
    int is_used;
    time_t last_used;
    pthread_mutex_t mutex;
} db_connection_t;

// 连接池结构
typedef struct {
    db_connection_t connections[MAX_CONNECTIONS];
    int total_connections;
    pthread_mutex_t pool_mutex;
} connection_pool_t;

static connection_pool_t g_pool = {0};

// 初始化连接池
void connection_pool_init(connection_pool_t* pool, int size) {
    pool->total_connections = size;
    pthread_mutex_init(&pool->pool_mutex, NULL);
    
    for (int i = 0; i < size; i++) {
        pool->connections[i].id = i;
        pool->connections[i].is_used = 0;
        pool->connections[i].last_used = 0;
        pthread_mutex_init(&pool->connections[i].mutex, NULL);
    }
    
    printf("连接池初始化完成，最大连接数: %d\n", size);
}

// 获取连接
db_connection_t* connection_pool_acquire(connection_pool_t* pool) {
    pthread_mutex_lock(&pool->pool_mutex);
    
    db_connection_t* conn = NULL;
    
    // 查找可用连接
    for (int i = 0; i < pool->total_connections; i++) {
        if (!pool->connections[i].is_used) {
            pool->connections[i].is_used = 1;
            pool->connections[i].last_used = time(NULL);
            conn = &pool->connections[i];
            break;
        }
    }
    
    pthread_mutex_unlock(&pool->pool_mutex);
    
    if (conn) {
        printf("获取连接 %d\n", conn->id);
    } else {
        printf("警告: 无可用连接\n");
    }
    
    return conn;
}

// 释放连接
void connection_pool_release(connection_pool_t* pool, db_connection_t* conn) {
    if (!conn) return;
    
    pthread_mutex_lock(&pool->pool_mutex);
    
    conn->is_used = 0;
    conn->last_used = time(NULL);
    
    printf("释放连接 %d\n", conn->id);
    
    pthread_mutex_unlock(&pool->pool_mutex);
}

// API 处理器 - 使用连接池
int api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 从连接池获取连接
    db_connection_t* conn = connection_pool_acquire(&g_pool);
    
    if (!conn) {
        const char* error = "{\"error\":\"无可用数据库连接\"}";
        uvhttp_response_set_status(res, 503);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }
    
    // 使用连接执行查询（模拟）
    pthread_mutex_lock(&conn->mutex);
    printf("使用连接 %d 执行查询\n", conn->id);
    usleep(5000); // 模拟查询延迟
    pthread_mutex_unlock(&conn->mutex);
    
    // 发送响应
    const char* json = "{\"status\":\"success\",\"connection_id\":1}";
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, json, strlen(json));
    uvhttp_response_send(res);
    
    // 释放连接
    connection_pool_release(&g_pool, conn);
    
    return 0;
}

int main() {
    // 初始化连接池
    connection_pool_init(&g_pool, MAX_CONNECTIONS);
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    uvhttp_router_add_route(router, "/api", api_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://localhost:8080\n");
    printf("连接池管理演示\n");
    printf("测试: curl http://localhost:8080/api\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    pthread_mutex_destroy(&g_pool.pool_mutex);
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        pthread_mutex_destroy(&g_pool.connections[i].mutex);
    }
    
    uvhttp_server_free(server);
    
    return 0;
}
```

---

### 第10章：负载均衡

#### 9.1 理解负载均衡

**负载均衡策略**：

```
客户端请求
    ↓
负载均衡器
    ↓
┌──────────┬──────────┬──────────┐
│ 服务器 1 │ 服务器 2 │ 服务器 3 │
│ (线程 1) │ (线程 2) │ (线程 3) │
└──────────┴──────────┴──────────┘
```

#### 9.2 简单负载均衡器

创建 `load_balancer.c`：

```c
#include "uvhttp.h"
#include <uv.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

#define BACKEND_COUNT 3
#define BACKEND_PORTS {8081, 8082, 8083}

// 后端服务器信息
typedef struct {
    char host[128];
    int port;
    int is_healthy;
    int request_count;
    pthread_mutex_t mutex;
} backend_server_t;

static backend_server_t backends[BACKEND_COUNT];

// 初始化后端服务器
void init_backends() {
    int ports[] = BACKEND_PORTS;
    
    for (int i = 0; i < BACKEND_COUNT; i++) {
        strcpy(backends[i].host, "localhost");
        backends[i].port = ports[i];
        backends[i].is_healthy = 1;
        backends[i].request_count = 0;
        pthread_mutex_init(&backends[i].mutex, NULL);
        
        printf("后端服务器 %d: %s:%d\n", i, backends[i].host, backends[i].port);
    }
}

// 轮询算法选择后端
backend_server_t* select_backend_round_robin() {
    static int current = 0;
    
    for (int i = 0; i < BACKEND_COUNT; i++) {
        int index = (current + i) % BACKEND_COUNT;
        if (backends[index].is_healthy) {
            current = (index + 1) % BACKEND_COUNT;
            return &backends[index];
        }
    }
    
    return NULL;
}

// 最少连接算法选择后端
backend_server_t* select_backend_least_connections() {
    backend_server_t* selected = NULL;
    int min_connections = -1;
    
    for (int i = 0; i < BACKEND_COUNT; i++) {
        if (backends[i].is_healthy) {
            pthread_mutex_lock(&backends[i].mutex);
            int connections = backends[i].request_count;
            pthread_mutex_unlock(&backends[i].mutex);
            
            if (min_connections == -1 || connections < min_connections) {
                min_connections = connections;
                selected = &backends[i];
            }
        }
    }
    
    return selected;
}

// 负载均衡处理器
int load_balance_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // 选择后端服务器（使用轮询算法）
    backend_server_t* backend = select_backend_round_robin();
    
    if (!backend) {
        const char* error = "{\"error\":\"无可用后端服务器\"}";
        uvhttp_response_set_status(res, 503);
        uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
        uvhttp_response_set_body(res, error, strlen(error));
        uvhttp_response_send(res);
        return 0;
    }
    
    // 增加请求计数
    pthread_mutex_lock(&backend->mutex);
    backend->request_count++;
    pthread_mutex_unlock(&backend->mutex);
    
    // 模拟转发请求到后端
    printf("转发请求到后端: %s:%d\n", backend->host, backend->port);
    
    // 发送响应
    char response[512];
    snprintf(response, sizeof(response),
        "{\"status\":\"forwarded\",\"backend\":\"%s:%d\",\"request_count\":%d}",
        backend->host, backend->port, backend->request_count);
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    uvhttp_response_send(res);
    
    // 减少请求计数
    pthread_mutex_lock(&backend->mutex);
    backend->request_count--;
    pthread_mutex_unlock(&backend->mutex);
    
    return 0;
}

// 健康检查处理器
int health_check_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    char response[1024];
    int pos = 0;
    
    pos += snprintf(response + pos, sizeof(response) - pos, "{\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "  \"backends\": [\n");
    
    for (int i = 0; i < BACKEND_COUNT; i++) {
        pthread_mutex_lock(&backends[i].mutex);
        pos += snprintf(response + pos, sizeof(response) - pos,
                       "    {\"id\":%d,\"host\":\"%s\",\"port\":%d,\"healthy\":%s,\"connections\":%d}%s\n",
                       i, backends[i].host, backends[i].port,
                       backends[i].is_healthy ? "true" : "false",
                       backends[i].request_count,
                       i < BACKEND_COUNT - 1 ? "," : "");
        pthread_mutex_unlock(&backends[i].mutex);
    }
    
    pos += snprintf(response + pos, sizeof(response) - pos, "  ]\n");
    pos += snprintf(response + pos, sizeof(response) - pos, "}\n");
    
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json; charset=utf-8");
    uvhttp_response_set_body(res, response, strlen(response));
    uvhttp_response_send(res);
    
    return 0;
}

int main() {
    // 初始化后端服务器
    init_backends();
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加路由
    uvhttp_router_add_route(router, "/", load_balance_handler);
    uvhttp_router_add_route(router, "/health", health_check_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("负载均衡器运行在 http://localhost:8080\n");
    printf("后端服务器:\n");
    for (int i = 0; i < BACKEND_COUNT; i++) {
        printf("  %d. %s:%d\n", i, backends[i].host, backends[i].port);
    }
    printf("测试:\n");
    printf("  curl http://localhost:8080/\n");
    printf("  curl http://localhost:8080/health\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    for (int i = 0; i < BACKEND_COUNT; i++) {
        pthread_mutex_destroy(&backends[i].mutex);
    }
    
    uvhttp_server_free(server);
    
    return 0;
}
```

---

## 第四部分：生产实践

### 第11章：性能优化

#### 10.1 内存优化

```c
// 使用内存池
typedef struct {
    void* pool;
    size_t block_size;
    size_t block_count;
    pthread_mutex_t mutex;
} memory_pool_t;

// 预分配内存
memory_pool_t* create_memory_pool(size_t block_size, size_t block_count) {
    memory_pool_t* pool = malloc(sizeof(memory_pool_t));
    pool->block_size = block_size;
    pool->block_count = block_count;
    pool->pool = malloc(block_size * block_count);
    pthread_mutex_init(&pool->mutex, NULL);
    
    return pool;
}

void* memory_pool_alloc(memory_pool_t* pool) {
    pthread_mutex_lock(&pool->mutex);
    // 从池中分配内存
    pthread_mutex_unlock(&pool->mutex);
    return NULL;
}
```

#### 10.2 连接优化

```c
// 启用 Keep-Alive
uvhttp_config_t* config = uvhttp_config_new();
config->keepalive_timeout = 30; // 30秒
server->config = config;

// 连接复用
// 在实际应用中，实现连接池和复用逻辑
```

---

### 第12章：安全配置

#### 11.1 TLS/SSL 配置

```c
// 启用 TLS
#if UVHTTP_FEATURE_TLS
uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
uvhttp_tls_context_load_cert(tls_ctx, "server.crt", "server.key");
uvhttp_server_enable_tls(server, tls_ctx);
#endif
```

#### 11.2 安全头设置

```c
// 设置安全响应头
int secure_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_header(res, "X-Content-Type-Options", "nosniff");
    uvhttp_response_set_header(res, "X-Frame-Options", "DENY");
    uvhttp_response_set_header(res, "X-XSS-Protection", "1; mode=block");
    uvhttp_response_set_header(res, "Strict-Transport-Security", "max-age=31536000");
    
    const char* json = "{\"message\":\"安全响应\"}";
    uvhttp_response_set_body(res, json, strlen(json));
    
    return uvhttp_response_send(res);
}
```

---

### 第13章：监控和日志

#### 12.1 请求日志

```c
// 日志记录
void log_request(uvhttp_request_t* req, int status, size_t response_size) {
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    const char* method = uvhttp_request_get_method(req);
    const char* url = uvhttp_request_get_url(req);
    const char* user_agent = uvhttp_request_get_header(req, "User-Agent");
    
    printf("[%s] %s %s %d %zu \"%s\"\n", 
           time_str, method, url, status, response_size, 
           user_agent ? user_agent : "-");
}
```

#### 12.2 性能监控

```c
// 性能统计
typedef struct {
    size_t total_requests;
    size_t total_bytes_sent;
    size_t total_bytes_received;
    double avg_response_time;
    pthread_mutex_t mutex;
} performance_stats_t;

static performance_stats_t g_stats = {0};

void update_stats(size_t bytes_sent, double response_time) {
    pthread_mutex_lock(&g_stats.mutex);
    g_stats.total_requests++;
    g_stats.total_bytes_sent += bytes_sent;
    g_stats.avg_response_time = 
        (g_stats.avg_response_time * (g_stats.total_requests - 1) + response_time) / 
        g_stats.total_requests;
    pthread_mutex_unlock(&g_stats.mutex);
}
```

---

## 总结

本教程涵盖了 UVHTTP 从入门到精通的完整学习路径：

1. **入门基础**：Hello World、核心概念、路由系统
2. **进阶开发**：复杂路由、请求处理、响应优化
3. **高级架构**：libuv 数据指针、多线程、异步数据库、负载均衡
4. **生产实践**：性能优化、安全配置、监控日志
5. **物联网通信**：实时通信、设备管理、消息推送

### 下一步

- 查看 `examples/` 目录中的完整示例
- 阅读 `docs/API_REFERENCE.md` 了解完整 API
- 参考 `docs/ARCHITECTURE.md` 了解架构设计
- 运行测试套件 `make test`

### 最佳实践总结

1. **使用核心 API**：避免过度抽象，直接使用核心函数
2. **异步优先**：充分利用 libuv 的异步特性
3. **错误处理**：检查所有返回值，处理错误情况
4. **内存管理**：使用统一分配器，避免内存泄漏
5. **性能优化**：合理配置连接数、缓冲区大小等参数
6. **安全第一**：启用 TLS，设置安全头，验证输入
7. **监控日志**：记录请求日志，监控性能指标
8. **避免全局变量**：使用 libuv 数据指针存储应用状态
9. **线程安全**：在多线程环境中使用互斥锁保护共享数据
10. **上下文管理**：创建和销毁上下文时遵循 RAII 原则

---

## 附录：快速参考

### A. 常用代码片段

#### 编译 UVHTTP
```bash
# 克隆仓库
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 编译（使用项目自带的依赖）
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

#### 编译示例程序
```bash
# 编译所有示例
cd build
cmake ..
make examples

# 编译特定示例
make hello_world
make simple_routing

# 运行示例
./examples/hello_world
./examples/simple_routing
```

#### 创建服务器

```c
uv_loop_t* loop = uv_default_loop();
uvhttp_server_t* server = uvhttp_server_new(loop);
uvhttp_router_t* router = uvhttp_router_new();
uvhttp_server_set_router(server, router);

uvhttp_router_add_route(router, "/", handler);
uvhttp_server_listen(server, "0.0.0.0", 8080);

uv_run(loop, UV_RUN_DEFAULT);
uvhttp_server_free(server);
```

#### 使用应用上下文

```c
typedef struct {
    uvhttp_server_t* server;
    int count;
} app_context_t;

app_context_t* ctx = malloc(sizeof(app_context_t));
ctx->server = uvhttp_server_new(loop);
loop->data = ctx;

// 在处理器中访问
app_context_t* ctx = (app_context_t*)loop->data;
ctx->count++;
```

#### JSON 响应

```c
const char* json = "{\"message\":\"Hello\"}";
uvhttp_response_set_status(res, 200);
uvhttp_response_set_header(res, "Content-Type", "application/json");
uvhttp_response_set_body(res, json, strlen(json));
return uvhttp_response_send(res);
```

#### 错误处理

```c
int result = uvhttp_server_listen(server, host, port);
if (result != UVHTTP_OK) {
    fprintf(stderr, "错误: %d\n", result);
    // 清理资源
    return 1;
}
```

### B. 配置参数

| 参数 | 默认值 | 说明 |
|------|--------|------|
| max_connections | 1000 | 最大连接数 |
| max_body_size | 1048576 | 最大请求体大小 (1MB) |
| read_buffer_size | 8192 | 读取缓冲区大小 |
| keepalive_timeout | 30 | Keep-Alive 超时 (秒) |
| request_timeout | 60 | 请求超时 (秒) |

### C. HTTP 状态码

| 状态码 | 含义 | 使用场景 |
|--------|------|----------|
| 200 | OK | 成功响应 |
| 201 | Created | 资源创建成功 |
| 400 | Bad Request | 请求参数错误 |
| 401 | Unauthorized | 未认证 |
| 403 | Forbidden | 无权限 |
| 404 | Not Found | 资源不存在 |
| 500 | Internal Server Error | 服务器错误 |

### D. 常见 Content-Type

| 类型 | Content-Type |
|------|--------------|
| JSON | application/json |
| HTML | text/html; charset=utf-8 |
| 纯文本 | text/plain; charset=utf-8 |
| XML | application/xml |
| CSS | text/css |
| JavaScript | application/javascript |
| WebSocket | websocket |
| 静态文件 | 根据文件扩展名自动检测 |

### E. 静态文件服务配置

```c
// 配置静态文件服务
uvhttp_static_config_t static_config = {
    .root_directory = "./public",
    .index_file = "index.html",
    .enable_directory_listing = 1,
    .enable_etag = 1,
    .enable_last_modified = 1,
    .max_cache_size = 10 * 1024 * 1024,
    .cache_ttl = 3600
};

// 创建上下文
uvhttp_static_context_t* ctx = uvhttp_static_create(&static_config);

// 处理请求
uvhttp_static_handle_request(ctx, req, res);
```

### F. WebSocket 配置

```c
// WebSocket 处理器
uvhttp_ws_handler_t ws_handler;
ws_handler.on_connect = ws_connect_handler;
ws_handler.on_message = ws_message_handler;
ws_handler.on_close = ws_close_handler;

// 注册处理器
uvhttp_server_register_ws_handler(server, "/ws", &ws_handler);

// 发送消息
uvhttp_server_ws_send(ws_conn, data, len);

// 关闭连接
uvhttp_server_ws_close(ws_conn, 1000, "Normal closure");
```

### F. 性能优化建议

1. **连接池**：复用数据库连接
2. **缓存**：使用 LRU 缓存减少数据库查询
3. **压缩**：启用响应压缩
4. **异步**：使用异步 I/O 避免阻塞
5. **负载均衡**：多线程或多进程处理请求
6. **监控**：实时监控性能指标
7. **日志**：记录关键操作和错误
8. **静态文件**：启用文件缓存和 ETag
9. **WebSocket**：使用连接池管理 WebSocket 连接
10. **内存分配**：使用 mimalloc 提高内存分配性能

### G. 安全检查清单

- [ ] 启用 TLS/SSL
- [ ] 设置安全响应头
- [ ] 验证所有输入
- [ ] 防止 SQL 注入
- [ ] 防止 XSS 攻击
- [ ] 限制请求速率
- [ ] 使用强密码
- [ ] 定期更新依赖
- [ ] 启用日志审计
- [ ] 实施访问控制
- [ ] 静态文件路径验证（防止目录遍历）
- [ ] WebSocket 连接认证
- [ ] WebSocket 消息大小限制
- [ ] 文件上传大小限制
- [ ] 文件类型白名单

---

## 附录：依赖管理和编译

### 依赖说明

UVHTTP 采用自包含的依赖管理方式，所有必需的依赖都包含在 `deps/` 目录中：

| 依赖 | 目录 | 用途 |
|------|------|------|
| libuv | `deps/libuv/` | 异步 I/O 库，事件循环核心 |
| llhttp | `deps/llhttp/` | HTTP 解析器 |
| mbedtls | `deps/mbedtls/` | TLS/SSL 支持 |
| cjson | `deps/cjson/` | JSON 解析和生成 |
| mimalloc | `deps/mimalloc/` | 高性能内存分配器 |
| uthash | `deps/uthash/` | 哈希表实现 |
| xxhash | `deps/xxhash/` | 快速哈希算法 |

### 编译选项

**调试版本**：
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

**发布版本**：
```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

**启用特定功能**：
```bash
# 启用 TLS 支持
cmake -DUVHTTP_FEATURE_TLS=ON ..

# 启用 WebSocket 支持
cmake -DUVHTTP_FEATURE_WEBSOCKET=ON ..

# 禁用 mimalloc（使用系统 malloc）
cmake -DUVHTTP_HAS_MIMALLOC=OFF ..
```

### 编译示例程序

**使用 CMake 编译单个示例**：
```bash
# 在项目根目录
cd build
cmake ..

# 编译特定示例
make hello_world
make simple_routing
make method_routing

# 或编译所有示例
make examples

# 运行示例
./examples/hello_world
./examples/simple_routing
./examples/method_routing
```

**手动创建 CMakeLists.txt（可选）**：
```bash
# 在 examples/ 目录创建 CMakeLists.txt
cat > CMakeLists.txt << 'EOF'
cmake_minimum_required(VERSION 3.10)
project(uvhttp_examples C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../include NO_DEFAULT_PATH)
find_library(UVHTTP_LIBRARY uvhttp PATHS ../build NO_DEFAULT_PATH)

include_directories(${UVHTTP_INCLUDE_DIR})

# 添加示例
add_subdirectory(01_basics)
add_subdirectory(02_routing)
EOF

# 在 01_basics/CMakeLists.txt
cat > 01_basics/CMakeLists.txt << 'EOF'
add_executable(hello_world 01_hello_world.c)
target_link_libraries(hello_world ${UVHTTP_LIBRARY} uv pthread m)
EOF
```

**快速编译脚本**：
```bash
#!/bin/bash
# build_example.sh
EXAMPLE_NAME=$1

cd build
cmake .. > /dev/null 2>&1
make $EXAMPLE_NAME

if [ $? -eq 0 ]; then
    echo "编译成功: $EXAMPLE_NAME"
    echo "运行: ./examples/$EXAMPLE_NAME"
else
    echo "编译失败: $EXAMPLE_NAME"
fi
```

使用：
```bash
chmod +x build_example.sh
./build_example.sh hello_world
```

### 常见编译问题

**问题 1：找不到头文件**
```bash
error: uvhttp.h: No such file or directory
```
**解决方案**：确保包含路径正确
```bash
gcc -I../include ...
```

**问题 2：链接错误**
```bash
undefined reference to `uvhttp_server_new'
```
**解决方案**：确保链接了 UVHTTP 库
```bash
gcc -L../build -luvhttp ...
```

**问题 3：运行时找不到库**
```bash
error while loading shared libraries: libuvhttp.so
```
**解决方案**：设置库路径
```bash
export LD_LIBRARY_PATH=../build:$LD_LIBRARY_PATH
```

---

## 附录：应用内负载均衡补充

### 应用内负载均衡 vs 外部网关

**应用内负载均衡**（推荐用于简单场景）：
- ✅ 无需额外组件
- ✅ 减少网络跳数
- ✅ 更简单的部署
- ✅ 更低的延迟
- ❌ 功能相对简单
- ❌ 扩展性有限

**外部负载均衡**（推荐用于生产环境）：
- ✅ 功能强大
- ✅ 易于扩展
- ✅ 支持多种算法
- ❌ 需要额外部署
- ❌ 增加网络延迟
- ❌ 更复杂的运维

### 应用内负载均衡实现

**多线程工作池模式**：
```c
#define WORKER_THREADS 4

// 工作线程上下文
typedef struct {
    int thread_id;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    int request_count;
    pthread_mutex_t mutex;
} worker_context_t;

// 轮询选择工作线程
int select_worker() {
    static int current = 0;
    return (current++) % WORKER_THREADS;
}

// 在主线程中分发请求
int request_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    int worker_id = select_worker();
    
    // 将请求信息发送给工作线程处理
    // 实际实现需要使用线程间通信机制
    
    return 0;
}
```

**单线程事件循环 + libuv 线程池**：
```c
// 使用 libuv 的线程池
void process_in_thread_pool(uv_work_t* req) {
    // 在线程池中执行耗时操作
}

void after_thread_pool(uv_work_t* req, int status) {
    // 在主线程中处理结果
}

int handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uv_work_t* work_req = malloc(sizeof(uv_work_t));
    work_req->data = req;
    
    uv_queue_work(loop, work_req, process_in_thread_pool, after_thread_pool);
    
    return 0;
}
```

### 负载均衡算法选择

| 算法 | 适用场景 | 优点 | 缺点 |
|------|----------|------|------|
| 轮询 | 请求相似 | 简单、公平 | 不考虑负载差异 |
| 最少连接 | 请求耗时不同 | 负载均衡好 | 需要维护连接计数 |
| IP 哈希 | 需要会话保持 | 同一IP到同一服务器 | 可能不均衡 |
| 随机 | 简单场景 | 简单 | 可能不均衡 |

---

## 相关资源

### 官方文档
- [API 参考](API_REFERENCE.md)
- [架构设计](ARCHITECTURE.md)
- [开发指南](DEVELOPER_GUIDE.md)
- [libuv 数据指针](LIBUV_DATA_POINTER.md)

### 示例程序
- [基础示例](../examples/01_basics/)
- [路由示例](../examples/02_routing/)
- [高级示例](../examples/05_advanced/)

### 外部资源
- [libuv 官方文档](https://docs.libuv.org/)
- [HTTP/1.1 规范](https://tools.ietf.org/html/rfc7231)
- [C 语言最佳实践](https://isocpp.github.io/CppCoreGuidelines/CppCoreGuidelines)

---

祝您使用 UVHTTP 构建高性能 HTTP 服务器！