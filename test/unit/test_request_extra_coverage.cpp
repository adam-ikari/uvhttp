/* uvhttp_request.c 扩展覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_request.h"
#include "uvhttp_allocator.h"
#include <string.h>

TEST(UvhttpRequestExtraCoverageTest, RequestGetMethod) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->method = UVHTTP_GET;
    const char* method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "GET");
    
    request->method = UVHTTP_POST;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "POST");
    
    request->method = UVHTTP_PUT;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "PUT");
    
    request->method = UVHTTP_DELETE;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "DELETE");
    
    request->method = UVHTTP_HEAD;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "HEAD");
    
    request->method = UVHTTP_OPTIONS;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "OPTIONS");
    
    request->method = UVHTTP_PATCH;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "PATCH");
    
    request->method = UVHTTP_ANY;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "ANY");
    
    method = uvhttp_request_get_method(nullptr);
    EXPECT_EQ(method, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetUrl) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    strncpy(request->url, "/api/users", MAX_URL_LEN - 1);
    const char* url = uvhttp_request_get_url(request);
    ASSERT_NE(url, nullptr);
    EXPECT_STREQ(url, "/api/users");
    
    strncpy(request->url, "/api/v1/posts/123", MAX_URL_LEN - 1);
    url = uvhttp_request_get_url(request);
    ASSERT_NE(url, nullptr);
    EXPECT_STREQ(url, "/api/v1/posts/123");
    
    url = uvhttp_request_get_url(nullptr);
    EXPECT_EQ(url, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetPath) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试无查询参数的路径 */
    strncpy(request->url, "/api/users", MAX_URL_LEN - 1);
    const char* path = uvhttp_request_get_path(request);
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "/api/users");
    
    /* 测试带查询参数的路径 */
    strncpy(request->url, "/api/users?key=value", MAX_URL_LEN - 1);
    path = uvhttp_request_get_path(request);
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "/api/users");
    
    /* 测试空 URL 返回空字符串 */
    request->url[0] = '\0';
    path = uvhttp_request_get_path(request);
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "");
    
    path = uvhttp_request_get_path(nullptr);
    EXPECT_EQ(path, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetQueryString) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试无查询参数的 URL */
    strncpy(request->url, "/api/users", MAX_URL_LEN - 1);
    const char* query = uvhttp_request_get_query_string(request);
    EXPECT_EQ(query, nullptr);
    
    query = uvhttp_request_get_query_string(nullptr);
    EXPECT_EQ(query, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetQueryParam) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试不存在的参数 */
    const char* param = uvhttp_request_get_query_param(request, "nonexistent");
    EXPECT_EQ(param, nullptr);
    
    /* 测试空参数名 */
    param = uvhttp_request_get_query_param(request, "");
    EXPECT_EQ(param, nullptr);
    
    /* 测试 nullptr 参数名 */
    param = uvhttp_request_get_query_param(request, nullptr);
    EXPECT_EQ(param, nullptr);
    
    /* 测试 nullptr request */
    param = uvhttp_request_get_query_param(nullptr, "key1");
    EXPECT_EQ(param, nullptr);
    
    /* 测试无查询参数的 URL */
    strncpy(request->url, "/api/users", MAX_URL_LEN - 1);
    param = uvhttp_request_get_query_param(request, "key1");
    EXPECT_EQ(param, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetClientIp) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试无 client 的情况 */
    const char* ip = uvhttp_request_get_client_ip(request);
    EXPECT_NE(ip, nullptr);
    
    ip = uvhttp_request_get_client_ip(nullptr);
    EXPECT_EQ(ip, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetHeader) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY;
    strcpy(request->headers[0].name, "Content-Type");
    strcpy(request->headers[0].value, "application/json");
    request->header_count = 1;
    
    const char* value = uvhttp_request_get_header(request, "Content-Type");
    ASSERT_NE(value, nullptr);
    EXPECT_STREQ(value, "application/json");
    
    value = uvhttp_request_get_header(request, "nonexistent");
    EXPECT_EQ(value, nullptr);
    
    value = uvhttp_request_get_header(request, "");
    EXPECT_EQ(value, nullptr);
    
    value = uvhttp_request_get_header(request, nullptr);
    EXPECT_EQ(value, nullptr);
    
    value = uvhttp_request_get_header(nullptr, "Content-Type");
    EXPECT_EQ(value, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetBody) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->body = (char*)uvhttp_alloc(1024);
    ASSERT_NE(request->body, nullptr);
    strcpy(request->body, "Hello, World!");
    request->body_length = strlen(request->body);
    
    const char* body = uvhttp_request_get_body(request);
    ASSERT_NE(body, nullptr);
    EXPECT_STREQ(body, "Hello, World!");
    
    body = uvhttp_request_get_body(nullptr);
    EXPECT_EQ(body, nullptr);
    
    uvhttp_free(request->body);
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestGetBodyLength) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->body_length = 1024;
    size_t length = uvhttp_request_get_body_length(request);
    EXPECT_EQ(length, 1024);
    
    length = uvhttp_request_get_body_length(nullptr);
    EXPECT_EQ(length, 0);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestFree) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    uvhttp_request_free(request);
    uvhttp_request_free(nullptr);
}

TEST(UvhttpRequestExtraCoverageTest, RequestCleanup) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试 cleanup 函数不会崩溃 */
    request->body = (char*)uvhttp_alloc(1024);
    request->parser = (llhttp_t*)uvhttp_alloc(sizeof(llhttp_t));
    request->parser_settings = (llhttp_settings_t*)uvhttp_alloc(sizeof(llhttp_settings_t));
    
    uvhttp_request_cleanup(request);
    
    uvhttp_request_cleanup(nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestMultipleHeaders) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY;
    strcpy(request->headers[0].name, "Content-Type");
    strcpy(request->headers[0].value, "application/json");
    
    strcpy(request->headers[1].name, "Authorization");
    strcpy(request->headers[1].value, "Bearer token123");
    
    request->header_count = 2;
    
    const char* value = uvhttp_request_get_header(request, "Content-Type");
    ASSERT_NE(value, nullptr);
    EXPECT_STREQ(value, "application/json");
    
    value = uvhttp_request_get_header(request, "Authorization");
    ASSERT_NE(value, nullptr);
    EXPECT_STREQ(value, "Bearer token123");
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestLargeBody) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->body = (char*)uvhttp_alloc(MAX_BODY_LEN);
    ASSERT_NE(request->body, nullptr);
    memset(request->body, 'A', MAX_BODY_LEN);
    request->body_length = MAX_BODY_LEN;
    
    const char* body = uvhttp_request_get_body(request);
    ASSERT_NE(body, nullptr);
    EXPECT_EQ(strlen(body), MAX_BODY_LEN);
    EXPECT_EQ(body[0], 'A');
    EXPECT_EQ(body[MAX_BODY_LEN - 1], 'A');
    
    uvhttp_free(request->body);
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestMemoryLeaks) {
    for (int i = 0; i < 100; i++) {
        uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
        ASSERT_NE(request, nullptr);
        memset(request, 0, sizeof(uvhttp_request_t));
        
        request->body = (char*)uvhttp_alloc(1024);
        request->parser = (llhttp_t*)uvhttp_alloc(sizeof(llhttp_t));
        request->parser_settings = (llhttp_settings_t*)uvhttp_alloc(sizeof(llhttp_settings_t));
        
        uvhttp_request_cleanup(request);
        uvhttp_free(request);
    }
}

TEST(UvhttpRequestExtraCoverageTest, RequestBoundaryValues) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试空 URL */
    request->url[0] = '\0';
    const char* url = uvhttp_request_get_url(request);
    ASSERT_NE(url, nullptr);
    EXPECT_STREQ(url, "");
    
    /* 测试最大 URL */
    memset(request->url, 'A', MAX_URL_LEN - 1);
    request->url[MAX_URL_LEN - 1] = '\0';
    url = uvhttp_request_get_url(request);
    ASSERT_NE(url, nullptr);
    EXPECT_EQ(strlen(url), MAX_URL_LEN - 1);
    
    /* 测试零 body 长度 */
    request->body_length = 0;
    size_t length = uvhttp_request_get_body_length(request);
    EXPECT_EQ(length, 0);
    
    /* 测试最大 body 长度 */
    request->body_length = MAX_BODY_LEN;
    length = uvhttp_request_get_body_length(request);
    EXPECT_EQ(length, MAX_BODY_LEN);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestUserData) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 设置用户数据 */
    request->user_data = (void*)0x12345678;
    EXPECT_EQ(request->user_data, (void*)0x12345678);
    
    request->user_data = nullptr;
    EXPECT_EQ(request->user_data, nullptr);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestParsingComplete) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->parsing_complete = 0;
    EXPECT_EQ(request->parsing_complete, 0);
    
    request->parsing_complete = 1;
    EXPECT_EQ(request->parsing_complete, 1);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestHeaderCount) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->header_count = 0;
    EXPECT_EQ(request->header_count, 0);
    
    request->header_count = MAX_HEADERS;
    EXPECT_EQ(request->header_count, MAX_HEADERS);
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestPathWithSpecialChars) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试带特殊字符的路径 */
    strncpy(request->url, "/api/users/123/comments/456", MAX_URL_LEN - 1);
    const char* path = uvhttp_request_get_path(request);
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "/api/users/123/comments/456");
    
    /* 测试带编码字符的路径 */
    strncpy(request->url, "/api/users?name=%E4%B8%AD%E6%96%87", MAX_URL_LEN - 1);
    path = uvhttp_request_get_path(request);
    ASSERT_NE(path, nullptr);
    EXPECT_STREQ(path, "/api/users");
    
    uvhttp_free(request);
}

TEST(UvhttpRequestExtraCoverageTest, RequestMethodAll) {
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试所有 HTTP 方法 */
    request->method = UVHTTP_GET;
    const char* method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "GET");
    
    request->method = UVHTTP_POST;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "POST");
    
    request->method = UVHTTP_PUT;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "PUT");
    
    request->method = UVHTTP_DELETE;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "DELETE");
    
    request->method = UVHTTP_HEAD;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "HEAD");
    
    request->method = UVHTTP_OPTIONS;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "OPTIONS");
    
    request->method = UVHTTP_PATCH;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "PATCH");

    request->method = UVHTTP_ANY;
    method = uvhttp_request_get_method(request);
    ASSERT_NE(method, nullptr);
    EXPECT_STREQ(method, "ANY");

    uvhttp_free(request);
}
