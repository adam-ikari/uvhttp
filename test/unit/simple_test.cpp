#include <gtest/gtest.h>
#include <string.h>
#include "../include/uvhttp_utils.h"
#include "../include/uvhttp_validation.h"

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

TEST(UvhttpValidationTest, ValidateMethodValid) {
    EXPECT_EQ(uvhttp_validate_http_method("GET"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("POST"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("PUT"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("DELETE"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("HEAD"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("OPTIONS"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("PATCH"), 1);
}

TEST(UvhttpValidationTest, ValidateMethodInvalid) {
    EXPECT_EQ(uvhttp_validate_http_method("INVALID"), 0);
    EXPECT_EQ(uvhttp_validate_http_method("get"), 0); // 小写
    EXPECT_EQ(uvhttp_validate_http_method(""), 0);
    EXPECT_EQ(uvhttp_validate_http_method(NULL), 0);
}

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
        EXPECT_EQ(uvhttp_validate_http_method("GET"), 1);
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
// 主函数
// ============================================================================

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}