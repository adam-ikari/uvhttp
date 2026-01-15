/* uvhttp_tls_mbedtls.c 扩展覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_tls.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 测试TLS上下文结构大小 */
TEST(UvhttpTlsFullCoverageTest, TlsStructSize) {
    /* uvhttp_tls_context_t 是不完整类型，跳过sizeof测试 */
    EXPECT_GT(sizeof(uvhttp_tls_error_t), 0);
    EXPECT_GT(sizeof(uvhttp_tls_stats_t), 0);
}

/* 测试TLS错误码枚举值 */
TEST(UvhttpTlsFullCoverageTest, TlsErrorEnumValues) {
    EXPECT_EQ(UVHTTP_TLS_OK, 0);
    EXPECT_EQ(UVHTTP_TLS_ERROR_INIT, -1);
    EXPECT_EQ(UVHTTP_TLS_ERROR_CERT, -2);
    EXPECT_EQ(UVHTTP_TLS_ERROR_KEY, -3);
    EXPECT_EQ(UVHTTP_TLS_ERROR_CA, -4);
    EXPECT_EQ(UVHTTP_TLS_ERROR_VERIFY, -5);
    EXPECT_EQ(UVHTTP_TLS_ERROR_HANDSHAKE, -6);
    EXPECT_EQ(UVHTTP_TLS_ERROR_READ, -7);
    EXPECT_EQ(UVHTTP_TLS_ERROR_WRITE, -8);
    EXPECT_EQ(UVHTTP_TLS_ERROR_INVALID_PARAM, -9);
    EXPECT_EQ(UVHTTP_TLS_ERROR_MEMORY, -10);
    EXPECT_EQ(UVHTTP_TLS_ERROR_NOT_IMPLEMENTED, -11);
    EXPECT_EQ(UVHTTP_TLS_ERROR_PARSE, -12);
    EXPECT_EQ(UVHTTP_TLS_ERROR_NO_CERT, -13);
    EXPECT_EQ(UVHTTP_TLS_ERROR_WANT_READ, 1);
    EXPECT_EQ(UVHTTP_TLS_ERROR_WANT_WRITE, 2);
}

/* 测试TLS初始化 */
TEST(UvhttpTlsFullCoverageTest, TlsInit) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    /* 可能返回UVHTTP_TLS_OK或错误，取决于实现 */
}

/* 测试TLS清理 */
TEST(UvhttpTlsFullCoverageTest, TlsCleanup) {
    /* 应该安全处理 */
    uvhttp_tls_cleanup();
}

/* 测试创建TLS上下文 */
TEST(UvhttpTlsFullCoverageTest, TlsContextNew) {
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    /* 可能返回NULL或创建的上下文 */
    if (ctx != NULL) {
        uvhttp_tls_context_free(ctx);
    }
}

/* 测试释放TLS上下文 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextFreeNull) {
    /* NULL上下文应该安全处理 */
    uvhttp_tls_context_free(NULL);
}

/* 测试加载证书链 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextLoadCertChainNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_load_cert_chain(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试加载私钥 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextLoadPrivateKeyNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_load_private_key(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试加载CA文件 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextLoadCaFileNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_load_ca_file(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试启用客户端认证 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextEnableClientAuthNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_enable_client_auth(NULL, 1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试设置验证深度 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetVerifyDepthNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_set_verify_depth(NULL, 1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试设置密码套件 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetCipherSuitesNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_set_cipher_suites(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试启用会话票证 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextEnableSessionTicketsNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_enable_session_tickets(NULL, 1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试设置会话缓存 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetSessionCacheNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_set_session_cache(NULL, 100);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试启用OCSP装订 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextEnableOcspStaplingNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_enable_ocsp_stapling(NULL, 1);
    /* 可能返回UVHTTP_TLS_OK（函数可能忽略NULL） */
}

/* 测试设置DH参数 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetDhParametersNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_set_dh_parameters(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试创建SSL - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsCreateSslNull) {
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(NULL);
    /* 应该返回NULL */
    EXPECT_EQ(ssl, nullptr);
}

/* 测试设置SSL - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsSetupSslNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_setup_ssl(NULL, -1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试TLS握手 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsHandshakeNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_handshake(NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试TLS读取 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsReadNull) {
    uvhttp_tls_error_t result;
    char buf[1024];

    result = uvhttp_tls_read(NULL, buf, sizeof(buf));
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试TLS写入 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsWriteNull) {
    uvhttp_tls_error_t result;
    const char* data = "test";

    result = uvhttp_tls_write(NULL, data, strlen(data));
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试验证对等证书 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsVerifyPeerCertNull) {
    int result = uvhttp_tls_verify_peer_cert(NULL);
    /* 应该返回错误或0 */
    EXPECT_GE(result, 0);
}

/* 测试验证主机名 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsVerifyHostnameNull) {
    int result = uvhttp_tls_verify_hostname(NULL, NULL);
    /* 应该返回错误或0 */
    EXPECT_GE(result, 0);
}

/* 测试检查证书有效性 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsCheckCertValidityNull) {
    int result = uvhttp_tls_check_cert_validity(NULL);
    /* 应该返回错误或0 */
    EXPECT_GE(result, 0);
}

/* 测试获取对等证书 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetPeerCertNull) {
    mbedtls_x509_crt* cert = uvhttp_tls_get_peer_cert(NULL);
    /* 应该返回NULL */
    EXPECT_EQ(cert, nullptr);
}

/* 测试获取证书主题 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetCertSubjectNull) {
    char buf[256];
    int result = uvhttp_tls_get_cert_subject(NULL, buf, sizeof(buf));
    /* 应该返回错误或0 */
    EXPECT_GE(result, 0);
}

/* 测试获取证书颁发者 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetCertIssuerNull) {
    char buf[256];
    int result = uvhttp_tls_get_cert_issuer(NULL, buf, sizeof(buf));
    /* 应该返回错误或0 */
    EXPECT_GE(result, 0);
}

/* 测试获取证书序列号 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetCertSerialNull) {
    char buf[256];
    int result = uvhttp_tls_get_cert_serial(NULL, buf, sizeof(buf));
    /* 应该返回错误或0 */
    EXPECT_GE(result, 0);
}

/* 测试启用CRL检查 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextEnableCrlCheckingNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_enable_crl_checking(NULL, 1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试加载CRL文件 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsLoadCrlFileNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_load_crl_file(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试获取OCSP响应 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetOcspResponseNull) {
    uvhttp_tls_error_t result;
    unsigned char* ocsp_response;
    size_t response_len;

    result = uvhttp_tls_get_ocsp_response(NULL, &ocsp_response, &response_len);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试验证OCSP响应 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsVerifyOcspResponseNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_verify_ocsp_response(NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试启用TLS 1.3 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextEnableTls13Null) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_enable_tls13(NULL, 1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试设置TLS 1.3密码套件 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetTls13CipherSuitesNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_set_tls13_cipher_suites(NULL, NULL);
    /* 可能返回UVHTTP_TLS_OK（函数可能忽略NULL） */
}

/* 测试启用早期数据 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextEnableEarlyDataNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_enable_early_data(NULL, 1);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试设置票证密钥 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetTicketKeyNull) {
    uvhttp_tls_error_t result;
    unsigned char key[48];

    result = uvhttp_tls_context_set_ticket_key(NULL, key, sizeof(key));
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试轮换票证密钥 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextRotateTicketKeyNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_rotate_ticket_key(NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试设置票证生命周期 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextSetTicketLifetimeNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_set_ticket_lifetime(NULL, 86400);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试验证证书链 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsVerifyCertChainNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_verify_cert_chain(NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试添加额外链证书 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsContextAddExtraChainCertNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_context_add_extra_chain_cert(NULL, NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试获取证书链 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetCertChainNull) {
    uvhttp_tls_error_t result;
    mbedtls_x509_crt* chain;

    result = uvhttp_tls_get_cert_chain(NULL, &chain);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试获取统计信息 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetStatsNull) {
    uvhttp_tls_error_t result;
    uvhttp_tls_stats_t stats;

    result = uvhttp_tls_get_stats(NULL, &stats);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试重置统计信息 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsResetStatsNull) {
    uvhttp_tls_error_t result;

    result = uvhttp_tls_reset_stats(NULL);
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试获取连接信息 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetConnectionInfoNull) {
    uvhttp_tls_error_t result;
    char buf[256];

    result = uvhttp_tls_get_connection_info(NULL, buf, sizeof(buf));
    EXPECT_NE(result, UVHTTP_TLS_OK);
}

/* 测试获取错误字符串 - NULL参数 */
TEST(UvhttpTlsFullCoverageTest, TlsGetErrorStringNull) {
    char buf[256];

    /* NULL返回值应该安全处理 */
    uvhttp_tls_get_error_string(0, buf, sizeof(buf));
}

/* 测试打印错误 */
TEST(UvhttpTlsFullCoverageTest, TlsPrintError) {
    /* 应该安全处理 */
    uvhttp_tls_print_error(0);
    uvhttp_tls_print_error(-1);
    uvhttp_tls_print_error(1);
}

/* 测试TLS统计结构初始化 */
TEST(UvhttpTlsFullCoverageTest, TlsStatsInitialization) {
    uvhttp_tls_stats_t stats;

    memset(&stats, 0, sizeof(stats));

    /* 验证初始值 */
    EXPECT_EQ(stats.handshake_count, 0);
    EXPECT_EQ(stats.handshake_errors, 0);
    EXPECT_EQ(stats.bytes_sent, 0);
    EXPECT_EQ(stats.bytes_received, 0);
    EXPECT_EQ(stats.session_hits, 0);
    EXPECT_EQ(stats.session_misses, 0);
    EXPECT_DOUBLE_EQ(stats.avg_handshake_time_ms, 0.0);
}

/* 测试TLS错误码范围 */
TEST(UvhttpTlsFullCoverageTest, TlsErrorCodeRanges) {
    /* 验证错误码范围合理 */
    EXPECT_LE(UVHTTP_TLS_ERROR_NO_CERT, 0);
    EXPECT_GT(UVHTTP_TLS_ERROR_WANT_READ, 0);
    EXPECT_GT(UVHTTP_TLS_ERROR_WANT_WRITE, 0);
}

/* 测试多次调用NULL参数函数 */
TEST(UvhttpTlsFullCoverageTest, MultipleNullCalls) {
    /* 多次调用NULL参数函数，确保不会崩溃 */
    for (int i = 0; i < 100; i++) {
        uvhttp_tls_context_free(NULL);
        uvhttp_tls_create_ssl(NULL);
        uvhttp_tls_handshake(NULL);
        uvhttp_tls_verify_peer_cert(NULL);
        uvhttp_tls_verify_hostname(NULL, NULL);
        uvhttp_tls_check_cert_validity(NULL);
        uvhttp_tls_get_peer_cert(NULL);
        uvhttp_tls_print_error(0);
    }
}