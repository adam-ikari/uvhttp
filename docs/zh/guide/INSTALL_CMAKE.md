# 在 CMake 项目中使用 uvhttp

本指南介绍如何将 uvhttp 作为静态库依赖集成到你的 CMake 项目中。

## 快速开始：add_subdirectory

最简单的方式是将 uvhttp 作为子目录添加到项目中：

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
```

然后在项目的 `CMakeLists.txt` 中：

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

# 添加 uvhttp 作为子目录
add_subdirectory(path/to/uvhttp)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp（静态库）
target_link_libraries(myapp uvhttp)
target_include_directories(myapp PRIVATE path/to/uvhttp/include)
```

编译：

```bash
make build
```

## 方法 1: 使用 find_package()

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

# 查找 uvhttp 包
find_package(uvhttp REQUIRED)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp
target_link_libraries(myapp uvhttp)
```

## 方法 2: 使用 pkg-config

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UVHTTP REQUIRED uvhttp)

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp
target_link_libraries(myapp ${UVHTTP_LIBRARIES})
target_include_directories(myapp PUBLIC ${UVHTTP_INCLUDE_DIRS})
```

## 方法 3: 使用 FetchContent

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

启用特定功能后构建 uvhttp：

```cmake
# 构建时启用功能
set(BUILD_WITH_WEBSOCKET ON CACHE BOOL "" FORCE)
set(BUILD_WITH_HTTPS ON CACHE BOOL "" FORCE)
add_subdirectory(path/to/uvhttp)
```

## CMake 变量

找到 uvhttp 后，可以使用以下变量：

| 变量 | 描述 |
|------|------|
| `UVHTTP_VERSION` | 包版本（例如 "2.6.0"） |
| `UVHTTP_INCLUDE_DIRS` | 包含目录 |
| `UVHTTP_LIBRARIES` | 库名称（uvhttp） |
| `UVHTTP_LIBRARY_DIRS` | 库目录路径 |
| `UVHTTP_FOUND` | 如果找到包则为 True |

## 功能标志

以下功能标志指示哪些功能可用：

```cmake
if(UVHTTP_FEATURE_WEBSOCKET)
    # WebSocket 支持可用
    add_definitions(-DUVHTTP_FEATURE_WEBSOCKET=1)
endif()

if(UVHTTP_FEATURE_HTTPS)
    # TLS/HTTPS 支持可用
    add_definitions(-DUVHTTP_FEATURE_HTTPS=1)
endif()
```

## 示例项目

以下是一个使用 uvhttp 的完整示例项目：

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(http_server C)

add_subdirectory(uvhttp)

add_executable(server server.c)

target_link_libraries(server uvhttp)

# 可选：启用特定功能
target_compile_definitions(server PRIVATE -DUVHTTP_FEATURE_WEBSOCKET=1)
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

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running on http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

## 注意事项

- uvhttp 遵循 CMake 包命名约定
- 推荐使用 `add_subdirectory` 或 `FetchContent` 方式集成，而非系统安装
- 库依赖（libuv、mbedtls、xxhash）会自动链接
- 功能兼容性：确保构建和使用 uvhttp 时启用相同的功能