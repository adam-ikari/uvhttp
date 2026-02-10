# CMake 库链接规范指南

本文档说明 UVHTTP 项目中使用的 CMake 库链接规范。

## 概述

UVHTTP 项目使用现代 CMake (3.10+) 的目标导向链接方式，遵循 CMake 最佳实践。

## 核心概念

### target_link_libraries 的可见性关键字

现代 CMake 使用三个关键字来控制库的可见性：

- **PRIVATE**: 仅在当前目标内部使用，不传递给依赖当前目标的其他目标
- **PUBLIC**: 当前目标内部使用，同时传递给依赖当前目标的其他目标
- **INTERFACE**: 不在当前目标内部使用，只传递给依赖当前目标的其他目标

### target_include_directories 的可见性关键字

同样使用 PRIVATE/PUBLIC/INTERFACE 关键字控制包含目录的可见性。

## UVHTTP 库的配置

### 主库 (uvhttp)

```cmake
add_library(uvhttp STATIC ${SOURCES})

# 设置属性
set_target_properties(uvhttp PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/dist/lib
    POSITION_INDEPENDENT_CODE ON
)

# 链接依赖库（PRIVATE - 不传递给使用者）
target_link_libraries(uvhttp
    PRIVATE
        ${LIBUV_LIB}
        ${MBEDTLS_LIBS}
        ${XXHASH_LIB}
        ${LLHTTP_LIB}
        ${CJSON_LIB}
)

# 平台特定库（PRIVATE）
if(IS_LINUX)
    target_link_libraries(uvhttp PRIVATE pthread m dl)
endif()

# 设置包含目录
target_include_directories(uvhttp
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${DEPS_INCLUDE_DIRS}
)
```

### 示例程序

```cmake
add_executable(hello_world 01_hello_world.c)

# 链接库（PRIVATE - 示例程序的私有依赖）
target_link_libraries(hello_world
    PRIVATE
        ${UVHTTP_LIB}
        ${LIBUV_LIB}
        ${MBEDTLS_LIBS}
        ${LLHTTP_LIB}
        pthread
        m
        dl
)

# 设置包含目录
target_include_directories(hello_world
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/../../include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/include
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson
)

# 添加依赖关系
add_dependencies(hello_world uvhttp)
```

## 优势

### 1. 明确的依赖关系

使用 PRIVATE/PUBLIC/INTERFACE 关键字可以明确表达依赖的可见性：

- **PRIVATE**: 实现细节，不暴露给使用者
- **PUBLIC**: API 的一部分，需要暴露给使用者
- **INTERFACE**: 纯接口，不在内部使用

### 2. 更好的可维护性

- 依赖关系清晰，易于理解和修改
- 避免意外的依赖传播
- 减少链接错误

### 3. 更好的构建性能

- CMake 可以更好地优化依赖图
- 减少不必要的重新编译
- 支持并行构建

### 4. 更好的可移植性

- 使用生成器表达式支持跨平台
- 自动处理不同平台的差异
- 支持现代 IDE 的智能感知

## 最佳实践

### 1. 使用目标导向的命令

```cmake
# ✅ 推荐
target_link_libraries(my_target PRIVATE some_lib)
target_include_directories(my_target PRIVATE include_dir)

# ❌ 不推荐
link_libraries(some_lib)
include_directories(include_dir)
```

### 2. 明确指定可见性

```cmake
# ✅ 推荐 - 明确指定
target_link_libraries(my_target PRIVATE some_lib)

# ❌ 不推荐 - 不明确
target_link_libraries(my_target some_lib)
```

### 3. 使用生成器表达式

```cmake
# ✅ 推荐 - 支持构建和安装
target_include_directories(mylib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# ❌ 不推荐 - 仅支持构建
target_include_directories(mylib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
```

### 4. 避免全局命令

```cmake
# ✅ 推荐 - 目标特定
target_include_directories(my_target PRIVATE include_dir)

# ❌ 不推荐 - 全局影响
include_directories(include_dir)
```

## 依赖库列表

### 核心依赖

| 库名称 | 类型 | 可见性 | 说明 |
|--------|------|--------|------|
| libuv | 静态库 | PRIVATE | 事件循环库 |
| mbedtls | 静态库 | PRIVATE | TLS/SSL 库 |
| llhttp | 静态库 | PRIVATE | HTTP 解析器 |
| xxhash | 静态库 | PRIVATE | 哈希库 |
| cjson | 静态库 | PRIVATE | JSON 库 |
| mimalloc | 静态库 | PRIVATE | 内存分配器（可选） |

### 系统库

| 库名称 | 平台 | 可见性 | 说明 |
|--------|------|--------|------|
| pthread | Linux/macOS | PRIVATE | 线程库 |
| m | Linux | PRIVATE | 数学库 |
| dl | Linux | PRIVATE | 动态链接库 |

## 常见问题

### Q: 为什么大多数依赖使用 PRIVATE？

A: 因为这些依赖是 uvhttp 的实现细节，使用者不需要直接访问它们。使用 PRIVATE 可以避免意外暴露内部依赖。

### Q: 什么时候使用 PUBLIC？

A: 当使用者的代码需要访问依赖的头文件时使用 PUBLIC。例如，uvhttp 的头文件需要被使用者访问。

### Q: 什么时候使用 INTERFACE？

A: 当库本身不使用某个依赖，但需要将其传递给使用者时使用 INTERFACE。

### Q: 如何处理可选依赖？

A: 使用条件编译和 target_link_libraries：

```cmake
if(BUILD_WITH_MIMALLOC)
    target_link_libraries(uvhttp PRIVATE ${MIMALLOC_LIB})
    target_compile_definitions(uvhttp PRIVATE UVHTTP_ENABLE_MIMALLOC)
endif()
```

## 迁移指南

### 从旧式链接迁移到现代链接

**旧式方式：**
```cmake
add_library(mylib STATIC sources.c)
link_libraries(${SOME_LIB})
include_directories(include_dir)
```

**现代方式：**
```cmake
add_library(mylib STATIC sources.c)
target_link_libraries(mylib PRIVATE ${SOME_LIB})
target_include_directories(mylib PRIVATE include_dir)
```

### 从全局包含迁移到目标包含

**旧式方式：**
```cmake
include_directories(include_dir)
add_executable(myapp main.c)
```

**现代方式：**
```cmake
add_executable(myapp main.c)
target_include_directories(myapp PRIVATE include_dir)
```

## 参考资料

- [CMake target_link_libraries 文档](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)
- [CMake target_include_directories 文档](https://cmake.org/cmake/help/latest/command/target_include_directories.html)
- [CMake 最佳实践](https://cliutils.gitlab.io/modern-cmake/chapters/basics/targets.html)
- [CMake 生成器表达式](https://cmake.org/cmake/help/latest/manual/cmake-generator-expressions.7.html)

## 总结

使用现代 CMake 的目标导向链接方式可以：

1. ✅ 明确表达依赖关系
2. ✅ 提高代码可维护性
3. ✅ 改善构建性能
4. ✅ 增强可移植性
5. ✅ 支持 IDE 智能感知

UVHTTP 项目遵循这些最佳实践，确保构建系统的现代化和可维护性。