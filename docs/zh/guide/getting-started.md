# 快速开始

## 📌 平台支持

**当前支持**: Linux

**未来计划**: macOS, Windows, FreeBSD, WebAssembly (WASM) 和其他 Unix-like 系统

UVHTTP 目前针对 Linux 平台进行了优化。我们计划在未来版本中扩展对其他操作系统和平台的支持。

## 环境要求

- CMake 3.10+
- C11 编译器（GCC 4.9+, Clang 3.5+）
- libuv 1.x
- llhttp

## 安装

### 从源码编译

```bash
# 克隆仓库
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp

# 创建构建目录
mkdir build && cd build

# 配置项目
cmake ..

# 编译
make -j$(nproc)

# 运行测试（可选）
make test
```

### 安装依赖

#### Ubuntu/Debian

```bash
sudo apt-get update
sudo apt-get install -y cmake build-essential libuv1-dev
```

#### macOS（未来支持）

```bash
brew install cmake libuv
```

#### Windows（未来支持）

使用 vcpkg 安装依赖：

```bash
vcpkg install libuv
```

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
    uvhttp_response_send(response);
    return UVHTTP_OK;
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    server->router = router;

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

### 编译

```bash
gcc -o hello hello.c -I./include -L./build/dist/lib -luvhttp -luv -lpthread
```

### 运行

```bash
./hello
```

访问 `http://localhost:8080` 查看 "Hello, World!"。

## 项目结构

```
uvhttp/
├── include/           # 公共头文件
├── src/              # 源代码实现
├── docs/             # 文档
├── examples/         # 示例程序
├── test/             # 测试
└── build/            # 构建输出目录
```

## 配置选项

```bash
# 启用 WebSocket 支持
cmake -DBUILD_WITH_WEBSOCKET=ON ..

# 启用 mimalloc 分配器
cmake -DBUILD_WITH_MIMALLOC=ON ..

# Debug 模式
cmake -DENABLE_DEBUG=ON ..

# 启用代码覆盖率
cmake -DENABLE_COVERAGE=ON ..

# 启用示例程序编译
cmake -DBUILD_EXAMPLES=ON ..
```

## 下一步

- [API 文档](/api/introduction) - 学习完整的 API