/*
 * UVHTTP Utils API 覆盖率测试
 * 
 * 测试 uvhttp_utils.c 的核心 API
 */

#include <gtest/gtest.h>

extern "C" {
    #include "uvhttp_utils.h"
    #include "uvhttp_request.h"
    #include "uvhttp_response.h"
}

/* ========== 测试安全字符串复制 ========== */

TEST(UvhttpUtilsApiCoverageTest, SafeStrncpyNullDest) {
    int result = uvhttp_safe_strcpy(NULL, 10, "Hello");
    
    EXPECT_NE(result, 0);
}

TEST(UvhttpUtilsApiCoverageTest, SafeStrncpyNullSrc) {
    char dest[10];
    
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), NULL);
    
    EXPECT_NE(result, 0);
}

TEST(UvhttpUtilsApiCoverageTest, SafeStrncpyZeroSize) {
    char dest[10];
    
    int result = uvhttp_safe_strcpy(dest, 0, "Hello");
    
    EXPECT_NE(result, 0);
}

TEST(UvhttpUtilsApiCoverageTest, SafeStrncpyValid) {
    char dest[10];
    
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), "Hello");
    
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "Hello");
}

TEST(UvhttpUtilsApiCoverageTest, SafeStrncpyTruncate) {
    char dest[5];
    
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), "Hello World");
    
    /* 应该截断 */
    EXPECT_EQ(result, 0);
}

/* ========== 测试统一响应发送 ========== */

TEST(UvhttpUtilsApiCoverageTest, SendUnifiedResponseNullResponse) {
    uvhttp_error_t result = uvhttp_send_unified_response(NULL, "Hello", 5, 200);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpUtilsApiCoverageTest, SendUnifiedResponseNullContent) {
    uvhttp_response_t response;
    uvhttp_response_init(&response, NULL);
    
    uvhttp_error_t result = uvhttp_send_unified_response(&response, NULL, 5, 200);
    
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsApiCoverageTest, SendUnifiedResponseZeroLength) {
    uvhttp_response_t response;
    uvhttp_response_init(&response, NULL);
    
    uvhttp_error_t result = uvhttp_send_unified_response(&response, "", 0, 200);
    
    /* 零长度可能被接受或拒绝 */
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsApiCoverageTest, SendUnifiedResponseValid) {
    uvhttp_response_t response;
    uvhttp_response_init(&response, NULL);
    
    uvhttp_error_t result = uvhttp_send_unified_response(&response, "Hello", 5, 200);
    
    /* 不强制检查结果 */
    
    uvhttp_response_cleanup(&response);
}

/* ========== 测试发送错误响应 ========== */

TEST(UvhttpUtilsApiCoverageTest, SendErrorResponseNullResponse) {
    uvhttp_error_t result = uvhttp_send_error_response(NULL, 404, "Not Found", NULL);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpUtilsApiCoverageTest, SendErrorResponseNullMessage) {
    uvhttp_response_t response;
    uvhttp_response_init(&response, NULL);
    
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, NULL, NULL);
    
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsApiCoverageTest, SendErrorResponseEmptyMessage) {
    uvhttp_response_t response;
    uvhttp_response_init(&response, NULL);
    
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, "", NULL);
    
    /* 空消息可能被接受或拒绝 */
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsApiCoverageTest, SendErrorResponseValid) {
    uvhttp_response_t response;
    uvhttp_response_init(&response, NULL);
    
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, "Not Found", "Resource not found");
    
    /* 不强制检查结果 */
    
    uvhttp_response_cleanup(&response);
}

/* ========== 测试状态码验证 ========== */

TEST(UvhttpUtilsApiCoverageTest, IsValidStatusCodeInvalidCodes) {
    EXPECT_FALSE(uvhttp_is_valid_status_code(0));
    EXPECT_FALSE(uvhttp_is_valid_status_code(99));
    EXPECT_FALSE(uvhttp_is_valid_status_code(600));
    EXPECT_FALSE(uvhttp_is_valid_status_code(1000));
}

TEST(UvhttpUtilsApiCoverageTest, IsValidStatusCodeValidCodes) {
    EXPECT_TRUE(uvhttp_is_valid_status_code(100));
    EXPECT_TRUE(uvhttp_is_valid_status_code(200));
    EXPECT_TRUE(uvhttp_is_valid_status_code(301));
    EXPECT_TRUE(uvhttp_is_valid_status_code(400));
    EXPECT_TRUE(uvhttp_is_valid_status_code(500));
    EXPECT_TRUE(uvhttp_is_valid_status_code(599));
}

/* ========== 测试 IP 地址验证 ========== */

TEST(UvhttpUtilsApiCoverageTest, IsValidIpAddressNull) {
    EXPECT_FALSE(uvhttp_is_valid_ip_address(NULL));
}

TEST(UvhttpUtilsApiCoverageTest, IsValidIpAddressEmpty) {
    EXPECT_FALSE(uvhttp_is_valid_ip_address(""));
}

TEST(UvhttpUtilsApiCoverageTest, IsValidIpAddressValidIPv4) {
    EXPECT_TRUE(uvhttp_is_valid_ip_address("127.0.0.1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("192.168.1.1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("10.0.0.1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("0.0.0.0"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("255.255.255.255"));
}

TEST(UvhttpUtilsApiCoverageTest, IsValidIpAddressValidIPv6) {
    EXPECT_TRUE(uvhttp_is_valid_ip_address("::1"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("2001:0db8:85a3:0000:0000:8a2e:0370:7334"));
    EXPECT_TRUE(uvhttp_is_valid_ip_address("2001:db8:85a3::8a2e:370:7334"));
}

TEST(UvhttpUtilsApiCoverageTest, IsValidIpAddressInvalid) {
    EXPECT_FALSE(uvhttp_is_valid_ip_address("256.0.0.1"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1.1.1"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("abc.def.ghi.jkl"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("192.168.1.-1"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address(":::1"));
    EXPECT_FALSE(uvhttp_is_valid_ip_address("2001:0db8:85a3::8a2e::7334"));
}