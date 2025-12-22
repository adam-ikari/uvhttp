/**
 * @file test_utils.c
 * @brief 工具函数测试
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_utils.h"
#include "../include/uvhttp_constants.h"

TEST_FUNC(safe_strcpy_normal) {
    char dest[10];
    int result = safe_strncpy(dest, "hello", sizeof(dest));
    
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_STREQ("hello", dest);
    
    return 0;
}

TEST_FUNC(safe_strcpy_overflow) {
    char dest[5];
    int result = safe_strncpy(dest, "123456789", sizeof(dest));
    
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_STREQ("1234", dest);
    
    return 0;
}

TEST_FUNC(safe_strcpy_empty) {
    char dest[10];
    int result = safe_strncpy(dest, "", sizeof(dest));
    
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_STREQ("", dest);
    
    return 0;
}

TEST_FUNC(safe_strcpy_null) {
    char dest[10];
    int result = safe_strncpy(dest, NULL, sizeof(dest));
    
    TEST_ASSERT_EQ(-1, result);
    
    return 0;
}

TEST_FUNC(validate_header_value_valid) {
    int result = validate_header_value("text/plain", 10);
    TEST_ASSERT_EQ(0, result);
    
    result = validate_header_value("example.com", 11);
    TEST_ASSERT_EQ(0, result);
    
    return 0;
}

TEST_FUNC(validate_header_value_invalid) {
    int result = validate_header_value(NULL, 0);
    TEST_ASSERT_EQ(-1, result);
    
    result = validate_header_value("", 0);
    TEST_ASSERT_EQ(-1, result);
    
    return 0;
}

TEST_FUNC(validate_url_valid) {
    int result = validate_url("/path", 5);
    TEST_ASSERT_EQ(0, result);
    
    result = validate_url("/", 1);
    TEST_ASSERT_EQ(0, result);
    
    return 0;
}

TEST_FUNC(validate_url_invalid) {
    int result = validate_url(NULL, 0);
    TEST_ASSERT_EQ(-1, result);
    
    result = validate_url("", 0);
    TEST_ASSERT_EQ(-1, result);
    
    return 0;
}

TEST_FUNC(validate_method_valid) {
    int result = validate_method("GET", 3);
    TEST_ASSERT_EQ(0, result);
    
    result = validate_method("POST", 4);
    TEST_ASSERT_EQ(0, result);
    
    return 0;
}

TEST_FUNC(validate_method_invalid) {
    int result = validate_method(NULL, 0);
    TEST_ASSERT_EQ(-1, result);
    
    result = validate_method("", 0);
    TEST_ASSERT_EQ(-1, result);
    
    return 0;
}

TEST_SUITE(utils) {
    TEST_CASE(safe_strcpy_normal);
    TEST_CASE(safe_strcpy_overflow);
    TEST_CASE(safe_strcpy_empty);
    TEST_CASE(safe_strcpy_null);
    TEST_CASE(validate_header_value_valid);
    TEST_CASE(validate_header_value_invalid);
    TEST_CASE(validate_url_valid);
    TEST_CASE(validate_url_invalid);
    TEST_CASE(validate_method_valid);
    TEST_CASE(validate_method_invalid);
    
    END_TEST_SUITE();
}