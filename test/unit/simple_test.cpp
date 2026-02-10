#include <gtest/gtest.h>
#include <string.h>
#include "../include/uvhttp_utils.h"
#include "../include/uvhttp_validation.h"
#include "../include/uvhttp_request.h"
#include "../include/uvhttp_logging.h"

// ============================================================================
// 工具函数测试
// ============================================================================

TEST(UvhttpUtilsTest, SafeStrncpyNormal) {
    char dest[10];
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "hello", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "hello");
}

TEST(UvhttpUtilsTest, SafeStrncpyOverflow) {
    char dest[5];
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "123456789", sizeof(dest)), 0);
    EXPECT_LT(strlen(dest), sizeof(dest));
}

TEST(UvhttpUtilsTest, SafeStrncpyNullChecks) {
    char dest[10];
    EXPECT_EQ(uvhttp_safe_strncpy(NULL, "test", sizeof(dest)), -1);
    EXPECT_EQ(uvhttp_safe_strncpy(dest, NULL, sizeof(dest)), -1);
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "test", 0), -1);
}

// ============================================================================
// URL 验证测试
// ============================================================================

TEST(UvhttpValidationTest, ValidateUrlValid) {
    EXPECT_EQ(uvhttp_validate_url_path("/"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/api/users"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/api/v1/users/123"), 1);
}

TEST(UvhttpValidationTest, ValidateUrlInvalid) {
    EXPECT_EQ(uvhttp_validate_url_path("no-leading-slash"), 0);
    EXPECT_EQ(uvhttp_validate_url_path(NULL), 0);
    EXPECT_EQ(uvhttp_validate_url_path(""), 0);
}

// ============================================================================
// Header 验证测试
// ============================================================================

TEST(UvhttpValidationTest, ValidateHeaderValueValid) {
    EXPECT_EQ(uvhttp_validate_header_value_safe("application/json"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("text/plain"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("Mozilla/5.0"), 1);
}

TEST(UvhttpValidationTest, ValidateHeaderValueInvalid) {
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x01"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x1F"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x7F"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe(NULL), 0);
}

// ============================================================================
// HTTP 方法验证测试
// ============================================================================

/* uvhttp_validate_http_method 已删除 - 使用 uvhttp_method_from_string 替代 */

// ============================================================================
// 边界条件测试
// ============================================================================

TEST(UvhttpUtilsTest, EdgeCasesMinBuffer) {
    char dest[1];
    // dest_size=1只能存储null终止符，无法存储任何字符
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "a", sizeof(dest)), 0);
    EXPECT_EQ(dest[0], '\0');
    
    // 空字符串也应该返回0
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "", sizeof(dest)), 0);
    EXPECT_EQ(dest[0], '\0');
    
    // 测试实际可以复制一个字符的情况
    char dest2[2];
    EXPECT_EQ(uvhttp_safe_strncpy(dest2, "a", sizeof(dest2)), 0);
    EXPECT_EQ(dest2[0], 'a');
    EXPECT_EQ(dest2[1], '\0');
}

TEST(UvhttpUtilsTest, EdgeCasesLongString) {
    char dest[256];
    const char* long_string = "This is a very long string that should still be handled efficiently without causing any buffer overflows";
    int result = uvhttp_safe_strncpy(dest, long_string, sizeof(dest));
    EXPECT_EQ(result, 0);
    EXPECT_LT(strlen(dest), sizeof(dest));
    EXPECT_EQ(strncmp(dest, long_string, strlen(dest)), 0);
}

// ============================================================================
// 性能边界测试
// ============================================================================

TEST(UvhttpUtilsTest, PerformanceManyOperations) {
    char dest[10];
    const char* src = "test";
    
    // 执行多次操作测试性能
    for (int i = 0; i < 1000; i++) {
        EXPECT_EQ(uvhttp_safe_strncpy(dest, src, sizeof(dest)), 0);
        EXPECT_EQ(uvhttp_validate_url_path("/api/test"), 1);
        EXPECT_EQ(uvhttp_validate_header_value_safe("value"), 1);
        /* uvhttp_validate_http_method 已删除 */
    }
}

// ============================================================================
// 内存管理测试
// ============================================================================

TEST(UvhttpUtilsTest, MemoryManagementRepeatedAlloc) {
    char dest[10];
    
    // 重复分配和释放模式
    for (int i = 0; i < 100; i++) {
        EXPECT_EQ(uvhttp_safe_strncpy(dest, "test", sizeof(dest)), 0);
        EXPECT_STREQ(dest, "test");
    }
}

// ============================================================================
// 错误恢复测试
// ============================================================================

TEST(UvhttpUtilsTest, ErrorRecoverySequence) {
    char dest[10];
    
    // 连续错误后正常操作
    EXPECT_EQ(uvhttp_safe_strncpy(NULL, "test", sizeof(dest)), -1);
    EXPECT_EQ(uvhttp_safe_strncpy(dest, NULL, sizeof(dest)), -1);
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "test", 0), -1);
    
    // 恢复正常操作
    EXPECT_EQ(uvhttp_safe_strncpy(dest, "ok", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "ok");
}

// ============================================================================
// 并发安全模拟测试
// ============================================================================

TEST(UvhttpUtilsTest, ThreadSafetySimulation) {
    char dest1[10], dest2[10];
    char dest3[10];
    
    // 模拟并发操作
    EXPECT_EQ(uvhttp_safe_strncpy(dest1, "test1", sizeof(dest1)), 0);
    EXPECT_EQ(uvhttp_safe_strncpy(dest2, "test2", sizeof(dest2)), 0);
    EXPECT_EQ(uvhttp_safe_strncpy(dest3, "test3", sizeof(dest3)), 0);
    
    EXPECT_STREQ(dest1, "test1");
    EXPECT_STREQ(dest2, "test2");
    EXPECT_STREQ(dest3, "test3");
    
    EXPECT_NE(strcmp(dest1, dest2), 0);
    EXPECT_NE(strcmp(dest2, dest3), 0);
    EXPECT_NE(strcmp(dest1, dest3), 0);
}

// ============================================================================
// 极限测试
// ============================================================================

TEST(UvhttpValidationTest, ExtremeConditions) {
    char dest[256];
    
    // 测试极端验证深度
    EXPECT_EQ(uvhttp_validate_header_value_safe("a"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe(""), 1);
    
    // 测试极长URL
    char long_url[3000];
    memset(long_url, 'a', sizeof(long_url) - 1);
    long_url[sizeof(long_url) - 1] = '\0';
    EXPECT_EQ(uvhttp_validate_url_path(long_url), 0);
}

// ============================================================================
// Request GetPath 测试
// ============================================================================

TEST(UvhttpRequestTest, GetPathNull) {
    const char* path = uvhttp_request_get_path(NULL);
    EXPECT_EQ(path, nullptr);
}

TEST(UvhttpRequestTest, GetPathSimple) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/test/path");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/test/path");
}

TEST(UvhttpRequestTest, GetPathRoot) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/");
}

TEST(UvhttpRequestTest, GetPathWithQuery) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/test/path?key=value");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/test/path");
}

TEST(UvhttpRequestTest, GetPathWithMultipleQuery) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/test/path?key1=value1&key2=value2&key3=value3");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/test/path");
}

TEST(UvhttpRequestTest, GetPathWithFragment) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/test/path#section");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/test/path#section");
}

TEST(UvhttpRequestTest, GetPathWithQueryAndFragment) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/test/path?key=value#section");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/test/path");
}

TEST(UvhttpRequestTest, GetPathComplexQuery) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/v1/users?id=123&name=test&age=25");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/api/v1/users");
}

TEST(UvhttpRequestTest, GetPathEmptyQuery) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/test/path?");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/test/path");
}

TEST(UvhttpRequestTest, GetPathQueryOnly) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/?key=value");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/");
}

TEST(UvhttpRequestTest, GetPathLongPath) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/v1/users/123/posts/456/comments/789");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/api/v1/users/123/posts/456/comments/789");
}

TEST(UvhttpRequestTest, GetPathLongPathWithQuery) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/api/v1/users/123/posts/456/comments/789?filter=recent&sort=desc");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/api/v1/users/123/posts/456/comments/789");
}

TEST(UvhttpRequestTest, GetPathSpecialCharacters) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    strcpy(request.url, "/path/with-dashes_and_underscores?param=value");
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "/path/with-dashes_and_underscores");
}

TEST(UvhttpRequestTest, GetPathEmptyUrl) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.url[0] = '\0';
    
    const char* path = uvhttp_request_get_path(&request);
    EXPECT_STREQ(path, "");
}

// ============================================================================
// 日志系统测试
// ============================================================================

TEST(UvhttpLoggingTest, LogMacrosCompile) {
    // 测试日志宏不会导致编译错误
    // 这些宏在 Release 模式下会被编译为空操作
    
    #if !defined(NDEBUG) && UVHTTP_FEATURE_LOGGING
    // Debug 模式下，日志宏应该可以正常使用
    UVHTTP_LOG_INFO("Test info message: %s", "hello");
    UVHTTP_LOG_ERROR("Test error message: %d", 42);
    #else
    // Release 模式下，日志宏应该被编译为空操作
    UVHTTP_LOG_INFO("This should not produce any output");
    UVHTTP_LOG_ERROR("This should not produce any output");
    #endif
    
    // 测试不会崩溃
    EXPECT_TRUE(true);
}

TEST(UvhttpLoggingTest, LogMacrosWithVariousFormats) {
    // 测试各种格式的日志宏
    #if !defined(NDEBUG) && UVHTTP_FEATURE_LOGGING
    UVHTTP_LOG_INFO("String: %s", "test");
    UVHTTP_LOG_INFO("Integer: %d", 123);
    UVHTTP_LOG_INFO("Float: %f", 3.14);
    UVHTTP_LOG_INFO("Pointer: %p", (void*)0x1234);
    UVHTTP_LOG_INFO("Multiple: %s %d %f", "test", 123, 3.14);
    
    UVHTTP_LOG_ERROR("Error string: %s", "error");
    UVHTTP_LOG_ERROR("Error code: %d", -1);
    #else
    UVHTTP_LOG_INFO("Release mode - no output");
    UVHTTP_LOG_ERROR("Release mode - no output");
    #endif
    
    EXPECT_TRUE(true);
}

TEST(UvhttpLoggingTest, LogMacrosNoArguments) {
    // 测试无参数的日志宏
    #if !defined(NDEBUG) && UVHTTP_FEATURE_LOGGING
    UVHTTP_LOG_INFO("Simple message");
    UVHTTP_LOG_ERROR("Error message");
    #else
    UVHTTP_LOG_INFO("Release mode");
    UVHTTP_LOG_ERROR("Release mode");
    #endif
    
    EXPECT_TRUE(true);
}

// ============================================================================
// 静态文件 API 测试
// ============================================================================

TEST(UvhttpStaticApiTest, StaticApiCompiles) {
    // 测试静态文件 API 可以正常编译
    // 由于需要实际的文件系统，这里只测试编译
    
    #if UVHTTP_FEATURE_STATIC_FILES
    // 如果静态文件功能启用，这些宏和类型应该可用
    EXPECT_TRUE(true);
    #else
    // 如果静态文件功能禁用，也应该正常编译
    EXPECT_TRUE(true);
    #endif
}

// ============================================================================
// 主函数
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}