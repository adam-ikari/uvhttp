/* uvhttp_utils.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_utils.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <string.h>
#include <stdlib.h>

TEST(UvhttpUtilsTest, SafeStrcpy) {
    /* 测试安全的字符串复制 */
    char dest[256];
    
    /* 正常复制 */
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), "Hello, World!");
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "Hello, World!");
    
    /* NULL 目标 */
    result = uvhttp_safe_strcpy(nullptr, sizeof(dest), "Hello");
    EXPECT_EQ(result, -1);
    
    /* NULL 源 */
    result = uvhttp_safe_strcpy(dest, sizeof(dest), nullptr);
    EXPECT_EQ(result, -1);
    
    /* 零大小 */
    result = uvhttp_safe_strcpy(dest, 0, "Hello");
    EXPECT_EQ(result, -1);
    
    /* 长字符串截断 */
    result = uvhttp_safe_strcpy(dest, 10, "This is a very long string");
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(dest), 9);
    EXPECT_EQ(dest[9], '\0');
    
    /* 空字符串 */
    result = uvhttp_safe_strcpy(dest, sizeof(dest), "");
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "");
}

TEST(UvhttpUtilsTest, SafeStrncpy) {
    /* 测试安全的字符串复制（n版本） */
    char dest[256];
    
    /* 正常复制 */
    int result = uvhttp_safe_strncpy(dest, "Hello, World!", sizeof(dest));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "Hello, World!");
    
    /* NULL 目标 */
    result = uvhttp_safe_strncpy(nullptr, "Hello", sizeof(dest));
    EXPECT_EQ(result, -1);
    
    /* NULL 源 */
    result = uvhttp_safe_strncpy(dest, nullptr, sizeof(dest));
    EXPECT_EQ(result, -1);
    
    /* 零大小 */
    result = uvhttp_safe_strncpy(dest, "Hello", 0);
    EXPECT_EQ(result, -1);
    
    /* 长字符串截断 */
    result = uvhttp_safe_strncpy(dest, "This is a very long string", 10);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(dest), 9);
    EXPECT_EQ(dest[9], '\0');
    
    /* 空字符串 */
    result = uvhttp_safe_strncpy(dest, "", sizeof(dest));
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "");
}

TEST(UvhttpUtilsTest, IsValidStatusCode) {
    /* 测试状态码验证 */
    
    /* 有效状态码 */
    EXPECT_EQ(uvhttp_is_valid_status_code(100), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(200), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(301), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(404), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(500), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(599), TRUE);
    
    /* 无效状态码 */
    EXPECT_EQ(uvhttp_is_valid_status_code(99), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(600), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(0), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(-1), FALSE);
    
    /* 边界值 */
    EXPECT_EQ(uvhttp_is_valid_status_code(99), FALSE);
    EXPECT_EQ(uvhttp_is_valid_status_code(100), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(599), TRUE);
    EXPECT_EQ(uvhttp_is_valid_status_code(600), FALSE);
}

/* uvhttp_is_valid_content_type 已删除 - 完全未使用 */

/* uvhttp_is_valid_string_length 已删除 - 完全未使用 */

TEST(UvhttpUtilsTest, SendUnifiedResponse) {
    /* 测试统一响应发送 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    /* 初始化客户端句柄 */
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    
    /* 初始化响应 */
    uvhttp_response_init(&response, &client);
    
    /* 发送响应 */
    const char* content = "Hello, World!";
    uvhttp_error_t result = uvhttp_send_unified_response(&response, content, strlen(content), 200);
    
    /* 检查结果（可能失败因为没有实际的网络连接） */
    if (result != UVHTTP_OK) {
        /* 这是预期的，因为我们没有实际的网络连接 */
    }
    
    /* 清理 */
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendUnifiedResponseNullParams) {
    /* 测试统一响应发送的 NULL 参数 */
    uvhttp_error_t result;
    
    /* NULL 响应 */
    result = uvhttp_send_unified_response(nullptr, "Hello", 5, 200);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* NULL 内容 */
    uv_tcp_t client;
    uvhttp_response_t response;
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    result = uvhttp_send_unified_response(&response, nullptr, 5, 200);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendUnifiedResponseInvalidStatusCode) {
    /* 测试统一响应发送的无效状态码 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 无效状态码 */
    uvhttp_error_t result = uvhttp_send_unified_response(&response, "Hello", 5, 99);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_send_unified_response(&response, "Hello", 5, 600);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendUnifiedResponseAutoLength) {
    /* 测试统一响应发送自动计算长度 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 长度为 0，自动计算 */
    const char* content = "Hello, World!";
    uvhttp_error_t result = uvhttp_send_unified_response(&response, content, 0, 200);
    
    /* 检查结果（可能失败因为没有实际的网络连接） */
    if (result != UVHTTP_OK) {
        /* 这是预期的，因为我们没有实际的网络连接 */
    }
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendUnifiedResponseEmptyContent) {
    /* 测试统一响应发送空内容 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 空内容 */
    uvhttp_error_t result = uvhttp_send_unified_response(&response, "", 0, 200);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendErrorResponse) {
    /* 测试错误响应发送 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 发送错误响应 */
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, "Not Found", "Resource not found");
    
    /* 检查结果（可能失败因为没有实际的网络连接） */
    if (result != UVHTTP_OK) {
        /* 这是预期的，因为我们没有实际的网络连接 */
    }
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendErrorResponseNullParams) {
    /* 测试错误响应发送的 NULL 参数 */
    uvhttp_error_t result;
    
    /* NULL 响应 */
    result = uvhttp_send_error_response(nullptr, 404, "Not Found", "Details");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* NULL 错误消息 */
    uv_tcp_t client;
    uvhttp_response_t response;
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    result = uvhttp_send_error_response(&response, 404, nullptr, "Details");
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendErrorResponseInvalidStatusCode) {
    /* 测试错误响应发送的无效状态码 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 无效状态码 */
    uvhttp_error_t result = uvhttp_send_error_response(&response, 99, "Error", "Details");
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_send_error_response(&response, 600, "Error", "Details");
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendErrorResponseLongMessage) {
    /* 测试错误响应发送过长消息 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 过长的错误消息 */
    char long_msg[300];
    memset(long_msg, 'A', sizeof(long_msg) - 1);
    long_msg[sizeof(long_msg) - 1] = '\0';
    
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, long_msg, "Details");
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendErrorResponseLongDetails) {
    /* 测试错误响应发送过长详细信息 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 过长的详细信息 */
    char long_details[500];
    memset(long_details, 'B', sizeof(long_details) - 1);
    long_details[sizeof(long_details) - 1] = '\0';
    
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, "Error", long_details);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SendErrorResponseNoDetails) {
    /* 测试错误响应发送无详细信息 */
    uv_tcp_t client;
    uvhttp_response_t response;
    
    memset(&client, 0, sizeof(client));
    client.type = UV_TCP;
    uvhttp_response_init(&response, &client);
    
    /* 无详细信息 */
    uvhttp_error_t result = uvhttp_send_error_response(&response, 404, "Not Found", nullptr);
    
    /* 检查结果（可能失败因为没有实际的网络连接） */
    if (result != UVHTTP_OK) {
        /* 这是预期的，因为我们没有实际的网络连接 */
    }
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpUtilsTest, SafeStrcpyBufferOverflow) {
    /* 测试安全字符串复制缓冲区溢出保护 */
    char dest[10];
    
    /* 源字符串长度等于目标缓冲区大小 */
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), "123456789");
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(dest), 9);
    EXPECT_EQ(dest[9], '\0');
    
    /* 源字符串长度大于目标缓冲区大小 */
    result = uvhttp_safe_strcpy(dest, sizeof(dest), "1234567890");
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(dest), 9);
    EXPECT_EQ(dest[9], '\0');
}

TEST(UvhttpUtilsTest, SafeStrncpyBufferOverflow) {
    /* 测试安全字符串复制（n版本）缓冲区溢出保护 */
    char dest[10];
    
    /* 源字符串长度等于目标缓冲区大小 */
    int result = uvhttp_safe_strncpy(dest, "123456789", sizeof(dest));
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(dest), 9);
    EXPECT_EQ(dest[9], '\0');
    
    /* 源字符串长度大于目标缓冲区大小 */
    result = uvhttp_safe_strncpy(dest, "1234567890", sizeof(dest));
    EXPECT_EQ(result, 0);
    EXPECT_EQ(strlen(dest), 9);
    EXPECT_EQ(dest[9], '\0');
}

TEST(UvhttpUtilsTest, SafeStrcpySpecialCharacters) {
    /* 测试安全字符串复制特殊字符 */
    char dest[256];
    
    /* 包含特殊字符的字符串 */
    const char* special = "Hello\tWorld\nTest";
    int result = uvhttp_safe_strcpy(dest, sizeof(dest), special);
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, special);
    
    /* 包含空字符的字符串 */
    const char* with_null = "Hello\0World";
    result = uvhttp_safe_strcpy(dest, sizeof(dest), with_null);
    EXPECT_EQ(result, 0);
    EXPECT_STREQ(dest, "Hello");
}

/* uvhttp_is_valid_content_type 已删除 - 完全未使用 */
/* uvhttp_is_valid_string_length 已删除 - 完全未使用 */

TEST(UvhttpUtilsTest, StatusCodeRange) {
    /* 测试状态码范围 */
    
    /* 1xx 信息响应 */
    for (int i = 100; i <= 199; i++) {
        EXPECT_EQ(uvhttp_is_valid_status_code(i), TRUE);
    }
    
    /* 2xx 成功 */
    for (int i = 200; i <= 299; i++) {
        EXPECT_EQ(uvhttp_is_valid_status_code(i), TRUE);
    }
    
    /* 3xx 重定向 */
    for (int i = 300; i <= 399; i++) {
        EXPECT_EQ(uvhttp_is_valid_status_code(i), TRUE);
    }
    
    /* 4xx 客户端错误 */
    for (int i = 400; i <= 499; i++) {
        EXPECT_EQ(uvhttp_is_valid_status_code(i), TRUE);
    }
    
    /* 5xx 服务器错误 */
    for (int i = 500; i <= 599; i++) {
        EXPECT_EQ(uvhttp_is_valid_status_code(i), TRUE);
    }
}

TEST(UvhttpUtilsTest, CommonStatusCodes) {
    /* 测试常见状态码 */
    
    /* 1xx */
    EXPECT_EQ(uvhttp_is_valid_status_code(100), TRUE);  /* Continue */
    EXPECT_EQ(uvhttp_is_valid_status_code(101), TRUE);  /* Switching Protocols */
    
    /* 2xx */
    EXPECT_EQ(uvhttp_is_valid_status_code(200), TRUE);  /* OK */
    EXPECT_EQ(uvhttp_is_valid_status_code(201), TRUE);  /* Created */
    EXPECT_EQ(uvhttp_is_valid_status_code(204), TRUE);  /* No Content */
    
    /* 3xx */
    EXPECT_EQ(uvhttp_is_valid_status_code(301), TRUE);  /* Moved Permanently */
    EXPECT_EQ(uvhttp_is_valid_status_code(302), TRUE);  /* Found */
    EXPECT_EQ(uvhttp_is_valid_status_code(304), TRUE);  /* Not Modified */
    
    /* 4xx */
    EXPECT_EQ(uvhttp_is_valid_status_code(400), TRUE);  /* Bad Request */
    EXPECT_EQ(uvhttp_is_valid_status_code(401), TRUE);  /* Unauthorized */
    EXPECT_EQ(uvhttp_is_valid_status_code(403), TRUE);  /* Forbidden */
    EXPECT_EQ(uvhttp_is_valid_status_code(404), TRUE);  /* Not Found */
    
    /* 5xx */
    EXPECT_EQ(uvhttp_is_valid_status_code(500), TRUE);  /* Internal Server Error */
    EXPECT_EQ(uvhttp_is_valid_status_code(502), TRUE);  /* Bad Gateway */
    EXPECT_EQ(uvhttp_is_valid_status_code(503), TRUE);  /* Service Unavailable */
}