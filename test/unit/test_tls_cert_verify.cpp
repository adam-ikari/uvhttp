/*
 * UVHTTP TLS 证书验证行为测试
 *
 * 与 test_tls_api_coverage.cpp 的分工：
 *   - test_tls_api_coverage.cpp 覆盖 null 参数 /
 * 无效输入（调用不崩、返回错误码）
 *   - 本文件用**真实 X.509 证书**驱动 verify_hostname / check_cert_validity
 *     的实际匹配与时间判断逻辑
 *
 * 为什么需要真实证书：这两个函数接收 mbedtls_x509_crt*，其内部逻辑是
 * 遍历 CN OID（2.5.4.3）、遍历 SAN 链表、与当前时间比较——这些分支只有
 * 传入真实解析出的证书时才会被执行。传 nullptr 或零初始化的空结构体
 * 只能覆盖到第一行 null 检查。
 *
 * 证书路径：__FILE__ 在本项目 CMake 构建下展开为绝对路径
 * （CMake 传 ${CMAKE_CURRENT_SOURCE_DIR}/test/unit/xxx.cpp），据此
 * 上溯两级得到源码根，拼出 test/certs/。这样无需修改 CMake 注入宏，
 * 且不受 ctest 工作目录（build/）影响。
 */

#include <gtest/gtest.h>

#if UVHTTP_FEATURE_TLS

#    include <string.h>

extern "C" {
#    include "uvhttp_tls.h"

#    include <mbedtls/x509_crt.h>
}

/* 源码根：__FILE__ = <root>/test/unit/test_tls_cert_verify.cpp */
static const char* ProjectRoot() {
    static std::string root;
    if (root.empty()) {
        std::string f(__FILE__);
        size_t unit = f.find("/test/unit/");
        root =
            (unit == std::string::npos) ? std::string(".") : f.substr(0, unit);
    }
    return root.c_str();
}

static std::string CertPath(const char* name) {
    return std::string(ProjectRoot()) + "/test/certs/" + name;
}

/* 载入真实证书；解析失败直接失败（不允许静默跳过测试） */
static bool LoadCert(mbedtls_x509_crt* cert, const char* file) {
    mbedtls_x509_crt_init(cert);
    return mbedtls_x509_crt_parse_file(cert, CertPath(file).c_str()) == 0;
}

/* server.crt 的 subject CN 为 localhost，有效期 2026-02-02 ~ 2027-02-02 */
class TlsCertVerify : public ::testing::Test {
   protected:
    mbedtls_x509_crt cert_;
    void SetUp() override {
        ASSERT_TRUE(LoadCert(&cert_, "server.crt"))
            << "无法载入测试证书（路径 " << CertPath("server.crt") << "）";
    }
    void TearDown() override {
        mbedtls_x509_crt_free(&cert_);
    }
};

/* ========== verify_hostname：CN 匹配 ========== */

TEST_F(TlsCertVerify, HostnameMatchesCommonName) {
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, "localhost"), 1);
}

/* 实现用 strncasecmp 比较，大小写应不敏感 */
TEST_F(TlsCertVerify, HostnameMatchIsCaseInsensitive) {
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, "LOCALHOST"), 1);
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, "LocalHost"), 1);
}

/* 实现要求 strlen(hostname) == cn_len 才比较，故前缀不算匹配 */
TEST_F(TlsCertVerify, HostnamePrefixDoesNotMatch) {
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, "local"), 0);
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, "localhost.example.com"), 0);
}

TEST_F(TlsCertVerify, HostnameMismatchReturnsZero) {
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, "example.com"), 0);
}

TEST_F(TlsCertVerify, EmptyHostnameDoesNotMatch) {
    EXPECT_EQ(uvhttp_tls_verify_hostname(&cert_, ""), 0);
}

/* 无 SAN 的证书：不带 SAN 的 hostname 仍靠 CN 通过，带 SAN 语义的检查
 * 只能确认不误报——server.crt 未配置 SAN */
TEST_F(TlsCertVerify, NoSubjectAltNamesPresent) {
    EXPECT_EQ(cert_.subject_alt_names.next, nullptr)
        << "测试假设 server.crt 无 SAN；若新增 SAN 需补对应用例";
}

/* ========== check_cert_validity：时间判断 ========== */

TEST_F(TlsCertVerify, FreshCertificateIsValid) {
    EXPECT_EQ(uvhttp_tls_check_cert_validity(&cert_), 1);
}

/* 把 valid_from 推到未来 → mbedtls_x509_time_is_future 应为真 → 判定无效 */
TEST_F(TlsCertVerify, NotYetValidCertificateRejected) {
    mbedtls_x509_crt future = cert_;
    future.valid_from.year += 10;
    EXPECT_EQ(uvhttp_tls_check_cert_validity(&future), 0);
}

/* 把 valid_to 推到过去 → mbedtls_x509_time_is_past 应为真 → 判定无效 */
TEST_F(TlsCertVerify, ExpiredCertificateRejected) {
    mbedtls_x509_crt expired = cert_;
    expired.valid_to.year -= 10;
    EXPECT_EQ(uvhttp_tls_check_cert_validity(&expired), 0);
}

#endif /* UVHTTP_FEATURE_TLS */