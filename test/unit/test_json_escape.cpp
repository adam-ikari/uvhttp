/*
 * UVHTTP JSON 错误响应转义测试
 *
 * 为什么需要独立测试：json_escape（src/uvhttp_utils.c:102）是 static 函数，
 * 由 uvhttp_send_error_response / uvhttp_send_unified_response 调用。lcov
 * 实测 src/uvhttp_utils.c 仅 65.7% 行覆盖（65/99），全部 34 个未覆盖行
 * （:108-143）集中在 json_escape 的 switch——
 *
 *     case '"'  case '\\'  case '\b'  case '\f'  case '\n'
 *     case '\r'  case '\t'  default(c < 0x20 → \u00xx)
 *
 * 现有 utils 测试（test_utils_api_coverage / test_utils_full_coverage）对
 * send_error_response 的 4 处引用全是 null 参数模式——传 nullptr 走函数
 * 第一行的参数校验就直接 return，json_escape 从未被真实执行过。
 *
 * 转义出错的实际危害：
 *   - 双引号/反斜杠未转义 → 生成的 JSON 语法非法
 *   - 换行/回车未转义 → 响应体注入（HTTP 头分割的邻近风险）
 *   - 控制字符未转义 → 非法 JSON / 终端注入
 *   - \u00xx 小写十六进制是 JSON 有效转义（\u00xx 需 4 位 hex）
 *
 * 测试通过公开 API uvhttp_send_error_response 驱动：client 用 NULL 使
 * uvhttp_response_send 在发送前返回 INVALID_PARAM，但 json_escape 的输出
 * 已存入 response->body——这正是转义发生的位置。
 */

#include <gtest/gtest.h>
#include <string.h>
#include <string>

extern "C" {
#include "uvhttp_response.h"
#include "uvhttp_utils.h"
}

namespace {

uvhttp_response_t MakeResponse() {
    uvhttp_response_t res;
    memset(&res, 0, sizeof(res));
    /* client=NULL → send 在 build_data+send_raw 的 !client 检查处返回
     * INVALID_PARAM，但 body 已由 set_body 写入。 */
    uvhttp_response_init(&res, nullptr);
    return res;
}

/* 调用 send_error_response 并取回 response body（string） */
std::string GetErrorBody(int code, const char* msg, const char* details) {
    uvhttp_response_t res = MakeResponse();
    uvhttp_send_error_response(&res, code, msg, details);
    std::string out(res.body ? res.body : "(null)", res.body_length);
    uvhttp_response_cleanup(&res);
    return out;
}

}  // namespace

class JsonEscape : public ::testing::Test {};

/* ========== 逐个特殊字符 ========== */

TEST_F(JsonEscape, DoubleQuoteIsEscaped) {
    std::string b = GetErrorBody(400, "say \"hi\"", nullptr);
    EXPECT_NE(b.find("say \\\"hi\\\""), std::string::npos)
        << "未转义的双引号会令 JSON 语法非法：\n"
        << b;
}

TEST_F(JsonEscape, BackslashIsEscaped) {
    std::string b = GetErrorBody(400, R"(a\b)", nullptr);
    EXPECT_NE(b.find(R"(a\\b)"), std::string::npos)
        << "未转义的反斜杠会耗尽 JSON 转义序列：\n"
        << b;
}

TEST_F(JsonEscape, BackspaceIsEscaped) {
    std::string b = GetErrorBody(400, "a\bb", nullptr);
    EXPECT_NE(b.find(R"(a\bb)"), std::string::npos);
}

TEST_F(JsonEscape, FormFeedIsEscaped) {
    std::string b = GetErrorBody(400, "a\fb", nullptr);
    EXPECT_NE(b.find(R"(a\fb)"), std::string::npos);
}

TEST_F(JsonEscape, NewlineIsEscaped) {
    std::string b = GetErrorBody(400, "line1\nline2", nullptr);
    EXPECT_NE(b.find("line1\\nline2"), std::string::npos)
        << "未转义的换行会把 JSON 拆成多行文档：\n"
        << b;
}

TEST_F(JsonEscape, CarriageReturnIsEscaped) {
    std::string b = GetErrorBody(400, "a\rb", nullptr);
    EXPECT_NE(b.find(R"(a\rb)"), std::string::npos);
}

TEST_F(JsonEscape, TabIsEscaped) {
    std::string b = GetErrorBody(400, "a\tb", nullptr);
    EXPECT_NE(b.find(R"(a\tb)"), std::string::npos);
}

/* ========== 控制字符 → \u00xx ========== */

/* 0x01（控制字符，不在 C 转义白名单）应变成 \u0001 */
TEST_F(JsonEscape, ControlCharBecomesUnicodeEscape) {
    std::string msg = "a";
    msg += (char)0x01;
    msg += "b";
    std::string b = GetErrorBody(400, msg.c_str(), nullptr);
    EXPECT_NE(b.find(R"(a\u0001b)"), std::string::npos)
        << "0x01 应转义为 \\u0001：\n"
        << b;
}

/* 0x1F 是最后一个需要 \u00xx 的字符 */
TEST_F(JsonEscape, UpperControlCharBecomesUnicodeEscape) {
    std::string msg = "a";
    msg += (char)0x1F;
    msg += "b";
    std::string b = GetErrorBody(400, msg.c_str(), nullptr);
    EXPECT_NE(b.find(R"(a\u001fb)"), std::string::npos);
}

/* 半角字符（0x20 空格以上）不应被转义 */
TEST_F(JsonEscape, PrintableAsciiIsNotEscaped) {
    std::string b = GetErrorBody(400, "plain ascii 123", nullptr);
    EXPECT_NE(b.find("plain ascii 123"), std::string::npos);
    EXPECT_EQ(b.find("\\u"), std::string::npos)
        << "可打印 ASCII 不应产生 \\u 转义";
}

/* ========== 混合 + details ========== */

TEST_F(JsonEscape, MixedSpecialCharsAllEscaped) {
    std::string msg;
    msg += "\"q\"\\\\b\bf\fn\nr\rt\tc";  // 全部 7 类转义
    msg += (char)0x0A;
    std::string b = GetErrorBody(400, msg.c_str(), nullptr);
    /* 一次请求覆盖全部 case：双引号、反斜杠、\b \f \n \r \t */
    EXPECT_NE(b.find("\\\"q\\\""), std::string::npos);
    EXPECT_NE(b.find("\\\\"), std::string::npos);
    EXPECT_NE(b.find("\\b"), std::string::npos);
    EXPECT_NE(b.find("\\f"), std::string::npos);
    EXPECT_NE(b.find("\\n"), std::string::npos);
    EXPECT_NE(b.find("\\r"), std::string::npos);
    EXPECT_NE(b.find("\\t"), std::string::npos);
}

TEST_F(JsonEscape, DetailsAreEscapedToo) {
    std::string b = GetErrorBody(400, "msg", "detail \"with\" quotes");
    EXPECT_NE(b.find("detail \\\"with\\\" quotes"), std::string::npos);
    /* message="msg" 无特殊字符，JSON 里是普通字段边界引号 */
    EXPECT_NE(b.find("\"error\":\"msg\""), std::string::npos);
}

TEST_F(JsonEscape, DetailsNullStillWorks) {
    /* details 为 NULL：只转义 message，JSON 不含 details 字段 */
    std::string b = GetErrorBody(404, "not found", nullptr);
    EXPECT_NE(b.find("\"error\":\"not found\""), std::string::npos);
    EXPECT_EQ(b.find("details"), std::string::npos);
}

/* ========== 结构合法性 ========== */

/* 转义后整个 body 应能 parse 为合法 JSON 对象 */
TEST_F(JsonEscape, EscapedBodyIsParseableKeyValuePrefix) {
    std::string b = GetErrorBody(500, "boom \"quote\"", "details \n newline");
    /* 以 { 开头、包含 error 字段、以 } 结尾——转义失败会破坏这些边界 */
    EXPECT_EQ(b[0], '{');
    EXPECT_EQ(b.back(), '}');
    EXPECT_NE(b.find("\"error\":\""), std::string::npos);
    EXPECT_NE(b.find("\"code\":"), std::string::npos);
    /* 尾部 } 前应是起始 }——若 details 把换行注入成原始字符，会提前截断 */
    EXPECT_EQ(b.back(), '}');
}

/* ========== 长输入截断 ========== */

/* message 超长时按 MAX_ERROR_MSG_LEN 校验拒绝（在 json_escape 之前） */
TEST_F(JsonEscape, OversizedMessageRejected) {
    std::string big(2048, 'x');
    uvhttp_response_t res = MakeResponse();
    uvhttp_error_t r =
        uvhttp_send_error_response(&res, 400, big.c_str(), nullptr);
    EXPECT_NE(r, UVHTTP_OK) << "超长 message 应被参数校验拒绝";
    uvhttp_response_cleanup(&res);
}

/* details 超长同样拒绝 */
TEST_F(JsonEscape, OversizedDetailsRejected) {
    std::string big(2048, 'y');
    uvhttp_response_t res = MakeResponse();
    uvhttp_error_t r = uvhttp_send_error_response(&res, 400, "ok", big.c_str());
    EXPECT_NE(r, UVHTTP_OK) << "超长 details 应被参数校验拒绝";
    uvhttp_response_cleanup(&res);
}