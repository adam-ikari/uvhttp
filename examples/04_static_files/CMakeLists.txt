# 静态文件示例

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

# 静态文件服务器
add_executable(static_file_server static_file_server.c)
target_link_libraries(static_file_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(static_file_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
add_dependencies(static_file_server uvhttp)
uvhttp_add_example_dependencies(static_file_server)

# 简单静态文件测试
add_executable(simple_static_test simple_static_test.c)
target_link_libraries(simple_static_test PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(simple_static_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
add_dependencies(simple_static_test uvhttp)
uvhttp_add_example_dependencies(simple_static_test)

# 高级静态文件服务器
add_executable(advanced_static_server advanced_static_server.c)
target_link_libraries(advanced_static_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp cjson pthread m dl)
target_include_directories(advanced_static_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
add_dependencies(advanced_static_server uvhttp)
uvhttp_add_example_dependencies(advanced_static_server)

# 缓存测试服务器
add_executable(cache_test_server cache_test_server.c)
target_link_libraries(cache_test_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(cache_test_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
add_dependencies(cache_test_server uvhttp)
uvhttp_add_example_dependencies(cache_test_server)

# 自定义TTL示例（暂时禁用，功能还未实现）
# add_executable(custom_ttl_demo custom_ttl_demo.c)
# target_link_libraries(custom_ttl_demo uvhttp)
