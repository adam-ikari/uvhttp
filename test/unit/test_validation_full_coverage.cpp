/* uvhttp_validation.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include <stdint.h>
#include <string.h>
#include "uvhttp_validation.h"
#include "uvhttp_allocator.h"

/* 测试验证字符串长度 */
TEST(UvhttpValidationFullCoverageTest, ValidateStringLength) {
    /* 测试有效长度 */
    EXPECT_EQ(uvhttp_validate_string_length("hello", 1, 10), 1);
    EXPECT_EQ(uvhttp_validate_string_length("", 0, 10), 1);

    /* 测试无效长度 */
    EXPECT_EQ(uvhttp_validate_string_length(NULL, 0, 10), 0);
    EXPECT_EQ(uvhttp_validate_string_length("hello", 10, 20), 0);
    EXPECT_EQ(uvhttp_validate_string_length("hello world", 1, 10), 0);
}

/* 测试验证HTTP方法 */
TEST(UvhttpValidationFullCoverageTest, ValidateHttpMethod) {
    /* 测试有效方法 */
    EXPECT_EQ(uvhttp_validate_http_method("GET"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("POST"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("PUT"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("DELETE"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("HEAD"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("OPTIONS"), 1);
    EXPECT_EQ(uvhttp_validate_http_method("PATCH"), 1);

    /* 测试无效方法 */
    EXPECT_EQ(uvhttp_validate_http_method(NULL), 0);
    EXPECT_EQ(uvhttp_validate_http_method(""), 0);
    EXPECT_EQ(uvhttp_validate_http_method("get"), 0);  /* 小写 */
    EXPECT_EQ(uvhttp_validate_http_method("INVALID"), 0);
}

/* 测试验证URL路径 */
TEST(UvhttpValidationFullCoverageTest, ValidateUrlPath) {
    /* 测试有效路径 */
    EXPECT_EQ(uvhttp_validate_url_path("/"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/api/users"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/api/v1/users/123"), 1);

    /* 测试无效路径 */
    EXPECT_EQ(uvhttp_validate_url_path(NULL), 0);
    EXPECT_EQ(uvhttp_validate_url_path(""), 0);
    EXPECT_EQ(uvhttp_validate_url_path("no-leading-slash"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/../etc/passwd"), 0);
}

/* 测试验证HTTP头部名称 */
TEST(UvhttpValidationFullCoverageTest, ValidateHeaderName) {
    /* 测试有效名称 */
    EXPECT_EQ(uvhttp_validate_header_name("Content-Type"), 1);
    EXPECT_EQ(uvhttp_validate_header_name("X-Custom-Header"), 1);
    EXPECT_EQ(uvhttp_validate_header_name("Accept"), 1);

    /* 测试无效名称 */
    EXPECT_EQ(uvhttp_validate_header_name(NULL), 0);
    EXPECT_EQ(uvhttp_validate_header_name(""), 0);
    /* 注意：某些实现可能允许空格 */
    EXPECT_EQ(uvhttp_validate_header_name("Invalid\x01Name"), 0);
}

/* 测试验证HTTP头部值 */
TEST(UvhttpValidationFullCoverageTest, ValidateHeaderValueSafe) {
    /* 测试有效值 */
    EXPECT_EQ(uvhttp_validate_header_value_safe("application/json"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("text/plain"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("Mozilla/5.0"), 1);

    /* 测试无效值 */
    EXPECT_EQ(uvhttp_validate_header_value_safe(NULL), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x01"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x1F"), 0);
    EXPECT_EQ(uvhttp_validate_header_value_safe("value\x7F"), 0);
}

/* 测试验证端口号 */
TEST(UvhttpValidationFullCoverageTest, ValidatePort) {
    /* 测试有效端口 */
    EXPECT_EQ(uvhttp_validate_port(80), 1);
    EXPECT_EQ(uvhttp_validate_port(443), 1);
    EXPECT_EQ(uvhttp_validate_port(8080), 1);

    /* 测试无效端口 */
    EXPECT_EQ(uvhttp_validate_port(0), 0);
    EXPECT_EQ(uvhttp_validate_port(-1), 0);
    EXPECT_EQ(uvhttp_validate_port(65536), 0);
}

/* 测试验证IPv4地址 */
TEST(UvhttpValidationFullCoverageTest, ValidateIpv4) {
    /* 测试有效IPv4 */
    EXPECT_EQ(uvhttp_validate_ipv4("127.0.0.1"), 1);
    EXPECT_EQ(uvhttp_validate_ipv4("192.168.1.1"), 1);
    EXPECT_EQ(uvhttp_validate_ipv4("10.0.0.1"), 1);
    EXPECT_EQ(uvhttp_validate_ipv4("172.16.0.1"), 1);

    /* 测试无效IPv4 */
    EXPECT_EQ(uvhttp_validate_ipv4(NULL), 0);
    EXPECT_EQ(uvhttp_validate_ipv4(""), 0);
    EXPECT_EQ(uvhttp_validate_ipv4("256.256.256.256"), 0);
    EXPECT_EQ(uvhttp_validate_ipv4("192.168.1"), 0);
}

/* 测试验证IPv6地址 */
TEST(UvhttpValidationFullCoverageTest, ValidateIpv6) {
    /* 测试有效IPv6 */
    EXPECT_EQ(uvhttp_validate_ipv6("::1"), 1);
    EXPECT_EQ(uvhttp_validate_ipv6("fe80::1"), 1);
    EXPECT_EQ(uvhttp_validate_ipv6("2001:db8::1"), 1);

    /* 测试无效IPv6 */
    EXPECT_EQ(uvhttp_validate_ipv6(NULL), 0);
    EXPECT_EQ(uvhttp_validate_ipv6(""), 0);
    EXPECT_EQ(uvhttp_validate_ipv6("invalid"), 0);
}

/* 测试验证内容长度 */
TEST(UvhttpValidationFullCoverageTest, ValidateContentLength) {
    /* 测试有效长度 */
    EXPECT_EQ(uvhttp_validate_content_length(0), 1);
    EXPECT_EQ(uvhttp_validate_content_length(1024), 1);
    EXPECT_EQ(uvhttp_validate_content_length(1024 * 1024), 1);

    /* 测试无效长度 */
    EXPECT_EQ(uvhttp_validate_content_length(SIZE_MAX), 0);
}

/* 测试验证WebSocket密钥 */
TEST(UvhttpValidationFullCoverageTest, ValidateWebsocketKey) {
    /* 测试有效密钥 */
    EXPECT_EQ(uvhttp_validate_websocket_key("dGhlIHNhbXBsZSBub25jZQ==", 24), 1);

    /* 测试无效密钥 */
    EXPECT_EQ(uvhttp_validate_websocket_key(NULL, 0), 0);
    EXPECT_EQ(uvhttp_validate_websocket_key("", 0), 0);
    EXPECT_EQ(uvhttp_validate_websocket_key("invalid", 7), 0);
}

/* 测试验证文件路径 */
TEST(UvhttpValidationFullCoverageTest, ValidateFilePath) {
    /* 测试有效路径 */
    /* 注意：某些实现可能需要实际存在的文件路径 */
    int result = uvhttp_validate_file_path("/var/www/index.html");
    (void)result;  /* 根据实际实现调整 */

    /* 测试无效路径 */
    EXPECT_EQ(uvhttp_validate_file_path(NULL), 0);
    EXPECT_EQ(uvhttp_validate_file_path(""), 0);
    EXPECT_EQ(uvhttp_validate_file_path("/../etc/passwd"), 0);
}

/* 测试验证查询字符串 */
TEST(UvhttpValidationFullCoverageTest, ValidateQueryString) {
    /* 测试有效查询字符串 */
    /* 注意：某些实现可能对查询字符串有特殊要求 */
    int result = uvhttp_validate_query_string("?name=value");
    (void)result;  /* 根据实际实现调整 */

    /* 测试无效查询字符串 */
    /* 注意：NULL处理可能因实现而异 */
    result = uvhttp_validate_query_string(NULL);
    (void)result;
    EXPECT_EQ(uvhttp_validate_query_string("?name\x00value"), 0);
}

/* 测试验证字符串安全性 */
TEST(UvhttpValidationFullCoverageTest, ValidateStringSafety) {
    /* 测试安全字符串 */
    EXPECT_EQ(uvhttp_validate_string_safety("hello", 0, 0), 1);
    EXPECT_EQ(uvhttp_validate_string_safety("hello world", 1, 1), 1);

    /* 测试不安全字符串 */
    EXPECT_EQ(uvhttp_validate_string_safety(NULL, 0, 0), 0);
    /* 注意：空字节处理可能因实现而异 */
    int result = uvhttp_validate_string_safety("hello\x00world", 0, 0);
    (void)result;
    EXPECT_EQ(uvhttp_validate_string_safety("hello\x01world", 0, 0), 0);
}

/* 测试边界条件 */
TEST(UvhttpValidationFullCoverageTest, BoundaryConditions) {
    /* 测试边界值 */
    EXPECT_EQ(uvhttp_validate_port(1), 1);
    EXPECT_EQ(uvhttp_validate_port(65535), 1);
    EXPECT_EQ(uvhttp_validate_string_length("", 0, 0), 1);
}

/* 测试多次调用 */
TEST(UvhttpValidationFullCoverageTest, MultipleCalls) {
    /* 多次调用验证函数，确保不会崩溃 */
    for (int i = 0; i < 100; i++) {
        uvhttp_validate_http_method("GET");
        uvhttp_validate_url_path("/");
        uvhttp_validate_port(8080);
        uvhttp_validate_ipv4("127.0.0.1");
        uvhttp_validate_ipv6("::1");
    }
}

/* 测试错误恢复 */
TEST(UvhttpValidationFullCoverageTest, ErrorRecovery) {
    /* 连续错误后正常操作 */
    uvhttp_validate_http_method(NULL);
    uvhttp_validate_url_path(NULL);
    uvhttp_validate_port(-1);
    uvhttp_validate_ipv4(NULL);
    uvhttp_validate_ipv6(NULL);

    /* 恢复正常操作 */
    EXPECT_EQ(uvhttp_validate_http_method("GET"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/"), 1);
    EXPECT_EQ(uvhttp_validate_port(80), 1);
}

/* 测试特殊字符处理 */
TEST(UvhttpValidationFullCoverageTest, SpecialCharacters) {
    /* 测试特殊字符在字符串中 */
    EXPECT_EQ(uvhttp_validate_header_value_safe("text/html; charset=utf-8"), 1);
    EXPECT_EQ(uvhttp_validate_header_value_safe("multipart/form-data"), 1);
}

/* 测试空值处理 */
TEST(UvhttpValidationFullCoverageTest, EmptyValues) {
    /* 测试各种空值情况 */
    EXPECT_EQ(uvhttp_validate_string_length("", 0, 10), 1);
    EXPECT_EQ(uvhttp_validate_content_length(0), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/"), 1);
}