# CMake Installation and Usage Guide

This guide explains how to install and use uvhttp with CMake.

## Installation

### From Source

```bash
git clone https://github.com/adam-ikari/uvhttp.git
cd uvhttp
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo make install
```

### Installation Options

```bash
cmake .. \
    -DCMAKE_INSTALL_PREFIX=/usr/local \
    -DBUILD_WITH_WEBSOCKET=ON \
    -DBUILD_WITH_HTTPS=ON \
    -DBUILD_WITH_MIMALLOC=ON
```

## Using uvhttp in Your Project

### Method 1: Using find_package()

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

# Find uvhttp package
find_package(uvhttp REQUIRED)

# Create executable
add_executable(myapp main.c)

# Link against uvhttp
target_link_libraries(myapp uvhttp)
```

### Method 2: Using pkg-config

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

find_package(PkgConfig REQUIRED)
pkg_check_modules(UVHTTP REQUIRED uvhttp)

# Create executable
add_executable(myapp main.c)

# Link against uvhttp
target_link_libraries(myapp ${UVHTTP_LIBRARIES})
target_include_directories(myapp PUBLIC ${UVHTTP_INCLUDE_DIRS})
```

### Method 3: Using uvhttpConfig.cmake directly

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

# Set uvhttp installation path
set(UVHTTP_DIR "/usr/local" CACHE PATH "Path to uvhttp installation")

# Find uvhttp
find_package(uvhttp CONFIG REQUIRED PATHS ${UVHTTP_DIR})

# Create executable
add_executable(myapp main.c)

# Link against uvhttp
target_link_libraries(myapp uvhttp)
```

## Building with Specific Features

When installing uvhttp with specific features, you need to ensure the same features are enabled when using find_package():

```cmake
# Install with features
cmake .. -DBUILD_WITH_WEBSOCKET=ON -DBUILD_WITH_HTTPS=ON
sudo make install

# In your project
set(BUILD_WITH_WEBSOCKET ON)
set(BUILD_WITH_HTTPS ON)
find_package(uvhttp REQUIRED)
```

## CMake Variables

After finding uvhttp, the following variables are available:

| Variable | Description |
|----------|-------------|
| `UVHTTP_VERSION` | Package version (e.g., "2.2.0") |
| `UVHTTP_INCLUDE_DIRS` | Include directories |
| `UVHTTP_LIBRARIES` | Library names (uvhttp) |
| `UVHTTP_LIBRARY_DIRS` | Library directory paths |
| `UVHTTP_FOUND` | True if package was found |

## Feature Flags

The following feature flags indicate which features are available:

```cmake
if(UVHTTP_FEATURE_WEBSOCKET)
    # WebSocket support is available
    add_definitions(-DUVHTTP_FEATURE_WEBSOCKET=1)
endif()

if(UVHTTP_FEATURE_HTTPS)
    # TLS/HTTPS support is available
    add_definitions(-DUVHTTP_FEATURE_HTTPS=1)
endif()
```

## Example Project

Here's a complete example project using uvhttp:

```cmake
# CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(http_server C)

find_package(uvhttp REQUIRED)

add_executable(server server.c)

target_link_libraries(server uvhttp)

# Optional: Enable specific features
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
    
    // Add routes
    uvhttp_router_add_route(router, "/", [](uvhttp_request_t* request, uvhttp_response_t* response) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Hello, World!");
        uvhttp_response_send(response);
        return 0;
    });
    
    server->router = router;
    uvhttp_server_listen(server, "0.0.0.0", 8080);
    
    printf("Server running on http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
```

## Uninstallation

```bash
# Remove installed files
sudo xargs rm -v < build/install_manifest.txt

# Or manually remove
sudo rm -rf /usr/local/lib/cmake/uvhttp
sudo rm -rf /usr/local/include/uvhttp*
sudo rm /usr/local/lib/libuvhttp*
sudo rm /usr/local/lib/pkgconfig/uvhttp.pc
```

## Notes

- uvhttp follows CMake package naming conventions
- The `find_package(uvhttp CONFIG)` command is preferred
- Feature compatibility: Ensure the same features are enabled when building and using uvhttp
- Library dependencies (libuv, mbedtls, xxhash) are automatically linked