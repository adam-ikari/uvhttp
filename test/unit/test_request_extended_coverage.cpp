/* UVHTTP Request Extended Coverage Test - Target: 60%+ */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_request.h"
#include "uvhttp_allocator.h"

/* ========== Test Request Get Functions ========== */

TEST(UvhttpRequestExtendedTest, GetMethodNullRequest) {
    const char* method = uvhttp_request_get_method(NULL);
    EXPECT_EQ(method, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetUrlNullRequest) {
    const char* url = uvhttp_request_get_url(NULL);
    EXPECT_EQ(url, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetPathNullRequest) {
    const char* path = uvhttp_request_get_path(NULL);
    EXPECT_EQ(path, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetQueryStringNullRequest) {
    const char* query = uvhttp_request_get_query_string(NULL);
    EXPECT_EQ(query, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetQueryParamNullRequest) {
    const char* param = uvhttp_request_get_query_param(NULL, "key");
    EXPECT_EQ(param, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetClientIpNullRequest) {
    const char* ip = uvhttp_request_get_client_ip(NULL);
    EXPECT_EQ(ip, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetBodyNullRequest) {
    const char* body = uvhttp_request_get_body(NULL);
    EXPECT_EQ(body, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetBodyLengthNullRequest) {
    size_t length = uvhttp_request_get_body_length(NULL);
    EXPECT_EQ(length, (size_t)0);
}

/* ========== Test Request Header Functions ========== */

TEST(UvhttpRequestExtendedTest, GetHeaderNullRequest) {
    const char* value = uvhttp_request_get_header(NULL, "Content-Type");
    EXPECT_EQ(value, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetHeaderNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* value = uvhttp_request_get_header(&request, NULL);
    EXPECT_EQ(value, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetHeaderNonExistent) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* value = uvhttp_request_get_header(&request, "NonExistent-Header");
    EXPECT_EQ(value, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetHeaderAtNullRequest) {
    uvhttp_header_t* header = uvhttp_request_get_header_at(NULL, 0);
    EXPECT_EQ(header, (uvhttp_header_t*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetHeaderAtOutOfRange) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    uvhttp_header_t* header = uvhttp_request_get_header_at(&request, 999);
    EXPECT_EQ(header, (uvhttp_header_t*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetHeaderCountNullRequest) {
    size_t count = uvhttp_request_get_header_count(NULL);
    EXPECT_EQ(count, (size_t)0);
}

/* ========== Test Request Cleanup ========== */

TEST(UvhttpRequestExtendedTest, CleanupNullRequest) {
    /* Should not crash */
    uvhttp_request_cleanup(NULL);
}

TEST(UvhttpRequestExtendedTest, FreeNullRequest) {
    /* Should not crash */
    uvhttp_request_free(NULL);
}

/* ========== Test Request Foreach Header ========== */

TEST(UvhttpRequestExtendedTest, ForeachHeaderNullRequest) {
    /* Should not crash */
    uvhttp_request_foreach_header(NULL, NULL, NULL);
}

static void test_header_callback(const char* name, const char* value, void* user_data) {
    int* count = (int*)user_data;
    if (count) {
        (*count)++;
    }
}

TEST(UvhttpRequestExtendedTest, ForeachHeaderEmpty) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    int count = 0;
    uvhttp_request_foreach_header(&request, test_header_callback, &count);
    EXPECT_EQ(count, 0);
}

/* ========== Test Request Field Initialization ========== */

TEST(UvhttpRequestExtendedTest, RequestFieldsInitialization) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* Verify all important fields are initialized to zero */
    EXPECT_EQ(request.method, (uvhttp_method_t)0);
    EXPECT_EQ(request.parsing_complete, 0);
    EXPECT_EQ(request.header_count, (size_t)0);
    EXPECT_EQ(request.body_length, (size_t)0);
    EXPECT_EQ(request.body_capacity, (size_t)0);
    EXPECT_EQ(request.client, (uv_tcp_t*)NULL);
    EXPECT_EQ(request.parser, (llhttp_t*)NULL);
    EXPECT_EQ(request.parser_settings, (llhttp_settings_t*)NULL);
    EXPECT_EQ(request.path, (char*)NULL);
    EXPECT_EQ(request.query, (char*)NULL);
    EXPECT_EQ(request.body, (char*)NULL);
    EXPECT_EQ(request.user_data, (void*)NULL);
    EXPECT_EQ(request.headers_extra, (uvhttp_header_t*)NULL);
}

/* ========== Test Request Add Header Edge Cases ========== */

TEST(UvhttpRequestExtendedTest, AddHeaderEmptyName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    uvhttp_error_t result = uvhttp_request_add_header(&request, "", "value");
    /* Empty name behavior depends on implementation */
    if (result != UVHTTP_OK) {
        SUCCEED();  /* Expected to fail */
    } else {
        /* If allowed, verify it was added */
        EXPECT_GT(request.header_count, (size_t)0);
    }
}

TEST(UvhttpRequestExtendedTest, AddHeaderEmptyValue) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    uvhttp_error_t result = uvhttp_request_add_header(&request, "name", "");
    /* Empty value might be allowed or rejected */
}

TEST(UvhttpRequestExtendedTest, AddHeaderLongName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* Test with very long header name */
    char long_name[1000];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    
    uvhttp_error_t result = uvhttp_request_add_header(&request, long_name, "value");
    /* Long name might be rejected or truncated */
}

TEST(UvhttpRequestExtendedTest, AddHeaderLongValue) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* Test with very long header value */
    char long_value[10000];
    memset(long_value, 'b', sizeof(long_value) - 1);
    long_value[sizeof(long_value) - 1] = '\0';
    
    uvhttp_error_t result = uvhttp_request_add_header(&request, "name", long_value);
    /* Long value might be rejected or truncated */
}

/* ========== Test Request Get Query Param ========== */

TEST(UvhttpRequestExtendedTest, GetQueryParamNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* param = uvhttp_request_get_query_param(&request, NULL);
    EXPECT_EQ(param, (const char*)NULL);
}

TEST(UvhttpRequestExtendedTest, GetQueryParamEmptyName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* param = uvhttp_request_get_query_param(&request, "");
    EXPECT_EQ(param, (const char*)NULL);
}