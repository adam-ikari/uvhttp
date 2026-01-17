/* UVHTTP 响应额外覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"

/* 测试 uvhttp_response_build_for_test */
TEST(UvhttpResponseExtraTest, ResponseBuildForTest) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应状态和头部 */
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    uvhttp_response_set_body(&response, "Hello, World!", 13);
    
    /* 构建响应数据（用于测试） */
    char* data;
    size_t data_size;
    uvhttp_error_t result = uvhttp_response_build_for_test(&response, &data, &data_size);
    
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);
    
    /* 验证响应数据包含关键内容 */
    EXPECT_NE(strstr(data, "HTTP/1.1 200"), nullptr);
    EXPECT_NE(strstr(data, "Content-Type: text/plain"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_build_for_test NULL 响应 */
TEST(UvhttpResponseExtraTest, ResponseBuildForTestNullResponse) {
    char* data;
    size_t data_size;
    uvhttp_error_t result = uvhttp_response_build_for_test(NULL, &data, &data_size);
    
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试 uvhttp_response_build_for_test NULL 数据大小 */
TEST(UvhttpResponseExtraTest, ResponseBuildForTestNullDataSize) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    char* data;
    uvhttp_error_t result = uvhttp_response_build_for_test(&response, &data, NULL);
    
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_send_mock */
TEST(UvhttpResponseExtraTest, ResponseSendMock) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应内容 */
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_body(&response, "OK", 2);
    
    /* 使用 mock 发送响应 */
    int result = uvhttp_response_send_mock(&response);
    
    /* 应该返回成功 */
    EXPECT_EQ(result, 0);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_send_mock NULL 响应 */
TEST(UvhttpResponseExtraTest, ResponseSendMockNullResponse) {
    int result = uvhttp_response_send_mock(NULL);
    
    /* 应该返回失败 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_response_build_for_test 空响应 */
TEST(UvhttpResponseExtraTest, ResponseBuildForTestEmptyResponse) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应状态但不设置头部和主体 */
    uvhttp_response_set_status(&response, 204);
    
    /* 构建响应数据 */
    char* data;
    size_t data_size;
    uvhttp_error_t result = uvhttp_response_build_for_test(&response, &data, &data_size);
    
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);
    
    /* 验证响应数据包含状态码 */
    EXPECT_NE(strstr(data, "HTTP/1.1 204"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_build_for_test 大响应 */
TEST(UvhttpResponseExtraTest, ResponseBuildForTestLargeResponse) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应状态和大量头部 */
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    uvhttp_response_set_header(&response, "Cache-Control", "no-cache");
    uvhttp_response_set_header(&response, "Pragma", "no-cache");
    uvhttp_response_set_header(&response, "Expires", "0");
    
    /* 设置大响应体 */
    char large_body[1024];
    memset(large_body, 'A', sizeof(large_body) - 1);
    large_body[sizeof(large_body) - 1] = '\0';
    uvhttp_response_set_body(&response, large_body, strlen(large_body));
    
    /* 构建响应数据 */
    char* data;
    size_t data_size;
    uvhttp_error_t result = uvhttp_response_build_for_test(&response, &data, &data_size);
    
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);
    
    /* 验证响应数据包含所有头部 */
    EXPECT_NE(strstr(data, "Content-Type: text/plain"), nullptr);
    EXPECT_NE(strstr(data, "Cache-Control: no-cache"), nullptr);
    
    /* 释放数据 */
    uvhttp_free(data);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_build_for_test 多次调用 */
TEST(UvhttpResponseExtraTest, ResponseBuildForTestMultipleCalls) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应内容 */
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    uvhttp_response_set_body(&response, "Test", 4);
    
    /* 多次构建响应数据 */
    for (int i = 0; i < 3; i++) {
        char* data;
        size_t data_size;
        uvhttp_error_t result = uvhttp_response_build_for_test(&response, &data, &data_size);
        
        EXPECT_EQ(result, UVHTTP_OK);
        ASSERT_NE(data, nullptr);
        EXPECT_GT(data_size, 0);
        
        /* 释放数据 */
        uvhttp_free(data);
    }
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_build_for_test 特殊字符 */
TEST(UvhttpResponseExtraTest, ResponseBuildForTestSpecialChars) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应内容包含特殊字符 */
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_body(&response, "Hello\r\n\tWorld", 14);
    
    /* 构建响应数据 */
    char* data;
    size_t data_size;
    uvhttp_error_t result = uvhttp_response_build_for_test(&response, &data, &data_size);
    
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_GT(data_size, 0);
    
    /* 释放数据 */
    uvhttp_free(data);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_send_mock 已发送的响应 */
TEST(UvhttpResponseExtraTest, ResponseSendMockAlreadySent) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置响应内容 */
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_body(&response, "OK", 2);
    
    /* 标记为已发送 */
    response.sent = 1;
    
    /* 使用 mock 发送响应 */
    int result = uvhttp_response_send_mock(&response);
    
    /* 应该返回成功（mock 函数不会检查是否已发送） */
    EXPECT_EQ(result, 0);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}

/* 测试 uvhttp_response_send_mock 空响应 */
TEST(UvhttpResponseExtraTest, ResponseSendMockEmptyResponse) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 不设置任何内容 */
    
    /* 使用 mock 发送响应 */
    int result = uvhttp_response_send_mock(&response);
    
    /* 应该返回成功（即使没有内容） */
    EXPECT_EQ(result, 0);
    
    /* 清理响应 */
    uvhttp_response_cleanup(&response);
}