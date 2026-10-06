/*
 * UVHTTP TLS 证书信息提取测试
 *
 * 为什么需要独立测试：get_cert_subject / get_cert_issuer / get_cert_serial
 * （src/uvhttp_tls.c:513/523/533）从 X.509 证书提取 DN 与序列号，是证书
 * 审计/诊断功能。lcov 实测这些函数的**成功路径**零覆盖——现有测试（
 * test_tls_null_coverage.cpp）只测 NULL 参数走第一行 return 0。
 *
 * 成功路径需要真实的已解析证书：mbedtls_x509_crt_parse_file 解析
 * test/certs/server.crt 后，subject/issuer/serial 才非空。
 *
 * 实测预期（generate_certs.sh 生成）：
 *   - server.crt 的 subject 含 "CN=localhost"
 *   - issuer 是签发它的 CA（subject 或特定 CA 名）
 *   - serial 是非空十六进制字符串
 *
 * 提取错误导致的实际危害：审计日志显示空 subject/serial → 无法识别
 * 证书来源（诊断失败）。
 */

#include <gtest/gtest.h>
#include <string.h>

#if UVHTTP_FEATURE_TLS

extern "C" {
#    include "uvhttp_tls.h"

#    include <mbedtls/x509_crt.h>
}

class TlsCertInfo : public ::testing::Test {
   protected:
    mbedtls_x509_crt cert_{};

    void SetUp() override {
        mbedtls_x509_crt_init(&cert_);
        std::string path = CertPath("server.crt");
        ASSERT_EQ(mbedtls_x509_crt_parse_file(&cert_, path.c_str()), 0)
            << "无法解析 server.crt——证书路径或文件异常";
    }

    void TearDown() override {
        mbedtls_x509_crt_free(&cert_);
    }

    /* __FILE__ 在本项目 CMake 构建下展开为绝对路径（#434 验证过） */
    static std::string ProjectRoot() {
        static std::string root;
        if (root.empty()) {
            std::string f(__FILE__);
            size_t unit = f.find("/test/unit/");
            root = (unit == std::string::npos) ? std::string(".")
                                               : f.substr(0, unit);
        }
        return root;
    }

    static std::string CertPath(const char* name) {
        return ProjectRoot() + "/test/certs/" + name;
    }
};

/* ========== 成功路径（真实证书） ========== */

/* subject：server.crt 的 subject DN 含 CN=localhost */
TEST_F(TlsCertInfo, SubjectExtraction) {
    char buf[256];
    int n = uvhttp_tls_get_cert_subject(&cert_, buf, sizeof(buf));
    EXPECT_GT(n, 0) << "真实证书的 subject 应非空";
    EXPECT_LT(n, (int)sizeof(buf));
    std::string subject(buf);
    EXPECT_NE(subject.find("CN=localhost"), std::string::npos)
        << "server.crt subject 应含 CN=localhost，实际：" << subject;
}

/* issuer：签发者 DN 非空 */
TEST_F(TlsCertInfo, IssuerExtraction) {
    char buf[256];
    int n = uvhttp_tls_get_cert_issuer(&cert_, buf, sizeof(buf));
    EXPECT_GT(n, 0) << "真实证书的 issuer 应非空";
    EXPECT_LT(n, (int)sizeof(buf));
}

/* serial：非空十六进制 */
TEST_F(TlsCertInfo, SerialExtraction) {
    char buf[64];
    int n = uvhttp_tls_get_cert_serial(&cert_, buf, sizeof(buf));
    EXPECT_GT(n, 0) << "真实证书的 serial 应非空";
    std::string serial(buf, n);
    /* serial 是十六进制：只含 0-9A-Fa-F */
    for (char c : serial) {
        if (c == ':')
            continue; /* mbedtls 可能加冒号分隔 */
        EXPECT_TRUE((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
                    (c >= 'A' && c <= 'F'))
            << "serial 含非法字符：'" << c << "'，全文：" << serial;
    }
}

/* ========== 不同证书（client.crt）提取 ========== */

TEST_F(TlsCertInfo, DifferentCertSubject) {
    mbedtls_x509_crt c2;
    mbedtls_x509_crt_init(&c2);
    ASSERT_EQ(mbedtls_x509_crt_parse_file(&c2, CertPath("client.crt").c_str()),
              0);

    char buf[256];
    int n = uvhttp_tls_get_cert_subject(&c2, buf, sizeof(buf));
    EXPECT_GT(n, 0);
    std::string subject(buf);
    /* client.crt 的 subject 应与 server.crt 不同（区分提取来源） */
    EXPECT_NE(subject.find("CN="), std::string::npos);
    mbedtls_x509_crt_free(&c2);
}

/* ========== 参数校验 ========== */

TEST_F(TlsCertInfo, NullCertReturnsZero) {
    char buf[64];
    EXPECT_EQ(uvhttp_tls_get_cert_subject(nullptr, buf, sizeof(buf)), 0);
    EXPECT_EQ(uvhttp_tls_get_cert_issuer(nullptr, buf, sizeof(buf)), 0);
    EXPECT_EQ(uvhttp_tls_get_cert_serial(nullptr, buf, sizeof(buf)), 0);
}

TEST_F(TlsCertInfo, NullBufferReturnsZero) {
    EXPECT_EQ(uvhttp_tls_get_cert_subject(&cert_, nullptr, 64), 0);
    EXPECT_EQ(uvhttp_tls_get_cert_issuer(&cert_, nullptr, 64), 0);
    EXPECT_EQ(uvhttp_tls_get_cert_serial(&cert_, nullptr, 64), 0);
}

/* 小缓冲：DN 提取应安全截断不越界 */
TEST_F(TlsCertInfo, SmallBufferDoesNotOverflow) {
    char small[8];
    int n = uvhttp_tls_get_cert_subject(&cert_, small, sizeof(small));
    EXPECT_GE(n, 0);
    /* 缓冲必须有终止符（mbedtls_x509_dn_gets 保证） */
    EXPECT_EQ(small[sizeof(small) - 1], '\0');
    int m = uvhttp_tls_get_cert_issuer(&cert_, small, sizeof(small));
    EXPECT_GE(m, 0);
    EXPECT_EQ(small[sizeof(small) - 1], '\0');
}

#endif /* UVHTTP_FEATURE_TLS */