# 中间件示例

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

# 中间件演示
add_executable(middleware_demo middleware_demo.c)
target_link_libraries(middleware_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(middleware_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(middleware_demo uvhttp)

# 简单中间件演示
add_executable(simple_middleware_demo simple_middleware_demo.c)
target_link_libraries(simple_middleware_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(simple_middleware_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(simple_middleware_demo uvhttp)

# 中间件宏演示
add_executable(middleware_macros_demo middleware_macros_demo.c)
target_link_libraries(middleware_macros_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(middleware_macros_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(middleware_macros_demo uvhttp)

# CORS 和限流演示
add_executable(cors_rate_limit_demo cors_rate_limit_demo.c)
target_link_libraries(cors_rate_limit_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(cors_rate_limit_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(cors_rate_limit_demo uvhttp)