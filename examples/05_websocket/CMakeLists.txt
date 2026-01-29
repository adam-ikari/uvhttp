# WebSocket 示例

# 从父 CMakeLists.txt 获取 UVHTTP 库路径
set(UVHTTP_LIB ${CMAKE_BINARY_DIR}/dist/lib/libuvhttp.a)

# WebSocket Echo 服务器
add_executable(websocket_echo_server websocket_echo_server.c)
target_link_libraries(websocket_echo_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(websocket_echo_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(websocket_echo_server uvhttp)

# WebSocket 测试服务器
add_executable(websocket_test_server websocket_test_server.c)
target_link_libraries(websocket_test_server PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(websocket_test_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(websocket_test_server uvhttp)

# WebSocket 连接管理测试
add_executable(test_ws_connection_management test_ws_connection_management.c)
target_link_libraries(test_ws_connection_management PRIVATE ${UVHTTP_LIB} libuv mbedtls xxhash llhttp pthread m dl)
target_include_directories(test_ws_connection_management PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../../include ${CMAKE_CURRENT_SOURCE_DIR}/../../deps/uthash/src)
add_dependencies(test_ws_connection_management uvhttp)