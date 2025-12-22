#include <gtest/gtest.h>
#include "../include/uvhttp_utils.h"

// 测试safe_strncpy函数
TEST(UtilsTest, SafeStrncpy) {
    char dest[10];
    
    // 正常情况
    EXPECT_EQ(safe_strncpy(dest, "hello", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "hello");
    
    // 边界情况
    EXPECT_EQ(safe_strncpy(dest, "123456789", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "123456789");
    
    // 溢出保护
    EXPECT_EQ(safe_strncpy(dest, "1234567890", sizeof(dest)), 0);
    EXPECT_TRUE(strlen(dest) < sizeof(dest));
    
    // 空指针检查
    EXPECT_EQ(safe_strncpy(NULL, "test", sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, NULL, sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, "test", 0), -1);
}

// 测试validate_url函数
TEST(UtilsTest, ValidateUrl) {
    // 有效URL
    EXPECT_EQ(validate_url("/", 1), 0);
    EXPECT_EQ(validate_url("/api/users", 10), 0);
    EXPECT_EQ(validate_url("/api/v1/users/123", 17), 0);
    EXPECT_EQ(validate_url("/test?param=value", 17), 0);
    
    // 无效URL（包含NULL字节）
    EXPECT_EQ(validate_url("/test\x00", 6), -1);
    
    // 过长URL
    char long_url[3000];
    memset(long_url, 'a', sizeof(long_url) - 1);
    long_url[sizeof(long_url) - 1] = '\0';
    EXPECT_EQ(validate_url(long_url, strlen(long_url)), -1);
    
    // 空指针检查
    EXPECT_EQ(validate_url(NULL, 5), -1);
    EXPECT_EQ(validate_url("/test", 0), -1);
}

// 测试validate_header_value函数
TEST(UtilsTest, ValidateHeaderValue) {
    // 有效头部值
    EXPECT_EQ(validate_header_value("application/json", 16), 0);
    EXPECT_EQ(validate_header_value("text/plain", 10), 0);
    EXPECT_EQ(validate_header_value("Mozilla/5.0", 11), 0);
    
    // 无效头部值（包含控制字符）
    EXPECT_EQ(validate_header_value("value\x01", 6), -1);
    EXPECT_EQ(validate_header_value("value\x1F", 6), -1);
    EXPECT_EQ(validate_header_value("value\x7F", 6), -1);
    
    // 空指针检查
    EXPECT_EQ(validate_header_value(NULL, 5), -1);
    EXPECT_EQ(validate_header_value("test", 0), -1);
}

// 测试validate_method函数
TEST(UtilsTest, ValidateMethod) {
    // 有效HTTP方法
    EXPECT_EQ(validate_method("GET", 3), 0);
    EXPECT_EQ(validate_method("POST", 4), 0);
    EXPECT_EQ(validate_method("PUT", 3), 0);
    EXPECT_EQ(validate_method("DELETE", 6), 0);
    EXPECT_EQ(validate_method("HEAD", 4), 0);
    EXPECT_EQ(validate_method("OPTIONS", 7), 0);
    EXPECT_EQ(validate_method("PATCH", 5), 0);
    
    // 无效HTTP方法
    EXPECT_EQ(validate_method("INVALID", 7), -1);
    EXPECT_EQ(validate_method("get", 3), -1); // 小写
    EXPECT_EQ(validate_method("", 0), -1);
    
    // 空指针检查
    EXPECT_EQ(validate_method(NULL, 3), -1);
}

// 测试边界条件
TEST(UtilsTest, EdgeCases) {
    char dest[1]; // 最小缓冲区
    
    // 最小缓冲区测试
    EXPECT_EQ(safe_strncpy(dest, "a", sizeof(dest)), 0);
    EXPECT_EQ(dest[0], 'a');
    
    // 空字符串测试
    EXPECT_EQ(safe_strncpy(dest, "", sizeof(dest)), 0);
    EXPECT_EQ(dest[0], '\0');
    
    // 单字符URL
    EXPECT_EQ(validate_url("/", 1), 0);
    
    // 单字符头部值
    EXPECT_EQ(validate_header_value("a", 1), 0);
}

// 测试性能相关边界
TEST(UtilsTest, PerformanceBoundaries) {
    char dest[256];
    const char* long_string = "This is a very long string that should still be handled efficiently without causing any buffer overflows or performance issues";
    
    // 长字符串处理
    EXPECT_EQ(safe_strncpy(dest, long_string, sizeof(dest)), 0);
    EXPECT_TRUE(strlen(dest) < sizeof(dest));
    EXPECT_TRUE(strncmp(dest, long_string, strlen(dest)) == 0);
    
    // 长URL验证
    const char* long_url = "/api/v1/very/long/path/with/many/segments/that/should/still/be/validated/properly/without/causing/issues";
    EXPECT_EQ(validate_url(long_url, strlen(long_url)), 0);
}

// 测试错误恢复
TEST(UtilsTest, ErrorRecovery) {
    char dest[10];
    
    // 连续错误处理
    EXPECT_EQ(safe_strncpy(NULL, "test", sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, NULL, sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, "test", 0), -1);
    
    // 确保错误后系统仍然正常工作
    EXPECT_EQ(safe_strncpy(dest, "ok", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "ok");
}

// 测试并发安全性（模拟）
TEST(UtilsTest, ThreadSafety) {
    char dest1[10], dest2[10];
    
    // 模拟并发调用
    EXPECT_EQ(safe_strncpy(dest1, "test1", sizeof(dest1)), 0);
    EXPECT_EQ(safe_strncpy(dest2, "test2", sizeof(dest2)), 0);
    
    EXPECT_STREQ(dest1, "test1");
    EXPECT_STREQ(dest2, "test2");
    EXPECT_NE(strcmp(dest1, dest2), 0);
}