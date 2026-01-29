# 高级特性示例

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)
set(CJSON_LIB ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/cjson/libcjson.a)

# API 演示
add_executable(api_demo api_demo.c)
target_link_libraries(api_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
target_include_directories(api_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(api_demo uvhttp)

# JSON API 演示
add_executable(json_api_demo json_api_demo.c)
target_link_libraries(json_api_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
target_include_directories(json_api_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(json_api_demo uvhttp)

# 配置演示 - 暂时禁用，需要重构
# add_executable(config_demo config_demo.c)
# target_link_libraries(config_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
# target_include_directories(config_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
# add_dependencies(config_demo uvhttp)

# 简单配置
add_executable(simple_config simple_config.c)
target_link_libraries(simple_config PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(simple_config PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(simple_config uvhttp)

# 统一响应演示
add_executable(unified_response_demo unified_response_demo.c)
target_link_libraries(unified_response_demo PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
target_include_directories(unified_response_demo PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(unified_response_demo uvhttp)

# 高级内存应用
add_executable(app_advanced_memory app_advanced_memory.c)
target_link_libraries(app_advanced_memory PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(app_advanced_memory PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(app_advanced_memory uvhttp)

# 上下文注入示例 - 暂时禁用，需要重构
# add_executable(context_injection context_injection.c)
# target_link_libraries(context_injection PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp ${CJSON_LIB} pthread m dl)
# target_include_directories(context_injection PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
# add_dependencies(context_injection uvhttp)

# 分层分配器示例 - 暂时禁用，缺少头文件
# add_executable(hierarchical_allocator_example hierarchical_allocator_example.c)
# target_link_libraries(hierarchical_allocator_example PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
# target_include_directories(hierarchical_allocator_example PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
# add_dependencies(hierarchical_allocator_example uvhttp)