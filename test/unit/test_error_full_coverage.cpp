/* UVHTTP 错误处理模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_error.h"
#include "uvhttp_constants.h"

TEST(UvhttpErrorFullCoverageTest, ErrorString) {
    EXPECT_NE(uvhttp_error_string(UVHTTP_OK), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_INVALID_PARAM), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_OUT_OF_MEMORY), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_NOT_FOUND), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_SERVER_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_SERVER_LISTEN), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_CONNECTION_ACCEPT), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_REQUEST_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_RESPONSE_INIT), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_TLS_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_TLS_HANDSHAKE), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_ROUTER_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_ROUTE_NOT_FOUND), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_RATE_LIMIT_EXCEEDED), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_CONFIG_PARSE), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_CONFIG_INVALID), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_MIDDLEWARE_EXECUTE), nullptr);
    
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_LOG_INIT), nullptr);
    EXPECT_NE(uvhttp_error_string(UVHTTP_ERROR_LOG_WRITE), nullptr);
}

TEST(UvhttpErrorFullCoverageTest, ErrorCategoryString) {
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_OK), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_INVALID_PARAM), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_SERVER_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_CONNECTION_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_REQUEST_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_TLS_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_ROUTER_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_RATE_LIMIT_EXCEEDED), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_WEBSOCKET_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_CONFIG_PARSE), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_MIDDLEWARE_INIT), nullptr);
    EXPECT_NE(uvhttp_error_category_string(UVHTTP_ERROR_LOG_INIT), nullptr);
}

TEST(UvhttpErrorFullCoverageTest, ErrorDescription) {
    EXPECT_NE(uvhttp_error_description(UVHTTP_OK), nullptr);
    EXPECT_NE(uvhttp_error_description(UVHTTP_ERROR_INVALID_PARAM), nullptr);
    EXPECT_NE(uvhttp_error_description(UVHTTP_ERROR_SERVER_INIT), nullptr);
    EXPECT_NE(uvhttp_error_description(UVHTTP_ERROR_CONNECTION_INIT), nullptr);
    EXPECT_NE(uvhttp_error_description(UVHTTP_ERROR_TLS_INIT), nullptr);
    EXPECT_NE(uvhttp_error_description(UVHTTP_ERROR_WEBSOCKET_INIT), nullptr);
}

TEST(UvhttpErrorFullCoverageTest, ErrorSuggestion) {
    EXPECT_NE(uvhttp_error_suggestion(UVHTTP_OK), nullptr);
    EXPECT_NE(uvhttp_error_suggestion(UVHTTP_ERROR_INVALID_PARAM), nullptr);
    EXPECT_NE(uvhttp_error_suggestion(UVHTTP_ERROR_SERVER_INIT), nullptr);
    EXPECT_NE(uvhttp_error_suggestion(UVHTTP_ERROR_CONNECTION_INIT), nullptr);
    EXPECT_NE(uvhttp_error_suggestion(UVHTTP_ERROR_TLS_INIT), nullptr);
    EXPECT_NE(uvhttp_error_suggestion(UVHTTP_ERROR_WEBSOCKET_INIT), nullptr);
}

TEST(UvhttpErrorFullCoverageTest, ErrorIsRecoverable) {
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_ACCEPT), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_RESET), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_TIMEOUT), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_SEND), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_TLS_HANDSHAKE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_WEBSOCKET_HANDSHAKE), 1);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_MIDDLEWARE_EXECUTE), 1);
    
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_OK), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_INVALID_PARAM), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_OUT_OF_MEMORY), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_NOT_FOUND), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_SERVER_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_CONNECTION_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_REQUEST_INIT), 0);
    EXPECT_EQ(uvhttp_error_is_recoverable(UVHTTP_ERROR_RESPONSE_INIT), 0);
}

TEST(UvhttpErrorFullCoverageTest, ErrorRecoveryConfig) {
    uvhttp_set_error_recovery_config(5, 100, 5000, 3.0);
    
    uvhttp_set_error_recovery_config(-1, 100, 5000, 3.0);
    uvhttp_set_error_recovery_config(5, -1, 5000, 3.0);
    uvhttp_set_error_recovery_config(5, 100, -1, 3.0);
    uvhttp_set_error_recovery_config(5, 100, 5000, 0.5);
}

TEST(UvhttpErrorFullCoverageTest, ErrorLog) {
    uvhttp_log_error(UVHTTP_OK, "Test context");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter");
    uvhttp_log_error(UVHTTP_ERROR_SERVER_INIT, "Server initialization failed");
    uvhttp_log_error(UVHTTP_ERROR_CONNECTION_INIT, "Connection initialization failed");
    uvhttp_log_error(UVHTTP_ERROR_TLS_INIT, "TLS initialization failed");
    
    uvhttp_log_error(UVHTTP_OK, nullptr);
}

TEST(UvhttpErrorFullCoverageTest, ErrorStats) {
    size_t error_counts[UVHTTP_ERROR_COUNT];
    time_t last_error_time;
    const char* last_error_context;
    
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Test error 1");
    uvhttp_log_error(UVHTTP_ERROR_SERVER_INIT, "Test error 2");
    uvhttp_log_error(UVHTTP_ERROR_CONNECTION_INIT, "Test error 3");
    
    uvhttp_get_error_stats(error_counts, &last_error_time, &last_error_context);
    
    uvhttp_reset_error_stats();
    
    uvhttp_get_error_stats(error_counts, &last_error_time, &last_error_context);
}

TEST(UvhttpErrorFullCoverageTest, MostFrequentError) {
    uvhttp_reset_error_stats();
    
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Test error 1");
    uvhttp_log_error(UVHTTP_ERROR_INVALID_PARAM, "Test error 2");
    uvhttp_log_error(UVHTTP_ERROR_SERVER_INIT, "Test error 3");
    
    uvhttp_error_t most_frequent = uvhttp_get_most_frequent_error();
    (void)most_frequent;
    
    uvhttp_reset_error_stats();
}

TEST(UvhttpErrorFullCoverageTest, RetryOperation) {
    auto test_operation = [](void* context) -> uvhttp_error_t {
        (void)context;
        return UVHTTP_OK;
    };
    
    uvhttp_error_t result = uvhttp_retry_operation(test_operation, nullptr, "Test operation");
    (void)result;
}

TEST(UvhttpErrorFullCoverageTest, AllErrorCodes) {
    for (int i = UVHTTP_ERROR_MAX; i <= 0; i++) {
        EXPECT_NE(uvhttp_error_string((uvhttp_error_t)i), nullptr);
    }
}

TEST(UvhttpErrorFullCoverageTest, EdgeCases) {
    uvhttp_error_t invalid_error = (uvhttp_error_t)9999;
    EXPECT_NE(uvhttp_error_string(invalid_error), nullptr);
    EXPECT_NE(uvhttp_error_category_string(invalid_error), nullptr);
    EXPECT_NE(uvhttp_error_description(invalid_error), nullptr);
    EXPECT_NE(uvhttp_error_suggestion(invalid_error), nullptr);
    
    int recoverable = uvhttp_error_is_recoverable(invalid_error);
    (void)recoverable;
}