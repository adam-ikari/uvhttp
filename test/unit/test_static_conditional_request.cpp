/*
 * UVHTTP 条件请求（RFC 7232）测试
 *
 * 为什么需要独立测试：uvhttp_static_check_conditional_request 是 304
 * Not Modified 的唯一决策点，内部有两段真实解析逻辑——
 *
 *   1. If-None-Match：逗号分隔 entity-tag 列表遍历、未引号逗号的边界
 *      识别（in_quotes 状态机）、W/ 弱验证前缀从两侧剥离、长度+内容比较
 *   2. If-Modified-Since：三种 HTTP-date 格式（IMF-fixdate / RFC 850 /
 *      asctime）依次 strptime，用 timegm 而非 mktime 转换（后者会引入
 *      本地时区偏移），与 last_modified 比较
 *
 * 现有测试（test_static_comprehensive_coverage.cpp:254/261 等）全部传
 * NULL 或 etag=NULL，从未让这两段解析执行。check_conditional_request
 * 传 NULL request 时函数第一行就 return 0。
 *
 * 另注：既有测试用 `char fake_request[256] = {0}` 充当 request 指针传入，
 * 那是类型双关——真实代码会把它当 uvhttp_request_t* 解引用。本文件改用
 * 真实的栈上 uvhttp_request_t（与 test_request_api_coverage.cpp 一致）。
 */

#include <gtest/gtest.h>
#include <string.h>
#include <time.h>

/* uvhttp_static_check_conditional_request 只在 UVHTTP_FEATURE_STATIC_FILES
 * 开启时编译（uvhttp_static.h 的守卫范围），故整个文件受同一条件控制。 */
#if UVHTTP_FEATURE_STATIC_FILES

extern "C" {
#    include "uvhttp_request.h"
#    include "uvhttp_static.h"
}

class ConditionalRequest : public ::testing::Test {
   protected:
    uvhttp_request_t req_;

    void SetUp() override {
        memset(&req_, 0, sizeof(req_));
        strcpy(req_.url, "/resource");
        req_.header_count = 0;
    }

    void TearDown() override {
        uvhttp_request_cleanup(&req_);
    }

    /* 构造带单个条件头的请求 */
    void WithHeader(const char* name, const char* value) {
        EXPECT_EQ(uvhttp_request_add_header(&req_, name, value), UVHTTP_OK);
    }

    /* 断言返回 1（应回 304） */
    void Expect304(bool cond) {
        EXPECT_TRUE(cond) << "应返回 304";
    }
};

static constexpr const char* kEtag = "\"abc123\"";

/* ========== If-None-Match：通配符 ========== */

TEST_F(ConditionalRequest, IfNoneMatchWildcardMatches) {
    WithHeader("If-None-Match", "*");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 1);
}

/* ========== If-None-Match：精确匹配 ========== */

TEST_F(ConditionalRequest, IfNoneMatchExactMatch) {
    WithHeader("If-None-Match", "\"abc123\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 1);
}

TEST_F(ConditionalRequest, IfNoneMatchDifferentTagDoesNotMatch) {
    WithHeader("If-None-Match", "\"different\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 0);
}

/* 长度相同但内容不同——验证是内容比较而非仅长度 */
TEST_F(ConditionalRequest, IfNoneMatchSameLengthDifferentContent) {
    WithHeader("If-None-Match", "\"abc124\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 0);
}

/* ========== If-None-Match：逗号分隔多值列表 ========== */

TEST_F(ConditionalRequest, IfNoneMatchMatchesMiddleEntry) {
    /* 目标在列表中间，验证会遍历到而非只看首项 */
    WithHeader("If-None-Match", "\"aaa\", \"bbb\", \"abc123\", \"ccc\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 1);
}

TEST_F(ConditionalRequest, IfNoneMatchNoEntryMatchesInList) {
    WithHeader("If-None-Match", "\"aaa\", \"bbb\", \"ccc\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 0);
}

TEST_F(ConditionalRequest, IfNoneMatchLastEntryMatches) {
    WithHeader("If-None-Match", "\"aaa\", \"abc123\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 1);
}

TEST_F(ConditionalRequest, IfNoneMatchTrailingCommaIsTolerated) {
    WithHeader("If-None-Match", "\"abc123\",");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 1);
}

/* ========== If-None-Match：弱验证 W/ 前缀（RFC 7232 §2.3.2） ========== */

/* 弱验证器应与强验证器互相匹配 */
TEST_F(ConditionalRequest, WeakValidatorMatchesStrongResource) {
    WithHeader("If-None-Match", "W/\"abc123\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 1);
}

/* 资源侧带 W/ 时也应剥离后比较（实现对两侧都剥离） */
TEST_F(ConditionalRequest, WeakResourceMatchesStrongHeader) {
    WithHeader("If-None-Match", "\"abc123\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, "W/\"abc123\"", 0),
              1);
}

TEST_F(ConditionalRequest, WeakValidatorDifferentContentDoesNotMatch) {
    WithHeader("If-None-Match", "W/\"zzzzzz\"");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 0);
}

/* ========== If-None-Match：前置条件（etag 为空时不进入匹配） ========== */

TEST_F(ConditionalRequest, NoEtagMeansNoMatchEvenWithHeader) {
    WithHeader("If-None-Match", "\"abc123\"");
    /* etag 为 NULL 或空串时，If-None-Match 分支被跳过 */
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, nullptr, 0), 0);
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, "", 0), 0);
}

/* ========== If-Modified-Since：三种 HTTP-date 格式 ========== */

/* 参考时间固定为 2026-01-15 12:00:00 GMT = 1768478400 */
static constexpr time_t kRef = 1768478400;

/* IMF-fixdate（RFC 7231 §7.1.1.1 首选格式） */
TEST_F(ConditionalRequest, IfModifiedSinceImfFixdateNotOlder) {
    WithHeader("If-Modified-Since", "Thu, 15 Jan 2026 12:00:00 GMT");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 1);
}

TEST_F(ConditionalRequest, IfModifiedSinceImfFixdateOlder) {
    WithHeader("If-Modified-Since", "Thu, 01 Jan 2026 12:00:00 GMT");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 0);
}

/* obsolete RFC 850 格式：Sunday, 06-Nov-94 08:49:37 GMT */
TEST_F(ConditionalRequest, IfModifiedSinceRfc850Format) {
    /* 1994 年，远早于 kRef → 应回完整内容 */
    WithHeader("If-Modified-Since", "Sunday, 06-Nov-94 08:49:37 GMT");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 0);
}

/* asctime 格式：Sun Nov  6 08:49:37 1994 */
TEST_F(ConditionalRequest, IfModifiedSinceAsctimeFormat) {
    WithHeader("If-Modified-Since", "Sun Nov  6 08:49:37 1994");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 0);
}

/* 三种格式都解析失败（垃圾输入）→ 不匹配 */
TEST_F(ConditionalRequest, IfModifiedSinceUnparseableIsIgnored) {
    WithHeader("If-Modified-Since", "not-a-date-at-all");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 0);
}

/* last_modified 为 0（未知）时不进入 If-Modified-Since 比较 */
TEST_F(ConditionalRequest, ZeroLastModifiedSkipsDateComparison) {
    WithHeader("If-Modified-Since", "Thu, 15 Jan 2026 12:00:00 GMT");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, 0), 0);
}

/* ========== 两个头同时存在：If-None-Match 优先 ========== */

TEST_F(ConditionalRequest, IfNoneMatchTakesPrecedenceOverIfModifiedSince) {
    /* If-None-Match 不匹配但 If-Modified-Since 匹配 → 仍回 304
     * （任一前置条件成立即为 not-modified） */
    WithHeader("If-None-Match", "\"nomatch\"");
    WithHeader("If-Modified-Since", "Thu, 15 Jan 2026 12:00:00 GMT");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 1);
}

TEST_F(ConditionalRequest, BothHeadersFailMeansFullContent) {
    WithHeader("If-None-Match", "\"nomatch\"");
    WithHeader("If-Modified-Since", "Thu, 01 Jan 2026 12:00:00 GMT");
    EXPECT_EQ(uvhttp_static_check_conditional_request(&req_, kEtag, kRef), 0);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
