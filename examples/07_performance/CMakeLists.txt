# 性能测试示例

# HTTPS 性能测试服务器 - 暂时禁用，需要完全重写以使用正确的 API
# if(BUILD_WITH_HTTPS)
#     add_executable(performance_https_server
#         performance_https_server.c
#     )
#     
#     target_include_directories(performance_https_server PRIVATE
#         ${UVHTTP_INCLUDE_DIR}
#         ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/include
#     )
#     
#     target_link_libraries(performance_https_server
#         ${UVHTTP_LIB}
#         ${LIBUV_LIB}
#         ${MBEDTLS_LIBS}
#         ${LLHTTP_LIB}
#         ${XXHASH_LIB}
#         -lpthread -lm
#     )
# endif()

# WebSocket 性能测试服务器 - 暂时禁用，需要完全重写以使用正确的 API
# if(BUILD_WITH_WEBSOCKET)
#     add_executable(performance_websocket_server
#         performance_websocket_server.c
#     )
#     
#     target_include_directories(performance_websocket_server PRIVATE
#         ${UVHTTP_INCLUDE_DIR}
#         ${CMAKE_CURRENT_SOURCE_DIR}/../deps/libuv/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/../deps/mbedtls/include
#         ${CMAKE_CURRENT_SOURCE_DIR}/../deps/cllhttp/include
#     )
#     
#     target_link_libraries(performance_websocket_server
#         ${UVHTTP_LIB}
#         ${LIBUV_LIB}
#         ${MBEDTLS_LIBS}
#         ${LLHTTP_LIB}
#         ${XXHASH_LIB}
#         -lpthread -lm
#     )
# endif()

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