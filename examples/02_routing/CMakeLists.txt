# 路由示例

# 从父 CMakeLists.txt 获取库路径
set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
set(MBEDTLS_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/build/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/build/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/build/library/libmbedcrypto.a
)
set(LLHTTP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/build/libllhttp.a)
set(XXHASH_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/xxhash/libxxhash.a)

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

add_executable(simple_routing 01_simple_routing.c)
target_link_libraries(simple_routing
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${LLHTTP_LIB}
    ${XXHASH_LIB}
    cjson
    pthread m dl)
target_include_directories(simple_routing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson)
add_dependencies(simple_routing uvhttp)

add_executable(method_routing 02_method_routing.c)
target_link_libraries(method_routing
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${LLHTTP_LIB}
    ${XXHASH_LIB}
    cjson
    pthread m dl)
target_include_directories(method_routing PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson)
add_dependencies(method_routing uvhttp)
