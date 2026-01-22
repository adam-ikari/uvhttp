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
add_executable(performance_test performance_test.c)
target_link_libraries(performance_test
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_test uvhttp)

add_executable(performance_test_static performance_test_static.c)
target_link_libraries(performance_test_static
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_test_static uvhttp)

add_executable(performance_static_server performance_static_server.c)
target_link_libraries(performance_static_server
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_static_server uvhttp)

add_executable(performance_static_server_broken performance_static_server_broken.c)
target_link_libraries(performance_static_server_broken
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_static_server_broken uvhttp)

add_executable(performance_test_tls performance_test_tls.c)
target_link_libraries(performance_test_tls
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_test_tls uvhttp)

add_executable(performance_test_websocket performance_test_websocket.c)
target_link_libraries(performance_test_websocket
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    m pthread dl)
add_dependencies(performance_test_websocket uvhttp)