cmake_minimum_required(VERSION 3.10)
project(uvhttp_examples C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ../include NO_DEFAULT_PATH)

if(NOT UVHTTP_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 uvhttp.h，请先编译 UVHTTP")
endif()

# 设置 UVHTTP 库路径（将在编译时生成）
set(UVHTTP_LIBRARY ${CMAKE_CURRENT_SOURCE_DIR}/../build/dist/lib/libuvhttp.a)

message(STATUS "UVHTTP 头文件: ${UVHTTP_INCLUDE_DIR}")
message(STATUS "UVHTTP 库文件: ${UVHTTP_LIBRARY}")

include_directories(${UVHTTP_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/include)

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

# 性能测试示例
add_executable(performance_test performance_test.c)
target_link_libraries(performance_test ${UVHTTP_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/.libs/libuv.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/libllhttp.a m pthread dl)

add_executable(performance_test_static performance_test_static.c)
target_link_libraries(performance_test_static ${UVHTTP_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/.libs/libuv.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/libllhttp.a m pthread dl)

# TLS 性能测试（如果启用）
if(BUILD_WITH_TLS)
    add_executable(performance_test_tls performance_test_tls.c)
    target_link_libraries(performance_test_tls ${UVHTTP_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/.libs/libuv.a m pthread dl)
endif()

# WebSocket 性能测试（如果启用）
if(BUILD_WITH_WEBSOCKET)
    add_executable(performance_test_websocket performance_test_websocket.c)
    target_link_libraries(performance_test_websocket ${UVHTTP_LIBRARY} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/.libs/libuv.a m pthread dl)
endif()
