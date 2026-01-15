/* UVHTTP 错误处理辅助函数完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_constants.h"

TEST(UvhttpErrorHelpersFullCoverageTest, CleanupConnectionNull) {
    uvhttp_cleanup_connection(nullptr, nullptr);
    uvhttp_cleanup_connection(nullptr, "error message");
}

TEST(UvhttpErrorHelpersFullCoverageTest, HandleMemoryFailureNull) {
    uvhttp_handle_memory_failure(nullptr, nullptr, nullptr);
    uvhttp_handle_memory_failure("test context", nullptr, nullptr);
    uvhttp_handle_memory_failure(nullptr, (void (*)(void*))0x1234, nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, HandleWriteErrorNull) {
    uvhttp_handle_write_error(nullptr, 0, "test context");
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageNull) {
    char buffer[256];
    
    EXPECT_NE(uvhttp_sanitize_error_message(nullptr, buffer, sizeof(buffer)), 0);
    EXPECT_NE(uvhttp_sanitize_error_message("message", nullptr, sizeof(buffer)), 0);
    EXPECT_NE(uvhttp_sanitize_error_message("message", buffer, 0), 0);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageNormal) {
    char buffer[256];
    
    EXPECT_EQ(uvhttp_sanitize_error_message("normal error message", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "normal error message");
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageSensitive) {
    char buffer[256];
    
    EXPECT_EQ(uvhttp_sanitize_error_message("password: secret123", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Sensitive information hidden");
    
    EXPECT_EQ(uvhttp_sanitize_error_message("token: abc123", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Sensitive information hidden");
    
    EXPECT_EQ(uvhttp_sanitize_error_message("secret key", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Sensitive information hidden");
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageLong) {
    char buffer[20];
    const char* long_message = "This is a very long error message that exceeds the buffer size";
    
    EXPECT_EQ(uvhttp_sanitize_error_message(long_message, buffer, sizeof(buffer)), 0);
    EXPECT_LT(strlen(buffer), sizeof(buffer));
    EXPECT_NE(strstr(buffer, "..."), nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageCase) {
    char buffer[256];
    
    EXPECT_EQ(uvhttp_sanitize_error_message("PASSWORD: secret", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Sensitive information hidden");
    
    EXPECT_EQ(uvhttp_sanitize_error_message("Password: secret", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "Sensitive information hidden");
}

TEST(UvhttpErrorHelpersFullCoverageTest, SafeFreeNull) {
    uvhttp_safe_free(nullptr, nullptr);
    
    void* ptr = nullptr;
    uvhttp_safe_free(&ptr, nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SafeFreeNormal) {
    void* ptr = malloc(100);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_safe_free(&ptr, nullptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SafeFreeCustom) {
    static int custom_free_called = 0;
    
    auto custom_free = [](void* data) {
        custom_free_called = 1;
        free(data);
    };
    
    void* ptr = malloc(100);
    ASSERT_NE(ptr, nullptr);
    
    custom_free_called = 0;
    uvhttp_safe_free(&ptr, custom_free);
    EXPECT_EQ(ptr, nullptr);
    EXPECT_EQ(custom_free_called, 1);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SafeFreeMultiple) {
    void* ptr = malloc(100);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_safe_free(&ptr, nullptr);
    EXPECT_EQ(ptr, nullptr);
    
    uvhttp_safe_free(&ptr, nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, HandleMemoryFailureNoCleanup) {
    uvhttp_handle_memory_failure("test context", nullptr, (void*)0x1234);
}

TEST(UvhttpErrorHelpersFullCoverageTest, HandleMemoryFailureNoData) {
    static int cleanup_called = 0;
    
    auto test_cleanup = [](void* data) {
        cleanup_called = 1;
        (void)data;
    };
    
    cleanup_called = 0;
    uvhttp_handle_memory_failure("test context", test_cleanup, nullptr);
    EXPECT_EQ(cleanup_called, 0);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageEmpty) {
    char buffer[256];
    
    EXPECT_EQ(uvhttp_sanitize_error_message("", buffer, sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "");
}

TEST(UvhttpErrorHelpersFullCoverageTest, SafeFreeNullFunc) {
    void* ptr = malloc(100);
    ASSERT_NE(ptr, nullptr);
    
    uvhttp_safe_free(&ptr, nullptr);
    EXPECT_EQ(ptr, nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SafeFreeNullPtrValue) {
    void* ptr = nullptr;
    
    uvhttp_safe_free(&ptr, (void (*)(void*))0x1234);
    EXPECT_EQ(ptr, nullptr);
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageKeywords) {
    char buffer[256];
    
    const char* keywords[] = {
        "password", "passwd", "secret", "key", "token",
        "auth", "credential", "private", "session"
    };
    
    for (int i = 0; i < 9; i++) {
        char test_msg[128];
        snprintf(test_msg, sizeof(test_msg), "%s: value123", keywords[i]);
        
        EXPECT_EQ(uvhttp_sanitize_error_message(test_msg, buffer, sizeof(buffer)), 0);
        EXPECT_STREQ(buffer, "Sensitive information hidden");
    }
}

TEST(UvhttpErrorHelpersFullCoverageTest, SanitizeErrorMessageBufferSizes) {
    char buffer1[1];
    char buffer2[10];
    char buffer3[100];
    
    EXPECT_EQ(uvhttp_sanitize_error_message("test", buffer1, sizeof(buffer1)), 0);
    
    EXPECT_EQ(uvhttp_sanitize_error_message("test message", buffer2, sizeof(buffer2)), 0);
    
    EXPECT_EQ(uvhttp_sanitize_error_message("normal message", buffer3, sizeof(buffer3)), 0);
}