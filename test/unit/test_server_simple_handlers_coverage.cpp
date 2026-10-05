/* UVHTTP 服务器简单处理器覆盖率测试 */

#include "uvhttp_allocator.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"

#include "uvhttp.h"

#include <fcntl.h>
#include <gtest/gtest.h>
#include <string.h>

/* 测试快速响应 */
TEST(UvhttpServerSimpleHandlersTest, QuickResponse) {
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
}

/* 测试获取参数 */
TEST(UvhttpServerSimpleHandlersTest, GetParam) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));

    /* 设置查询字符串 */
    strcpy(request.url, "/test?name=value&test=123");
    request.path = request.url;

    /* 获取参数 */
    const char* param = uvhttp_get_param(&request, "name");
    /* probe 实测正常返回。去掉 if(param) 守卫——param 为 NULL 时应显式
     * 失败而非跳过断言（恒绿假阳性）。 */
    ASSERT_NE(param, nullptr);
    EXPECT_STREQ(param, "value");
}

/* 测试获取参数 NULL 请求 */
TEST(UvhttpServerSimpleHandlersTest, GetParamNullRequest) {
    const char* param = uvhttp_get_param(NULL, "name");
    EXPECT_EQ(param, nullptr);
}

/* 测试获取参数 NULL 名称 */
TEST(UvhttpServerSimpleHandlersTest, GetParamNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));

    const char* param = uvhttp_get_param(&request, NULL);
    EXPECT_EQ(param, nullptr);
}

/* 测试获取请求头 */
TEST(UvhttpServerSimpleHandlersTest, GetHeader) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));

    /* 添加请求头 */
    strcpy(request.headers[0].name, "Content-Type");
    strcpy(request.headers[0].value, "application/json");
    request.header_count = 1;

    /* 获取请求头 */
    /* 手填 request.headers[0] 不被 uvhttp_request_get_header 识别（它查的是
     * add_header 走的存储路径，probe 实测返回 NULL）——原 if(header) 断言
     * 恒被跳过。改用 add_header 构造真实可查询的 header。 */
    ASSERT_EQ(
        uvhttp_request_add_header(&request, "Content-Type", "application/json"),
        UVHTTP_OK);
    const char* header = uvhttp_get_header(&request, "Content-Type");
    ASSERT_NE(header, nullptr);
    EXPECT_STREQ(header, "application/json");
}

/* 测试获取请求头 NULL 请求 */
TEST(UvhttpServerSimpleHandlersTest, GetHeaderNullRequest) {
    const char* header = uvhttp_get_header(NULL, "Content-Type");
    EXPECT_EQ(header, nullptr);
}

/* 测试获取请求头 NULL 名称 */
TEST(UvhttpServerSimpleHandlersTest, GetHeaderNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));

    const char* header = uvhttp_get_header(&request, NULL);
    EXPECT_EQ(header, nullptr);
}

/* 测试获取请求体 */
TEST(UvhttpServerSimpleHandlersTest, GetBody) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));

    /* 设置请求体 */
    char body[] = "Test body content";
    request.body = body;
    request.body_length = strlen(body);

    /* 获取请求体 */
    const char* body_content = uvhttp_get_body(&request);
    ASSERT_NE(body_content, nullptr);
    EXPECT_STREQ(body_content, "Test body content");
}

/* 测试获取请求体 NULL 请求 */
TEST(UvhttpServerSimpleHandlersTest, GetBodyNullRequest) {
    const char* body = uvhttp_get_body(NULL);
    EXPECT_EQ(body, nullptr);
}