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

# 高级特性示例
add_subdirectory(05_advanced)

# 生产环境示例（待添加）
# add_subdirectory(06_production)

# WebSocket 连接管理测试示例（如果启用）
if(BUILD_WITH_WEBSOCKET)
    # WebSocket 连接管理测试
    add_executable(test_ws_connection_management test_ws_connection_management.c)
    target_link_libraries(test_ws_connection_management ${UVHTTP_LIB} ${LIBUV_LIB} ${MBEDTLS_LIBS} ${LLHTTP_LIB} ${CJSON_LIB} ${XXHASH_LIB} m pthread dl)
    add_dependencies(test_ws_connection_management uvhttp)

    # WebSocket 认证示例
    add_executable(websocket_auth_server websocket_auth_server.c)
    target_link_libraries(websocket_auth_server ${UVHTTP_LIB} ${LIBUV_LIB} ${MBEDTLS_LIBS} ${LLHTTP_LIB} ${CJSON_LIB} ${XXHASH_LIB} m pthread dl)
    add_dependencies(websocket_auth_server uvhttp)
endif()

# 内存管理示例
# add_subdirectory(08_memory) # Temporarily disabled due to format warnings

# 性能测试示例
add_executable(performance_static_server performance_static_server.c)
target_link_libraries(performance_static_server ${UVHTTP_LIB} ${LIBUV_LIB} ${MBEDTLS_LIBS} ${LLHTTP_LIB} ${CJSON_LIB} ${XXHASH_LIB} m pthread dl)
add_dependencies(performance_static_server uvhttp)

# 性能基准测试专用服务器（无日志输出，最小化响应体）
add_executable(performance_test performance_test.c)
target_link_libraries(performance_test ${UVHTTP_LIB} ${LIBUV_LIB} ${MBEDTLS_LIBS} ${LLHTTP_LIB} ${CJSON_LIB} ${XXHASH_LIB} m pthread dl)
add_dependencies(performance_test uvhttp)

# 完整版 Hello World 示例
add_executable(helloworld_complete helloworld_complete.c)
target_link_libraries(helloworld_complete ${UVHTTP_LIB} ${LIBUV_LIB} ${MBEDTLS_LIBS} ${LLHTTP_LIB} ${CJSON_LIB} ${XXHASH_LIB} m pthread dl)
add_dependencies(helloworld_complete uvhttp)
