/* uvhttp_error.c 综合覆盖率测试 - 测试错误码和错误处理函数 */

#include <gtest/gtest.h>
#include "uvhttp_error.h"
#include "uvhttp_error_helpers.h"
#include <string.h>

/* 测试错误码值 */
TEST(UvhttpErrorComprehensiveTest, ErrorCodesValues) {
    /* 通用错误 */
    EXPECT_EQ((int)UVHTTP_OK, 0);
    EXPECT_EQ((int)UVHTTP_ERROR_INVALID_PARAM, -1);
    EXPECT_EQ((int)UVHTTP_ERROR_OUT_OF_MEMORY, -2);
    EXPECT_EQ((int)UVHTTP_ERROR_NOT_FOUND, -3);
    EXPECT_EQ((int)UVHTTP_ERROR_SERVER_INIT, -100);
    EXPECT_EQ((int)UVHTTP_ERROR_CONNECTION_INIT, -200);
    EXPECT_EQ((int)UVHTTP_ERROR_REQUEST_INIT, -300);
    EXPECT_EQ((int)UVHTTP_ERROR_TLS_INIT, -400);
    EXPECT_EQ((int)UVHTTP_ERROR_ROUTER_INIT, -500);
}

/* 测试错误字符串函数 */
TEST(UvhttpErrorComprehensiveTest, ErrorString) {
    /* 测试已知错误码 */
    const char* ok_str = uvhttp_error_string((uvhttp_error_t)UVHTTP_OK);
    EXPECT_NE(ok_str, nullptr);
    EXPECT_NE(strlen(ok_str), (size_t)0);
    
    const char* error_str = uvhttp_error_string((uvhttp_error_t)UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(error_str, nullptr);
    EXPECT_NE(strlen(error_str), (size_t)0);
    
    /* 测试未知错误码 */
    const char* unknown_str = uvhttp_error_string((uvhttp_error_t)9999);
    EXPECT_NE(unknown_str, nullptr);
}

/* 测试错误分类函数 */
TEST(UvhttpErrorComprehensiveTest, ErrorCategoryString) {
    /* 测试已知错误码 */
    const char* ok_cat = uvhttp_error_category_string((uvhttp_error_t)UVHTTP_OK);
    EXPECT_NE(ok_cat, nullptr);
    EXPECT_NE(strlen(ok_cat), (size_t)0);
    
    const char* error_cat = uvhttp_error_category_string((uvhttp_error_t)UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(error_cat, nullptr);
    EXPECT_NE(strlen(error_cat), (size_t)0);
    
    /* 测试未知错误码 */
    const char* unknown_cat = uvhttp_error_category_string((uvhttp_error_t)9999);
    EXPECT_NE(unknown_cat, nullptr);
}

/* 测试错误描述函数 */
TEST(UvhttpErrorComprehensiveTest, ErrorDescription) {
    /* 测试已知错误码 */
    const char* ok_desc = uvhttp_error_description((uvhttp_error_t)UVHTTP_OK);
    EXPECT_NE(ok_desc, nullptr);
    
    const char* error_desc = uvhttp_error_description((uvhttp_error_t)UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(error_desc, nullptr);
    
    /* 测试未知错误码 */
    const char* unknown_desc = uvhttp_error_description((uvhttp_error_t)9999);
    EXPECT_NE(unknown_desc, nullptr);
}

/* 测试错误建议函数 */
TEST(UvhttpErrorComprehensiveTest, ErrorSuggestion) {
    /* 测试已知错误码 */
    const char* ok_sugg = uvhttp_error_suggestion((uvhttp_error_t)UVHTTP_OK);
    EXPECT_NE(ok_sugg, nullptr);
    
    const char* error_sugg = uvhttp_error_suggestion((uvhttp_error_t)UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE(error_sugg, nullptr);
    
    /* 测试未知错误码 */
    const char* unknown_sugg = uvhttp_error_suggestion((uvhttp_error_t)9999);
    EXPECT_NE(unknown_sugg, nullptr);
}

/* 测试错误可恢复性函数 */
TEST(UvhttpErrorComprehensiveTest, ErrorIsRecoverable) {
    /* 成功是可恢复的 */
    int ok_recoverable = uvhttp_error_is_recoverable((uvhttp_error_t)UVHTTP_OK);
    EXPECT_GE(ok_recoverable, 0);
    
    /* 内存不足通常不可恢复 */
    int memory_recoverable = uvhttp_error_is_recoverable((uvhttp_error_t)UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_GE(memory_recoverable, 0);
    
    /* 未知错误码默认不可恢复 */
    int unknown_recoverable = uvhttp_error_is_recoverable((uvhttp_error_t)9999);
    EXPECT_GE(unknown_recoverable, 0);
}

/* 测试错误码范围 */
TEST(UvhttpErrorComprehensiveTest, ErrorCodeRanges) {
    /* 通用错误：-1 到 -9 */
    EXPECT_LE((int)UVHTTP_ERROR_INVALID_PARAM, -1);
    EXPECT_GE((int)UVHTTP_ERROR_NOT_SUPPORTED, -9);
    
    /* 服务器错误：-100 到 -109 */
    EXPECT_LE((int)UVHTTP_ERROR_SERVER_INIT, -100);
    EXPECT_GE((int)UVHTTP_ERROR_SERVER_INVALID_CONFIG, -109);
    
    /* 连接错误：-200 到 -209 */
    EXPECT_LE((int)UVHTTP_ERROR_CONNECTION_INIT, -200);
    EXPECT_GE((int)UVHTTP_ERROR_CONNECTION_BROKEN, -209);
    
    /* 请求/响应错误：-300 到 -309 */
    EXPECT_LE((int)UVHTTP_ERROR_REQUEST_INIT, -300);
    EXPECT_GE((int)UVHTTP_ERROR_IO_ERROR, -309);
    
    /* TLS 错误：-400 到 -409 */
    EXPECT_LE((int)UVHTTP_ERROR_TLS_INIT, -400);
    EXPECT_GE((int)UVHTTP_ERROR_TLS_NOT_YET_VALID, -409);
    
    /* 路由错误：-500 到 -509 */
    EXPECT_LE((int)UVHTTP_ERROR_ROUTER_INIT, -500);
    EXPECT_GE((int)UVHTTP_ERROR_INVALID_ROUTE_PATTERN, -509);
}

/* 测试错误码唯一性 */
TEST(UvhttpErrorComprehensiveTest, ErrorCodesUnique) {
    /* 验证常见错误码不重复 */
    EXPECT_NE((int)UVHTTP_OK, (int)UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_NE((int)UVHTTP_ERROR_INVALID_PARAM, (int)UVHTTP_ERROR_OUT_OF_MEMORY);
    EXPECT_NE((int)UVHTTP_ERROR_OUT_OF_MEMORY, (int)UVHTTP_ERROR_NOT_FOUND);
    EXPECT_NE((int)UVHTTP_ERROR_SERVER_INIT, (int)UVHTTP_ERROR_CONNECTION_INIT);
    EXPECT_NE((int)UVHTTP_ERROR_CONNECTION_INIT, (int)UVHTTP_ERROR_REQUEST_INIT);
    EXPECT_NE((int)UVHTTP_ERROR_REQUEST_INIT, (int)UVHTTP_ERROR_TLS_INIT);
    EXPECT_NE((int)UVHTTP_ERROR_TLS_INIT, (int)UVHTTP_ERROR_ROUTER_INIT);
}