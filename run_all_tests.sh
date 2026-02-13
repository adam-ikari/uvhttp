#!/bin/bash
export LD_LIBRARY_PATH=/home/zhaodi-chen/project/uvhttp/dist/lib:$LD_LIBRARY_PATH
echo "Running all unit tests..."
echo ""
TEST_COUNT=0
TEST_PASSED=0
TEST_FAILED=0
TOTAL_TESTS=58
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_allocator..."
case "test_allocator" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_allocator; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_cmocka_simple..."
case "test_cmocka_simple" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_cmocka_simple; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_config_enhanced_coverage..."
case "test_config_enhanced_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_config_enhanced_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_api_coverage..."
case "test_connection_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_comprehensive_coverage..."
case "test_connection_comprehensive_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_comprehensive_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_enhanced_coverage..."
case "test_connection_enhanced_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_enhanced_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_full_api_coverage..."
case "test_connection_full_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_full_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_full_coverage..."
case "test_connection_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_lifecycle..."
case "test_connection_lifecycle" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_lifecycle; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_connection_websocket_integration..."
case "test_connection_websocket_integration" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_connection_websocket_integration; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_context_simple..."
case "test_context_simple" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_context_simple; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_death..."
case "test_death" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_death; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_error_api_coverage..."
case "test_error_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_error_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_error_comprehensive_coverage..."
case "test_error_comprehensive_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_error_comprehensive_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_error_coverage..."
case "test_error_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_error_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_error_enhanced_coverage..."
case "test_error_enhanced_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_error_enhanced_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_error_handler_full_coverage..."
case "test_error_handler_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_error_handler_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_error_helpers_full_coverage..."
case "test_error_helpers_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_error_helpers_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_hash_full_coverage..."
case "test_hash_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_hash_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_lru_cache_full_coverage..."
case "test_lru_cache_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_lru_cache_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_memory..."
case "test_memory" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_memory; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_query_validation..."
case "test_query_validation" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_query_validation; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_request_api_coverage..."
case "test_request_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_request_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_request_comprehensive_coverage..."
case "test_request_comprehensive_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_request_comprehensive_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_request_extra_coverage..."
case "test_request_extra_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_request_extra_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_request_full_coverage..."
case "test_request_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_request_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_request_full_coverage_enhanced..."
case "test_request_full_coverage_enhanced" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_request_full_coverage_enhanced; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_request_null_coverage..."
case "test_request_null_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_request_null_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_response_full_coverage..."
case "test_response_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_response_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_response_full_coverage_enhanced..."
case "test_response_full_coverage_enhanced" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_response_full_coverage_enhanced; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_router_enhanced_coverage..."
case "test_router_enhanced_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_router_enhanced_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_router_full_coverage..."
case "test_router_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_router_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_sendfile_timeout..."
case "test_sendfile_timeout" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_sendfile_timeout; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_server_api_coverage..."
case "test_server_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_server_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_server_error_coverage..."
case "test_server_error_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_server_error_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_server_rate_limit_full_coverage..."
case "test_server_rate_limit_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_server_rate_limit_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_server_simple_api_coverage..."
case "test_server_simple_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_server_simple_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_server_simple_handlers_coverage..."
case "test_server_simple_handlers_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_server_simple_handlers_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_simple_main..."
case "test_simple_main" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_simple_main; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_static_api_coverage..."
case "test_static_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_static_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_static_comprehensive_coverage..."
case "test_static_comprehensive_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_static_comprehensive_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_static_coverage..."
case "test_static_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_static_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_static_enhanced_coverage..."
case "test_static_enhanced_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_static_enhanced_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_static_file_operations..."
case "test_static_file_operations" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_static_file_operations; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_static_more_coverage..."
case "test_static_more_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_static_more_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_stress..."
case "test_stress" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_stress; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_tls_api_coverage..."
case "test_tls_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_tls_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_tls_null_coverage..."
case "test_tls_null_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_tls_null_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_utils_api_coverage..."
case "test_utils_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_utils_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_utils_full_coverage..."
case "test_utils_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_utils_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_validation_full_coverage..."
case "test_validation_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_validation_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_websocket_api_coverage..."
case "test_websocket_api_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_websocket_api_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_websocket_enhanced_full_coverage..."
case "test_websocket_enhanced_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_websocket_enhanced_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_websocket_full_coverage..."
case "test_websocket_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_websocket_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_websocket_native_full_coverage..."
case "test_websocket_native_full_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_websocket_native_full_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_websocket_native_simple..."
case "test_websocket_native_simple" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_websocket_native_simple; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_websocket_null_coverage..."
case "test_websocket_null_coverage" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_websocket_null_coverage; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
TEST_COUNT=$((TEST_COUNT + 1))
echo "[$TEST_COUNT/$TOTAL_TESTS] Running test_whitelist_hash..."
case "test_whitelist_hash" in
    *stress*|*performance*)
        TIMEOUT=60
        ;;
    *)
        TIMEOUT=30
        ;;
esac
if /home/zhaodi-chen/project/uvhttp/dist/bin/test_whitelist_hash; then
    TEST_PASSED=$((TEST_PASSED + 1))
    echo "PASSED"
else
    TEST_FAILED=$((TEST_FAILED + 1))
    echo "FAILED"
fi
echo ""
echo "========================================="
echo "Test Summary"
echo "========================================="
echo "Total tests: $TEST_COUNT"
echo "Passed: $TEST_PASSED"
echo "Failed: $TEST_FAILED"
echo "========================================="
if [ $TEST_FAILED -gt 0 ]; then
    echo "Some tests failed"
    exit 1
else
    echo "All tests passed"
    exit 0
fi
