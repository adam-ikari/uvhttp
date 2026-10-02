/*
 * UVHTTP URL 路径编码与危险字符校验测试
 *
 * 为什么需要独立测试：uvhttp_validate_url_path 有三个独立校验循环
 * —— URL 编码格式 (%XX)、Windows 风格穿越 (..\) 、危险字符扫描
 * ——它们只对含特定字符的输入才执行。test_validation_full_coverage.cpp
 * 只测了 NULL / 空串 / 普通路径 / ".." 穿越，因此这三个循环的分支
 * 从未被真实输入触发过。
 *
 * 这些是攻击面：编码截断 (%2) 与非十六进制 (%zz) 让路径解码后可能
 * 与校验时的字符串不一致；危险字符 (\n \r) 若被下游日志或重定向使用
 * 会有响应拆分风险。
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_validation.h"
}

class UrlPathEncoding : public ::testing::Test {};

/* ========== URL 编码格式校验（%XX） ========== */

TEST_F(UrlPathEncoding, PercentFollowedByTwoHexDigitsIsValid) {
    EXPECT_EQ(uvhttp_validate_url_path("/a%20b"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/%41%42%43"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/file%2Etxt"), 1);
}

TEST_F(UrlPathEncoding, PercentWithUppercaseHexIsValid) {
    EXPECT_EQ(uvhttp_validate_url_path("/a%2Fb"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/a%2fb"), 1);
}

/* % 后面不足两位十六进制 —— 解码后路径与校验时的字符串会不一致 */
TEST_F(UrlPathEncoding, TruncatedPercentSequenceIsRejected) {
    EXPECT_EQ(uvhttp_validate_url_path("/a%2"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a%"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/%"), 0);
}

/* % 后是非十六进制字符 */
TEST_F(UrlPathEncoding, NonHexAfterPercentIsRejected) {
    EXPECT_EQ(uvhttp_validate_url_path("/a%zz"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a%2z"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a%g0"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/100%off"), 0);
}

/* 编码穿越：%2e%2e = ".." —— 校验必须在解码前拦住 */
TEST_F(UrlPathEncoding, EncodedDotDotTraversalIsRejected) {
    EXPECT_EQ(uvhttp_validate_url_path("/%2e%2e/etc/passwd"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/%2E%2E/etc/passwd"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a/%2e%2e/b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/%2e%2e%2fetc"), 0);
}

/* ========== Windows 风格穿越（..\） ========== */

TEST_F(UrlPathEncoding, BackslashTraversalIsRejected) {
    EXPECT_EQ(uvhttp_validate_url_path("/a..\\b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/..\\windows"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/dir\\..\\..\\file"), 0);
}

/* ========== 危险字符扫描（< > : " | * \n \r） ========== */

TEST_F(UrlPathEncoding, DangerousCharactersAreRejected) {
    /* \r \n 是响应拆分风险 */
    std::string with_crlf = "/a\r\nX-Injected: 1";
    EXPECT_EQ(uvhttp_validate_url_path(with_crlf.c_str()), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a\nb"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a\rb"), 0);
    /* 其余危险字符 */
    EXPECT_EQ(uvhttp_validate_url_path("/a<b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a>b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a:b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a\"b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a|b"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/a*b"), 0);
}

/* 危险字符在路径任何位置都应被拒，不只是开头 */
TEST_F(UrlPathEncoding, DangerousCharacterMidPathIsRejected) {
    EXPECT_EQ(uvhttp_validate_url_path("/very/long/path/with:colon"), 0);
    EXPECT_EQ(uvhttp_validate_url_path("/deep/nested|pipe/path"), 0);
}

/* ========== 边界：编码校验的 p += 2 跳跃 ========== */

/* 连续多个编码序列：验证 p += 2 后循环正确跳过已消费的十六进制位，
 * 否则会把十六进制位误当 % 再检查一次 */
TEST_F(UrlPathEncoding, ConsecutiveEncodedSequencesAreHandled) {
    EXPECT_EQ(uvhttp_validate_url_path("/%41%42%43%44%45"), 1);
    EXPECT_EQ(uvhttp_validate_url_path("/a%20b%20c%20d"), 1);
}

/* 十六进制位里恰好含 0-9A-F，不会被当作 % 再次检查；此处确认合法 */
TEST_F(UrlPathEncoding, HexDigitsContainingPercentLikeValuesAreValid) {
    /* %2e 解码为 '.'，不是编码穿越（不是连续 %2e%2e），应通过 */
    EXPECT_EQ(uvhttp_validate_url_path("/file%2ename"), 1);
    /* %ff 是合法十六进制 */
    EXPECT_EQ(uvhttp_validate_url_path("/%ff"), 1);
}