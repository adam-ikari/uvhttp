/* UVHTTP 错误处理模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_constants.h"

TEST(UvhttpErrorHandlerFullCoverageTest, ErrorInit) {
    uvhttp_error_init();
}

TEST(UvhttpErrorHandlerFullCoverageTest, ErrorCleanup) {
    uvhttp_error_cleanup();
}

TEST(UvhttpErrorHandlerFullCoverageTest, ErrorSetConfig) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_DEBUG,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
    uvhttp_error_set_config(nullptr);
}

TEST(UvhttpErrorHandlerFullCoverageTest, ErrorReport) {
    uvhttp_error_report_(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter", 
                        "test_function", "test_file.c", 100, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_OUT_OF_MEMORY, "Out of memory", 
                        "test_function", "test_file.c", 200, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_CONNECTION_ACCEPT, "Connection accept failed", 
                        "test_function", "test_file.c", 300, nullptr);
    
    uvhttp_error_report_(UVHTTP_OK, "Success", 
                        "test_function", "test_file.c", 400, nullptr);
}

TEST(UvhttpErrorHandlerFullCoverageTest, ErrorRecovery) {
    uvhttp_error_config_t fast_config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    uvhttp_error_set_config(&fast_config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_CONNECTION_ACCEPT,
        .function = "test_function",
        .file = "test_file.c",
        .line = 100,
        .message = "Connection accept failed",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    (void)result;
    
    context.error_code = UVHTTP_ERROR_OUT_OF_MEMORY;
    result = uvhttp_error_attempt_recovery(&context);
    (void)result;
    
    fast_config.enableRecovery = 0;
    uvhttp_error_set_config(&fast_config);
    
    result = uvhttp_error_attempt_recovery(&context);
}

TEST(UvhttpErrorHandlerFullCoverageTest, LogFunctions) {
    uvhttp_log(UVHTTP_LOG_LEVEL_DEBUG, "Debug message");
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, "Info message");
    uvhttp_log(UVHTTP_LOG_LEVEL_WARN, "Warning message");
    uvhttp_log(UVHTTP_LOG_LEVEL_ERROR, "Error message");
    uvhttp_log(UVHTTP_LOG_LEVEL_FATAL, "Fatal message");
    
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, "Formatted message: %d, %s, %f", 42, "test", 3.14);
    
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, nullptr);
}

TEST(UvhttpErrorHandlerFullCoverageTest, LogLevelFiltering) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_ERROR,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    uvhttp_error_set_config(&config);
    
    uvhttp_log(UVHTTP_LOG_LEVEL_DEBUG, "Debug message (should be filtered)");
    uvhttp_log(UVHTTP_LOG_LEVEL_INFO, "Info message (should be filtered)");
    uvhttp_log(UVHTTP_LOG_LEVEL_WARN, "Warning message (should be filtered)");
    
    uvhttp_log(UVHTTP_LOG_LEVEL_ERROR, "Error message (should be output)");
    uvhttp_log(UVHTTP_LOG_LEVEL_FATAL, "Fatal message (should be output)");
    
    config.min_logLevel = UVHTTP_LOG_LEVEL_INFO;
    uvhttp_error_set_config(&config);
}

TEST(UvhttpErrorHandlerFullCoverageTest, CustomHandler) {
    auto custom_error_handler = [](const uvhttp_error_context_t* context) {
        (void)context;
    };
    
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = custom_error_handler,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    uvhttp_error_set_config(&config);
    
    uvhttp_error_report_(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter", 
                        "test_function", "test_file.c", 100, nullptr);
    
    config.customHandler = nullptr;
    uvhttp_error_set_config(&config);
}

TEST(UvhttpErrorHandlerFullCoverageTest, DifferentErrorTypes) {
    uvhttp_error_report_(UVHTTP_ERROR_SERVER_INIT, "Server init failed", 
                        "test_function", "test_file.c", 100, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_CONNECTION_INIT, "Connection init failed", 
                        "test_function", "test_file.c", 200, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_REQUEST_INIT, "Request init failed", 
                        "test_function", "test_file.c", 300, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_RESPONSE_INIT, "Response init failed", 
                        "test_function", "test_file.c", 400, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_TLS_INIT, "TLS init failed", 
                        "test_function", "test_file.c", 500, nullptr);
    
    uvhttp_error_report_(UVHTTP_ERROR_ROUTER_INIT, "Router init failed", 
                        "test_function", "test_file.c", 600, nullptr);
}

TEST(UvhttpErrorHandlerFullCoverageTest, MultipleInitCleanup) {
    for (int i = 0; i < 5; i++) {
        uvhttp_error_init();
        uvhttp_error_cleanup();
    }
}