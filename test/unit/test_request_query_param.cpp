/*
 * UVHTTP query 参数解析行为测试
 *
 * 为什么需要独立测试：uvhttp_request_get_query_param（src/uvhttp_request.c:752）
 * 是手写扫描循环，有三条当前完全未被验证的行为契约：
 *
 *   1. **重复 key 首个胜出**——循环一旦命中即 return，不继续查找后续同名 key
 *   2. **不做 URL 解码**——返回值保持原始 %XX 与 '+'，不解码成空格/字符
 *   3. **超长值截断**——value_len >=
 * sizeof(param_value)(UVHTTP_MAX_URL_SIZE=2048) 时截断到 2047 字节并补 '\0'
 *
 * 现有 42 处引用全部集中在 test_request_comprehensive_coverage.cpp:148-179 与
 * test_request_extended_coverage.cpp:213-227，输入只有两类：
 *   - url 无 '?'（走 NULL 早退）
 *   - url='/api/users?id=123&name=test'（短 ASCII、无编码、无 '+'、
 *     无重复 key、值远小于 2048）
 *
 * 这些契约（尤其"不解码"）是调用方依赖的既有行为，改动会静默改变 API 语义，
 * 因此需要测试钉死而非任其漂移。
 */

#include <gtest/gtest.h>
#include <string.h>

extern "C" {
#include "uvhttp_request.h"
}

class QueryParam : public ::testing::Test {
   protected:
    uvhttp_request_t req_{};

    void SetUp() override {
        memset(&req_, 0, sizeof(req_));
    }

    void TearDown() override {
        uvhttp_request_cleanup(&req_);
    }

    /* 用给定 url 构造 request 并返回该参数的查询结果 */
    std::string Query(const char* url, const char* name) {
        memset(&req_, 0, sizeof(req_));
        snprintf(req_.url, sizeof(req_.url), "%s", url);
        const char* v = uvhttp_request_get_query_param(&req_, name);
        return v ? std::string(v) : std::string("<NULL>");
    }
};

/* ========== 基本命中 ========== */

TEST_F(QueryParam, SingleParamIsFound) {
    EXPECT_EQ(Query("/s?id=123&name=test", "id"), "123");
}

TEST_F(QueryParam, LastParamIsFound) {
    EXPECT_EQ(Query("/s?a=1&b=2&c=3", "c"), "3");
}

TEST_F(QueryParam, MissingParamReturnsNull) {
    EXPECT_EQ(Query("/s?a=1", "zzz"), "<NULL>");
}

/* 无 query string 时返回 NULL（早退路径） */
TEST_F(QueryParam, NoQueryStringReturnsNull) {
    EXPECT_EQ(Query("/plain/path", "id"), "<NULL>");
}

/* ========== 契约 1：重复 key 首个胜出 ========== */

/* 首个命中即返回，不取最后一个 */
TEST_F(QueryParam, DuplicateKeyReturnsFirstOccurrence) {
    EXPECT_EQ(Query("/s?a=1&a=2&a=3", "a"), "1")
        << "当前实现首个 key 胜出，不应取最后一个";
}

/* 重复 key 且第一个在末尾（先遍历过后面的再命中前面的） */
TEST_F(QueryParam, DuplicateKeyAfterOthersStillReturnsFirst) {
    EXPECT_EQ(Query("/s?x=0&y=1&target=first&target=second", "target"),
              "first");
}

/* key 是另一个 key 的前缀时必须按 '=' 边界匹配，不能误命中 */
TEST_F(QueryParam, PrefixKeyDoesNotMatchLongerKey) {
    /* 查询 "id"，但存在 "idx"——"idx=" 不应被当成 "id=" */
    EXPECT_EQ(Query("/s?idx=99&id=7", "id"), "7");
}

TEST_F(QueryParam, PrefixKeyWithoutValueDoesNotMatch) {
    /* "identity" 无 '='，查询 "id" 不应命中 */
    EXPECT_EQ(Query("/s?identity&other=1", "id"), "<NULL>");
}

/* ========== 契约 2：不��� URL 解码 ========== */

/* %20 不解码成空格，保持原始 %20 */
TEST_F(QueryParam, PercentEncodingIsNotDecoded) {
    EXPECT_EQ(Query("/s?q=hello%20world", "q"), "hello%20world")
        << "当前实现不做 URL 解码，应保持原始 %XX";
}

TEST_F(QueryParam, PercentEncodedSlashIsNotDecoded) {
    EXPECT_EQ(Query("/s?p=a%2Fb", "p"), "a%2Fb");
}

/* 小写十六进制也不解码 */
TEST_F(QueryParam, LowercasePercentEncodingIsNotDecoded) {
    EXPECT_EQ(Query("/s?p=x%2fy", "p"), "x%2fy");
}

/* '+' 不还原成空格 */
TEST_F(QueryParam, PlusIsNotConvertedToSpace) {
    EXPECT_EQ(Query("/s?q=hello+world", "q"), "hello+world")
        << "当前实现不把 '+' 还原为空格";
}

/* 多个编码同时存在时全部保持原样 */
TEST_F(QueryParam, MultipleEncodingsAllPreserved) {
    EXPECT_EQ(Query("/s?v=a%20b+c%2Fd%25e", "v"), "a%20b+c%2Fd%25e");
}

/* ========== 契约 3：空值 ========== */

/* key 存在但值为空串（非 NULL） */
TEST_F(QueryParam, EmptyValueReturnsEmptyString) {
    EXPECT_EQ(Query("/s?a=&b=2", "a"), "");
}

/* 空值在末尾（后面没有 &） */
TEST_F(QueryParam, TrailingEmptyValueReturnsEmptyString) {
    EXPECT_EQ(Query("/s?b=2&a=", "a"), "");
}

/* 空值后又跟其它 key */
TEST_F(QueryParam, EmptyValueFollowedByOtherKey) {
    EXPECT_EQ(Query("/s?a=&b=2", "b"), "2");
}

/* ========== 值长度上界（截断分支不可达） ========== */

/* get_query_param 的 value 取自 request->url，而两者容量相同：
 *   request->url[MAX_URL_LEN]，            MAX_URL_LEN = 2048
 *   static char param_value[UVHTTP_MAX_URL_SIZE]，UVHTTP_MAX_URL_SIZE = 2048
 * 因 value 必是 url 的子串，其长度 ≤ 2047 - 前缀长度，故
 * `value_len >= sizeof(param_value)`（即 >= 2048）这一截断分支在当前
 * 结构下不可达——url 字段物理上装不下那么长的 value。
 *
 * 该截断属防御性代码（防止将来 url 缓冲加大后遗漏），此处不写断言，
 * 只钉死「url 装不下的部分会被 snprintf 截断」这一实际行为。 */
TEST_F(QueryParam, UrlExceedingBufferIsTruncatedBySnprintf) {
    /* 2100 字节的值放不进 2048 字节的 url 字段 */
    std::string big_value(2100, 'v');
    std::string url = "/s?big=" + big_value;
    std::string got = Query(url.c_str(), "big");
    EXPECT_LT(got.size(), 2048u)
        << "返回值必受 url 缓冲上限约束（当前 " << got.size() << "）";
    /* 内容是原始值的前缀，未出现乱码或未初始化字节 */
    EXPECT_EQ(got, big_value.substr(0, got.size()));
}

/* 恰好填满 url 字段（2047 字符 + \0）时的行为 */
TEST_F(QueryParam, UrlExactlyFillingBufferIsHandled) {
    std::string prefix = "/s?big=";
    std::string exact(2047 - prefix.size(), 'w');
    std::string url = prefix + exact;
    ASSERT_EQ(url.size(), 2047u);
    std::string got = Query(url.c_str(), "big");
    EXPECT_EQ(got, exact) << "恰好装满时值应完整且不截断";
}

/* ========== 静态缓冲复用语义 ========== */

/* 返回的是共享静态缓冲：两次调用后者覆盖前者。文档已声明
 * "valid until the next call to this function"，此测试钉死该行为。 */
TEST_F(QueryParam, ReturnedPointerIsReusedAcrossCalls) {
    memset(&req_, 0, sizeof(req_));
    snprintf(req_.url, sizeof(req_.url), "/s?a=first&b=second");
    const char* first = uvhttp_request_get_query_param(&req_, "a");
    ASSERT_NE(first, nullptr);
    EXPECT_STREQ(first, "first");
    const char* second = uvhttp_request_get_query_param(&req_, "b");
    /* 同一缓冲：指针相同，内容被后者覆盖 */
    EXPECT_EQ(first, second) << "返回值应复用同一静态缓冲";
    EXPECT_STREQ(first, "second");
}

/* ========== 畸形输入 ========== */

/* 空 name 返回 NULL（strlen 为 0 但循环会匹配 p[0]=='='） */
TEST_F(QueryParam, EmptyNameReturnsNull) {
    memset(&req_, 0, sizeof(req_));
    snprintf(req_.url, sizeof(req_.url), "/s?a=1");
    EXPECT_EQ(uvhttp_request_get_query_param(&req_, ""), nullptr);
}

/* query 以 & 开头（畸形）不应崩溃 */
TEST_F(QueryParam, QueryStartingWithAmpersandIsHandled) {
    EXPECT_EQ(Query("/s?&a=1", "a"), "1");
}

/* 连续 && 的畸形输入 */
TEST_F(QueryParam, ConsecutiveAmpersandsIsHandled) {
    EXPECT_EQ(Query("/s?a=1&&b=2", "b"), "2");
}

/* 末尾裸 & （无 key）不应崩溃 */
TEST_F(QueryParam, TrailingAmpersandIsHandled) {
    EXPECT_EQ(Query("/s?a=1&", "a"), "1");
}

/* 空 query（只有 ?） */
TEST_F(QueryParam, EmptyQueryStringReturnsNull) {
    EXPECT_EQ(Query("/s?", "a"), "<NULL>");
}