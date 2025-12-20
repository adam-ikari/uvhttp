#include "../deps/googletest/gtest_fixed.h"
#include "../include/uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>

// 简化的测试夹具示例
typedef struct {
    char* test_buffer;
    size_t buffer_size;
    int setup_completed;
} HttpRequestTest;

// 全局测试夹具实例
static HttpRequestTest* g_fixture = NULL;

// 设置函数
static void setup_http_request_test() {
    g_fixture = malloc(sizeof(HttpRequestTest));
    if (g_fixture) {
        g_fixture->buffer_size = 1024;
        g_fixture->test_buffer = malloc(g_fixture->buffer_size);
        g_fixture->setup_completed = 1;
    }
}

// 清理函数
static void teardown_http_request_test() {
    if (g_fixture) {
        if (g_fixture->test_buffer) {
            free(g_fixture->test_buffer);
        }
        free(g_fixture);
        g_fixture = NULL;
    }
}

// 使用测试夹具的测试
TEST(HttpRequestTest, BufferInitialization) {
    setup_http_request_test();
    
    EXPECT_NOTNULL_PTR(g_fixture);
    EXPECT_NOTNULL_PTR(g_fixture->test_buffer);
    EXPECT_EQ(g_fixture->buffer_size, 1024);
    EXPECT_EQ(g_fixture->setup_completed, 1);
    
TEST_CLEANUP_LABEL:
    teardown_http_request_test();
    return;
}

TEST(HttpRequestTest, SafeStrncpyWithFixture) {
    setup_http_request_test();
    
    // 使用夹具提供的缓冲区
    EXPECT_EQ(safe_strncpy(g_fixture->test_buffer, "test message", g_fixture->buffer_size), 0);
    EXPECT_STREQ(g_fixture->test_buffer, "test message");
    
TEST_CLEANUP_LABEL:
    teardown_http_request_test();
    return;
}

// 普通测试用例（不使用夹具）
TEST(UtilsTest, BasicValidation) {
    char buffer[100];
    
    EXPECT_EQ(safe_strncpy(buffer, "hello", sizeof(buffer)), 0);
    EXPECT_STREQ(buffer, "hello");
    
TEST_CLEANUP_LABEL:
    return;
}

RUN_ALL_TESTS()