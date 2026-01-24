cmake_minimum_required(VERSION 3.10)
project(uvhttp_examples C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../include NO_DEFAULT_PATH)

if(NOT UVHTTP_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 uvhttp.h，请先编译 UVHTTP")
endif()

include_directories(${UVHTTP_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/include)

# 获取主项目中定义的库
# 这些库应该在主 CMakeLists.txt 中定义
set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/build/libuv.a)
set(MBEDTLS_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/build/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/build/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/build/library/libmbedcrypto.a
)
set(LLHTTP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/libllhttp.a)
set(CJSON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cjson/build/libcjson.a)
set(XXHASH_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/xxhash/libxxhash.a)

# UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../build/dist/lib/libuvhttp.a)

message(STATUS "UVHTTP 头文件: ${UVHTTP_INCLUDE_DIR}")
message(STATUS "UVHTTP 库文件: ${UVHTTP_LIB}")

# 基础示例

# 基础示例
add_subdirectory(01_basics)

# 路由示例
add_subdirectory(02_routing)

# 请求处理示例（待添加）
# add_subdirectory(03_requests)

# 响应处理示例（待添加）
# add_subdirectory(04_responses)

# 响应处理示例
add_subdirectory(04_responses)

# RESTful API 示例
add_subdirectory(03_api_examples)

# 高级特性示例
add_subdirectory(05_advanced)

# 静态文件服务示例
add_subdirectory(06_static_files)

# 配置管理示例
add_subdirectory(07_config)

# 内存管理示例
# add_subdirectory(08_memory) # Temporarily disabled due to format warnings

# 性能测试示例
add_subdirectory(performance_only)
