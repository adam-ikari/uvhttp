cmake_minimum_required(VERSION 3.10)
project(uvhttp_examples C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../include NO_DEFAULT_PATH)

if(NOT UVHTTP_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 uvhttp.h，请先编译 UVHTTP")
endif()

# 设置包含目录（将在每个target中单独配置）

# 获取主项目中定义的库
# 这些库应该在主 CMakeLists.txt 中定义
set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/build/libuv.a)
set(MBEDTLS_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/build/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/build/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/build/library/libmbedcrypto.a
)
set(LLHTTP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/build/libllhttp.a)
set(XXHASH_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/xxhash/libxxhash.a)

# UVHTTP 库路径
# 使用 CMAKE_BINARY_DIR 来获取当前的构建目录
# 注意：当 examples 作为子项目添加时，CMAKE_BINARY_DIR 指向主项目的构建目录
# 当单独配置 examples 时，需要使用 PROJECT_SOURCE_DIR 的父目录

# 获取主项目的根目录（包含 CMakeLists.txt 的目录）
get_filename_component(UVHTTP_ROOT ${CMAKE_CURRENT_SOURCE_DIR} DIRECTORY)

# 直接使用 CMAKE_BINARY_DIR 作为库路径
# 不检查文件是否存在，因为在配置时库还没有被构建
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

message(STATUS "UVHTTP 头文件: ${UVHTTP_INCLUDE_DIR}")
message(STATUS "UVHTTP 库文件: ${UVHTTP_LIB}")
message(STATUS "UVHTTP_ROOT: ${UVHTTP_ROOT}")
message(STATUS "CMAKE_BINARY_DIR: ${CMAKE_BINARY_DIR}")

# 为所有示例禁用未使用参数警告
add_compile_options(-Wno-error=unused-parameter -Wno-error=unused-function -Wno-error=unused-result -Wno-error=format -Wno-error=discarded-qualifiers)

# 基础示例
add_subdirectory(01_basics)

# 路由示例
add_subdirectory(02_routing)

# 中间件示例 - 编译期配置
add_subdirectory(03_middleware)

# 静态文件示例
add_subdirectory(04_static_files)

# WebSocket 示例
if(BUILD_WITH_WEBSOCKET)
    add_subdirectory(05_websocket)
endif()

# 高级功能示例
add_subdirectory(06_advanced)
