# 路由示例

# 确定libuv库路径
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
else()
    message(FATAL_ERROR "libuv library not found")
endif()

add_executable(simple_routing 01_simple_routing.c)
target_link_libraries(simple_routing
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    pthread m dl)

add_executable(method_routing 02_method_routing.c)
target_link_libraries(method_routing
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    pthread m dl)
