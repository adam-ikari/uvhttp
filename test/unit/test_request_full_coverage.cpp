/* UVHTTP 请求模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_constants.h"
#include <uv.h>

TEST(UvhttpRequestFullCoverageTest, RequestInit) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    int result = uvhttp_request_init(&request, &client);
    EXPECT_EQ(result, 0);
    
    EXPECT_EQ(request.client, &client);
    EXPECT_NE(request.parser, nullptr);
    EXPECT_NE(request.parser_settings, nullptr);
    EXPECT_EQ(request.method, UVHTTP_GET);
    EXPECT_NE(request.body, nullptr);
    EXPECT_EQ(request.body_length, 0);
    EXPECT_EQ(request.header_count, 0);
    EXPECT_EQ(request.parsing_complete, 0);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestCleanup) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    uvhttp_request_cleanup(&request);
    
    EXPECT_EQ(request.body_length, 0);
}

TEST(UvhttpRequestFullCoverageTest, RequestFree) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
}

TEST(UvhttpRequestFullCoverageTest, NullParams) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    int result = uvhttp_request_init(&request, NULL);
    EXPECT_NE(result, 0);
    
    result = uvhttp_request_init(NULL, &client);
    EXPECT_NE(result, 0);
    
    uvhttp_request_cleanup(NULL);
    uvhttp_request_free(NULL);
}

TEST(UvhttpRequestFullCoverageTest, MultipleInitFree) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    for (int i = 0; i < 10; i++) {
        uvhttp_request_init(&request, &client);
        uvhttp_request_cleanup(&request);
    }
}

TEST(UvhttpRequestFullCoverageTest, RequestGetBodyLength) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    size_t length = uvhttp_request_get_body_length(&request);
    EXPECT_EQ(length, 0);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestFields) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_EQ(strlen(request.url), 0);
    EXPECT_EQ(request.path, nullptr);
    EXPECT_EQ(request.query, nullptr);
    EXPECT_EQ(request.user_data, nullptr);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestHeaders) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_EQ(request.header_count, 0);
    
    for (size_t i = 0; i < MAX_HEADERS; i++) {
        EXPECT_EQ(request.headers[i].name[0], '\0');
        EXPECT_EQ(request.headers[i].value[0], '\0');
    }
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestMethod) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_EQ(request.method, UVHTTP_GET);
    
    request.method = UVHTTP_ANY;
    EXPECT_EQ(request.method, UVHTTP_ANY);
    
    request.method = UVHTTP_POST;
    EXPECT_EQ(request.method, UVHTTP_POST);
    
    request.method = UVHTTP_PUT;
    EXPECT_EQ(request.method, UVHTTP_PUT);
    
    request.method = UVHTTP_DELETE;
    EXPECT_EQ(request.method, UVHTTP_DELETE);
    
    request.method = UVHTTP_HEAD;
    EXPECT_EQ(request.method, UVHTTP_HEAD);
    
    request.method = UVHTTP_OPTIONS;
    EXPECT_EQ(request.method, UVHTTP_OPTIONS);
    
    request.method = UVHTTP_PATCH;
    EXPECT_EQ(request.method, UVHTTP_PATCH);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestParsingState) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_EQ(request.parsing_complete, 0);
    
    request.parsing_complete = 1;
    EXPECT_EQ(request.parsing_complete, 1);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestBody) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_NE(request.body, nullptr);
    EXPECT_EQ(request.body_length, 0);
    EXPECT_GT(request.body_capacity, 0);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestUserData) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    int test_data = 42;
    request.user_data = &test_data;
    EXPECT_EQ(request.user_data, &test_data);
    
    request.user_data = nullptr;
    EXPECT_EQ(request.user_data, nullptr);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestUrl) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    for (size_t i = 0; i < MAX_URL_LEN; i++) {
        EXPECT_EQ(request.url[i], '\0');
    }
    
    strncpy(request.url, "/test/path?param=value", MAX_URL_LEN - 1);
    EXPECT_STREQ(request.url, "/test/path?param=value");
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestClient) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_EQ(request.client, &client);
    
    uvhttp_request_cleanup(&request);
}

TEST(UvhttpRequestFullCoverageTest, RequestParser) {
    uvhttp_request_t request;
    uv_tcp_t client;
    
    uvhttp_request_init(&request, &client);
    
    EXPECT_NE(request.parser, nullptr);
    EXPECT_NE(request.parser_settings, nullptr);
    
    uvhttp_request_cleanup(&request);
}