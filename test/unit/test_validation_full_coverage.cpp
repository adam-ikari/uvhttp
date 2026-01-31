/* uvhttp_validation.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_validation.h"
#include "uvhttp_constants.h"
#include <string.h>

// ============================================================================
// 测试用例
// ============================================================================

TEST(UvhttpValidationFullCoverageTest, ValidateStringLengthNull) {
    EXPECT_EQ(uvhttp_validate_string_length(nullptr, 1, 100), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateStringLengthValid) {
    EXPECT_EQ(uvhttp_validate_string_length("hello", 1, 100), 1);
    EXPECT_EQ(uvhttp_validate_string_length("hello", 5, 5), 1);
    EXPECT_EQ(uvhttp_validate_string_length("hello", 1, 10), 1);
}

TEST(UvhttpValidationFullCoverageTest, ValidateStringLengthTooShort) {
    EXPECT_EQ(uvhttp_validate_string_length("hi", 5, 100), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateStringLengthTooLong) {
    EXPECT_EQ(uvhttp_validate_string_length("hello world", 1, 5), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateStringLengthEmpty) {
    EXPECT_EQ(uvhttp_validate_string_length("", 0, 10), 1);
    EXPECT_EQ(uvhttp_validate_string_length("", 1, 10), 0);
}

/* uvhttp_validate_http_method 已删除 - 使用 uvhttp_method_from_string 替代 */

TEST(UvhttpValidationFullCoverageTest, ValidateUrlPathNull) {
    EXPECT_EQ(uvhttp_validate_url_path(nullptr), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateUrlPathValid) {
    EXPECT_EQ(uvhttp_validate_url_path("/"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/index.html"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/api/users"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/api/users?id=1"), 1);
}

TEST(UvhttpValidationFullCoverageTest, ValidateUrlPathInvalid) {
    EXPECT_EQ(uvhttp_validate_url_path(""), 0);
    EXPECT_EQ(uvhttp_validate_url_path("../etc/passwd"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/path/../etc"), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateHeaderNameNull) {
    EXPECT_EQ(uvhttp_validate_header_name(nullptr), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateHeaderNameValid) {
    EXPECT_EQ(uvhttp_validate_header_name("Content-Type"), 1);
    EXPECT_EQ(uvhttp_validate_header_name("Accept"), 1);
    EXPECT_EQ(uvhttp_validate_header_name("X-Custom-Header"), 1);
}

TEST(UvhttpValidationFullCoverageTest, ValidateHeaderNameInvalid) {
    EXPECT_EQ(uvhttp_validate_header_name(""), 0);
    EXPECT_EQ(uvhttp_validate_header_name("Content Type"), 0);
    EXPECT_EQ(uvhttp_validate_header_name("Content:"), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateHeaderValueSafeNull) {
    EXPECT_EQ(uvhttp_validate_header_value_safe(nullptr), 0);
}

TEST(UvhttpValidationFullCoverageTest, ValidateHeaderValueSafeValid) {
    EXPECT_EQ(uvhttp_validate_header_value_safe("text/html"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("application/json"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("utf-8"), 1);
}

TEST(UvhttpValidationFullCoverageTest, ValidateHeaderValueSafeInvalid) {
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\r\n"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x01"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x7F"), 0);
}

/* uvhttp_validate_port 已删除 - 完全未使用 */
/* uvhttp_validate_content_length 已删除 - 完全未使用 */

TEST(UvhttpValidationFullCoverageTest, ValidateQueryStringNull) {
    EXPECT_EQ(uvhttp_validate_query_string(nullptr), 1);
}

TEST(UvhttpValidationFullCoverageTest, ValidateQueryStringValid) {
    EXPECT_EQ(uvhttp_validate_query_string(""), 1);
    EXPECT_EQ(uvhttp_validate_query_string("id=1"), 1);
    EXPECT_EQ(uvhttp_validate_query_string("id=1&name=test"), 1);
}

TEST(UvhttpValidationFullCoverageTest, ValidateQueryStringInvalid) {
    EXPECT_EQ(uvhttp_validate_query_string("id=1<script>alert('xss')</script>"), 0);
}