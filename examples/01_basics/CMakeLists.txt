# 基础示例

# 从父 CMakeLists.txt 获取库路径
set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
set(MBEDTLS_LIBS
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/build/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/build/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/build/library/libmbedcrypto.a
)
set(LLHTTP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a)
set(CJSON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson/build/libcjson.a)
set(XXHASH_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/xxhash/libxxhash.a)

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

add_executable(hello_world 01_hello_world.c)
target_link_libraries(hello_world
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
    ${XXHASH_LIB}
    pthread m dl)
add_dependencies(hello_world uvhttp)
