/*
 * UVHTTP TLS 证书/密钥加载成功路径测试
 *
 * 为什么需要独立测试：load_cert_chain / load_private_key / load_ca_file
 * （src/uvhttp_tls.c:211/230/245）的**成功路径**从未被执行——现有测试
 * （test_tls_api_coverage.cpp:110-260）只测空串 / 不存在文件，全部走
 * mbedtls parse 失败分支就 return。lcov 实测这些成功分支零覆盖。
 *
 * 成功路径是真实 TLS 服务器的核心启动序列：
 *   1. load_private_key(key.pem)  → mbedtls_pk_parse_keyfile 成功
 *   2. load_cert_chain(cert.pem)   → x509 parse + conf_own_cert 成功
 *   3. load_ca_file(ca.crt)        → x509 parse + conf_ca_chain 成功
 *
 * 只有这三个都成功，服务器才能启动 TLS。任何一个静默失败 → 握手失败。
 *
 * 注：probe 实测 load_cert_chain 即使私钥不匹配也返回 OK——mbedtls 的
 * mbedtls_ssl_conf_own_cert 只检查 cert/key 非空，不校验配对。这是
 * mbedtls 行为，非本库缺口，故不把它当失败路径测。
 */

#include <gtest/gtest.h>
#include <string.h>

#if UVHTTP_FEATURE_TLS

extern "C" {
#    include "uvhttp_tls.h"
}

class TlsLoad : public ::testing::Test {
   protected:
    uvhttp_tls_context_t* ctx_ = nullptr;

    void SetUp() override {
        ASSERT_EQ(uvhttp_tls_context_new(&ctx_), UVHTTP_OK);
        ASSERT_NE(ctx_, nullptr);
    }

    void TearDown() override {
        if (ctx_) {
            uvhttp_tls_context_free(ctx_);
            ctx_ = nullptr;
        }
    }

    /* __FILE__ 在本项目 CMake 构建下展开为绝对路径（#434/#461 验证过） */
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

/* ========== 成功路径（真实证书/密钥） ========== */

/* key.pem 是 generate_certs.sh 生成的私钥 */
TEST_F(TlsLoad, LoadPrivateKeySucceeds) {
    EXPECT_EQ(
        uvhttp_tls_context_load_private_key(ctx_, CertPath("key.pem").c_str()),
        UVHTTP_OK)
        << "真实私钥文件应成功解析";
}

/* cert.pem 与 key.pem 配对（服务器证书+私钥） */
TEST_F(TlsLoad, LoadCertChainAfterKeySucceeds) {
    ASSERT_EQ(
        uvhttp_tls_context_load_private_key(ctx_, CertPath("key.pem").c_str()),
        UVHTTP_OK);
    EXPECT_EQ(
        uvhttp_tls_context_load_cert_chain(ctx_, CertPath("cert.pem").c_str()),
        UVHTTP_OK)
        << "先加载私钥后，证书链应成功（conf_own_cert 需要 pkey 已就绪）";
}

/* 独立加载：load_cert_chain 在未加载私钥时（pkey 零初始化）行为由
 * conf_own_cert 决定——实测仍 OK（mbedtls 不校验），记录实际行为 */
TEST_F(TlsLoad, LoadCertChainWithoutKeyStillSucceeds) {
    EXPECT_EQ(
        uvhttp_tls_context_load_cert_chain(ctx_, CertPath("cert.pem").c_str()),
        UVHTTP_OK)
        << "实测 conf_own_cert 不因 pkey 未加载而失败（mbedtls 行为）";
}

/* ca.crt 是项目自带 CA */
TEST_F(TlsLoad, LoadCaFileSucceeds) {
    EXPECT_EQ(uvhttp_tls_context_load_ca_file(ctx_, CertPath("ca.crt").c_str()),
              UVHTTP_OK)
        << "真实 CA 证书应成功解析并配置到 ca_chain";
}

/* ========== 完整启动序列 ========== */

/* 服务器 TLS 启动的标准顺序：私钥 → 证书链 → CA */
TEST_F(TlsLoad, FullStartupSequenceSucceeds) {
    ASSERT_EQ(
        uvhttp_tls_context_load_private_key(ctx_, CertPath("key.pem").c_str()),
        UVHTTP_OK);
    ASSERT_EQ(
        uvhttp_tls_context_load_cert_chain(ctx_, CertPath("cert.pem").c_str()),
        UVHTTP_OK);
    EXPECT_EQ(uvhttp_tls_context_load_ca_file(ctx_, CertPath("ca.crt").c_str()),
              UVHTTP_OK)
        << "私钥+证书链+CA 三段加载都应成功";
}

/* ========== 参数校验（成功侧的对照） ========== */

/* 缺失参数仍是 INVALID_PARAM（回归保护） */
TEST_F(TlsLoad, NullContextReturnsInvalidParam) {
    EXPECT_EQ(uvhttp_tls_context_load_private_key(nullptr, "x.pem"),
              UVHTTP_ERROR_TLS_INVALID_PARAM);
    EXPECT_EQ(uvhttp_tls_context_load_cert_chain(nullptr, "x.crt"),
              UVHTTP_ERROR_TLS_INVALID_PARAM);
    EXPECT_EQ(uvhttp_tls_context_load_ca_file(nullptr, "x.crt"),
              UVHTTP_ERROR_TLS_INVALID_PARAM);
}

#endif /* UVHTTP_FEATURE_TLS */