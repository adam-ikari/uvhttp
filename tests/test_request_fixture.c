#include "../deps/googletest/gtest_fixed.h"
#include "../include/uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>

// 定义HTTP请求测试夹具
typedef struct {
    char* test_buffer;
    size_t buffer_size;
    int setup_completed;
} HttpRequestTest;

// 定义测试夹具类
DEFINE_TEST_F(HttpRequestTest);

void HttpRequestTest_setup(HttpRequestTest* fixture) {
    fixture->buffer_size = 1024;
    fixture->test_buffer = malloc(fixture->buffer_size);
    fixture->setup_completed = 1;
}

void HttpRequestTest_teardown(HttpRequestTest* fixture) {
    if (fixture->test_buffer) {
        free(fixture->test_buffer);
        fixture->test_buffer = NULL;
    }
    fixture->setup_completed = 0;
}

// 使用TEST_F的测试用例
TEST_F(HttpRequestTest, BufferInitialization) {
    EXPECT_NOTNULL_PTR(fixture->test_buffer);
    EXPECT_EQ(fixture->buffer_size, 1024);
    EXPECT_EQ(fixture->setup_completed, 1);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST_F(HttpRequestTest, SafeStrncpyWithFixture) {
    // 使用夹具提供的缓冲区
    EXPECT_EQ(safe_strncpy(fixture->test_buffer, "test message", fixture->buffer_size), 0);
    EXPECT_STREQ(fixture->test_buffer, "test message");
    
TEST_CLEANUP_LABEL:
    return;
}

TEST_F(HttpRequestTest, UrlValidationWithFixture) {
    // 测试URL验证功能
    EXPECT_EQ(validate_url("http://example.com", 18), 0);
    EXPECT_EQ(validate_url("/api/v1/users", 14), 0);
    EXPECT_EQ(validate_url("", 0), -1);
    
TEST_CLEANUP_LABEL:
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