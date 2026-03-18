# E2E Tests - Comprehensive end-to-end tests for all core features
# E2E 测试 - 所有核心功能的综合端到端测试

# 为所有 E2E 测试添加 zlib 链接（如果启用压缩）
if(BUILD_WITH_COMPRESSION)
    link_libraries(${ZLIB_LIBRARIES})
endif()

# E2E Test 1: Simple E2E Test
add_executable(test_simple_e2e test_simple_e2e.c)
target_include_directories(test_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_simple_e2e)

# E2E Test 2: WebSocket E2E Test
if(BUILD_WITH_WEBSOCKET)
    add_executable(test_websocket_simple_e2e test_websocket_simple_e2e.c)
    target_include_directories(test_websocket_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
    target_link_libraries(test_websocket_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
    uvhttp_add_example_dependencies(test_websocket_simple_e2e)
endif()

# E2E Test 3: Static Files E2E Test
if(BUILD_WITH_STATIC_FILES)
    add_executable(test_static_simple_e2e test_static_simple_e2e.c)
    target_include_directories(test_static_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
    target_link_libraries(test_static_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
    uvhttp_add_example_dependencies(test_static_simple_e2e)
endif()

# E2E Test 4: Error Handling E2E Test
add_executable(test_error_handling_simple_e2e test_error_handling_simple_e2e.c)
target_include_directories(test_error_handling_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_error_handling_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_error_handling_simple_e2e)

# E2E Test 5: Rate Limit E2E Test
add_executable(test_rate_limit_simple_e2e test_rate_limit_simple_e2e.c)
target_include_directories(test_rate_limit_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_rate_limit_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_rate_limit_simple_e2e)

# E2E Test 6: Routing E2E Test
add_executable(test_routing_simple_e2e test_routing_simple_e2e.c)
target_include_directories(test_routing_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_routing_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_routing_simple_e2e)

# E2E Test 7: LRU Cache E2E Test
add_executable(test_lru_cache_simple_e2e test_lru_cache_simple_e2e.c)
target_include_directories(test_lru_cache_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_lru_cache_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_lru_cache_simple_e2e)

# E2E Test 8: Connection E2E Test
add_executable(test_connection_simple_e2e test_connection_simple_e2e.c)
target_include_directories(test_connection_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_connection_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_connection_simple_e2e)

# E2E Test 9: Protocol Upgrade E2E Test
add_executable(test_protocol_upgrade_simple_e2e test_protocol_upgrade_simple_e2e.c)
target_include_directories(test_protocol_upgrade_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_protocol_upgrade_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_protocol_upgrade_simple_e2e)

# E2E Test 10: Error Handler E2E Test
add_executable(test_error_handler_simple_e2e test_error_handler_simple_e2e.c)
target_include_directories(test_error_handler_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_error_handler_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_error_handler_simple_e2e)

# E2E Test 11: Validation E2E Test
add_executable(test_validation_simple_e2e test_validation_simple_e2e.c)
target_include_directories(test_validation_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_validation_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_validation_simple_e2e)

# E2E Test 12: Utils E2E Test
add_executable(test_utils_simple_e2e test_utils_simple_e2e.c)
target_include_directories(test_utils_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_utils_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_utils_simple_e2e)

# E2E Test 13: Config E2E Test
add_executable(test_config_simple_e2e test_config_simple_e2e.c)
target_include_directories(test_config_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_config_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_config_simple_e2e)

# E2E Test 14: Logging E2E Test
add_executable(test_logging_simple_e2e test_logging_simple_e2e.c)
target_include_directories(test_logging_simple_e2e PRIVATE ${UVHTTP_INCLUDE_DIR})
target_link_libraries(test_logging_simple_e2e PRIVATE ${UVHTTP_LIB} ${LIBUV_LIB} ${XXHASH_LIB} ${LLHTTP_LIB} ${MBEDTLS_LIBS})
uvhttp_add_example_dependencies(test_logging_simple_e2e)