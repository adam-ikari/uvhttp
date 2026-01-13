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

# WebSocket 连接管理测试示例（如果启用）
if(BUILD_WITH_WEBSOCKET)
    # 确定libuv库路径
    if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/build/libuv.a)
        set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/build/libuv.a)
    elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/.libs/libuv.a)
        set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/.libs/libuv.a)
    else()
        message(FATAL_ERROR "libuv library not found")
    endif()

    add_executable(test_ws_connection_management test_ws_connection_management.c)
    target_link_libraries(test_ws_connection_management ${UVHTTP_LIBRARY} ${LIBUV_LIB} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedcrypto.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedtls.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedx509.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/libllhttp.a m pthread dl)

    # WebSocket 认证示例
    add_executable(websocket_auth_server websocket_auth_server.c)
    target_link_libraries(websocket_auth_server ${UVHTTP_LIBRARY} ${LIBUV_LIB} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedcrypto.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedtls.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedx509.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/libllhttp.a m pthread dl)
endif()

# 性能测试示例
add_executable(performance_static_server performance_static_server.c)
target_link_libraries(performance_static_server ${UVHTTP_LIBRARY} ${LIBUV_LIB} ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedcrypto.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedtls.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/library/libmbedx509.a ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/libllhttp.a m pthread dl)
