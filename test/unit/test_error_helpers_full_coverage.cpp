#include <gtest/gtest.h>
#include "uvhttp_error_helpers.h"
#include "uvhttp_allocator.h"
#include "test_loop_helper.h"
#include <string.h>

/* 静态函数用于测试 */
static int g_cleanup_called = 0;
static void cleanup_func_test(void* data) {
    g_cleanup_called = 1;
}

static int g_custom_free_called = 0;
static void custom_free_func_test(void* data) {
    g_custom_free_called = 1;
    uvhttp_free(data);
}

/* uvhttp_cleanup_connection 函数已删除，符合极简工程原则 */

/* 测试处理内存分配失败 - 正常情况 */
TEST(UvhttpErrorHelpersTest, HandleMemoryFailureNormal) {
    g_cleanup_called = 0;
    uvhttp_handle_memory_failure("test_context", cleanup_func_test, nullptr);
    EXPECT_EQ(g_cleanup_called, 0); /* cleanup_data 是 nullptr */
}

/* 测试处理内存分配失败 - 带清理函数 */
TEST(UvhttpErrorHelpersTest, HandleMemoryFailureWithCleanup) {
    g_cleanup_called = 0;
    int cleanup_data = 42;
    
    uvhttp_handle_memory_failure("test_context", cleanup_func_test, &cleanup_data);
    EXPECT_EQ(g_cleanup_called, 1);
}

/* 测试处理内存分配失败 - NULL 上下文 */
TEST(UvhttpErrorHelpersTest, HandleMemoryFailureNullContext) {
    /* 测试 NULL 上下文不会崩溃 */
    uvhttp_handle_memory_failure(nullptr, cleanup_func_test, nullptr);
}

/* 测试处理内存分配失败 - NULL 清理函数 */
TEST(UvhttpErrorHelpersTest, HandleMemoryFailureNullCleanupFunc) {
    /* 测试 NULL 清理函数不会崩溃 */
    uvhttp_handle_memory_failure("test_context", nullptr, nullptr);
}

/* 测试处理写操作错误 - 正常情况 */
TEST(UvhttpErrorHelpersTest, HandleWriteErrorNormal) {
    uv_write_t* req = (uv_write_t*)uvhttp_alloc(sizeof(uv_write_t));
    ASSERT_NE(req, nullptr);
    
    /* 测试处理写操作错误 */
    uvhttp_handle_write_error(req, UV_ECONNRESET, "test_context");
    
    /* req 应该被释放 */
}

/* 测试处理写操作错误 - NULL 请求 */
TEST(UvhttpErrorHelpersTest, HandleWriteErrorNullRequest) {
    /* 测试 NULL 请求不会崩溃 */
    uvhttp_handle_write_error(nullptr, UV_ECONNRESET, "test_context");
}

/* 测试处理写操作错误 - NULL 上下文 */
TEST(UvhttpErrorHelpersTest, HandleWriteErrorNullContext) {
    uv_write_t* req = (uv_write_t*)uvhttp_alloc(sizeof(uv_write_t));
    ASSERT_NE(req, nullptr);
    
    /* 测试 NULL 上下文不会崩溃 */
    uvhttp_handle_write_error(req, UV_ECONNRESET, nullptr);
}

/* 测试处理写操作错误 - 不同错误码 */
TEST(UvhttpErrorHelpersTest, HandleWriteErrorDifferentCodes) {
    uv_write_t* req1 = (uv_write_t*)uvhttp_alloc(sizeof(uv_write_t));
    uv_write_t* req2 = (uv_write_t*)uvhttp_alloc(sizeof(uv_write_t));
    uv_write_t* req3 = (uv_write_t*)uvhttp_alloc(sizeof(uv_write_t));
    
    ASSERT_NE(req1, nullptr);
    ASSERT_NE(req2, nullptr);
    ASSERT_NE(req3, nullptr);
    
    /* 测试不同的错误码 */
    uvhttp_handle_write_error(req1, UV_EPIPE, "test_context");
    uvhttp_handle_write_error(req2, UV_ENOTCONN, "test_context");
    uvhttp_handle_write_error(req3, UV_ECANCELED, "test_context");
}

/* 测试安全错误日志记录 - 正常情况 */
TEST(UvhttpErrorHelpersTest, LogSafeErrorNormal) {
    /* 测试安全错误日志记录 */
    uvhttp_log_safe_error(UV_ECONNRESET, "test_context", "Connection reset by peer");
}

/* 测试安全错误日志记录 - NULL 上下文 */
TEST(UvhttpErrorHelpersTest, LogSafeErrorNullContext) {
    /* 测试 NULL 上下文不会崩溃 */
    uvhttp_log_safe_error(UV_ECONNRESET, nullptr, "Connection reset by peer");
}

/* 测试安全错误日志记录 - NULL 用户消息 */
TEST(UvhttpErrorHelpersTest, LogSafeErrorNullUserMessage) {
    /* 测试 NULL 用户消息不会崩溃 */
    uvhttp_log_safe_error(UV_ECONNRESET, "test_context", nullptr);
}

/* 测试安全错误日志记录 - 零错误码 */
TEST(UvhttpErrorHelpersTest, LogSafeErrorZeroErrorCode) {
    /* 测试零错误码使用用户消息 */
    uvhttp_log_safe_error(0, "test_context", "Custom error message");
}

/* 测试验证错误消息安全性 - 正常情况 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageNormal) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Normal error message", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Normal error message");
}

/* 测试验证错误消息安全性 - 包含敏感信息 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageSensitive) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Password: secret123", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* 测试验证错误消息安全性 - 包含 token */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageToken) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Token: abc123", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* 测试验证错误消息安全性 - 包含 key */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageKey) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("API key: xyz789", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* 测试验证错误消息安全性 - 包含 secret */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageSecret) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Secret: confidential", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* 测试验证错误消息安全性 - 包含 auth */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageAuth) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Auth: basic credentials", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* 测试验证错误消息安全性 - NULL 消息 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageNullMessage) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message(nullptr, safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, -1);
}

/* 测试验证错误消息安全性 - NULL 缓冲区 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageNullBuffer) {
    int result = uvhttp_sanitize_error_message("Test message", nullptr, 256);
    
    EXPECT_EQ(result, -1);
}

/* 测试验证错误消息安全性 - 零缓冲区大小 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageZeroBufferSize) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Test message", safe_buffer, 0);
    
    EXPECT_EQ(result, -1);
}

/* 测试验证错误消息安全性 - 消息过长 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageTooLong) {
    char safe_buffer[20];
    const char* long_message = "This is a very long error message that exceeds the buffer size";
    
    int result = uvhttp_sanitize_error_message(long_message, safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(safe_buffer), sizeof(safe_buffer) - 1);
    EXPECT_EQ(safe_buffer[sizeof(safe_buffer) - 4], '.');
    EXPECT_EQ(safe_buffer[sizeof(safe_buffer) - 3], '.');
    EXPECT_EQ(safe_buffer[sizeof(safe_buffer) - 2], '.');
}

/* 测试验证错误消息安全性 - 小缓冲区 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageSmallBuffer) {
    char safe_buffer[3];
    const char* message = "Test";
    
    int result = uvhttp_sanitize_error_message(message, safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(safe_buffer), 2);
    EXPECT_EQ(safe_buffer[0], 'T');
    EXPECT_EQ(safe_buffer[1], 'e');
    EXPECT_EQ(safe_buffer[2], '\0');
}

/* 测试验证错误消息安全性 - 空消息 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageEmptyMessage) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "");
}

/* 测试验证错误消息安全性 - 大小写不敏感 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageCaseInsensitive) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("PASSWORD: secret123", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* 测试验证错误消息安全性 - 包含多个敏感词 */
TEST(UvhttpErrorHelpersTest, SanitizeErrorMessageMultipleSensitive) {
    char safe_buffer[256];
    
    int result = uvhttp_sanitize_error_message("Password and token are sensitive", safe_buffer, sizeof(safe_buffer));
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(safe_buffer, "Sensitive information hidden");
}

/* uvhttp_safe_free 已删除 - 完全未使用，直接使用 uvhttp_free */

/* uvhttp_safe_free 已删除 - 完全未使用，直接使用 uvhttp_free */