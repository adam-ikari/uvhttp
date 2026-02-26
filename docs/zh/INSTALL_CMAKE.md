# CMake 安装和使用指南

本指南介绍如何使用 CMake 安装和使用 uvhttp。

## 安装

### 从源码安装

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### 安装选项

```bash
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_HTTPS=ON \
    -DBUILD_WITH_MIMALLOC=ON
```

## 在项目中使用 uvhttp

### 方法 1: 使用 find_package()

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

### 方法 2: 使用 pkg-config

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

### 方法 3: 直接使用 uvhttpConfig.cmake

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

# 设置 uvhttp 安装路径
set(UVHTTP_DIR "/usr/local" CACHE PATH "uvhttp 安装路径")

# 查找 uvhttp
find_package(uvhttp CONFIG REQUIRED PATHS ${UVHTTP_DIR})

# 创建可执行文件
add_executable(myapp main.c)

# 链接 uvhttp
target_link_libraries(myapp uvhttp)
```

## 使用特定功能构建

安装 uvhttp 时启用特定功能后，在使用 find_package() 时需要确保启用相同的功能：

```cmake
# 安装时启用功能
cmake .. -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_HTTPS=ON
sudo make install

# 在项目中
set(BUILD_WITH_WEBSOCKET ON)
set(BUILD_WITH_HTTPS ON)
find_package(uvhttp REQUIRED)
```

## CMake 变量

找到 uvhttp 后，可以使用以下变量：

| 变量 | 描述 |
|------|------|
| `UVHTTP_VERSION` | 包版本（例如 "2.2.0"） |
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

find_package(uvhttp REQUIRED)

add_executable(server server.c)

target_link_libraries(server uvhttp)

# 可选：启用特定功能
target_compile_definitions(server PRIVATE -DUVHTTP_FEATURE_WEBSOCKET=1)
```

```c
// server.c
#include "uvhttp.h"

#include <stdio.h>

int main() {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    uvhttp_router_t* router = uvhttp_router_new();
    
    // 添加路由
    uvhttp_router_add_route(router, "/", [](uvhttp_request_t* request, uvhttp_response_t* response) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Hello, World!");
        uvhttp_response_send(response);
        return 0;
    });
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("服务器运行在 http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## 卸载

```bash
# 删除已安装的文件
sudo xargs rm -v < build/install_manifest.txt

# 或手动删除
sudo rm -rf /usr/local/lib/cmake/uvhttp
sudo rm -rf /usr/local/include/uvhttp*
sudo rm /usr/local/lib/libuvhttp*
sudo rm /usr/local/lib/pkgconfig/uvhttp.pc
```

## 注意事项

- uvhttp 遵循 CMake 包命名约定
- 首选使用 `find_package(uvhttp CONFIG)` 命令
- 功能兼容性：确保构建和使用 uvhttp 时启用相同的功能
- 库依赖（libuv、mbedtls、xxhash）会自动链接