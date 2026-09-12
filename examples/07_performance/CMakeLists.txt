# 性能测试示例

# 压缩性能测试服务器
add_executable(compression_server compression_server.c)
target_link_libraries(compression_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(compression_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
add_dependencies(compression_server uvhttp)
uvhttp_add_example_dependencies(compression_server)

# HTTPS 性能测试服务器
if(BUILD_WITH_HTTPS)
    add_executable(performance_https_server performance_https_server.c)
    target_link_libraries(performance_https_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
    target_include_directories(performance_https_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
    add_dependencies(performance_https_server uvhttp)
    uvhttp_add_example_dependencies(performance_https_server)
endif()

# WebSocket 性能测试服务器
if(BUILD_WITH_WEBSOCKET)
    add_executable(performance_websocket_server performance_websocket_server.c)
    target_link_libraries(performance_websocket_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
    target_include_directories(performance_websocket_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/llhttp/include)
    add_dependencies(performance_websocket_server uvhttp)
    uvhttp_add_example_dependencies(performance_websocket_server)
endif()

# 版本 API 测试 - 暂时禁用，API 实现不完整
# add_executable(test_version_api
#     test_version_api.c
# )
#
# target_include_directories(test_version_api PRIVATE
#     ${UVHTTP_INCLUDE_DIR}
#     ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/include
# )
#
# target_link_libraries(test_version_api
#     ${UVHTTP_LIB}
#     ${LIBUV_LIB}
#     ${XXHASH_LIB}
#     -lpthread -lm
# )