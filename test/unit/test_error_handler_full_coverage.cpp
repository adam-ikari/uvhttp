#include <gtest/gtest.h>
#include "uvhttp_error_handler.h"
#include "uvhttp_allocator.h"
#include "uvhttp_logging.h"
#include <string.h>

/* 测试错误处理初始化 */
TEST(UvhttpErrorHandlerTest, ErrorInit) {
    /* 测试初始化错误处理系统 */
    uvhttp_error_recovery_init();
}

/* 测试错误处理清理 */
TEST(UvhttpErrorHandlerTest, ErrorCleanup) {
    /* 测试清理错误处理系统 */
    uvhttp_error_recovery_init();
    uvhttp_error_cleanup();
}

/* 测试设置错误处理配置 - 正常情况 */
TEST(UvhttpErrorHandlerTest, SetConfigNormal) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 1,
        .max_retries = 5,
        .base_delay_ms = 200,
        .max_delay_ms = 10000,
        .backoff_multiplier = 3.0
    };
    
    uvhttp_error_set_recovery_config(&config);
}

/* 测试设置错误处理配置 - NULL 配置 */
TEST(UvhttpErrorHandlerTest, SetConfigNull) {
    /* 测试 NULL 配置不会崩溃 */
    uvhttp_error_set_recovery_config(nullptr);
}

/* 测试错误报告 - 正常情况 */
TEST(UvhttpErrorHandlerTest, ErrorReportNormal) {
    uvhttp_error_recovery_init();
    
    /* 测试错误报告 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter test");
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - 带用户数据 */
TEST(UvhttpErrorHandlerTest, ErrorReportWithData) {
    uvhttp_error_recovery_init();
    
    int user_data = 42;
    UVHTTP_ERROR_REPORT_WITH_DATA(UVHTTP_ERROR_OUT_OF_MEMORY, "Out of memory test", &user_data);
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - 不同错误码 */
TEST(UvhttpErrorHandlerTest, ErrorReportDifferentCodes) {
    uvhttp_error_recovery_init();
    
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Test 1");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_OUT_OF_MEMORY, "Test 2");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_NOT_FOUND, "Test 3");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_SERVER_INIT, "Test 4");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_CONNECTION_INIT, "Test 5");
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - 空消息 */
TEST(UvhttpErrorHandlerTest, ErrorReportEmptyMessage) {
    uvhttp_error_recovery_init();
    
    /* 测试空消息不会崩溃 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "");
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - NULL 消息 */
TEST(UvhttpErrorHandlerTest, ErrorReportNullMessage) {
    uvhttp_error_recovery_init();
    
    /* 测试 NULL 消息不会崩溃 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, nullptr);
    
    uvhttp_error_cleanup();
}

/* 测试错误恢复 - 禁用恢复 */
TEST(UvhttpErrorHandlerTest, ErrorAttemptRecoveryDisabled) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 0,
        .max_retries = 3,
        .base_delay_ms = 100,
        .max_delay_ms = 5000,
        .backoff_multiplier = 2.0
    };
    
    uvhttp_error_set_recovery_config(&config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_CONNECTION_ACCEPT,
        .function = "test_function",
        .file = "test_file.c",
        .line = 42,
        .message = "Test error",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    
    /* 恢复被禁用，应该返回原始错误码 */
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_ACCEPT);
}

/* 测试错误恢复 - 连接接受错误 */
TEST(UvhttpErrorHandlerTest, ErrorAttemptRecoveryConnectionAccept) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 1,
        .max_retries = 2,
        .base_delay_ms = 10,
        .max_delay_ms = 100,
        .backoff_multiplier = 2.0
    };
    
    uvhttp_error_set_recovery_config(&config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_CONNECTION_ACCEPT,
        .function = "test_function",
        .file = "test_file.c",
        .line = 42,
        .message = "Test error",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    
    /* 恢复失败，应该返回原始错误码 */
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_ACCEPT);
}

/* 测试错误恢复 - 内存不足错误 */
TEST(UvhttpErrorHandlerTest, ErrorAttemptRecoveryOutOfMemory) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 1,
        .max_retries = 3,
        .base_delay_ms = 100,
        .max_delay_ms = 5000,
        .backoff_multiplier = 2.0
    };
    
    uvhttp_error_set_recovery_config(&config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_OUT_OF_MEMORY,
        .function = "test_function",
        .file = "test_file.c",
        .line = 42,
        .message = "Test error",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    
    /* 内存恢复应该返回成功 */
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试错误恢复 - 其他错误 */
TEST(UvhttpErrorHandlerTest, ErrorAttemptRecoveryOtherError) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 1,
        .max_retries = 3,
        .base_delay_ms = 100,
        .max_delay_ms = 5000,
        .backoff_multiplier = 2.0
    };
    
    uvhttp_error_set_recovery_config(&config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_INVALID_PARAM,
        .function = "test_function",
        .file = "test_file.c",
        .line = 42,
        .message = "Test error",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    
    /* 其他错误不进行恢复，应该返回成功 */
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试日志函数 - DEBUG 级别 */
TEST(UvhttpErrorHandlerTest, LogDebug) {
    /* 测试日志函数 - DEBUG 级别 */
    UVHTTP_LOG_DEBUG("Debug message");
}

/* 测试日志函数 - INFO 级别 */
TEST(UvhttpErrorHandlerTest, LogInfo) {
    /* 测试日志函数 - INFO 级别 */
    UVHTTP_LOG_INFO("Info message");
}

/* 测试日志函数 - WARN 级别 */
TEST(UvhttpErrorHandlerTest, LogWarn) {
    /* 测试日志函数 - WARN 级别 */
    UVHTTP_LOG_WARN("Warning message");
}

/* 测试日志函数 - ERROR 级别 */
TEST(UvhttpErrorHandlerTest, LogError) {
    /* 测试日志函数 - ERROR 级别 */
    UVHTTP_LOG_ERROR("Error message");
}

/* 测试日志函数 - FATAL 级别 */
TEST(UvhttpErrorHandlerTest, LogFatal) {
    /* 测试日志函数 - FATAL 级别 */
    UVHTTP_LOG_FATAL("Fatal message");
}

/* 测试日志函数 - 格式化字符串 */
TEST(UvhttpErrorHandlerTest, LogFormatString) {
    /* 测试格式化字符串 */
    UVHTTP_LOG_INFO("Test message: %d, %s", 42, "hello");
}

/* 测试错误报告多次调用 */
TEST(UvhttpErrorHandlerTest, ErrorReportMultiple) {
    uvhttp_error_recovery_init();
    
    /* 测试多次错误报告 */
    for (int i = 0; i < 10; i++) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Test error");
    }
    
    uvhttp_error_cleanup();
}

/* 测试错误恢复配置 - 不同重试次数 */
TEST(UvhttpErrorHandlerTest, ErrorRecoveryDifferentRetries) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 1,
        .max_retries = 1,
        .base_delay_ms = 10,
        .max_delay_ms = 100,
        .backoff_multiplier = 2.0
    };
    
    uvhttp_error_set_recovery_config(&config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_CONNECTION_ACCEPT,
        .function = "test_function",
        .file = "test_file.c",
        .line = 42,
        .message = "Test error",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    
    /* 恢复失败，应该返回原始错误码 */
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_ACCEPT);
}

/* 测试错误恢复配置 - 不同延迟参数 */
TEST(UvhttpErrorHandlerTest, ErrorRecoveryDifferentDelays) {
    uvhttp_error_recovery_config_t config = {
        .custom_handler = nullptr,
        .enable_recovery = 1,
        .max_retries = 3,
        .base_delay_ms = 50,
        .max_delay_ms = 200,
        .backoff_multiplier = 1.5
    };
    
    uvhttp_error_set_recovery_config(&config);
    
    uvhttp_error_context_t context = {
        .error_code = UVHTTP_ERROR_CONNECTION_ACCEPT,
        .function = "test_function",
        .file = "test_file.c",
        .line = 42,
        .message = "Test error",
        .timestamp = 0,
        .user_data = nullptr
    };
    
    uvhttp_error_t result = uvhttp_error_attempt_recovery(&context);
    
    /* 恢复失败，应该返回原始错误码 */
    EXPECT_EQ(result, UVHTTP_ERROR_CONNECTION_ACCEPT);
}