#include <gtest/gtest.h>
#include "uvhttp_error_handler.h"
#include "uvhttp_allocator.h"
#include "uvhttp_logging.h"
#include <string.h>

/* 测试错误报告 - 正常情况 */
TEST(UvhttpErrorHandlerTest, ErrorReportNormal) {
    /* 测试错误报告 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Invalid parameter test");
}

/* 测试错误报告 - 带用户数据 */
TEST(UvhttpErrorHandlerTest, ErrorReportWithData) {
    int user_data = 42;
    UVHTTP_ERROR_REPORT_WITH_DATA(UVHTTP_ERROR_OUT_OF_MEMORY, "Out of memory test", &user_data);
}

/* 测试错误报告 - 不同错误码 */
TEST(UvhttpErrorHandlerTest, ErrorReportDifferentCodes) {
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Test 1");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_OUT_OF_MEMORY, "Test 2");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_NOT_FOUND, "Test 3");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_SERVER_INIT, "Test 4");
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_CONNECTION_INIT, "Test 5");
}

/* 测试错误报告 - 空消息 */
TEST(UvhttpErrorHandlerTest, ErrorReportEmptyMessage) {
    /* 测试空消息不会崩溃 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "");
}

/* 测试错误报告 - NULL 消息 */
TEST(UvhttpErrorHandlerTest, ErrorReportNullMessage) {
    /* 测试 NULL 消息不会崩溃 */
    UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, nullptr);
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
    /* 测试多次错误报告 */
    for (int i = 0; i < 10; i++) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_INVALID_PARAM, "Test error");
    }
}