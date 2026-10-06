/*
 * UVHTTP Content-Length + 压缩分帧一致性测试
 *
 * 背景：uvhttp_response_prepare 压缩成功后改写 response->body_length 为
 * 压缩后长度，但 build_response_headers 的 has_content_length gate：
 *
 *     if (!has_content_length) {
 *         UVHTTP_SNAPPEND("Content-Length: %zu\r\n", response->body_length);
 *     }
 *
 * ——只在没有手动 Content-Length 时才用 body_length 生成。若调用方手动
 * 设了 Content-Length（静态文件/用户路径），它会被原样保留（声明原始长度），
 * 而实际发送的是压缩后的 body（更短）→ Content-Length 与实际不符 →
 * 客户端按声明的长度读取会卡住或读到后续连接数据（分帧错位，数据损坏级）。
 *
 * 修复：压缩成功后 uvhttp_response_sync_content_length() 把已存在的
 * Content-Length header 值同步为压缩后长度。
 *
 * 本测试钉住该 bug 不回归。
 */

#include <gtest/gtest.h>
#include <string.h>
#include <string>

extern "C" {
#include "uvhttp_response.h"
}

namespace {

/* 高压缩比 body（大量重复），保证压缩后明显更小 */
std::string CompressibleBody(size_t n) {
    return std::string(n, 'A');
}

/* 构建响应并返回完整响应字节（headers + body） */
std::string BuildResponse(const std::string& body,
                          const char* manual_content_length, int compress) {
    uvhttp_response_t res;
    memset(&res, 0, sizeof(res));
    uvhttp_response_init(&res, (void*)0x1234);
    uvhttp_response_set_status(&res, 200);
    res.compress = compress;
    res.compress_threshold = 1024;
    uvhttp_response_set_body(&res, body.data(), body.size());
    if (manual_content_length) {
        uvhttp_response_set_header(&res, "Content-Length",
                                   manual_content_length);
    }

    char* data = nullptr;
    size_t len = 0;
    uvhttp_error_t r = uvhttp_response_build_data(&res, &data, &len);
    std::string out;
    if (r == UVHTTP_OK && data) {
        out.assign(data, len);
        free(data);
    }
    uvhttp_response_cleanup(&res);
    return out;
}

/* 从响应字节里提取 Content-Length 声明值与实际 body 字节数 */
void ParseResponse(const std::string& s, int* declared_cl, size_t* actual_body,
                   bool* has_gzip) {
    *declared_cl = -1;
    *actual_body = 0;
    *has_gzip = false;
    auto cl = s.find("Content-Length: ");
    if (cl != std::string::npos) {
        *declared_cl = atoi(s.c_str() + cl + 16);
    }
    *has_gzip = s.find("Content-Encoding: gzip") != std::string::npos;
    auto sep = s.find("\r\n\r\n");
    if (sep != std::string::npos) {
        *actual_body = s.size() - sep - 4;
    }
}

}  // namespace

class ContentLengthCompression : public ::testing::Test {};

/* 手动 Content-Length + 压缩 → 声明值必须与压缩后 body 一致（修复的核心） */
TEST_F(ContentLengthCompression, ManualContentLengthMatchesCompressedBody) {
    std::string body = CompressibleBody(10000);
    std::string s = BuildResponse(body, "10000", /*compress=*/1);

    int declared;
    size_t actual;
    bool gz;
    ParseResponse(s, &declared, &actual, &gz);

    ASSERT_TRUE(gz) << "body 应被压缩";
    EXPECT_GT(actual, 0u);
    EXPECT_LT(actual, body.size()) << "压缩后应明显更小";

    /* 核心：声明长度 == 实际 body（修复前声明 10000、实际 ~45，错位） */
    EXPECT_EQ(declared, (int)actual)
        << "Content-Length 声明 " << declared << " 与实际发送 body " << actual
        << " 不符——客户端会分帧错位（数据损坏）";
}

/* 不压缩时手动 Content-Length 保留原值（修复不影响未压缩路径） */
TEST_F(ContentLengthCompression,
       ManualContentLengthPreservedWithoutCompression) {
    std::string body = CompressibleBody(100);
    std::string s = BuildResponse(body, "100", /*compress=*/0);

    int declared;
    size_t actual;
    bool gz;
    ParseResponse(s, &declared, &actual, &gz);

    EXPECT_FALSE(gz) << "compress=0 不应压缩";
    EXPECT_EQ(declared, 100);
    EXPECT_EQ(actual, body.size());
}

/* 无手动 Content-Length + 压缩 → 自动生成压缩后长度（修复前的正常路径） */
TEST_F(ContentLengthCompression, AutoContentLengthMatchesCompressedBody) {
    std::string body = CompressibleBody(10000);
    std::string s = BuildResponse(body, nullptr, /*compress=*/1);

    int declared;
    size_t actual;
    bool gz;
    ParseResponse(s, &declared, &actual, &gz);

    ASSERT_TRUE(gz);
    EXPECT_EQ(declared, (int)actual)
        << "自动生成的 Content-Length 也应与压缩后 body 一致";
}

/* 压缩无效（body 太小，未达阈值）→ 不压缩，手动 CL 保留 */
TEST_F(ContentLengthCompression, SmallBodyNoCompressionPreservesManualCL) {
    std::string body = "short";
    std::string s = BuildResponse(body, "5", /*compress=*/1);

    int declared;
    size_t actual;
    bool gz;
    ParseResponse(s, &declared, &actual, &gz);

    EXPECT_FALSE(gz) << "body 小于阈值不应压缩";
    EXPECT_EQ(declared, 5);
    EXPECT_EQ(actual, body.size());
}
