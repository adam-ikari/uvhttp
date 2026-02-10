#include <gtest/gtest.h>
#include "uvhttp_request.h"
#include "uvhttp_allocator.h"

/* 测试请求方法获取 */
TEST(UvhttpRequestComprehensiveTest, GetMethod) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试各种方法 */
    request->method = UVHTTP_GET;
    EXPECT_STREQ(uvhttp_request_get_method(request), "GET");
    
    request->method = UVHTTP_POST;
    EXPECT_STREQ(uvhttp_request_get_method(request), "POST");
    
    request->method = UVHTTP_PUT;
    EXPECT_STREQ(uvhttp_request_get_method(request), "PUT");
    
    request->method = UVHTTP_DELETE;
    EXPECT_STREQ(uvhttp_request_get_method(request), "DELETE");
    
    request->method = UVHTTP_HEAD;
    EXPECT_STREQ(uvhttp_request_get_method(request), "HEAD");
    
    request->method = UVHTTP_OPTIONS;
    EXPECT_STREQ(uvhttp_request_get_method(request), "OPTIONS");
    
    request->method = UVHTTP_PATCH;
    EXPECT_STREQ(uvhttp_request_get_method(request), "PATCH");
    
    request->method = UVHTTP_ANY;
    EXPECT_STREQ(uvhttp_request_get_method(request), "ANY");
    
    /* 测试无效方法 */
    request->method = (uvhttp_method_t)999;
    EXPECT_STREQ(uvhttp_request_get_method(request), "UNKNOWN");
    
    /* 测试 NULL 请求 */
    EXPECT_EQ(uvhttp_request_get_method(NULL), nullptr);
    
    uvhttp_free(request);
}

/* 测试 URL 获取 */
TEST(UvhttpRequestComprehensiveTest, GetUrl) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试空 URL（未初始化的 url 字段） */
    const char* url = uvhttp_request_get_url(request);
    /* url 字段是结构体中的数组，不是指针 */
    EXPECT_STREQ(url, "");
    
    /* 测试设置的 URL */
    strcpy(request->url, "/api/users");
    url = uvhttp_request_get_url(request);
    EXPECT_STREQ(url, "/api/users");
    
    /* 测试 NULL 请求 */
    url = uvhttp_request_get_url(NULL);
    EXPECT_EQ(url, nullptr);
    
    uvhttp_free(request);
}

/* 测试路径获取 */
TEST(UvhttpRequestComprehensiveTest, GetPath) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试空 URL */
    const char* path = uvhttp_request_get_path(request);
    /* url 字段是结构体中的数组，不是指针 */
    EXPECT_STREQ(path, "");
    
    /* 测试设置的路径（无查询参数） */
    strcpy(request->url, "/api/users");
    path = uvhttp_request_get_path(request);
    EXPECT_STREQ(path, "/api/users");
    
    /* 测试带查询参数的路径 */
    strcpy(request->url, "/api/users?id=123");
    path = uvhttp_request_get_path(request);
    EXPECT_STREQ(path, "/api/users");
    
    /* 测试 NULL 请求 */
    path = uvhttp_request_get_path(NULL);
    EXPECT_EQ(path, nullptr);
    
    /* Test path exceeding buffer size */
    memset(request->url, 'a', UVHTTP_MAX_URL_SIZE - 1);
    request->url[UVHTTP_MAX_URL_SIZE - 1] = '\0';
    path = uvhttp_request_get_path(request);
    /* Path exceeds UVHTTP_MAX_PATH_SIZE (1024), returns url itself */
    EXPECT_EQ(path, request->url);
    
    /* 测试无效路径（包含 ..） */
    strcpy(request->url, "/api/../../etc/passwd");
    path = uvhttp_request_get_path(request);
    /* 路径验证失败，返回 url 本身 */
    EXPECT_STREQ(path, "/api/../../etc/passwd");
    
    /* 测试包含危险字符的路径（< > : " | *） */
    strcpy(request->url, "/api/users/<test>");
    path = uvhttp_request_get_path(request);
    /* 路径验证失败，返回 url 本身 */
    EXPECT_STREQ(path, "/api/users/<test>");
    
    uvhttp_free(request);
}

/* 测试查询字符串获取 */
TEST(UvhttpRequestComprehensiveTest, GetQueryString) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试 NULL 查询字符串 */
    const char* query = uvhttp_request_get_query_string(request);
    EXPECT_EQ(query, nullptr);
    
    /* 测试设置的查询字符串 */
    strcpy(request->url, "/api/users?id=123&name=test");
    query = uvhttp_request_get_query_string(request);
    EXPECT_STREQ(query, "id=123&name=test");
    
    /* 测试 NULL 请求 */
    query = uvhttp_request_get_query_string(NULL);
    EXPECT_EQ(query, nullptr);
    
    /* 测试无效查询字符串（包含非法字符） */
    strcpy(request->url, "/api/users?id=<script>");
    query = uvhttp_request_get_query_string(request);
    EXPECT_EQ(query, nullptr);
    
    uvhttp_free(request);
}

/* 测试查询参数获取 */
TEST(UvhttpRequestComprehensiveTest, GetQueryParam) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试 NULL 查询字符串 */
    const char* param = uvhttp_request_get_query_param(request, "id");
    EXPECT_EQ(param, nullptr);
    
    /* 测试有查询字符串且参数存在 */
    strcpy(request->url, "/api/users?id=123&name=test");
    param = uvhttp_request_get_query_param(request, "id");
    EXPECT_STREQ(param, "123");
    
    param = uvhttp_request_get_query_param(request, "name");
    EXPECT_STREQ(param, "test");
    
    /* 测试参数不存在 */
    param = uvhttp_request_get_query_param(request, "age");
    EXPECT_EQ(param, nullptr);
    
    /* 测试 NULL 请求 */
    param = uvhttp_request_get_query_param(NULL, "id");
    EXPECT_EQ(param, nullptr);
    
    /* 测试 NULL 参数名 */
    param = uvhttp_request_get_query_param(request, NULL);
    EXPECT_EQ(param, nullptr);
    
    uvhttp_free(request);
}

/* 测试请求释放 */
TEST(UvhttpRequestComprehensiveTest, RequestFree) {
    /* 测试释放 NULL 请求 */
    uvhttp_request_free(NULL);
    
    /* 测试释放正常请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 分配一些内存 */
    request->path = (char*)uvhttp_alloc(256);
    request->query = (char*)uvhttp_alloc(256);
    request->body = (char*)uvhttp_alloc(1024);
    
    /* 释放请求 */
    uvhttp_request_free(request);
    
    /* 验证请求已被释放（不能访问已释放的内存） */
}

/* 测试请求清理 */
TEST(UvhttpRequestComprehensiveTest, RequestCleanup) {
    /* 测试清理 NULL 请求 */
    uvhttp_request_cleanup(NULL);
    
    /* 测试清理正常请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 分配一些内存 */
    request->path = (char*)uvhttp_alloc(256);
    request->query = (char*)uvhttp_alloc(256);
    request->body = (char*)uvhttp_alloc(1024);
    
    /* 注意：uvhttp_request_cleanup 只释放 body、parser、parser_settings、headers_extra */
    /* 不释放 path 和 query */
    uvhttp_request_cleanup(request);
    
    /* 验证 body 已被清理（指针未设置为 NULL） */
    /* uvhttp_request_cleanup 不会将指针设置为 NULL，只是释放内存 */
    /* 所以我们需要验证内存已经被释放，但这会导致未定义行为 */
    /* 因此我们只验证函数不会崩溃即可 */
    
    /* 手动清理 path 和 query */
    uvhttp_free(request->path);
    uvhttp_free(request->query);
    uvhttp_free(request);
}

/* 测试 Header 获取 */
TEST(UvhttpRequestComprehensiveTest, GetHeader) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试 NULL 请求 */
    const char* header = uvhttp_request_get_header(NULL, "Content-Type");
    EXPECT_EQ(header, nullptr);
    
    /* 测试 NULL header 名称 */
    header = uvhttp_request_get_header(request, NULL);
    EXPECT_EQ(header, nullptr);
    
    /* 测试空 header 名称 */
    header = uvhttp_request_get_header(request, "");
    EXPECT_EQ(header, nullptr);
    
    /* 添加一些 headers */
    request->header_count = 2;
    request->headers_capacity = 2;
    strcpy(request->headers[0].name, "Content-Type");
    strcpy(request->headers[0].value, "application/json");
    
    strcpy(request->headers[1].name, "Authorization");
    strcpy(request->headers[1].value, "Bearer token");
    
    /* 测试获取 header */
    header = uvhttp_request_get_header(request, "Content-Type");
    EXPECT_STREQ(header, "application/json");
    
    header = uvhttp_request_get_header(request, "Authorization");
    EXPECT_STREQ(header, "Bearer token");
    
    /* 测试不存在的 header */
    header = uvhttp_request_get_header(request, "User-Agent");
    EXPECT_EQ(header, nullptr);
    
    /* 测试大小写不敏感 */
    header = uvhttp_request_get_header(request, "content-type");
    EXPECT_STREQ(header, "application/json");
    
    uvhttp_free(request);
}

/* 测试请求体获取 */
TEST(UvhttpRequestComprehensiveTest, GetBody) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试 NULL 请求 */
    const char* body = uvhttp_request_get_body(NULL);
    EXPECT_EQ(body, nullptr);
    
    /* 测试空请求体 */
    body = uvhttp_request_get_body(request);
    EXPECT_EQ(body, nullptr);
    
    /* 测试设置的请求体 */
    request->body = (char*)uvhttp_alloc(1024);
    strcpy(request->body, "Hello, World!");
    request->body_length = 13;
    
    body = uvhttp_request_get_body(request);
    EXPECT_STREQ(body, "Hello, World!");
    
    /* 测试请求体长度 */
    size_t length = uvhttp_request_get_body_length(request);
    EXPECT_EQ(length, 13);
    
    /* 测试 NULL 请求的长度 */
    length = uvhttp_request_get_body_length(NULL);
    EXPECT_EQ(length, 0);
    
    uvhttp_free(request->body);
    uvhttp_free(request);
}

/* 测试 Header 数量获取 */
TEST(UvhttpRequestComprehensiveTest, GetHeaderCount) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试空 headers */
    size_t count = uvhttp_request_get_header_count(request);
    EXPECT_EQ(count, 0);
    
    /* 测试设置 headers 数量 */
    request->header_count = 3;
    count = uvhttp_request_get_header_count(request);
    EXPECT_EQ(count, 3);
    
    /* 测试 NULL 请求 */
    count = uvhttp_request_get_header_count(NULL);
    EXPECT_EQ(count, 0);
    
    uvhttp_free(request);
}

/* 测试客户端 IP 获取 */
TEST(UvhttpRequestComprehensiveTest, GetClientIp) {
    /* 创建测试请求 */
    uvhttp_request_t* request = (uvhttp_request_t*)uvhttp_alloc(sizeof(uvhttp_request_t));
    ASSERT_NE(request, nullptr);
    memset(request, 0, sizeof(uvhttp_request_t));
    
    /* 测试 NULL 请求 */
    const char* ip = uvhttp_request_get_client_ip(NULL);
    EXPECT_EQ(ip, nullptr);
    
    /* 测试没有连接的请求 */
    ip = uvhttp_request_get_client_ip(request);
    EXPECT_STREQ(ip, "127.0.0.1");
    
    /* 测试有 X-Forwarded-For header */
    request->header_count = 1;
    request->headers_capacity = 1;
    strcpy(request->headers[0].name, "X-Forwarded-For");
    strcpy(request->headers[0].value, "192.168.1.1");
    ip = uvhttp_request_get_client_ip(request);
    EXPECT_STREQ(ip, "192.168.1.1");
    
    /* 测试有多个 IP 的 X-Forwarded-For */
    strcpy(request->headers[0].value, "192.168.1.1, 10.0.0.1");
    ip = uvhttp_request_get_client_ip(request);
    EXPECT_STREQ(ip, "192.168.1.1");
    
    uvhttp_free(request);
}