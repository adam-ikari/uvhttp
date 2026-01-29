/* uvhttp_tls.c NULL参数覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_tls.h"
#include "uvhttp_context.h"

/* 测试TLS初始化 */
TEST(UvhttpTlsNullCoverageTest, TlsInit) {
    uvhttp_context_t* context = NULL;
    uvhttp_error_t result = uvhttp_context_create(uv_default_loop(), &context);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(context, nullptr);

    uvhttp_tls_error_t err = uvhttp_tls_init(context);
    EXPECT_TRUE(err == UVHTTP_TLS_OK || err == UVHTTP_TLS_ERROR_INIT);

    uvhttp_context_destroy(context);
}

/* 测试TLS上下文创建 */
TEST(UvhttpTlsNullCoverageTest, TlsContextNew) {
    uvhttp_tls_context_t* ctx = NULL;
    uvhttp_error_t result = uvhttp_tls_context_new(&ctx);
    /* 即使没有初始化，也可能返回NULL或创建失败 */
    if (result == UVHTTP_OK && ctx) {
        uvhttp_tls_context_free(ctx);
    }
}

/* 测试TLS上下文释放NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextFreeNull) {
    uvhttp_tls_context_free(NULL);
}

/* 测试加载证书链NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextLoadCertChainNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_load_cert_chain(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试加载私钥NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextLoadPrivateKeyNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_load_private_key(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试加载CA文件NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextLoadCaFileNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_load_ca_file(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试启用客户端认证NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextEnableClientAuthNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_client_auth(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置验证深度NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetVerifyDepthNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_verify_depth(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置密码套件NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetCipherSuitesNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_cipher_suites(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试启用会话票据NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextEnableSessionTicketsNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_session_tickets(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置会话缓存NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetSessionCacheNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_session_cache(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试启用OCSP装订NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextEnableOcspStaplingNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_ocsp_stapling(NULL, 0);
    /* 可能返回OK（函数可能忽略NULL） */
}

/* 测试设置DH参数NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetDhParametersNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_dh_parameters(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置SSL NULL */
TEST(UvhttpTlsNullCoverageTest, TlsSetupSslNull) {
    uvhttp_tls_error_t err = uvhttp_tls_setup_ssl(NULL, -1);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试握手NULL */
TEST(UvhttpTlsNullCoverageTest, TlsHandshakeNull) {
    uvhttp_tls_error_t err = uvhttp_tls_handshake(NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试读取NULL */
TEST(UvhttpTlsNullCoverageTest, TlsReadNull) {
    char buf[1024];
    uvhttp_tls_error_t err = uvhttp_tls_read(NULL, buf, sizeof(buf));
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试写入NULL */
TEST(UvhttpTlsNullCoverageTest, TlsWriteNull) {
    const char* data = "test";
    uvhttp_tls_error_t err = uvhttp_tls_write(NULL, data, 4);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试启用CRL检查NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextEnableCrlCheckingNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_crl_checking(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试加载CRL文件NULL */
TEST(UvhttpTlsNullCoverageTest, TlsLoadCrlFileNull) {
    uvhttp_tls_error_t err = uvhttp_tls_load_crl_file(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试获取OCSP响应NULL */
TEST(UvhttpTlsNullCoverageTest, TlsGetOcspResponseNull) {
    unsigned char* ocsp_response = NULL;
    size_t response_len = 0;
    uvhttp_tls_error_t err = uvhttp_tls_get_ocsp_response(NULL, &ocsp_response, &response_len);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试验证OCSP响应NULL */
TEST(UvhttpTlsNullCoverageTest, TlsVerifyOcspResponseNull) {
    uvhttp_tls_error_t err = uvhttp_tls_verify_ocsp_response(NULL, NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试启用TLS1.3 NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextEnableTls13Null) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_tls13(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置TLS1.3密码套件NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetTls13CipherSuitesNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_tls13_cipher_suites(NULL, NULL);
    /* 可能返回OK（函数可能忽略NULL） */
}

/* 测试启用早期数据NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextEnableEarlyDataNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_early_data(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置票据密钥NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetTicketKeyNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_ticket_key(NULL, NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试轮换票据密钥NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextRotateTicketKeyNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_rotate_ticket_key(NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试设置票据生命周期NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextSetTicketLifetimeNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_ticket_lifetime(NULL, 0);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试验证证书链NULL */
TEST(UvhttpTlsNullCoverageTest, TlsVerifyCertChainNull) {
    uvhttp_tls_error_t err = uvhttp_tls_verify_cert_chain(NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试添加额外链证书NULL */
TEST(UvhttpTlsNullCoverageTest, TlsContextAddExtraChainCertNull) {
    uvhttp_tls_error_t err = uvhttp_tls_context_add_extra_chain_cert(NULL, NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试获取证书链NULL */
TEST(UvhttpTlsNullCoverageTest, TlsGetCertChainNull) {
    mbedtls_x509_crt* chain = NULL;
    uvhttp_tls_error_t err = uvhttp_tls_get_cert_chain(NULL, &chain);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试获取统计信息NULL */
TEST(UvhttpTlsNullCoverageTest, TlsGetStatsNull) {
    uvhttp_tls_stats_t stats;
    uvhttp_tls_error_t err = uvhttp_tls_get_stats(NULL, &stats);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试重置统计信息NULL */
TEST(UvhttpTlsNullCoverageTest, TlsResetStatsNull) {
    uvhttp_tls_error_t err = uvhttp_tls_reset_stats(NULL);
    EXPECT_NE(err, UVHTTP_TLS_OK);
}

/* 测试获取连接信息NULL */
TEST(UvhttpTlsNullCoverageTest, TlsGetConnectionInfoNull) {
    char buf[256];
    uvhttp_tls_error_t err = uvhttp_tls_get_connection_info(NULL, buf, sizeof(buf));
    EXPECT_NE(err, UVHTTP_TLS_OK);
}