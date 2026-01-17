/* UVHTTP 查询字符串验证测试 */

#include <gtest/gtest.h>
#include "uvhttp_validation.h"

/* 测试简单查询字符串验证 */
TEST(UvhttpQueryValidationTest, SimpleQueryString) {
    const char* query = "key=value";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试多个键值对查询字符串验证 */
TEST(UvhttpQueryValidationTest, MultipleKeyValuePairs) {
    const char* query = "key1=value1&key2=value2";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试空查询字符串 */
TEST(UvhttpQueryValidationTest, EmptyQueryString) {
    const char* query = "";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试NULL查询字符串 */
TEST(UvhttpQueryValidationTest, NullQueryString) {
    int result = uvhttp_validate_query_string(NULL);
    EXPECT_EQ(result, 1);  // NULL 查询字符串是有效的
}

/* 测试无效查询字符串（缺少等号） */
TEST(UvhttpValidationTest, InvalidQueryStringNoEquals) {
    const char* query = "keyvalue";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);  // 简单字符串也是有效的
}

/* 测试无效查询字符串（缺少键） */
TEST(UvhttpQueryValidationTest, InvalidQueryStringNoKey) {
    const char* query = "=value";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);  // 只有值的查询字符串也是有效的
}

/* 测试无效查询字符串（缺少值） */
TEST(UvhttpQueryValidationTest, InvalidQueryStringNoValue) {
    const char* query = "key=";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);  // 只有键的查询字符串也是有效的
}

/* 测试查询字符串包含URL编码字符 */
TEST(UvhttpQueryValidationTest, QueryStringWithSpecialChars) {
    const char* query = "key=value%20with%20spaces";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含多个等号 */
TEST(UvhttpQueryValidationTest, QueryStringMultipleEquals) {
    const char* query = "key=value=test";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含多个&符号 */
TEST(UvhttpQueryValidationTest, QueryStringMultipleAmpersands) {
    const char* query = "key1=value1&&key2=value2";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);  // 多个 & 符号是允许的
}

/* 测试查询字符串包含URL编码字符 */
TEST(UvhttpQueryValidationTest, QueryStringWithURLEncoding) {
    const char* query = "key=value%20test";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含中文字符 */
TEST(UvhttpQueryValidationTest, QueryStringWithChinese) {
    const char* query = "key=测试";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含空格 */
TEST(UvhttpQueryValidationTest, QueryStringWithSpaces) {
    const char* query = "key=value test";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含引号 - 应该被拒绝 */
TEST(UvhttpQueryValidationTest, QueryStringWithQuotes) {
    const char* query = "key=\"value\"";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 0);  // 双引号是危险字符
}

/* 测试查询字符串包含单引号 - 应该被拒绝 */
TEST(UvhttpQueryValidationTest, QueryStringWithSingleQuotes) {
    const char* query = "key='value'";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 0);  // 单引号是危险字符
}

/* 测试查询字符串包含特殊符号 */
TEST(UvhttpQueryValidationTest, QueryStringWithSpecialSymbols) {
    const char* query = "key=!@#$%^&*()";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);  // 大部分特殊符号是允许的
}

/* 测试查询字符串包含数字 */
TEST(UvhttpValidationTest, QueryStringWithNumbers) {
    const char* query = "key=12345";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含混合内容 */
TEST(UvhttpValidationTest, QueryStringWithMixedContent) {
    const char* query = "key1=value1&key2=12345&key3=test";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串长度边界 */
TEST(UvhttpValidationTest, QueryStringLengthBoundary) {
    char long_query[2048];
    memset(long_query, 'a', sizeof(long_query) - 1);
    long_query[sizeof(long_query) - 1] = '\0';
    long_query[0] = '=';

    int result = uvhttp_validate_query_string(long_query);
    EXPECT_EQ(result, 1);
}

/* 测试查询字符串包含换行符 - 应该被拒绝 */
TEST(UvhttpQueryValidationTest, QueryStringWithNewline) {
    const char* query = "key=value\ntest";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 0);  // 换行符是危险字符
}

/* 测试查询字符串包含制表符 */
TEST(UvhttpQueryValidationTest, QueryStringWithTab) {
    const char* query = "key=value\ttest";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 1);  // 制表符不在危险字符列表中
}

/* 测试查询字符串包含回车符 - 应该被拒绝 */
TEST(UvhttpQueryValidationTest, QueryStringWithCarriageReturn) {
    const char* query = "key=value\rtest";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 0);  // 回车符是危险字符
}

/* 测试查询字符串包含小于号 - 应该被拒绝 */
TEST(UvhttpQueryValidationTest, QueryStringWithLessThan) {
    const char* query = "key=<value>";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 0);  // < 和 > 是危险字符
}

/* 测试查询字符串包含大于号 - 应该被拒绝 */
TEST(UvhttpQueryValidationTest, QueryStringWithGreaterThan) {
    const char* query = "key=value>test";
    int result = uvhttp_validate_query_string(query);
    EXPECT_EQ(result, 0);  // > 是危险字符
}