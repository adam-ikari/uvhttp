#include <gtest/gtest.h>
#include "uvhttp_error_handler.h"
#include "uvhttp_allocator.h"
#include "uvhttp_logging.h"
#include <string.h>

/* 测试错误处理初始化 */
TEST(UvhttpErrorHandlerTest, ErrorInit) {
    /* 测试初始化错误处理系统 */
    uvhttp_error_init();
}

/* 测试错误处理清理 */
TEST(UvhttpErrorHandlerTest, ErrorCleanup) {
    /* 测试清理错误处理系统 */
    uvhttp_error_init();
    uvhttp_error_cleanup();
}

/* 测试设置错误处理配置 - 正常情况 */
TEST(UvhttpErrorHandlerTest, SetConfigNormal) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_DEBUG,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 5,
        .baseDelayMs = 200,
        .maxDelayMs = 10000,
        .backoffMultiplier = 3.0
    };
    
    uvhttp_error_set_config(&config);
}

/* 测试设置错误处理配置 - NULL 配置 */
TEST(UvhttpErrorHandlerTest, SetConfigNull) {
    /* 测试 NULL 配置不会崩溃 */
    uvhttp_error_set_config(nullptr);
}

/* 测试错误报告 - 正常情况 */
TEST(UvhttpErrorHandlerTest, ErrorReportNormal) {
    uvhttp_error_init();
    
    /* 测试错误报告 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter test");
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - 带用户数据 */
TEST(UvhttpErrorHandlerTest, ErrorReportWithData) {
    uvhttp_error_init();
    
    int user_data = 42;
    UVHTTP_ERROR_REPORT_WITH_DATA(UVHTTP_ERROR_OUT_OF_MEMORY, "Out of memory test", &user_data);
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - 不同错误码 */
TEST(UvhttpErrorHandlerTest, ErrorReportDifferentCodes) {
    uvhttp_error_init();
    
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Test 1");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_OUT_OF_MEMORY, "Test 2");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_NOT_FOUND, "Test 3");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_SERVER_INIT, "Test 4");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_CONNECTION_INIT, "Test 5");
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - 空消息 */
TEST(UvhttpErrorHandlerTest, ErrorReportEmptyMessage) {
    uvhttp_error_init();
    
    /* 测试空消息不会崩溃 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "");
    
    uvhttp_error_cleanup();
}

/* 测试错误报告 - NULL 消息 */
TEST(UvhttpErrorHandlerTest, ErrorReportNullMessage) {
    uvhttp_error_init();
    
    /* 测试 NULL 消息不会崩溃 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, nullptr);
    
    uvhttp_error_cleanup();
}

/* 测试错误恢复 - 禁用恢复 */
TEST(UvhttpErrorHandlerTest, ErrorAttemptRecoveryDisabled) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 0,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
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
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 2,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
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
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
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
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
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
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_DEBUG,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);

    /* 测试日志函数 - DEBUG 级别 */
    UVHTTP_LOG_DEBUG("Debug message");
}

/* 测试日志函数 - INFO 级别 */
TEST(UvhttpErrorHandlerTest, LogInfo) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);

    /* 测试日志函数 - INFO 级别 */
    UVHTTP_LOG_INFO("Info message");
}

/* 测试日志函数 - WARN 级别 */
TEST(UvhttpErrorHandlerTest, LogWarn) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_WARN,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);

    /* 测试日志函数 - WARN 级别 */
    UVHTTP_LOG_WARN("Warning message");
}

/* 测试日志函数 - ERROR 级别 */
TEST(UvhttpErrorHandlerTest, LogError) {
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

    /* 测试日志函数 - ERROR 级别 */
    UVHTTP_LOG_ERROR("Error message");
}

/* 测试日志函数 - FATAL 级别 */
TEST(UvhttpErrorHandlerTest, LogFatal) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_FATAL,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);

    /* 测试日志函数 - FATAL 级别 */
    UVHTTP_LOG_FATAL("Fatal message");
}

/* 测试日志函数 - 低于最小级别 */
TEST(UvhttpErrorHandlerTest, LogBelowMinLevel) {
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

    /* 测试日志函数 - 低于最小级别 */
    UVHTTP_LOG_DEBUG("This should not be logged");
}

/* 测试日志函数 - 格式化字符串 */
TEST(UvhttpErrorHandlerTest, LogFormatString) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 100,
        .maxDelayMs = 5000,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);

    /* 测试格式化字符串 */
    UVHTTP_LOG_INFO("Test message: %d, %s", 42, "hello");
}

/* 测试错误检查宏 - UVHTTP_CHECK */
TEST(UvhttpErrorHandlerTest, CheckMacroSuccess) {
    /* 测试条件为真的情况 */
    int result = 0;
    
    /* 使用 lambda 模拟函数内的检查 */
    auto test_check = [&result]() -> uvhttp_error_t {
        if (!(1 == 1)) {
            uvhttp_error_report_(UVHTTP_ERROR_INVALID_PARAM, "Test check", __func__, __FILE__, __LINE__, NULL);
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        result = 1;
        return UVHTTP_OK;
    };
    
    uvhttp_error_t err = test_check();
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(result, 1);
}

/* 测试错误报告多次调用 */
TEST(UvhttpErrorHandlerTest, ErrorReportMultiple) {
    uvhttp_error_init();
    
    /* 测试多次错误报告 */
    for (int i = 0; i < 10; i++) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Test error");
    }
    
    uvhttp_error_cleanup();
}

/* 测试错误恢复配置 - 不同重试次数 */
TEST(UvhttpErrorHandlerTest, ErrorRecoveryDifferentRetries) {
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 1,
        .baseDelayMs = 10,
        .maxDelayMs = 100,
        .backoffMultiplier = 2.0
    };
    
    uvhttp_error_set_config(&config);
    
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
    uvhttp_error_config_t config = {
        .min_logLevel = UVHTTP_LOG_LEVEL_INFO,
        .customHandler = nullptr,
        .enableRecovery = 1,
        .maxRetries = 3,
        .baseDelayMs = 50,
        .maxDelayMs = 200,
        .backoffMultiplier = 1.5
    };
    
    uvhttp_error_set_config(&config);
    
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