---
title: 快速开始
description: 几分钟上手 UVHTTP——用 CMake 与 Make 构建 C99 HTTP/WebSocket 服务器库，运行 hello-world 示例，添加第一条路由。
---

# 快速开始

构建库、编译一个 C 文件、加一条路由，服务器就能跑起来。

## 📌 平台支持

**当前支持**: Linux

**未来计划**: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

> **注意**：UVHTTP 目前只支持 Linux。其他操作系统放在后续版本的计划里。

## 环境要求

- CMake 3.13 或更高版本
- C99 编译器（GCC 4.9+、Clang 3.5+）
- libuv 1.x（作为子模块自动克隆）
- llhttp（作为子模块自动克隆）

## 安装

### 安装依赖

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential
```

### 克隆并构建

```bash
# 克隆仓库（包含子模块）
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 构建项目
make build

# 运行测试（可选）
make test
```

> **注意**：libuv、llhttp 和其他依赖都靠 `--recurse-submodules` 拉下来。忘了带这个参数，事后运行 `git submodule update --init --recursive` 补上即可。

## 第一个程序

创建 `hello.c`：

```c
#include <uvhttp.h>
#include <stdio.h>
#include <string.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", strlen("Hello, World!"));
    return uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_server_take_router(server, router);

    // 添加路由
    uvhttp_router_add_route(router, "/", hello_handler);

    // 启动服务器
    uvhttp_error_t result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        return 1;
    }

    printf("Server running at http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

handler 设置状态码、Content-Type 和 body，调用 uvhttp_response_send 并把它的返回值交出去。uvhttp_server_take_router 把 router 交给 server，此后所有权归 server。

### 编译并运行

```bash
gcc -o hello hello.c -I./include -L./build/dist/lib -luvhttp -luv -lpthread
./hello
```

-I 指向公共头文件，-L 指向 build/dist/lib 下构建好的库。

访问 `http://localhost:8080` 查看 "Hello, World!"。端口被占用时，程序会打印 uvhttp_server_listen 返回的错误信息并退出。

## 项目结构

```
uvhttp/
├── include/           # 公共头文件
├── src/               # 源代码实现
├── docs/              # 文档
├── examples/          # 示例程序
├── test/              # 测试
└── build/             # 构建输出目录
```

## 配置选项

构建选项就是 `CMakeLists.txt` 里的 `option()` 条目。在配置阶段设好，再重新构建：

```bash
# 修改 CMakeLists.txt 中的选项后构建
make build
```

## 下一步

- [API 参考](/api/introduction) - 学习完整的 API
- [教程](./TUTORIAL.md) - 构建完整应用
- [WebSocket 支持](./websocket.md) - 实时消息

## 需要帮助？

- 查阅[文档](/)
- 打开 [Issue](https://github.com/adam-ikari/uvhttp/issues)
- 加入 GitHub 讨论
