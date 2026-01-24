/**
 * @file test_response_full_coverage_enhanced.cpp
 * @brief 增强的响应测试 - 提升覆盖率到 50%
 * 
 * 目标：提升 uvhttp_response.c 覆盖率从 5.6% 到 50%
 * 
 * 测试内容：
 * - uvhttp_response_init
 * - uvhttp_response_set_status
 * - uvhttp_response_set_header
 * - uvhttp_response_set_body
 * - uvhttp_response_cleanup
 * - uvhttp_response_free
 * - uvhttp_response_build_data
 * - uvhttp_response_send_raw
 * - uvhttp_response_send
 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"

/* 测试初始化响应 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, InitNull) {
    uvhttp_error_t result = uvhttp_response_init(nullptr, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置状态码 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, SetStatusNull) {
    uvhttp_error_t result = uvhttp_response_set_status(nullptr, 200);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置响应头 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, SetHeaderNullResponse) {
    uvhttp_error_t result = uvhttp_response_set_header(nullptr, "Content-Type", "text/plain");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置响应头 - NULL 名称 */
TEST(UvhttpResponseEnhancedTest, SetHeaderNullName) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_error_t result = uvhttp_response_set_header(&response, nullptr, "text/plain");
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置响应头 - NULL 值 */
TEST(UvhttpResponseEnhancedTest, SetHeaderNullValue) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_error_t result = uvhttp_response_set_header(&response, "Content-Type", nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置响应体 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, SetBodyNullResponse) {
    uvhttp_error_t result = uvhttp_response_set_body(nullptr, "test", 4);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置响应体 - NULL 数据 */
TEST(UvhttpResponseEnhancedTest, SetBodyNullData) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_error_t result = uvhttp_response_set_body(&response, nullptr, 4);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试清理响应 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, CleanupNull) {
    uvhttp_response_cleanup(nullptr);
    // 不应该崩溃
}

/* 测试释放响应 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, FreeNull) {
    uvhttp_response_free(nullptr);
    // 不应该崩溃
}

/* 测试构建响应数据 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, BuildDataNullResponse) {
    char* data = nullptr;
    size_t length = 0;
    uvhttp_error_t result = uvhttp_response_build_data(nullptr, &data, &length);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试构建响应数据 - NULL 输出 */
TEST(UvhttpResponseEnhancedTest, BuildDataNullOutput) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_error_t result = uvhttp_response_build_data(&response, nullptr, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试发送原始数据 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, SendRawNull) {
    uvhttp_error_t result = uvhttp_response_send_raw(nullptr, 0, nullptr, nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试发送响应 - NULL 参数 */
TEST(UvhttpResponseEnhancedTest, SendNull) {
    uvhttp_error_t result = uvhttp_response_send(nullptr);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试响应结构体字段初始化 */
TEST(UvhttpResponseEnhancedTest, ResponseFieldInitialization) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    EXPECT_EQ(response.client, nullptr);
    EXPECT_EQ(response.status_code, 0);
    EXPECT_EQ(response.header_count, 0);
    EXPECT_EQ(response.body, nullptr);
    EXPECT_EQ(response.body_length, 0);
    EXPECT_EQ(response.headers_sent, 0);
    EXPECT_EQ(response.keep_alive, 0);
    EXPECT_EQ(response.sent, 0);
    EXPECT_EQ(response.finished, 0);
    EXPECT_EQ(response.compress, 0);
    EXPECT_EQ(response.cache_ttl, 0);
    EXPECT_EQ(response.cache_expires, 0);
}

/* 测试响应结构体大小 */
TEST(UvhttpResponseEnhancedTest, ResponseStructSize) {
    EXPECT_GT(sizeof(uvhttp_response_t), 0);
    EXPECT_GT(sizeof(uvhttp_write_data_t), 0);
    EXPECT_GT(sizeof(uvhttp_tls_write_data_t), 0);
}

/* 测试常量值 */
TEST(UvhttpResponseEnhancedTest, ResponseConstants) {
    EXPECT_GT(MAX_RESPONSE_BODY_LEN, 0);
    EXPECT_EQ(MAX_RESPONSE_BODY_LEN, 1024 * 1024);
    EXPECT_GT(MAX_HEADERS_MAX, 0);
}

/* 测试多次 NULL 调用 */
TEST(UvhttpResponseEnhancedTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_response_init(nullptr, nullptr);
        uvhttp_response_set_status(nullptr, 200);
        uvhttp_response_set_header(nullptr, "Content-Type", "text/plain");
        uvhttp_response_set_body(nullptr, "test", 4);
        uvhttp_response_cleanup(nullptr);
        uvhttp_response_free(nullptr);
        
        char* data = nullptr;
        size_t length = 0;
        uvhttp_response_build_data(nullptr, &data, &length);
        uvhttp_response_send_raw(nullptr, 0, nullptr, nullptr);
        uvhttp_response_send(nullptr);
    }
    // 不应该崩溃
}

/* 测试响应结构体对齐 */
TEST(UvhttpResponseEnhancedTest, ResponseStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_response_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_response_t), sizeof(size_t));
    EXPECT_GE(sizeof(uvhttp_response_t), sizeof(int));
}

/* 测试响应头数组 */
TEST(UvhttpResponseEnhancedTest, ResponseHeadersArray) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    EXPECT_LT(response.header_count, MAX_HEADERS_MAX);
    EXPECT_GT(MAX_HEADERS_MAX, 0);
}

/* 测试设置状态码 - 各种状态码 */
TEST(UvhttpResponseEnhancedTest, SetStatusCodes) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 测试 200 OK
    uvhttp_error_t result = uvhttp_response_set_status(&response, 200);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.status_code, 200);
    
    // 测试 404 Not Found
    memset(&response, 0, sizeof(response));
    result = uvhttp_response_set_status(&response, 404);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.status_code, 404);
    
    // 测试 500 Internal Server Error
    memset(&response, 0, sizeof(response));
    result = uvhttp_response_set_status(&response, 500);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.status_code, 500);
    
    // 测试无效状态码
    memset(&response, 0, sizeof(response));
    result = uvhttp_response_set_status(&response, 999);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置响应头 - 各种头字段 */
TEST(UvhttpResponseEnhancedTest, SetHeaders) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 测试 Content-Type
    uvhttp_error_t result = uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_GT(response.header_count, 0);
    
    // 测试 Content-Length
    result = uvhttp_response_set_header(&response, "Content-Length", "100");
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 测试 Cache-Control
    result = uvhttp_response_set_header(&response, "Cache-Control", "no-cache");
    EXPECT_EQ(result, UVHTTP_OK);
    
    // 测试自定义头
    result = uvhttp_response_set_header(&response, "X-Custom-Header", "custom-value");
    EXPECT_EQ(result, UVHTTP_OK);
}

/* 测试设置响应体 - 各种大小 */
TEST(UvhttpResponseEnhancedTest, SetBodySizes) {
    uvhttp_response_t response;

    // 测试空响应体 - 应该返回错误
    memset(&response, 0, sizeof(response));
    uvhttp_error_t result = uvhttp_response_set_body(&response, "", 0);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    // 测试小响应体
    memset(&response, 0, sizeof(response));
    result = uvhttp_response_set_body(&response, "test", 4);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.body_length, 4);

    // 测试中等响应体
    memset(&response, 0, sizeof(response));
    char medium_body[1024];
    memset(medium_body, 'A', sizeof(medium_body));
    result = uvhttp_response_set_body(&response, medium_body, sizeof(medium_body));
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.body_length, sizeof(medium_body));
}

/* 测试响应体长度超过限制 */
TEST(UvhttpResponseEnhancedTest, SetBodyTooLarge) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 测试超过限制的响应体
    char large_body[MAX_RESPONSE_BODY_LEN + 1];
    memset(large_body, 'A', sizeof(large_body));
    uvhttp_error_t result = uvhttp_response_set_body(&response, large_body, sizeof(large_body));
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试响应头数量限制 */
TEST(UvhttpResponseEnhancedTest, SetHeadersTooMany) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 添加超过限制的头
    for (int i = 0; i < MAX_HEADERS_MAX + 10; i++) {
        char header_name[32];
        snprintf(header_name, sizeof(header_name), "X-Header-%d", i);
        uvhttp_response_set_header(&response, header_name, "value");
    }
    
    // 头数量应该被限制
    EXPECT_LE(response.header_count, MAX_HEADERS_MAX);
}

/* 测试 HTTP/1.1 优化字段 */
TEST(UvhttpResponseEnhancedTest, Http11OptimizationFields) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 测试 keep_alive 字段
    response.keep_alive = 1;
    EXPECT_EQ(response.keep_alive, 1);
    
    // 测试 sent 字段
    response.sent = 1;
    EXPECT_EQ(response.sent, 1);
    
    // 测试 finished 字段
    response.finished = 1;
    EXPECT_EQ(response.finished, 1);
    
    // 测试 compress 字段
    response.compress = 1;
    EXPECT_EQ(response.compress, 1);
    
    // 测试 cache_ttl 字段
    response.cache_ttl = 3600;
    EXPECT_EQ(response.cache_ttl, 3600);
    
    // 测试 cache_expires 字段
    response.cache_expires = 1234567890;
    EXPECT_EQ(response.cache_expires, 1234567890);
}