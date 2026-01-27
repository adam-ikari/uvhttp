# 性能测试示例

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

# 性能测试
add_executable(performance_test performance_test.c)
target_link_libraries(performance_test PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(performance_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(performance_test uvhttp)

# 性能静态文件服务器
add_executable(performance_static_server performance_static_server.c)
target_link_libraries(performance_static_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(performance_static_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(performance_static_server uvhttp)

# 性能测试静态
add_executable(performance_test_static performance_test_static.c)
target_link_libraries(performance_test_static PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(performance_test_static PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(performance_test_static uvhttp)

# 性能测试 TLS
add_executable(performance_test_tls performance_test_tls.c)
target_link_libraries(performance_test_tls PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(performance_test_tls PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(performance_test_tls uvhttp)

# mimalloc 演示
add_executable(mimalloc_demo mimalloc_demo.c)
target_link_libraries(mimalloc_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(mimalloc_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(mimalloc_demo uvhttp)