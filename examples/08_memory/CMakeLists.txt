# 内存管理示例

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
set(UVHTTP_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../build/dist/lib/libuvhttp.a)

add_executable(mimalloc_demo mimalloc_demo.c)
target_link_libraries(mimalloc_demo
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
    ${XXHASH_LIB}
    pthread m dl)
add_dependencies(mimalloc_demo uvhttp)

add_executable(app_advanced_memory app_advanced_memory.c)
target_link_libraries(app_advanced_memory
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
    ${XXHASH_LIB}
    pthread m dl)
add_dependencies(app_advanced_memory uvhttp)

add_executable(app_custom_pool app_custom_pool.c)
target_link_libraries(app_custom_pool
    ${UVHTTP_LIB}
    ${LIBUV_LIB}
    ${MBEDTLS_LIBS}
    ${LLHTTP_LIB}
    ${CJSON_LIB}
    ${XXHASH_LIB}
    pthread m dl)
add_dependencies(app_custom_pool uvhttp)
