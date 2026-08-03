# Using uvhttp in Your CMake Project

This guide explains how to integrate uvhttp as a static library dependency in your CMake project.

## Quick Start: add_subdirectory

The simplest way is to add uvhttp as a subdirectory:

```bash
git clone --recurse-submodules https://github.com/adam-ikari/uvhttp.git
```

Then in your project's `CMakeLists.txt`:

```cmake
cmake_minimum_required(VERSION 3.10)
project(myapp C)

# Add uvhttp as a subdirectory
add_subdirectory(path/to/uvhttp)

# Create executable
add_executable(myapp main.c)

# Link against uvhttp (static)
target_link_libraries(myapp uvhttp)
target_include_directories(myapp PRIVATE path/to/uvhttp/include)
```

Build:

```bash
make build
```

## Method 1: Using find_package()

If uvhttp is in a known location:

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

## Method 2: Using pkg-config

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

## Method 3: Using FetchContent

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

# Create executable
add_executable(myapp main.c)

# Link against uvhttp
target_link_libraries(myapp uvhttp)
```

## Building with Specific Features

When building uvhttp with specific features, ensure the same features are enabled:

```cmake
# Build with features
set(BUILD_WITH_WEBSOCKET ON CACHE BOOL "" FORCE)
set(BUILD_WITH_HTTPS ON CACHE BOOL "" FORCE)
add_subdirectory(path/to/uvhttp)
```

## CMake Variables

After finding uvhttp, the following variables are available:

| Variable | Description |
|----------|-------------|
| `UVHTTP_VERSION` | Package version (e.g., "2.6.0") |
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

add_subdirectory(uvhttp)

add_executable(server server.c)

target_link_libraries(server uvhttp)

# Optional: Enable specific features
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

    // Add routes
    uvhttp_router_add_route(router, "/", home_handler);

    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8080);

    printf("Server running on http://0.0.0.0:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);

    return 0;
}
```

## Notes

- uvhttp follows CMake package naming conventions
- The `add_subdirectory` or `FetchContent` approach is recommended over system installation
- Library dependencies (libuv, mbedtls, xxhash) are automatically linked
- Feature compatibility: Ensure the same features are enabled when building and using uvhttp