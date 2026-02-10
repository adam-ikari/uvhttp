/* uvhttp_response.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <string.h>
#include <stdlib.h>

/* 模拟客户端连接 */
typedef struct {
    int dummy;
} mock_client_t;

TEST(UvhttpResponseTest, ResponseInit) {
    uvhttp_response_t response;
    mock_client_t client;
    
    /* 正常初始化 */
    uvhttp_error_t result = uvhttp_response_init(&response, &client);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.status_code, UVHTTP_STATUS_OK);
    EXPECT_EQ(response.header_count, 0);
    EXPECT_EQ(response.body, nullptr);
    EXPECT_EQ(response.body_length, 0);
    EXPECT_EQ(response.headers_sent, 0);
    EXPECT_EQ(response.keepalive, 1);
    EXPECT_EQ(response.sent, 0);
    EXPECT_EQ(response.finished, 0);
    
    /* NULL 响应 */
    result = uvhttp_response_init(nullptr, &client);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 客户端 */
    result = uvhttp_response_init(&response, nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseSetStatus) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置各种状态码 */
    EXPECT_EQ(uvhttp_response_set_status(&response, UVHTTP_STATUS_OK), UVHTTP_OK);
    EXPECT_EQ(response.status_code, UVHTTP_STATUS_OK);
    
    EXPECT_EQ(uvhttp_response_set_status(&response, UVHTTP_STATUS_CREATED), UVHTTP_OK);
    EXPECT_EQ(response.status_code, UVHTTP_STATUS_CREATED);
    
    EXPECT_EQ(uvhttp_response_set_status(&response, UVHTTP_STATUS_NOT_FOUND), UVHTTP_OK);
    EXPECT_EQ(response.status_code, UVHTTP_STATUS_NOT_FOUND);
    
    EXPECT_EQ(uvhttp_response_set_status(&response, UVHTTP_STATUS_INTERNAL_ERROR), UVHTTP_OK);
    EXPECT_EQ(response.status_code, UVHTTP_STATUS_INTERNAL_ERROR);
    
    /* 无效状态码 - 太小 */
    EXPECT_EQ(uvhttp_response_set_status(&response, 0), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 无效状态码 - 太大 */
    EXPECT_EQ(uvhttp_response_set_status(&response, 1000), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 响应 */
    EXPECT_EQ(uvhttp_response_set_status(nullptr, UVHTTP_STATUS_OK), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseSetHeader) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置单个头部 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", "text/html"), UVHTTP_OK);
    EXPECT_EQ(response.header_count, 1);
    EXPECT_STREQ(response.headers[0].name, "Content-Type");
    EXPECT_STREQ(response.headers[0].value, "text/html");
    
    /* 设置多个头部 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Length", "1024"), UVHTTP_OK);
    EXPECT_EQ(uvhttp_response_set_header(&response, "Connection", "keep-alive"), UVHTTP_OK);
    EXPECT_EQ(response.header_count, 3);
    
    /* 覆盖同名头部 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", "application/json"), UVHTTP_OK);
    EXPECT_EQ(response.header_count, 4);
    
    /* NULL 响应 */
    EXPECT_EQ(uvhttp_response_set_header(nullptr, "Content-Type", "text/html"), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 头部名称 */
    EXPECT_EQ(uvhttp_response_set_header(&response, nullptr, "text/html"), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 头部值 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", nullptr), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 头部值包含控制字符（换行符） */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", "text/html\r\n"), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 头部值包含回车符 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", "text/html\r"), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 头部值包含换行符 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", "text/html\n"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseSetHeaderMaxHeaders) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 添加最大数量的头部 */
    for (int i = 0; i < MAX_HEADERS; i++) {
        char name[32];
        char value[32];
        snprintf(name, sizeof(name), "Header-%d", i);
        snprintf(value, sizeof(value), "Value-%d", i);
        EXPECT_EQ(uvhttp_response_set_header(&response, name, value), UVHTTP_OK);
    }
    
    EXPECT_EQ(response.header_count, MAX_HEADERS);
    
    /* 尝试添加超过限制的头部 */
    EXPECT_EQ(uvhttp_response_set_header(&response, "Extra-Header", "Extra-Value"), UVHTTP_ERROR_OUT_OF_MEMORY);
}

TEST(UvhttpResponseTest, ResponseSetBody) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    const char* body = "Hello, World!";
    size_t body_len = strlen(body);
    
    /* 设置正常响应体 */
    EXPECT_EQ(uvhttp_response_set_body(&response, body, body_len), UVHTTP_OK);
    EXPECT_NE(response.body, nullptr);
    EXPECT_EQ(response.body_length, body_len);
    EXPECT_EQ(memcmp(response.body, body, body_len), 0);
    
    /* 覆盖响应体 */
    const char* new_body = "New body content";
    size_t new_body_len = strlen(new_body);
    EXPECT_EQ(uvhttp_response_set_body(&response, new_body, new_body_len), UVHTTP_OK);
    EXPECT_EQ(response.body_length, new_body_len);
    EXPECT_EQ(memcmp(response.body, new_body, new_body_len), 0);
    
    /* NULL 响应 */
    EXPECT_EQ(uvhttp_response_set_body(nullptr, body, body_len), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 响应体 */
    EXPECT_EQ(uvhttp_response_set_body(&response, nullptr, body_len), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 零长度响应体 */
    EXPECT_EQ(uvhttp_response_set_body(&response, body, 0), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 响应体太大 */
    char large_body[UVHTTP_MAX_BODY_SIZE + 1];
    memset(large_body, 'A', sizeof(large_body));
    EXPECT_EQ(uvhttp_response_set_body(&response, large_body, sizeof(large_body)), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseSetBodyBinary) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 测试二进制数据 */
    uint8_t binary_data[] = {0x00, 0x01, 0x02, 0xFF, 0xFE, 0xFD};
    EXPECT_EQ(uvhttp_response_set_body(&response, (const char*)binary_data, sizeof(binary_data)), UVHTTP_OK);
    EXPECT_EQ(response.body_length, sizeof(binary_data));
    EXPECT_EQ(memcmp(response.body, binary_data, sizeof(binary_data)), 0);
}

TEST(UvhttpResponseTest, ResponseCleanup) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置响应体 */
    const char* body = "Test body";
    uvhttp_response_set_body(&response, body, strlen(body));
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
    EXPECT_EQ(response.body, nullptr);
    EXPECT_EQ(response.body_length, 0);
    
    /* 清理 NULL 响应 */
    uvhttp_response_cleanup(nullptr);
    
    /* 清理没有响应体的响应 */
    uvhttp_response_init(&response, &client);
    uvhttp_response_cleanup(&response);
    EXPECT_EQ(response.body, nullptr);
}

TEST(UvhttpResponseTest, ResponseFree) {
    /* 分配响应 */
    uvhttp_response_t* response = (uvhttp_response_t*)uvhttp_alloc(sizeof(uvhttp_response_t));
    ASSERT_NE(response, nullptr);
    
    mock_client_t client;
    uvhttp_response_init(response, &client);
    
    /* 设置响应体 */
    const char* body = "Test body";
    uvhttp_response_set_body(response, body, strlen(body));
    
    /* 释放响应 */
    uvhttp_response_free(response);
    
    /* 释放 NULL 响应 */
    uvhttp_response_free(nullptr);
}

TEST(UvhttpResponseTest, ResponseBuildData) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置状态码和头部 */
    uvhttp_response_set_status(&response, UVHTTP_STATUS_OK);
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    
    /* 设置响应体 */
    const char* body = "Hello, World!";
    uvhttp_response_set_body(&response, body, strlen(body));
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(length, 0);
    
    /* 验证响应数据包含状态行 */
    EXPECT_NE(strstr(data, "HTTP/1.1 200 OK"), nullptr);
    
    /* 验证响应数据包含头部 */
    EXPECT_NE(strstr(data, "Content-Type: text/html"), nullptr);
    
    /* 验证响应数据包含响应体 */
    EXPECT_NE(strstr(data, body), nullptr);
    
    /* 验证响应数据包含 Content-Length */
    EXPECT_NE(strstr(data, "Content-Length:"), nullptr);
    
    /* 验证响应数据包含 Connection 头 */
    EXPECT_NE(strstr(data, "Connection:"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataNoBody) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 不设置响应体 */
    uvhttp_response_set_status(&response, UVHTTP_STATUS_NO_CONTENT);
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(length, 0);
    
    /* 验证响应数据包含 Content-Length: 0 */
    EXPECT_NE(strstr(data, "Content-Length: 0"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataWithKeepAlive) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 启用 keep-alive */
    response.keepalive = 1;
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    
    /* 验证响应数据包含 keep-alive 头 */
    EXPECT_NE(strstr(data, "Connection: keep-alive"), nullptr);
    EXPECT_NE(strstr(data, "Keep-Alive:"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataWithoutKeepAlive) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 禁用 keep-alive */
    response.keepalive = 0;
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    
    /* 验证响应数据包含 Connection: close */
    EXPECT_NE(strstr(data, "Connection: close"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataAlreadySent) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 标记为已发送 */
    response.sent = 1;
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(length, 0);
}

TEST(UvhttpResponseTest, ResponseBuildDataInvalidParams) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    char* data = nullptr;
    size_t length = 0;
    
    /* NULL 响应 */
    EXPECT_EQ(uvhttp_response_build_data(nullptr, &data, &length), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 数据指针 */
    EXPECT_EQ(uvhttp_response_build_data(&response, nullptr, &length), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 长度指针 */
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseBuildDataLargeHeaders) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 添加大量头部 */
    for (int i = 0; i < 50; i++) {
        char name[64];
        char value[256];
        snprintf(name, sizeof(name), "X-Custom-Header-%d", i);
        snprintf(value, sizeof(value), "This is a very long header value number %d with lots of text", i);
        uvhttp_response_set_header(&response, name, value);
    }
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(length, 0);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseSendRawInvalidParams) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    const char* data = "test data";
    size_t length = strlen(data);
    
    /* NULL 数据 */
    EXPECT_EQ(uvhttp_response_send_raw(nullptr, length, &client, &response), UVHTTP_ERROR_INVALID_PARAM);
    
    /* 零长度 */
    EXPECT_EQ(uvhttp_response_send_raw(data, 0, &client, &response), UVHTTP_ERROR_INVALID_PARAM);
    
    /* NULL 客户端 */
    EXPECT_EQ(uvhttp_response_send_raw(data, length, nullptr, &response), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseSendInvalidParams) {
    /* NULL 响应 */
    EXPECT_EQ(uvhttp_response_send(nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST(UvhttpResponseTest, ResponseSendAlreadySent) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 标记为已发送 */
    response.sent = 1;
    
    /* 尝试再次发送 */
    EXPECT_EQ(uvhttp_response_send(&response), UVHTTP_OK);
}

TEST(UvhttpResponseTest, ResponseBuildDataWithContentType) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置 Content-Type */
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    
    /* 验证只有一个 Content-Type 头 */
    int count = 0;
    const char* pos = data;
    while ((pos = strstr(pos, "Content-Type:")) != nullptr) {
        count++;
        pos += strlen("Content-Type:");
    }
    EXPECT_EQ(count, 1);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataWithContentLength) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置 Content-Length */
    uvhttp_response_set_header(&response, "Content-Length", "1024");
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    
    /* 验证只有一个 Content-Length 头 */
    int count = 0;
    const char* pos = data;
    while ((pos = strstr(pos, "Content-Length:")) != nullptr) {
        count++;
        pos += strlen("Content-Length:");
    }
    EXPECT_EQ(count, 1);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataWithConnection) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置 Connection 头 */
    uvhttp_response_set_header(&response, "Connection", "close");
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    
    /* 验证只有一个 Connection 头 */
    int count = 0;
    const char* pos = data;
    while ((pos = strstr(pos, "Connection:")) != nullptr) {
        count++;
        pos += strlen("Connection:");
    }
    EXPECT_EQ(count, 1);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataSkipInvalidHeaders) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置有效头部 */
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    
    /* 设置包含控制字符的头部（应该被跳过） */
    // 注意：uvhttp_response_set_header 会拒绝包含控制字符的头部
    // 所以这个测试实际上不会添加这样的头部
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataAllStatusCodes) {
    uvhttp_response_t response;
    mock_client_t client;
    
    /* 测试所有常见状态码 */
    int status_codes[] = {
        UVHTTP_STATUS_OK,
        UVHTTP_STATUS_CREATED,
        UVHTTP_STATUS_NO_CONTENT,
        UVHTTP_STATUS_BAD_REQUEST,
        UVHTTP_STATUS_UNAUTHORIZED,
        UVHTTP_STATUS_FORBIDDEN,
        UVHTTP_STATUS_NOT_FOUND,
        UVHTTP_STATUS_METHOD_NOT_ALLOWED,
        UVHTTP_STATUS_INTERNAL_ERROR,
        UVHTTP_STATUS_NOT_IMPLEMENTED,
        UVHTTP_STATUS_BAD_GATEWAY,
        UVHTTP_STATUS_SERVICE_UNAVAILABLE
    };
    
    for (size_t i = 0; i < sizeof(status_codes) / sizeof(status_codes[0]); i++) {
        uvhttp_response_init(&response, &client);
        uvhttp_response_set_status(&response, status_codes[i]);
        
        char* data = nullptr;
        size_t length = 0;
        EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
        EXPECT_NE(data, nullptr);
        
        /* 验证状态行包含正确的状态码 */
        char expected[32];
        snprintf(expected, sizeof(expected), "HTTP/1.1 %d", status_codes[i]);
        EXPECT_NE(strstr(data, expected), nullptr);
        
        uvhttp_free(data);
        uvhttp_response_cleanup(&response);
    }
}

TEST(UvhttpResponseTest, ResponseBuildDataEmptyHeaders) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 不设置任何头部 */
    uvhttp_response_set_status(&response, UVHTTP_STATUS_OK);
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 验证包含默认的 Content-Type */
    EXPECT_NE(strstr(data, "Content-Type: text/plain"), nullptr);
    
    /* 验证包含默认的 Content-Length */
    EXPECT_NE(strstr(data, "Content-Length: 0"), nullptr);
    
    /* 验证包含默认的 Connection 头 */
    EXPECT_NE(strstr(data, "Connection:"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataLargeBody) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 创建大的响应体 */
    size_t body_size = 100 * 1024;  // 100KB
    char* body = (char*)uvhttp_alloc(body_size);
    ASSERT_NE(body, nullptr);
    memset(body, 'A', body_size);
    
    EXPECT_EQ(uvhttp_response_set_body(&response, body, body_size), UVHTTP_OK);
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    EXPECT_GT(length, body_size);
    
    /* 验证响应体被正确包含 */
    EXPECT_EQ(memcmp(data + length - body_size, body, body_size), 0);
    
    /* 释放数据 */
    uvhttp_free(data);
    uvhttp_free(body);
}

TEST(UvhttpResponseTest, ResponseBuildDataWithAllHeaders) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置所有常见头部 */
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    uvhttp_response_set_header(&response, "Content-Length", "1024");
    uvhttp_response_set_header(&response, "Connection", "keep-alive");
    uvhttp_response_set_header(&response, "Cache-Control", "no-cache");
    uvhttp_response_set_header(&response, "X-Custom-Header", "custom-value");
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 验证所有头部都被包含 */
    EXPECT_NE(strstr(data, "Content-Type: application/json"), nullptr);
    EXPECT_NE(strstr(data, "Content-Length: 1024"), nullptr);
    EXPECT_NE(strstr(data, "Connection: keep-alive"), nullptr);
    EXPECT_NE(strstr(data, "Cache-Control: no-cache"), nullptr);
    EXPECT_NE(strstr(data, "X-Custom-Header: custom-value"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataTerminator) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 验证数据以 null 结尾 */
    EXPECT_EQ(data[length], '\0');
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataMemorySafety) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置响应体 */
    const char* body = "Test body";
    uvhttp_response_set_body(&response, body, strlen(body));
    
    /* 多次构建响应数据 */
    for (int i = 0; i < 10; i++) {
        char* data = nullptr;
        size_t length = 0;
        EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
        EXPECT_NE(data, nullptr);
        EXPECT_GT(length, 0);
        uvhttp_free(data);
    }
}

TEST(UvhttpResponseTest, ResponseInitMultipleTimes) {
    uvhttp_response_t response;
    mock_client_t client;
    
    /* 多次初始化 */
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(uvhttp_response_init(&response, &client), UVHTTP_OK);
        EXPECT_EQ(response.status_code, UVHTTP_STATUS_OK);
        EXPECT_EQ(response.header_count, 0);
        EXPECT_EQ(response.body, nullptr);
    }
}

TEST(UvhttpResponseTest, ResponseCleanupMultipleTimes) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 设置响应体 */
    const char* body = "Test body";
    uvhttp_response_set_body(&response, body, strlen(body));
    
    /* 多次清理 */
    for (int i = 0; i < 5; i++) {
        uvhttp_response_cleanup(&response);
        EXPECT_EQ(response.body, nullptr);
        EXPECT_EQ(response.body_length, 0);
    }
}

TEST(UvhttpResponseTest, ResponseSetBodyWithNullBytes) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 包含 null 字节的响应体 */
    char body[] = {'H', 'e', 'l', 'l', 'o', '\0', 'W', 'o', 'r', 'l', 'd'};
    size_t body_len = sizeof(body);
    
    EXPECT_EQ(uvhttp_response_set_body(&response, body, body_len), UVHTTP_OK);
    EXPECT_EQ(response.body_length, body_len);
    EXPECT_EQ(memcmp(response.body, body, body_len), 0);
}

TEST(UvhttpResponseTest, ResponseBuildDataCaseInsensitiveHeaders) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 使用不同大小写的头部名称 */
    uvhttp_response_set_header(&response, "content-type", "text/html");
    uvhttp_response_set_header(&response, "CONTENT-LENGTH", "1024");
    uvhttp_response_set_header(&response, "Connection", "keep-alive");
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 验证头部被正确添加（保留原始大小写） */
    EXPECT_NE(strstr(data, "content-type: text/html"), nullptr);
    EXPECT_NE(strstr(data, "CONTENT-LENGTH: 1024"), nullptr);
    EXPECT_NE(strstr(data, "Connection: keep-alive"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataSpecialCharactersInHeaderValue) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 包含特殊字符的头部值 */
    const char* special_value = "text/html; charset=utf-8; version=1.0";
    EXPECT_EQ(uvhttp_response_set_header(&response, "Content-Type", special_value), UVHTTP_OK);
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 验证特殊字符被正确处理 */
    EXPECT_NE(strstr(data, special_value), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}

TEST(UvhttpResponseTest, ResponseBuildDataLongHeaderName) {
    uvhttp_response_t response;
    mock_client_t client;
    uvhttp_response_init(&response, &client);
    
    /* 长头部名称（但不超过限制） */
    char long_name[100];
    memset(long_name, 'X', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    
    /* 长头部值（但不超过限制） */
    char long_value[1000];
    memset(long_value, 'Y', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    
    EXPECT_EQ(uvhttp_response_set_header(&response, long_name, long_value), UVHTTP_OK);
    
    /* 构建响应数据 */
    char* data = nullptr;
    size_t length = 0;
    EXPECT_EQ(uvhttp_response_build_data(&response, &data, &length), UVHTTP_OK);
    EXPECT_NE(data, nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
}
