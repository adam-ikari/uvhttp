/**
 * @file test_request.c
 * @brief HTTP请求模块测试
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_request.h"
#include "../include/uvhttp_allocator.h"

TEST_FUNC(request_create_cleanup) {
    uvhttp_request_t* request = uvhttp_malloc(sizeof(uvhttp_request_t));
    TEST_ASSERT_NOT_NULL(request);
    
    /* 手动初始化 */
    memset(request, 0, sizeof(uvhttp_request_t));
    request->method = UVHTTP_GET;
    request->client = uvhttp_test_create_mock_client();
    TEST_ASSERT_NOT_NULL(request->client);
    
    /* 测试cleanup */
    uvhttp_request_cleanup(request);
    uvhttp_test_destroy_mock_client(request->client);
    uvhttp_free(request);
    
    return 0;
}

TEST_FUNC(request_get_method) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    request.method = UVHTTP_GET;
    const char* method = uvhttp_request_get_method(&request);
    TEST_ASSERT_STREQ("GET", method);
    
    request.method = UVHTTP_POST;
    method = uvhttp_request_get_method(&request);
    TEST_ASSERT_STREQ("POST", method);
    
    return 0;
}

TEST_FUNC(request_get_path_simple) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    strcpy(request.url, "/hello");
    request.path = request.url;
    
    const char* path = uvhttp_request_get_path(&request);
    TEST_ASSERT_STREQ("/hello", path);
    
    return 0;
}

TEST_FUNC(request_get_path_with_query) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    strcpy(request.url, "/hello?name=test&value=123");
    const char* path = uvhttp_request_get_path(&request);
    TEST_ASSERT_STREQ("/hello", path);
    
    const char* query = uvhttp_request_get_query_string(&request);
    TEST_ASSERT_STREQ("name=test&value=123", query);
    
    return 0;
}

TEST_FUNC(request_get_path_null_request) {
    const char* path = uvhttp_request_get_path(NULL);
    TEST_ASSERT_STREQ("/", path); /* 默认返回根路径 */
    
    return 0;
}

TEST_FUNC(request_get_query_param) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    strcpy(request.url, "/search?q=test&lang=en");
    
    const char* q_param = uvhttp_request_get_query_param(&request, "q");
    TEST_ASSERT_STREQ("test", q_param);
    
    const char* lang_param = uvhttp_request_get_query_param(&request, "lang");
    TEST_ASSERT_STREQ("en", lang_param);
    
    const char* missing_param = uvhttp_request_get_query_param(&request, "missing");
    TEST_ASSERT_NULL(missing_param);
    
    return 0;
}

TEST_FUNC(request_get_header) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    /* 添加测试header */
    strcpy(request.headers[0].name, "Content-Type");
    strcpy(request.headers[0].value, "application/json");
    request.header_count = 1;
    
    const char* content_type = uvhttp_request_get_header(&request, "Content-Type");
    TEST_ASSERT_STREQ("application/json", content_type);
    
    const char* missing = uvhttp_request_get_header(&request, "Missing-Header");
    TEST_ASSERT_NULL(missing);
    
    return 0;
}

TEST_FUNC(request_get_body) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* test_body = "Hello, World!";
    size_t body_len = strlen(test_body);
    
    request.body = uvhttp_malloc(body_len + 1);
    strcpy(request.body, test_body);
    request.body_length = body_len;
    
    const char* body = uvhttp_request_get_body(&request);
    TEST_ASSERT_STREQ(test_body, body);
    TEST_ASSERT_EQ(body_len, uvhttp_request_get_body_length(&request));
    
    uvhttp_free(request.body);
    
    return 0;
}

TEST_SUITE(request) {
    TEST_CASE(request_create_cleanup);
    TEST_CASE(request_get_method);
    TEST_CASE(request_get_path_simple);
    TEST_CASE(request_get_path_with_query);
    TEST_CASE(request_get_path_null_request);
    TEST_CASE(request_get_query_param);
    TEST_CASE(request_get_header);
    TEST_CASE(request_get_body);
    
    END_TEST_SUITE();
}