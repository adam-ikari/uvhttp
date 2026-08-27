# 嵌入式接入指南

本指南面向**新嵌入者**——第一次把 uvhttp 集成到自己 CMake 项目中的开发者——
完整走一遍接入流程：前置条件、每种集成方式、最小可运行服务器、特性裁剪与故障排查。

> 可独立运行、自包含的示例项目位于
> [`examples/embedding/`](../../../examples/embedding/)。本指南的所有内容
> 都在那里得到体现——可以直接作为模板使用。

---

## 1. 前置条件

- **Linux**（当前支持平台）。
- **CMake ≥ 3.10** 与 **C11** 编译器（GCC 或 Clang）。
- uvhttp 的依赖位于 `deps/` 下，均为 **git submodule**。克隆时必须递归：

  ```bash
  git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
  ```

  如果已克隆但未带子模块：

  ```bash
  git submodule update --init --recursive
  ```

- uvhttp 是**静态库**，自带所需一切：libuv、llhttp、xxhash、mbedtls、uthash
  都随构建一起编译，**无需**单独安装这些依赖。

---

## 2. 集成方式

有三种方式把 uvhttp 引入你的项目，最终都会产生一个 `uvhttp` CMake target（静态库）。

| 方式 | 适用场景 | 成本 |
|------|----------|------|
| `add_subdirectory` | 把 uvhttp 放进你的目录树（submodule 或拷贝） | 低——推荐 |
| `FetchContent` | 配置时自动拉取 uvhttp | 低 |
| 系统安装 + `find_package` | 多个项目共享一份 uvhttp 构建 | 中（先安装一次） |

### 方式 A — `add_subdirectory`（推荐）

把 uvhttp 放进你的目录树（例如 `third_party/uvhttp`），然后：

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

set(CMAKE_C_STANDARD 11)

# 指向你的 uvhttp 检出目录
add_subdirectory(third_party/uvhttp)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE uvhttp)
```

### 方式 B — `FetchContent`

```cmake
cmake_minimum_required(VERSION 3.14)
project(myapp C)

include(FetchContent)
FetchContent_Declare(uvhttp
  GIT_REPOSITORY https://github.com/adam-ikari/uvhttp.git
  GIT_TAG        v2.7.1
)
FetchContent_MakeAvailable(uvhttp)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE uvhttp)
```

> FetchContent 默认**不带 submodule** 克隆仓库。uvhttp 的 `deps/` 子模块在其
> 自身 CMakeLists 中补齐，因此请固定到已发布 tag；若遇到子模块缺失，可在
> 检出目录执行 `git submodule update --init --recursive`。
> 能控制检出目录时，`add_subdirectory`（方式 A）更稳妥。

### 方式 C — 系统安装 + `find_package`

先安装一次，再使用导出的 CMake 包：

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
cmake --install . --prefix /usr/local
```

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

find_package(uvhttp REQUIRED)

add_executable(myapp main.c)
target_link_libraries(myapp PRIVATE uvhttp::uvhttp)
```

---

## 3. 最小服务器

创建 `main.c`（与 `examples/embedding/main.c` 完全一致）：

```c
#include "uvhttp.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 应用上下文：通过 loop 关联，避免全局变量（uvhttp 设计原则） */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
} app_context_t;

/* 信号句柄，用于优雅退出 */
static uv_signal_t g_sigint;
static uv_signal_t g_sigterm;
static app_context_t g_app; /* 单实例示例用静态上下文 */

static void on_signal(uv_signal_t* handle, int signum) {
    (void)signum;
    /* 停止事件循环；uv_signal 会唤醒正在阻塞的 loop */
    uv_stop(handle->loop);
    printf("\nReceived signal, shutting down...\n");
    fflush(stdout);
}

static int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    const char* body = "Hello from embedded uvhttp!\n";
    uvhttp_response_set_body(res, body, strlen(body));
    return (int)uvhttp_response_send(res);
}

int main(int argc, char** argv) {
    /* 端口可通过命令行参数指定，默认 8080 */
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    uv_loop_t* loop = uv_default_loop();
    if (loop == NULL) {
        fprintf(stderr, "error: cannot create libuv event loop\n");
        return 1;
    }

    if (uvhttp_server_new(loop, &g_app.server) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot create uvhttp server\n");
        return 1;
    }
    if (uvhttp_router_new(&g_app.router) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot create router\n");
        uvhttp_server_free(g_app.server);
        return 1;
    }
    if (uvhttp_router_add_route(g_app.router, "/", hello_handler) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot add route /\n");
        uvhttp_router_free(g_app.router);
        uvhttp_server_free(g_app.server);
        return 1;
    }
    uvhttp_server_set_router(g_app.server, g_app.router);

    uv_signal_init(loop, &g_sigint);
    uv_signal_start(&g_sigint, on_signal, SIGINT);
    uv_signal_init(loop, &g_sigterm);
    uv_signal_start(&g_sigterm, on_signal, SIGTERM);

    if (uvhttp_server_listen(g_app.server, "0.0.0.0", port) != UVHTTP_OK) {
        fprintf(stderr, "error: cannot listen on 0.0.0.0:%d\n", port);
        uvhttp_router_free(g_app.router);
        uvhttp_server_free(g_app.server);
        return 1;
    }

    printf("Embedded uvhttp listening on http://0.0.0.0:%d\n", port);
    printf("Test: curl http://localhost:%d/\n", port);

    uv_run(loop, UV_RUN_DEFAULT);

    /* 清理：
     * 1. 先关闭自建的 uv_signal 句柄——uvhttp_server_free 内部会跑
     *    UV_RUN_ONCE 清理循环，loop 中残留活跃句柄会使其阻塞。
     * 2. uvhttp_server_free 会一并释放其持有的 router 与 tcp_handle，
     *    因此不要重复调用 uvhttp_router_free。
     */
    uv_close((uv_handle_t*)&g_sigint, NULL);
    uv_close((uv_handle_t*)&g_sigterm, NULL);
    uvhttp_server_free(g_app.server);
    printf("Shutdown complete.\n");
    fflush(stdout);
    return 0;
}
```

构建并运行：

```bash
cmake -S . -B build
cmake --build build -j
./build/embedding_server   # 或 ./build/myapp
```

在另一个终端验证：

```bash
curl http://localhost:8080/
# Hello from embedded uvhttp!
```

---

## 4. 特性裁剪

所有特性都是编译期选项，在配置时传入：

```bash
cmake -DBUILD_WITH_WEBSOCKET=OFF -DBUILD_WITH_HTTPS=OFF ..
```

| 选项 | 默认 | 作用 |
|------|------|------|
| `BUILD_WITH_WEBSOCKET` | ON | WebSocket 支持（RFC 6455） |
| `BUILD_WITH_HTTPS` | ON | HTTPS/TLS（mbedtls） |
| `BUILD_WITH_STATIC_FILES` | OFF | 静态文件服务（sendfile） |
| `BUILD_WITH_LRU_CACHE` | ON | 静态文件 / 压缩 LRU 缓存 |
| `BUILD_WITH_ROUTER_CACHE` | OFF | 路由查找缓存 |
| `BUILD_WITH_COMPRESSION` | ON | gzip 响应压缩 |
| `BUILD_WITH_MIMALLOC` | OFF* | mimalloc 分配器（`UVHTTP_ALLOCATOR_TYPE=1`） |
| `UVHTTP_ALLOCATOR_TYPE` | 0 | 分配器：`0` 系统、`1` mimalloc、`2` 自定义 |
| `BUILD_EXAMPLES` | OFF | uvhttp 自带示例 |
| `BUILD_BENCHMARKS` | ON | uvhttp 自带基准（嵌入时建议关闭） |

\* `BUILD_WITH_MIMALLOC` 默认值取决于 `UVHTTP_ALLOCATOR_TYPE`，精确规则见
`CMakeLists.txt`。

嵌入时建议同时传入：

```bash
cmake -DBUILD_BENCHMARKS=OFF -DBUILD_EXAMPLES=OFF ..
```

完整选项集见 [构建配置矩阵](./BUILD_CONFIGURATION_MATRIX.md) 与
[高级构建选项](./ADVANCED_BUILD_OPTIONS.md)。

---

## 5. 验证你的集成

1. **编译**——你的项目静态链接 uvhttp 并构建成功。
2. **冒烟测试**——启动服务器，`curl` 一个路由，确认响应正确。
3. **优雅退出**——发送 SIGINT/SIGTERM，进程无泄漏退出。
4. **内存安全**——开启 sanitizer 重新构建并重跑：

   ```bash
   cmake -DENABLE_ASAN=ON -DENABLE_UBSAN=ON ..
   ```

5. 运行一次 uvhttp 自带的测试套件，确认库本身健康：

   ```bash
   cd <uvhttp>/build && ctest --output-on-failure
   ```

逐版本完整检查清单见[嵌入验证清单](../embedding-checklist.md)
（第 9 节专门写给首次接入的嵌入者）。

---

## 6. 故障排查

| 现象 | 原因 / 解决 |
|------|-------------|
| `fatal error: llhttp.h: No such file or directory`（或 `uv.h`、`uthash.h`、mbedtls 头文件） | 版本早于 **v2.7.1**。公开头文件依赖在 v2.7.1 中改为 `PUBLIC` 以传播给嵌入者。升级到 v2.7.1+。 |
| 链接时 `undefined reference to <symbol>` | `uvhttp` target 已传递链接 libuv/llhttp/xxhash/mbedtls——**不要**再手动添加。老工具链缺 `-lpthread -lm -ldl`：`target_link_libraries(myapp PRIVATE uvhttp pthread m dl)`。 |
| `deps/` 目录为空 | 子模块未初始化。在 uvhttp 检出目录执行 `git submodule update --init --recursive`。 |
| 构建慢 / 出现大量 uvhttp 测试目标 | uvhttp 测试目标无条件构建。用 `-DBUILD_BENCHMARKS=OFF` 关闭可关项；测试不影响你的二进制。 |
| 意外启用了 mbedtls 或 mimalloc | 用 `-DBUILD_WITH_HTTPS=OFF` / `-DBUILD_WITH_WEBSOCKET=OFF` / `-DBUILD_WITH_MIMALLOC=OFF` 裁剪。 |

---

## 7. 下一步

- 运行实际示例：[`examples/embedding/`](../../../examples/embedding/)
- 嵌入式配置与画像：[嵌入式配置](../embedded-profile.md)
- CMake 细节：[CMake 集成](./INSTALL_CMAKE.md)、[CMake 配置](./CMAKE_CONFIGURATION.md)
- 逐版本验证：[嵌入验证清单](../embedding-checklist.md)
