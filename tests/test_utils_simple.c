#include "../deps/googletest/gtest_fixed.h"
#include "../include/uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>

TEST(UtilsTest, SafeStrncpy) {
    char dest[100];
    
    // 正常复制
    EXPECT_EQ(safe_strncpy(dest, "hello", sizeof(dest)), 0);
    EXPECT_STREQ(dest, "hello");
    
    // 空指针检查
    EXPECT_EQ(safe_strncpy(NULL, "hello", sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, NULL, sizeof(dest)), -1);
    EXPECT_EQ(safe_strncpy(dest, "hello", 0), -1);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(UtilsTest, ValidateUrl) {
    // 有效URL
    EXPECT_EQ(validate_url("http://example.com", 18), 0);
    EXPECT_EQ(validate_url("/api/v1/users", 14), 0);
    
    // 无效URL
    EXPECT_EQ(validate_url("", 0), -1);
    EXPECT_EQ(validate_url(NULL, 0), -1);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST(UtilsTest, ValidateMethod) {
    // 有效方法
    EXPECT_EQ(validate_method("GET", 3), 0);
    EXPECT_EQ(validate_method("POST", 4), 0);
    
    // 无效方法
    EXPECT_EQ(validate_method(NULL, 3), -1);
    EXPECT_EQ(validate_method("", 0), -1);
    
TEST_CLEANUP_LABEL:
    return;
}

RUN_ALL_TESTS()