#include <gtest/gtest.h>
#include "uvhttp_request.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 手动初始化请求结构 */
static void init_request(uvhttp_request_t* request) {
    memset(request, 0, sizeof(uvhttp_request_t));
    request->method = UVHTTP_GET;
    request->parsing_complete = 0;
    request->header_count = 0;
    request->body_length = 0;
    request->body_capacity = 0;
    request->parser = NULL;
    request->parser_settings = NULL;
    request->path = NULL;
    request->query = NULL;
    request->body = NULL;
    request->user_data = NULL;
    request->headers_extra = NULL;
    request->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY;
}

/* 测试 uvhttp_request_add_header - 成功添加 header */
TEST(UvhttpRequestApiTest, AddHeaderSuccess) {
    uvhttp_request_t request;
    init_request(&request);
    
    /* 添加 header */
    uvhttp_error_t result = uvhttp_request_add_header(&request, "Content-Type", "application/json");
    EXPECT_EQ(result, 0);
    EXPECT_EQ(request.header_count, 1);
    
    /* 验证 header 内容 */
    const char* value = uvhttp_request_get_header(&request, "Content-Type");
    EXPECT_STREQ(value, "application/json");
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_add_header - NULL 请求 */
TEST(UvhttpRequestApiTest, AddHeaderNullRequest) {
    uvhttp_error_t result = uvhttp_request_add_header(NULL, "Content-Type", "application/json");
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_request_add_header - NULL 名称 */
TEST(UvhttpRequestApiTest, AddHeaderNullName) {
    uvhttp_request_t request;
    init_request(&request);
    
    uvhttp_error_t result = uvhttp_request_add_header(&request, NULL, "application/json");
    EXPECT_NE(result, 0);
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_add_header - NULL 值 */
TEST(UvhttpRequestApiTest, AddHeaderNullValue) {
    uvhttp_request_t request;
    init_request(&request);
    
    uvhttp_error_t result = uvhttp_request_add_header(&request, "Content-Type", NULL);
    EXPECT_NE(result, 0);
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_add_header - 多个 header */
TEST(UvhttpRequestApiTest, AddHeaderMultiple) {
    uvhttp_request_t request;
    init_request(&request);
    
    /* 添加多个 header */
    ASSERT_EQ(uvhttp_request_add_header(&request, "Content-Type", "application/json"), 0);
    ASSERT_EQ(uvhttp_request_add_header(&request, "Authorization", "Bearer token"), 0);
    ASSERT_EQ(uvhttp_request_add_header(&request, "User-Agent", "uvhttp/2.2.0"), 0);
    
    EXPECT_EQ(request.header_count, 3);
    
    /* 验证所有 header */
    EXPECT_STREQ(uvhttp_request_get_header(&request, "Content-Type"), "application/json");
    EXPECT_STREQ(uvhttp_request_get_header(&request, "Authorization"), "Bearer token");
    EXPECT_STREQ(uvhttp_request_get_header(&request, "User-Agent"), "uvhttp/2.2.0");
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_add_header - 超过内联容量 */
TEST(UvhttpRequestApiTest, AddHeaderExceedInlineCapacity) {
    uvhttp_request_t request;
    init_request(&request);
    
    /* 添加超过内联容量的 header */
    for (int i = 0; i < 50; i++) {
        char name[32];
        char value[32];
        snprintf(name, sizeof(name), "Header-%d", i);
        snprintf(value, sizeof(value), "Value-%d", i);
        ASSERT_EQ(uvhttp_request_add_header(&request, name, value), 0);
    }
    
    EXPECT_EQ(request.header_count, 50);
    
    /* 验证第一个和最后一个 header */
    EXPECT_STREQ(uvhttp_request_get_header(&request, "Header-0"), "Value-0");
    EXPECT_STREQ(uvhttp_request_get_header(&request, "Header-49"), "Value-49");
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_foreach_header - 遍历所有 header */
TEST(UvhttpRequestApiTest, ForeachHeader) {
    uvhttp_request_t request;
    init_request(&request);
    
    /* 添加多个 header */
    ASSERT_EQ(uvhttp_request_add_header(&request, "Content-Type", "application/json"), 0);
    ASSERT_EQ(uvhttp_request_add_header(&request, "Authorization", "Bearer token"), 0);
    ASSERT_EQ(uvhttp_request_add_header(&request, "User-Agent", "uvhttp/2.2.0"), 0);
    
    /* 遍历 header */
    int count = 0;
    uvhttp_request_foreach_header(&request, 
        [](const char* name, const char* value, void* user_data) {
            int* counter = (int*)user_data;
            (*counter)++;
            return;
        }, 
        &count
    );
    
    EXPECT_EQ(count, 3);
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_foreach_header - NULL 请求 */
TEST(UvhttpRequestApiTest, ForeachHeaderNullRequest) {
    int count = 0;
    uvhttp_request_foreach_header(NULL, 
        [](const char* name, const char* value, void* user_data) {
            return;
        }, 
        &count
    );
    EXPECT_EQ(count, 0);
}

/* 测试 uvhttp_request_foreach_header - NULL 回调 */
TEST(UvhttpRequestApiTest, ForeachHeaderNullCallback) {
    uvhttp_request_t request;
    init_request(&request);
    
    /* NULL 回调应该不会崩溃 */
    uvhttp_request_foreach_header(&request, NULL, NULL);
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_get_header_at - 获取指定索引的 header */
TEST(UvhttpRequestApiTest, GetHeaderAt) {
    uvhttp_request_t request;
    init_request(&request);
    
    /* 添加多个 header */
    ASSERT_EQ(uvhttp_request_add_header(&request, "Content-Type", "application/json"), 0);
    ASSERT_EQ(uvhttp_request_add_header(&request, "Authorization", "Bearer token"), 0);
    
    /* 获取指定索引的 header */
    uvhttp_header_t* header0 = uvhttp_request_get_header_at(&request, 0);
    ASSERT_NE(header0, nullptr);
    EXPECT_STREQ(header0->name, "Content-Type");
    EXPECT_STREQ(header0->value, "application/json");
    
    uvhttp_header_t* header1 = uvhttp_request_get_header_at(&request, 1);
    ASSERT_NE(header1, nullptr);
    EXPECT_STREQ(header1->name, "Authorization");
    EXPECT_STREQ(header1->value, "Bearer token");
    
    /* 超出范围的索引 */
    uvhttp_header_t* header_invalid = uvhttp_request_get_header_at(&request, 100);
    EXPECT_EQ(header_invalid, nullptr);
    
    uvhttp_request_cleanup(&request);
}

/* 测试 uvhttp_request_get_header_at - NULL 请求 */
TEST(UvhttpRequestApiTest, GetHeaderAtNullRequest) {
    uvhttp_header_t* header = uvhttp_request_get_header_at(NULL, 0);
    EXPECT_EQ(header, nullptr);
}