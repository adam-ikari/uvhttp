/*
 * UVHTTP TLS 证书链扩展行为测试
 *
 * 为什么需要独立测试：uvhttp_tls_context_add_extra_chain_cert
 * （src/uvhttp_tls.c:669）实现「向 server 的证书链追加中间证书」，
 * 有三条错误分支从未被执行——
 *
 *     mbedtls_x509_crt_parse_file(&extra_cert, cert_file)
 *     if (ret != 0) {            ← ① TLS_PARSE：未覆盖，用不存在/垃圾文件触发
 *         ...
 *         return UVHTTP_ERROR_TLS_PARSE;
 *     }
 *     current->next = uvhttp_calloc(1, sizeof(mbedtls_x509_crt));
 *     if (!current->next) {      ← ② TLS_MEMORY：OOM 路径，单测不可达
 *         ...
 *         return UVHTTP_ERROR_TLS_MEMORY;
 *     }
 *     memcpy(current->next, &extra_cert, ...);   ← ③ 成功：未覆盖
 *
 * 现有 test_tls_null_coverage.cpp:171 只测 `add_extra_chain_cert(NULL, NULL)`，
 * 走第一行参数校验就 return，函数体（链遍历 while、calloc、memcpy）从未执行。
 *
 * 证书链扩展失败的实际危害：服务器只发 leaf 证书、不发中间证书 →
 * 客户端因缺中间 CA 无法验证链 → TLS 握手失败（浏览器报
 * NET::ERR_CERT_AUTHORITY_INVALID）。
 */

#include <gtest/gtest.h>
#include <stdio.h>
#include <string.h>

#if UVHTTP_FEATURE_TLS

extern "C" {
#    include "uvhttp_tls.h"

#    include <mbedtls/x509_crt.h>
}

class TlsCertChain : public ::testing::Test {
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

    /* __FILE__ 在本项目 CMake 构建下展开为绝对路径（#434 验证过）。
     * 上溯到源码根，拼出 test/certs/。 */
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

/* ========== 参数校验 ========== */

TEST_F(TlsCertChain, NullContextReturnsInvalidParam) {
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(nullptr, "x.crt"),
              UVHTTP_ERROR_TLS_INVALID_PARAM);
}

TEST_F(TlsCertChain, NullCertFileReturnsInvalidParam) {
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(ctx_, nullptr),
              UVHTTP_ERROR_TLS_INVALID_PARAM);
}

/* ========== parse 失败分支 ========== */

/* 不存在的文件：mbedtls_x509_crt_parse_file 返回非 0 → TLS_PARSE */
TEST_F(TlsCertChain, NonexistentFileReturnsParseError) {
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("does_not_exist.crt").c_str()),
              UVHTTP_ERROR_TLS_PARSE)
        << "不存在的证书应被拒绝（否则会静默吞掉错误，链中混入 NULL）";
}

/* 空文件同样 parse 失败 */
TEST_F(TlsCertChain, EmptyFileReturnsParseError) {
    std::string path = CertPath("empty_extra.crt");
    FILE* f = fopen(path.c_str(), "w");
    ASSERT_NE(f, nullptr);
    ASSERT_EQ(fputc('\n', f), '\n');
    fclose(f);

    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(ctx_, path.c_str()),
              UVHTTP_ERROR_TLS_PARSE);

    remove(path.c_str());
}

/* 非 PEM 内容（垃圾）parse 失败 */
TEST_F(TlsCertChain, GarbageContentReturnsParseError) {
    std::string path = CertPath("garbage_extra.crt");
    FILE* f = fopen(path.c_str(), "w");
    ASSERT_NE(f, nullptr);
    fputs("this is definitely not a certificate\n", f);
    fclose(f);

    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(ctx_, path.c_str()),
              UVHTTP_ERROR_TLS_PARSE);

    remove(path.c_str());
}

/* ========== 成功路径 ========== */

/* ca.crt 是项目自带的 CA 证书（test/certs/generate_certs.sh 生成），
 * 可作为合法的额外链证书。 */
TEST_F(TlsCertChain, ValidExtraCertReturnsOk) {
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("ca.crt").c_str()),
              UVHTTP_OK)
        << "合法证书应成功追加到链尾";
}

/* server.crt 同样是合法证书，可作链中间 */
TEST_F(TlsCertChain, ServerCertAsExtraReturnsOk) {
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("server.crt").c_str()),
              UVHTTP_OK);
}

/* client.crt 是 client 端证书，也是合法 X.509 */
TEST_F(TlsCertChain, ClientCertAsExtraReturnsOk) {
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("client.crt").c_str()),
              UVHTTP_OK);
}

/* ========== 链追加（while current->next） ========== */

/* 连续追加多个证书：链应不断增长（while 循环走到新链尾） */
TEST_F(TlsCertChain, MultipleExtraCertsAppendToChain) {
    ASSERT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("ca.crt").c_str()),
              UVHTTP_OK);
    ASSERT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("client.crt").c_str()),
              UVHTTP_OK);
    ASSERT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("server.crt").c_str()),
              UVHTTP_OK);

    /* 第 4 个仍应成功——链已增长到 3 个 extra，while 仍能找到链尾 */
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("ca.crt").c_str()),
              UVHTTP_OK)
        << "链已增长后追加应仍成功（while 必须遍历到新链尾）";
}

/* 成功后失败不应破坏已成功的链——后续再追加合法证书仍应成功 */
TEST_F(TlsCertChain, FailedAppendAfterSuccessDoesNotCorruptChain) {
    ASSERT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("ca.crt").c_str()),
              UVHTTP_OK);

    /* 失败的追加 */
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("nonexistent.crt").c_str()),
              UVHTTP_ERROR_TLS_PARSE);

    /* 链应仍可继续追加（失败分支已 free extra_cert，不应损坏链） */
    EXPECT_EQ(uvhttp_tls_context_add_extra_chain_cert(
                  ctx_, CertPath("client.crt").c_str()),
              UVHTTP_OK)
        << "失败的 parse 不应损坏已建立的链";
}

#endif /* UVHTTP_FEATURE_TLS */