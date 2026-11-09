# 在 CMake 项目中使用 uvhttp

uvhttp 以静态库形式接入 CMake 工程。下面几种方式里，add_subdirectory 最省事。

## 快速开始：add_subdirectory

把 uvhttp 放进源码树当子目录，链接一个 target 就够：

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
```

项目的 `CMakeLists.txt` 里加这几行：

```cmake
cmake_minimum_required(VERSION 3.13)
project(myapp C)

# 将 uvhttp 作为子目录
add_subdirectory(path/to/uvhttp)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp（静态）；include 目录由 target 自动传播
target_link_libraries(myapp PRIVATE uvhttp)
```

构建：

```bash
make build
```

## 方法 1：使用 find_package()

uvhttp 已经装进系统，或者装在 CMAKE_PREFIX_PATH 能找到的位置：

```cmake
cmake_minimum_required(VERSION 3.13)
project(myapp C)

# 查找 uvhttp 包
find_package(uvhttp REQUIRED)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp
target_link_libraries(myapp PRIVATE uvhttp::uvhttp)
```

## 方法 2：使用 pkg-config

```cmake
cmake_minimum_required(VERSION 3.13)
project(myapp C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UVHTTP REQUIRED uvhttp)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp
target_link_libraries(myapp ${UVHTTP_LIBRARIES})
target_include_directories(myapp PUBLIC ${UVHTTP_INCLUDE_DIRS})
```

## 方法 3：使用 FetchContent

```cmake
cmake_minimum_required(VERSION 3.14)
project(myapp C)

include(FetchContent)
FetchContent_Declare(
  uvhttp
  GIT_REPOSITORY https://github.com/adam-ikari/uvhttp.git
  GIT_TAG main
)
FetchContent_MakeAvailable(uvhttp)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp
target_link_libraries(myapp uvhttp)
```

## 使用特定功能构建

uvhttp 按哪套特性编译，使用方就得按同一套特性链接。在 add_subdirectory 之前把开关写进缓存：

```cmake
# 使用功能构建
set(BUILD_WITH_WEBSOCKET ON CACHE BOOL "" FORCE)
set(BUILD_WITH_HTTPS ON CACHE BOOL "" FORCE)
add_subdirectory(path/to/uvhttp)
```

## CMake 变量

查找 uvhttp 后，以下变量可用：

| 变量 | 说明 |
|----------|-------------|
| `UVHTTP_VERSION` | 包版本（如 "3.0.0"） |
| `UVHTTP_INCLUDE_DIRS` | 包含目录 |
| `UVHTTP_LIBRARIES` | 链接 target（`uvhttp::uvhttp`） |
| `UVHTTP_LIBRARY_DIRS` | 库目录路径 |
| `UVHTTP_FOUND` | 如果找到包则为 True |

## 示例项目

一个能直接跑的最小工程，两个文件：

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.13)
project(http_server C)

add_subdirectory(uvhttp)

add_executable(server server.c)

target_link_libraries(server uvhttp)
```

```c
// server.c
#include "uvhttp.h"

#include <stdio.h>
#include <string.h>

int home_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    const char* body = "Hello, World!";
    uvhttp_response_set_body(response, body, strlen(body));
    return uvhttp_response_send(response);
}

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);

    // 添加路由
    uvhttp_router_add_route(router, "/", home_handler);

    uvhttp_server_take_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running on http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

## 注意

- uvhttp 遵循 CMake 包命名约定
- 优先用 `add_subdirectory` 或 `FetchContent`；系统安装适合多个项目共用同一份库
- 库依赖（libuv、mbedtls、xxhash）会自动链接
- 功能兼容性：构建和使用 uvhttp 时，两边启用同一套开关
