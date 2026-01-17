cmake_minimum_required(VERSION 3.10)
project(uvhttp_performance_only C)

set(CMAKE_C_STANDARD 11)

# 查找 UVHTTP
find_path(UVHTTP_INCLUDE_DIR uvhttp.h PATHS ${CMAKE_CURRENT_SOURCE_DIR}/../../include NO_DEFAULT_PATH)

if(NOT UVHTTP_INCLUDE_DIR)
    message(FATAL_ERROR "找不到 uvhttp.h，请先编译 UVHTTP")
endif()

# 设置 UVHTTP 库路径
set(UVHTTP_LIBRARY ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

message(STATUS "UVHTTP 头文件: ${UVHTTP_INCLUDE_DIR}")
message(STATUS "UVHTTP 库文件: ${UVHTTP_LIBRARY}")

include_directories(${UVHTTP_INCLUDE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson)

# 确定libuv库路径
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
else()
    message(FATAL_ERROR "libuv library not found")
endif()

# 性能测试示例
add_executable(performance_test ../performance_test.c)
target_link_libraries(performance_test
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_test uvhttp)

add_executable(performance_test_static ../performance_test_static.c)
target_link_libraries(performance_test_static
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_test_static uvhttp)

add_executable(performance_static_server ../performance_static_server.c)
target_link_libraries(performance_static_server
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_static_server uvhttp)

add_executable(test_static_middleware ../test_static_middleware.c)
target_link_libraries(test_static_middleware ${UVHTTP_LIBRARY} ${LIBUV_LIB} ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a m pthread dl)
add_dependencies(test_static_middleware uvhttp)

# 健康检查示例
add_executable(health_check_demo ../health_check_demo.c)
target_compile_definitions(health_check_demo PRIVATE UVHTTP_TEST_MODE=0 UVHTTP_ALLOCATOR_TYPE=1)
target_link_libraries(health_check_demo
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mimalloc/build/libmimalloc.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson/libcjson.a
    m pthread dl)
add_dependencies(health_check_demo uvhttp)

# WebSocket 示例（如果启用）
if(BUILD_WITH_WEBSOCKET)
    add_executable(websocket_echo_server ../websocket_echo_server.c)
    target_link_libraries(websocket_echo_server
        ${UVHTTP_LIBRARY}
        ${LIBUV_LIB}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
        m pthread dl)
    add_dependencies(websocket_echo_server uvhttp)

    add_executable(websocket_test_server ../websocket_test_server.c)
    target_link_libraries(websocket_test_server
        ${UVHTTP_LIBRARY}
        ${LIBUV_LIB}
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
        ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
        m pthread dl)
    add_dependencies(websocket_test_server uvhttp)
endif()
