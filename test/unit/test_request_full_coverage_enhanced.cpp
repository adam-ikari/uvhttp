/**
 * @file test_request_full_coverage_enhanced.cpp
 * @brief 增强的请求测试 - 提升覆盖率到 50%
 * 
 * 目标：提升 uvhttp_request.c 覆盖率从 10.6% 到 50%
 * 
 * 测试内容：
 * - uvhttp_request_get_method
 * - uvhttp_request_get_url
 * - uvhttp_request_free
 * - uvhttp_request_cleanup
 * - uvhttp_request_get_path
 * - uvhttp_request_get_query_string
 * - uvhttp_request_get_query_param
 * - uvhttp_request_get_client_ip
 * - uvhttp_request_get_header
 * - uvhttp_request_get_body
 * - uvhttp_request_get_body_length
 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_request.h"
#include "uvhttp_allocator.h"

/* 测试获取请求方法 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetMethodNull) {
    const char* method = uvhttp_request_get_method(nullptr);
    EXPECT_EQ(method, nullptr);
}

/* 测试获取 URL - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetUrlNull) {
    const char* url = uvhttp_request_get_url(nullptr);
    EXPECT_EQ(url, nullptr);
}

/* 测试释放请求 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, FreeNull) {
    uvhttp_request_free(nullptr);
    // 不应该崩溃
}

/* 测试清理请求 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, CleanupNull) {
    uvhttp_request_cleanup(nullptr);
    // 不应该崩溃
}

/* 测试获取路径 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetPathNull) {
    const char* path = uvhttp_request_get_path(nullptr);
    EXPECT_EQ(path, nullptr);
}

/* 测试获取查询字符串 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetQueryStringNull) {
    const char* query = uvhttp_request_get_query_string(nullptr);
    EXPECT_EQ(query, nullptr);
}

/* 测试获取查询参数 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetQueryParamNullRequest) {
    const char* param = uvhttp_request_get_query_param(nullptr, "test");
    EXPECT_EQ(param, nullptr);
}

/* 测试获取查询参数 - NULL 名称 */
TEST(UvhttpRequestEnhancedTest, GetQueryParamNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    const char* param = uvhttp_request_get_query_param(&request, nullptr);
    EXPECT_EQ(param, nullptr);
}

/* 测试获取客户端 IP - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetClientIpNull) {
    const char* ip = uvhttp_request_get_client_ip(nullptr);
    EXPECT_EQ(ip, nullptr);
}

/* 测试获取请求头 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetHeaderNullRequest) {
    const char* header = uvhttp_request_get_header(nullptr, "Content-Type");
    EXPECT_EQ(header, nullptr);
}

/* 测试获取请求头 - NULL 名称 */
TEST(UvhttpRequestEnhancedTest, GetHeaderNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    const char* header = uvhttp_request_get_header(&request, nullptr);
    EXPECT_EQ(header, nullptr);
}

/* 测试获取请求体 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetBodyNull) {
    const char* body = uvhttp_request_get_body(nullptr);
    EXPECT_EQ(body, nullptr);
}

/* 测试获取请求体长度 - NULL 参数 */
TEST(UvhttpRequestEnhancedTest, GetBodyLengthNull) {
    size_t length = uvhttp_request_get_body_length(nullptr);
    EXPECT_EQ(length, 0);
}

/* 测试请求结构体字段初始化 */
TEST(UvhttpRequestEnhancedTest, RequestFieldInitialization) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    EXPECT_EQ(request.client, nullptr);
    EXPECT_EQ(request.parser, nullptr);
    EXPECT_EQ(request.parser_settings, nullptr);
    EXPECT_EQ(request.method, UVHTTP_ANY);
    EXPECT_EQ(request.url[0], '\0');
    EXPECT_EQ(request.path, nullptr);
    EXPECT_EQ(request.query, nullptr);
    EXPECT_EQ(request.body, nullptr);
    EXPECT_EQ(request.body_length, 0);
    EXPECT_EQ(request.body_capacity, 0);
    EXPECT_EQ(request.header_count, 0);
    EXPECT_EQ(request.parsing_complete, 0);
    EXPECT_EQ(request.user_data, nullptr);
}

/* 测试请求方法枚举值 */
TEST(UvhttpRequestEnhancedTest, RequestMethodEnumValues) {
    EXPECT_EQ(UVHTTP_ANY, 0);
    EXPECT_EQ(UVHTTP_GET, 1);
    EXPECT_EQ(UVHTTP_POST, 2);
    EXPECT_EQ(UVHTTP_PUT, 3);
    EXPECT_EQ(UVHTTP_DELETE, 4);
    EXPECT_EQ(UVHTTP_HEAD, 5);
    EXPECT_EQ(UVHTTP_OPTIONS, 6);
    EXPECT_EQ(UVHTTP_PATCH, 7);
}

/* 测试请求结构体大小 */
TEST(UvhttpRequestEnhancedTest, RequestStructSize) {
    EXPECT_GT(sizeof(uvhttp_request_t), 0);
    EXPECT_GT(sizeof(uvhttp_method_t), 0);
}

/* 测试常量值 */
TEST(UvhttpRequestEnhancedTest, RequestConstants) {
    EXPECT_GT(MAX_URL_LEN, 0);
    EXPECT_GT(MAX_BODY_LEN, 0);
    EXPECT_EQ(MAX_URL_LEN, 2048);
    EXPECT_EQ(MAX_BODY_LEN, 1024 * 1024);
}

/* 测试多次 NULL 调用 */
TEST(UvhttpRequestEnhancedTest, MultipleNullCalls) {
    for (int i = 0; i < 100; i++) {
        uvhttp_request_get_method(nullptr);
        uvhttp_request_get_url(nullptr);
        uvhttp_request_free(nullptr);
        uvhttp_request_cleanup(nullptr);
        uvhttp_request_get_path(nullptr);
        uvhttp_request_get_query_string(nullptr);
        uvhttp_request_get_query_param(nullptr, "test");
        uvhttp_request_get_client_ip(nullptr);
        uvhttp_request_get_header(nullptr, "test");
        uvhttp_request_get_body(nullptr);
        uvhttp_request_get_body_length(nullptr);
    }
    // 不应该崩溃
}

/* 测试请求结构体对齐 */
TEST(UvhttpRequestEnhancedTest, RequestStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_request_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_request_t), sizeof(size_t));
    EXPECT_GE(sizeof(uvhttp_request_t), sizeof(int));
}

/* 测试请求头数组 */
TEST(UvhttpRequestEnhancedTest, RequestHeadersArray) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    EXPECT_LT(request.header_count, MAX_HEADERS_MAX);
    EXPECT_GT(MAX_HEADERS_MAX, 0);
}

/* 测试所有请求方法 */
TEST(UvhttpRequestEnhancedTest, AllRequestMethods) {
    uvhttp_request_t request;
    
    // 测试 GET 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_GET;
    const char* get_method = uvhttp_request_get_method(&request);
    EXPECT_NE(get_method, nullptr);
    EXPECT_STREQ(get_method, "GET");
    
    // 测试 POST 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_POST;
    const char* post_method = uvhttp_request_get_method(&request);
    EXPECT_NE(post_method, nullptr);
    EXPECT_STREQ(post_method, "POST");
    
    // 测试 PUT 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_PUT;
    const char* put_method = uvhttp_request_get_method(&request);
    EXPECT_NE(put_method, nullptr);
    EXPECT_STREQ(put_method, "PUT");
    
    // 测试 DELETE 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_DELETE;
    const char* delete_method = uvhttp_request_get_method(&request);
    EXPECT_NE(delete_method, nullptr);
    EXPECT_STREQ(delete_method, "DELETE");
    
    // 测试 HEAD 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_HEAD;
    const char* head_method = uvhttp_request_get_method(&request);
    EXPECT_NE(head_method, nullptr);
    EXPECT_STREQ(head_method, "HEAD");
    
    // 测试 OPTIONS 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_OPTIONS;
    const char* options_method = uvhttp_request_get_method(&request);
    EXPECT_NE(options_method, nullptr);
    EXPECT_STREQ(options_method, "OPTIONS");
    
    // 测试 PATCH 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_PATCH;
    const char* patch_method = uvhttp_request_get_method(&request);
    EXPECT_NE(patch_method, nullptr);
    EXPECT_STREQ(patch_method, "PATCH");
    
    // 测试 ANY 方法
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_ANY;
    const char* any_method = uvhttp_request_get_method(&request);
    EXPECT_NE(any_method, nullptr);
    EXPECT_STREQ(any_method, "ANY");
}

/* 测试 URL 字段 */
TEST(UvhttpRequestEnhancedTest, UrlField) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    // 设置 URL
    strcpy(request.url, "/test/path?query=value");
    const char* url = uvhttp_request_get_url(&request);
    EXPECT_NE(url, nullptr);
    EXPECT_STREQ(url, "/test/path?query=value");
}

/* 测试不存在的请求头 */
TEST(UvhttpRequestEnhancedTest, GetNonExistentHeader) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.header_count = 0;
    
    const char* header = uvhttp_request_get_header(&request, "X-Non-Existent-Header");
    EXPECT_EQ(header, nullptr);
}

/* 测试不存在的查询参数 */
TEST(UvhttpRequestEnhancedTest, GetNonExistentQueryParam) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.query = nullptr;
    
    const char* param = uvhttp_request_get_query_param(&request, "nonexistent");
    EXPECT_EQ(param, nullptr);
}