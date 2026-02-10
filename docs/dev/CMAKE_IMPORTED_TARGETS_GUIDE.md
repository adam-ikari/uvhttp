# CMake 声明库（Imported Targets）使用指南

本文档说明 UVHTTP 项目中使用的 CMake 声明库（IMPORTED targets）方式。

## 概述

UVHTTP 项目使用 CMake 的 IMPORTED targets 来管理外部依赖库，这是现代 CMake 的最佳实践。

## 什么是 IMPORTED targets

IMPORTED targets 是 CMake 提供的一种机制，用于表示预构建的库或外部依赖。它们具有以下优势：

1. **封装性**: 将库的路径、包含目录、链接选项等封装在一个目标中
2. **可传递性**: 自动处理依赖的传递
3. **一致性**: 确保所有目标使用相同的库配置
4. **可维护性**: 修改库配置只需在一个地方

## UVHTTP 中的 IMPORTED targets

### 声明的库

在 `cmake/Dependencies.cmake` 中，我们声明了以下 IMPORTED targets：

```cmake
# libuv
add_library(libuv STATIC IMPORTED GLOBAL)
set_target_properties(libuv PROPERTIES
    IMPORTED_LOCATION ${LIBUV_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
)

# mbedtls
add_library(mbedtls STATIC IMPORTED GLOBAL)
set_target_properties(mbedtls PROPERTIES
    IMPORTED_LOCATION ${MBEDTLS_BUILD_DIR}/library/libmbedtls.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include
)

add_library(mbedx509 STATIC IMPORTED GLOBAL)
set_target_properties(mbedx509 PROPERTIES
    IMPORTED_LOCATION ${MBEDTLS_BUILD_DIR}/library/libmbedx509.a
)

add_library(mbedcrypto STATIC IMPORTED GLOBAL)
set_target_properties(mbedcrypto PROPERTIES
    IMPORTED_LOCATION ${MBEDTLS_BUILD_DIR}/library/libmbedcrypto.a
)

# xxhash
add_library(xxhash STATIC IMPORTED GLOBAL)
set_target_properties(xxhash PROPERTIES
    IMPORTED_LOCATION ${XXHASH_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/xxhash
)

# llhttp
add_library(llhttp STATIC IMPORTED GLOBAL)
set_target_properties(llhttp PROPERTIES
    IMPORTED_LOCATION ${LLHTTP_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/cllhttp
)

# cjson
add_library(cjson STATIC IMPORTED GLOBAL)
set_target_properties(cjson PROPERTIES
    IMPORTED_LOCATION ${CJSON_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/cjson
)

# mimalloc (可选)
if(BUILD_WITH_MIMALLOC)
    add_library(mimalloc STATIC IMPORTED GLOBAL)
    set_target_properties(mimalloc PROPERTIES
        IMPORTED_LOCATION ${MIMALLOC_LIB}
        INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/mimalloc/include
    )
endif()
```

## 使用 IMPORTED targets

### 在主库中使用

```cmake
# 在 CMakeLists.txt 中
target_link_libraries(uvhttp
    PRIVATE
        libuv
        mbedtls
        mbedx509
        mbedcrypto
        xxhash
        llhttp
        cjson
)

if(BUILD_WITH_MIMALLOC)
    target_link_libraries(uvhttp PRIVATE mimalloc)
endif()

# 平台特定库
if(IS_LINUX)
    target_link_libraries(uvhttp PRIVATE pthread m dl)
endif()
```

### 在示例程序中使用

```cmake
# 在 examples/01_basics/CMakeLists.txt 中
add_executable(hello_world 01_hello_world.c)

# 链接 UVHTTP 库
target_link_libraries(hello_world PRIVATE uvhttp)

# UVHTTP 会自动传递其依赖的 IMPORTED targets
# 无需手动链接 libuv、mbedtls 等
```

## IMPORTED targets 的优势

### 1. 自动传递包含目录

```cmake
# 使用 IMPORTED targets
target_link_libraries(my_app PRIVATE uvhttp)

# 自动获得所有必要的包含目录：
# - uvhttp 的 include 目录
# - libuv 的 include 目录
# - mbedtls 的 include 目录
# - xxhash 的 include 目录
# - llhttp 的 include 目录
# - cjson 的 include 目录
# - mimalloc 的 include 目录 (如果启用)

# 无需手动设置：
# include_directories(...)
```

### 2. 一致的库配置

```cmake
# 所有目标使用相同的库配置
target_link_libraries(app1 PRIVATE uvhttp)
target_link_libraries(app2 PRIVATE uvhttp)
target_link_libraries(test1 PRIVATE uvhttp)

# 确保所有目标使用相同版本的库和相同的编译选项
```

### 3. 简化的依赖管理

```cmake
# 旧方式（不推荐）
target_link_libraries(my_app
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${XXHASH_LIB}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
    pthread m dl
)

# 新方式（推荐）
target_link_libraries(my_app PRIVATE uvhttp)
```

### 4. 更好的错误检查

```cmake
# 如果库文件不存在，CMake 会在配置时报错
# 而不是在链接时报错
add_library(libuv STATIC IMPORTED GLOBAL)
set_target_properties(libuv PROPERTIES
    IMPORTED_LOCATION ${LIBUV_LIB}  # 如果文件不存在，立即报错
)
```

## 创建 IMPORTED targets 的步骤

### 1. 构建或找到预构建的库

```cmake
# 构建库
execute_process(
    COMMAND ${CMAKE_COMMAND} --build ${LIBUV_BUILD_DIR} --config ${CMAKE_BUILD_TYPE} -j
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv
)
```

### 2. 声明 IMPORTED target

```cmake
# 声明静态库
add_library(libuv STATIC IMPORTED GLOBAL)

# 或者声明共享库
add_library(libuv SHARED IMPORTED GLOBAL)
```

### 3. 设置库属性

```cmake
# 设置库文件位置
set_target_properties(libuv PROPERTIES
    IMPORTED_LOCATION ${LIBUV_LIB}
    # 设置包含目录（INTERFACE 表示传递给使用者）
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
    # 设置编译定义（可选）
    INTERFACE_COMPILE_DEFINITIONS UVHTTP_USE_LIBUV
)
```

### 4. 使用 IMPORTED target

```cmake
# 直接链接
target_link_libraries(my_app PRIVATE libuv)
```

## 常见属性

### IMPORTED_LOCATION

库文件的位置：

```cmake
set_target_properties(mylib PROPERTIES
    IMPORTED_LOCATION /path/to/libmylib.a
)
```

### INTERFACE_INCLUDE_DIRECTORIES

传递给使用者的包含目录：

```cmake
set_target_properties(mylib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES /path/to/include
)
```

### INTERFACE_COMPILE_DEFINITIONS

传递给使用者的编译定义：

```cmake
set_target_properties(mylib PROPERTIES
    INTERFACE_COMPILE_DEFINITIONS MYLIB_VERSION=1.0
)
```

### INTERFACE_COMPILE_OPTIONS

传递给使用者的编译选项：

```cmake
set_target_properties(mylib PROPERTIES
    INTERFACE_COMPILE_OPTIONS -Wall -Wextra
)
```

### IMPORTED_LINK_INTERFACE_LANGUAGES

链接接口语言：

```cmake
set_target_properties(mylib PROPERTIES
    IMPORTED_LINK_INTERFACE_LANGUAGES "C"
)
```

## 最佳实践

### 1. 使用 GLOBAL 关键字

```cmake
# 推荐：使用 GLOBAL 使目标在整个项目中可见
add_library(libuv STATIC IMPORTED GLOBAL)

# 不推荐：只在当前目录可见
add_library(libuv STATIC IMPORTED)
```

### 2. 使用 INTERFACE 属性传递依赖

```cmake
# 推荐：使用 INTERFACE 属性传递包含目录
set_target_properties(libuv PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES ${LIBUV_INCLUDE_DIR}
)

# 不推荐：手动设置 include_directories
include_directories(${LIBUV_INCLUDE_DIR})
```

### 3. 使用 target_link_libraries 而非 link_libraries

```cmake
# 推荐：使用 target_link_libraries
target_link_libraries(my_app PRIVATE libuv)

# 不推荐：使用 link_libraries
link_libraries(${LIBUV_LIB})
```

### 4. 使用可见性关键字

```cmake
# PRIVATE: 仅在当前目标内部使用
target_link_libraries(my_lib PRIVATE libuv)

# PUBLIC: 在当前目标内部使用，并传递给依赖者
target_link_libraries(my_lib PUBLIC libuv)

# INTERFACE: 不在当前目标内部使用，只传递给依赖者
target_link_libraries(my_lib INTERFACE libuv)
```

## 调试 IMPORTED targets

### 查看目标属性

```bash
# 使用 cmake 命令查看目标属性
cmake --build build --target help

# 或者在 CMakeLists.txt 中添加
get_target_property(TARGET_TYPE libuv TYPE)
message(STATUS "libuv type: ${TARGET_TYPE}")

get_target_property(IMPORTED_LOCATION libuv IMPORTED_LOCATION)
message(STATUS "libuv location: ${IMPORTED_LOCATION}")
```

### 查看依赖关系

```bash
# 使用 graphviz 生成依赖图
cmake --graphviz=deps.dot build
dot -Tpng deps.dot -o deps.png
```

## 迁移指南

### 从旧方式迁移到 IMPORTED targets

**旧方式：**
```cmake
set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/build/libuv.a)
set(MBEDTLS_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/build/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/build/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/build/library/libmbedcrypto.a
)

target_link_libraries(uvhttp
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
)
```

**新方式：**
```cmake
# 在 Dependencies.cmake 中声明
add_library(libuv STATIC IMPORTED GLOBAL)
set_target_properties(libuv PROPERTIES
    IMPORTED_LOCATION ${LIBUV_LIB}
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/libuv/include
)

add_library(mbedtls STATIC IMPORTED GLOBAL)
set_target_properties(mbedtls PROPERTIES
    IMPORTED_LOCATION ${MBEDTLS_BUILD_DIR}/library/libmbedtls.a
    INTERFACE_INCLUDE_DIRECTORIES ${CMAKE_CURRENT_SOURCE_DIR}/deps/mbedtls/include
)

# 在 CMakeLists.txt 中使用
target_link_libraries(uvhttp PRIVATE libuv mbedtls)
```

## 参考资料

- [CMake IMPORTED targets 文档](https://cmake.org/cmake/help/latest/manual/cmake-buildsystem.7.html#imported-targets)
- [CMake target_link_libraries 文档](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)
- [CMake Modern CMake 最佳实践](https://cliutils.gitlab.io/modern-cmake/)

## 总结

使用 IMPORTED targets 是现代 CMake 的最佳实践，它提供了：

- ✅ 更好的封装性
- ✅ 自动依赖传递
- ✅ 一致的配置
- ✅ 简化的依赖管理
- ✅ 更好的错误检查

UVHTTP 项目通过使用 IMPORTED targets，实现了更清晰、更可维护的构建系统。