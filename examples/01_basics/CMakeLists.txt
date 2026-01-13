# 基础示例

# 确定libuv库路径
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
else()
    message(FATAL_ERROR "libuv library not found")
endif()

add_executable(hello_world 01_hello_world.c)
target_link_libraries(hello_world
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    pthread m dl)
