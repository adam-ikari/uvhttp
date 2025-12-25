/**
 * @file test_utils.c
 * @brief 工具函数测试
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_utils.h"
#include "../include/uvhttp_validation.h"
#include "../include/uvhttp_constants.h"

TEST_FUNC(safe_strcpy_normal) {
    char dest[10];
    int result = uvhttp_safe_strncpy(dest, "hello", sizeof(dest));
    
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_STREQ("hello", dest);
    
    return 0;
}

TEST_FUNC(safe_strcpy_overflow) {
    char dest[5];
    int result = uvhttp_safe_strncpy(dest, "123456789", sizeof(dest));
    
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_STREQ("1234", dest);
    
    return 0;
}

TEST_FUNC(safe_strcpy_empty) {
    char dest[10];
    int result = uvhttp_safe_strncpy(dest, "", sizeof(dest));
    
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_STREQ("", dest);
    
    return 0;
}

TEST_FUNC(safe_strcpy_null) {
    char dest[10];
    int result = uvhttp_safe_strncpy(dest, NULL, sizeof(dest));
    
    TEST_ASSERT_EQ(-1, result);
    
    return 0;
}

TEST_FUNC(validate_header_value_valid) {
    int result = uvhttp_validate_header_value_safe("text/plain");
    TEST_ASSERT_EQ(1, result);
    
    result = uvhttp_validate_header_value_safe("example.com");
    TEST_ASSERT_EQ(1, result);
    
    return 0;
}

TEST_FUNC(validate_header_value_invalid) {
    int result = uvhttp_validate_header_value_safe(NULL);
    TEST_ASSERT_EQ(0, result);
    
    result = uvhttp_validate_header_value_safe("");
    TEST_ASSERT_EQ(1, result); /* 空字符串是有效的 */
    
    return 0;
}

TEST_FUNC(validate_url_valid) {
    int result = uvhttp_validate_url_path("/path");
    TEST_ASSERT_EQ(1, result);
    
    result = uvhttp_validate_url_path("/");
    TEST_ASSERT_EQ(1, result);
    
    return 0;
}

TEST_FUNC(validate_url_invalid) {
    int result = uvhttp_validate_url_path(NULL);
    TEST_ASSERT_EQ(0, result);
    
    result = uvhttp_validate_url_path("");
    TEST_ASSERT_EQ(0, result);
    
    return 0;
}

TEST_FUNC(validate_method_valid) {
    int result = uvhttp_validate_http_method("GET");
    TEST_ASSERT_EQ(1, result);
    
    result = uvhttp_validate_http_method("POST");
    TEST_ASSERT_EQ(1, result);
    
    return 0;
}

TEST_FUNC(validate_method_invalid) {
    int result = uvhttp_validate_http_method(NULL);
    TEST_ASSERT_EQ(0, result);
    
    result = uvhttp_validate_http_method("");
    TEST_ASSERT_EQ(0, result);
    
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