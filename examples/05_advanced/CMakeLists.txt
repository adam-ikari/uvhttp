# 高级特性示例

# 确定libuv库路径
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/build/libuv.a)
elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
    set(LIBUV_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/libuv/.libs/libuv.a)
else()
    message(FATAL_ERROR "libuv library not found")
endif()

add_executable(libuv_data_pointer 01_libuv_data_pointer.c)
target_link_libraries(libuv_data_pointer
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    pthread m dl)

add_executable(middleware_demo ../middleware_demo.c)
target_link_libraries(middleware_demo
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    pthread m dl)

add_executable(cors_rate_limit_demo ../cors_rate_limit_demo.c)
target_link_libraries(cors_rate_limit_demo
    ${UVHTTP_LIBRARY}
    ${LIBUV_LIB}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cllhttp/libllhttp.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedtls.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedx509.a
    ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/mbedtls/library/libmbedcrypto.a
    pthread m dl)
