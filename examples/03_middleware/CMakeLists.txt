# 中间件示例 - 编译期配置

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)
set(CJSON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson/libcjson.a)

# 编译期中间件配置示例
add_executable(middleware_compile_time_demo middleware_compile_time_demo.c)
target_link_libraries(middleware_compile_time_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
target_include_directories(middleware_compile_time_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson)
add_dependencies(middleware_compile_time_demo uvhttp)
# 中间件链复用示例
add_executable(middleware_chain_demo middleware_chain_demo.c)
target_link_libraries(middleware_chain_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
target_include_directories(middleware_chain_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson)
add_dependencies(middleware_chain_demo uvhttp)

# 限流中间件示例
add_executable(rate_limit_demo rate_limit_demo.c)
target_link_libraries(rate_limit_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
target_include_directories(rate_limit_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson)
add_dependencies(rate_limit_demo uvhttp)

# 中间件测试
add_executable(test_middleware test_middleware.c)
target_link_libraries(test_middleware PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(test_middleware PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(test_middleware uvhttp)
