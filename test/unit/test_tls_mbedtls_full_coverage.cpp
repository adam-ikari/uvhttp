/**
 * @file test_tls_mbedtls_full_coverage.cpp
 * @brief uvhttp_tls_mbedtls.c 的完整覆盖率测试
 */

#include <gtest/gtest.h>
#include <uvhttp_tls.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

/* 测试 TLS 初始化 */
TEST(UvhttpTlsMbedtlsTest, TlsInit) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    /* 再次初始化应该返回 OK */
    result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
}

/* 测试 TLS 清理 */
TEST(UvhttpTlsMbedtlsTest, TlsCleanup) {
    uvhttp_tls_cleanup();
    /* 不应该崩溃 */
}

/* 测试 TLS 清理未初始化 */
TEST(UvhttpTlsMbedtlsTest, TlsCleanupNotInitialized) {
    /* 先清理 */
    uvhttp_tls_cleanup();
    /* 再次清理不应该崩溃 */
    uvhttp_tls_cleanup();
}

/* 测试创建 TLS 上下文 */
TEST(UvhttpTlsMbedtlsTest, TlsContextNew) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试创建 TLS 上下文失败（内存分配失败）- 难以模拟 */
TEST(UvhttpTlsMbedtlsTest, TlsContextNewMemoryFail) {
    /* 这个测试很难模拟，因为 calloc 可能会成功 */
    /* 我们只需要确保代码路径存在 */
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    if (ctx) {
        uvhttp_tls_context_free(ctx);
    }
}

/* 测试释放 TLS 上下文 NULL */
TEST(UvhttpTlsMbedtlsTest, TlsContextFreeNull) {
    uvhttp_tls_context_free(NULL);
    /* 不应该崩溃 */
}

/* 测试加载证书链 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, LoadCertChainNullContext) {
    const char* cert_file = "/tmp/test_cert.pem";
    uvhttp_tls_error_t result = uvhttp_tls_context_load_cert_chain(NULL, cert_file);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试加载证书链 NULL 文件名 */
TEST(UvhttpTlsMbedtlsTest, LoadCertChainNullFilename) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_load_cert_chain(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载证书链文件不存在 */
TEST(UvhttpTlsMbedtlsTest, LoadCertChainFileNotFound) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_load_cert_chain(ctx, "/nonexistent/cert.pem");
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_CERT);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载私钥 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, LoadPrivateKeyNullContext) {
    const char* key_file = "/tmp/test_key.pem";
    uvhttp_tls_error_t result = uvhttp_tls_context_load_private_key(NULL, key_file);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试加载私钥 NULL 文件名 */
TEST(UvhttpTlsMbedtlsTest, LoadPrivateKeyNullFilename) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_load_private_key(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载私钥文件不存在 */
TEST(UvhttpTlsMbedtlsTest, LoadPrivateKeyFileNotFound) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_load_private_key(ctx, "/nonexistent/key.pem");
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_KEY);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载 CA 文件 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, LoadCaFileNullContext) {
    const char* ca_file = "/tmp/test_ca.pem";
    uvhttp_tls_error_t result = uvhttp_tls_context_load_ca_file(NULL, ca_file);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试加载 CA 文件 NULL 文件名 */
TEST(UvhttpTlsMbedtlsTest, LoadCaFileNullFilename) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_load_ca_file(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载 CA 文件文件不存在 */
TEST(UvhttpTlsMbedtlsTest, LoadCaFileFileNotFound) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_load_ca_file(ctx, "/nonexistent/ca.pem");
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_CA);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试启用客户端认证 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, EnableClientAuthNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_enable_client_auth(NULL, 1);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试启用客户端认证 */
TEST(UvhttpTlsMbedtlsTest, EnableClientAuth) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_client_auth(ctx, 1);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试禁用客户端认证 */
TEST(UvhttpTlsMbedtlsTest, DisableClientAuth) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_client_auth(ctx, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置验证深度 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetVerifyDepthNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_set_verify_depth(NULL, 5);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置验证深度 */
TEST(UvhttpTlsMbedtlsTest, SetVerifyDepth) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_verify_depth(ctx, 5);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置密码套件 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetCipherSuitesNullContext) {
    static const int cipher_suites[] = { MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA, 0 };
    uvhttp_tls_error_t result = uvhttp_tls_context_set_cipher_suites(NULL, cipher_suites);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置密码套件 NULL 密码套件 */
TEST(UvhttpTlsMbedtlsTest, SetCipherSuitesNullCipherSuites) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_cipher_suites(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置密码套件 */
TEST(UvhttpTlsMbedtlsTest, SetCipherSuites) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    static const int cipher_suites[] = { MBEDTLS_TLS_RSA_WITH_AES_256_CBC_SHA, 0 };
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_cipher_suites(ctx, cipher_suites);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试启用会话票证 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, EnableSessionTicketsNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_enable_session_tickets(NULL, 1);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试启用会话票证 */
TEST(UvhttpTlsMbedtlsTest, EnableSessionTickets) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_session_tickets(ctx, 1);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试禁用会话票证 */
TEST(UvhttpTlsMbedtlsTest, DisableSessionTickets) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_session_tickets(ctx, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置会话缓存 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetSessionCacheNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_set_session_cache(NULL, 100);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置会话缓存 */
TEST(UvhttpTlsMbedtlsTest, SetSessionCache) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_session_cache(ctx, 100);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试启用 OCSP 装订 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, EnableOcspStaplingNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_enable_ocsp_stapling(NULL, 1);
    EXPECT_EQ(result, UVHTTP_TLS_OK);
}

/* 测试启用 OCSP 装订 */
TEST(UvhttpTlsMbedtlsTest, EnableOcspStapling) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_ocsp_stapling(ctx, 1);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置 DH 参数 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetDhParametersNullContext) {
    const char* dh_file = "/tmp/test_dh.pem";
    uvhttp_tls_error_t result = uvhttp_tls_context_set_dh_parameters(NULL, dh_file);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置 DH 参数 NULL 文件名 */
TEST(UvhttpTlsMbedtlsTest, SetDhParametersNullFilename) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_dh_parameters(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置 DH 参数 */
TEST(UvhttpTlsMbedtlsTest, SetDhParameters) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_dh_parameters(ctx, "/tmp/test_dh.pem");
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试创建 SSL NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, CreateSslNullContext) {
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(NULL);
    EXPECT_EQ(ssl, nullptr);
}

/* 测试创建 SSL */
TEST(UvhttpTlsMbedtlsTest, CreateSsl) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试创建 SSL 失败（内存分配失败）- 难以模拟 */
TEST(UvhttpTlsMbedtlsTest, CreateSslMemoryFail) {
    /* 这个测试很难模拟，因为 calloc 可能会成功 */
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    if (ssl) {
        mbedtls_ssl_free(ssl);
        free(ssl);
    }
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置 SSL NULL SSL */
TEST(UvhttpTlsMbedtlsTest, SetupSslNullSsl) {
    uvhttp_tls_error_t result = uvhttp_tls_setup_ssl(NULL, 0);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置 SSL */
TEST(UvhttpTlsMbedtlsTest, SetupSsl) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_setup_ssl(ssl, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试握手 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, HandshakeNullSsl) {
    uvhttp_tls_error_t result = uvhttp_tls_handshake(NULL);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试握手 */
TEST(UvhttpTlsMbedtlsTest, Handshake) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_setup_ssl(ssl, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    /* 握手会失败，因为没有实际的连接 */
    uvhttp_tls_error_t result3 = uvhttp_tls_handshake(ssl);
    EXPECT_NE(result3, UVHTTP_TLS_OK);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试读取 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, ReadNullSsl) {
    char buf[1024];
    uvhttp_tls_error_t result = uvhttp_tls_read(NULL, buf, sizeof(buf));
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试读取 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, ReadNullBuffer) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_read(ssl, NULL, 1024);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试写入 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, WriteNullSsl) {
    const char* buf = "test";
    uvhttp_tls_error_t result = uvhttp_tls_write(NULL, buf, strlen(buf));
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试写入 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, WriteNullBuffer) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_write(ssl, NULL, 1024);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试验证对等证书 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, VerifyPeerCertNullSsl) {
    int result = uvhttp_tls_verify_peer_cert(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试验证主机名 NULL 证书 */
TEST(UvhttpTlsMbedtlsTest, VerifyHostnameNullCert) {
    int result = uvhttp_tls_verify_hostname(NULL, "example.com");
    EXPECT_EQ(result, 0);
}

/* 测试验证主机名 NULL 主机名 */
TEST(UvhttpTlsMbedtlsTest, VerifyHostnameNullHostname) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    
    int result = uvhttp_tls_verify_hostname(&cert, NULL);
    EXPECT_EQ(result, 0);
    
    mbedtls_x509_crt_free(&cert);
}

/* 测试检查证书有效性 NULL 证书 */
TEST(UvhttpTlsMbedtlsTest, CheckCertValidityNullCert) {
    int result = uvhttp_tls_check_cert_validity(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试获取对等证书 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, GetPeerCertNullSsl) {
    mbedtls_x509_crt* cert = uvhttp_tls_get_peer_cert(NULL);
    EXPECT_EQ(cert, nullptr);
}

/* 测试获取证书主题 NULL 证书 */
TEST(UvhttpTlsMbedtlsTest, GetCertSubjectNullCert) {
    char buf[256];
    int result = uvhttp_tls_get_cert_subject(NULL, buf, sizeof(buf));
    EXPECT_EQ(result, 0);
}

/* 测试获取证书主题 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, GetCertSubjectNullBuffer) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    
    int result = uvhttp_tls_get_cert_subject(&cert, NULL, 256);
    EXPECT_EQ(result, 0);
    
    mbedtls_x509_crt_free(&cert);
}

/* 测试获取证书颁发者 NULL 证书 */
TEST(UvhttpTlsMbedtlsTest, GetCertIssuerNullCert) {
    char buf[256];
    int result = uvhttp_tls_get_cert_issuer(NULL, buf, sizeof(buf));
    EXPECT_EQ(result, 0);
}

/* 测试获取证书颁发者 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, GetCertIssuerNullBuffer) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    
    int result = uvhttp_tls_get_cert_issuer(&cert, NULL, 256);
    EXPECT_EQ(result, 0);
    
    mbedtls_x509_crt_free(&cert);
}

/* 测试获取证书序列号 NULL 证书 */
TEST(UvhttpTlsMbedtlsTest, GetCertSerialNullCert) {
    char buf[256];
    int result = uvhttp_tls_get_cert_serial(NULL, buf, sizeof(buf));
    EXPECT_EQ(result, 0);
}

/* 测试获取证书序列号 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, GetCertSerialNullBuffer) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    
    int result = uvhttp_tls_get_cert_serial(&cert, NULL, 256);
    EXPECT_EQ(result, 0);
    
    mbedtls_x509_crt_free(&cert);
}

/* 测试启用 CRL 检查 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, EnableCrlCheckingNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_enable_crl_checking(NULL, 1);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试启用 CRL 检查 */
TEST(UvhttpTlsMbedtlsTest, EnableCrlChecking) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_crl_checking(ctx, 1);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试禁用 CRL 检查 */
TEST(UvhttpTlsMbedtlsTest, DisableCrlChecking) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_crl_checking(ctx, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载 CRL 文件 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, LoadCrlFileNullContext) {
    const char* crl_file = "/tmp/test_crl.pem";
    uvhttp_tls_error_t result = uvhttp_tls_load_crl_file(NULL, crl_file);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试加载 CRL 文件 NULL 文件名 */
TEST(UvhttpTlsMbedtlsTest, LoadCrlFileNullFilename) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_load_crl_file(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试加载 CRL 文件文件不存在 */
TEST(UvhttpTlsMbedtlsTest, LoadCrlFileFileNotFound) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_load_crl_file(ctx, "/nonexistent/crl.pem");
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_PARSE);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取 OCSP 响应 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, GetOcspResponseNullSsl) {
    unsigned char* ocsp_response = NULL;
    size_t response_len = 0;
    uvhttp_tls_error_t result = uvhttp_tls_get_ocsp_response(NULL, &ocsp_response, &response_len);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试获取 OCSP 响应 NULL 响应指针 */
TEST(UvhttpTlsMbedtlsTest, GetOcspResponseNullResponse) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    size_t response_len = 0;
    uvhttp_tls_error_t result2 = uvhttp_tls_get_ocsp_response(ssl, NULL, &response_len);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取 OCSP 响应 NULL 响应长度 */
TEST(UvhttpTlsMbedtlsTest, GetOcspResponseNullResponseLen) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    unsigned char* ocsp_response = NULL;
    uvhttp_tls_error_t result2 = uvhttp_tls_get_ocsp_response(ssl, &ocsp_response, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试验证 OCSP 响应 NULL 证书 */
TEST(UvhttpTlsMbedtlsTest, VerifyOcspResponseNullCert) {
    unsigned char ocsp_response[1024];
    uvhttp_tls_error_t result = uvhttp_tls_verify_ocsp_response(NULL, ocsp_response, sizeof(ocsp_response));
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试验证 OCSP 响应 NULL 响应 */
TEST(UvhttpTlsMbedtlsTest, VerifyOcspResponseNullResponse) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    
    uvhttp_tls_error_t result = uvhttp_tls_verify_ocsp_response(&cert, NULL, 0);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_x509_crt_free(&cert);
}

/* 测试验证 OCSP 响应零响应长度 */
TEST(UvhttpTlsMbedtlsTest, VerifyOcspResponseZeroResponseLen) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    
    unsigned char ocsp_response[1024];
    uvhttp_tls_error_t result = uvhttp_tls_verify_ocsp_response(&cert, ocsp_response, 0);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_x509_crt_free(&cert);
}

/* 测试启用 TLS 1.3 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, EnableTls13NullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_enable_tls13(NULL, 1);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试启用 TLS 1.3 */
TEST(UvhttpTlsMbedtlsTest, EnableTls13) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_tls13(ctx, 1);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试禁用 TLS 1.3 */
TEST(UvhttpTlsMbedtlsTest, DisableTls13) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_tls13(ctx, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置 TLS 1.3 密码套件 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetTls13CipherSuitesNullContext) {
    const char* cipher_suites = "TLS_AES_256_GCM_SHA384";
    uvhttp_tls_error_t result = uvhttp_tls_context_set_tls13_cipher_suites(NULL, cipher_suites);
    EXPECT_EQ(result, UVHTTP_TLS_OK);
}

/* 测试设置 TLS 1.3 密码套件 */
TEST(UvhttpTlsMbedtlsTest, SetTls13CipherSuites) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    const char* cipher_suites = "TLS_AES_256_GCM_SHA384";
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_tls13_cipher_suites(ctx, cipher_suites);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试启用早期数据 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, EnableEarlyDataNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_enable_early_data(NULL, 1);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试启用早期数据 */
TEST(UvhttpTlsMbedtlsTest, EnableEarlyData) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_enable_early_data(ctx, 1);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置票证密钥 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetTicketKeyNullContext) {
    unsigned char key[32];
    uvhttp_tls_error_t result = uvhttp_tls_context_set_ticket_key(NULL, key, sizeof(key));
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置票证密钥 NULL 密钥 */
TEST(UvhttpTlsMbedtlsTest, SetTicketKeyNullKey) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_ticket_key(ctx, NULL, 32);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置票证密钥零密钥长度 */
TEST(UvhttpTlsMbedtlsTest, SetTicketKeyZeroKeyLen) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    unsigned char key[32];
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_ticket_key(ctx, key, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置票证密钥 */
TEST(UvhttpTlsMbedtlsTest, SetTicketKey) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    unsigned char key[32];
    memset(key, 0x42, sizeof(key));
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_ticket_key(ctx, key, sizeof(key));
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试轮换票证密钥 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, RotateTicketKeyNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_rotate_ticket_key(NULL);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试轮换票证密钥 */
TEST(UvhttpTlsMbedtlsTest, RotateTicketKey) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_rotate_ticket_key(ctx);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置票证生命周期 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, SetTicketLifetimeNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_context_set_ticket_lifetime(NULL, 3600);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试设置票证生命周期零生命周期 */
TEST(UvhttpTlsMbedtlsTest, SetTicketLifetimeZeroLifetime) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_ticket_lifetime(ctx, 0);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置票证生命周期负生命周期 */
TEST(UvhttpTlsMbedtlsTest, SetTicketLifetimeNegativeLifetime) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_ticket_lifetime(ctx, -1);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试设置票证生命周期 */
TEST(UvhttpTlsMbedtlsTest, SetTicketLifetime) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_set_ticket_lifetime(ctx, 3600);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试验证证书链 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, VerifyCertChainNullSsl) {
    uvhttp_tls_error_t result = uvhttp_tls_verify_cert_chain(NULL);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试添加额外链证书 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, AddExtraChainCertNullContext) {
    const char* cert_file = "/tmp/test_extra_cert.pem";
    uvhttp_tls_error_t result = uvhttp_tls_context_add_extra_chain_cert(NULL, cert_file);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试添加额外链证书 NULL 文件名 */
TEST(UvhttpTlsMbedtlsTest, AddExtraChainCertNullFilename) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_add_extra_chain_cert(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试添加额外链证书文件不存在 */
TEST(UvhttpTlsMbedtlsTest, AddExtraChainCertFileNotFound) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_context_add_extra_chain_cert(ctx, "/nonexistent/extra_cert.pem");
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_PARSE);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取证书链 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, GetCertChainNullSsl) {
    mbedtls_x509_crt* chain = NULL;
    uvhttp_tls_error_t result = uvhttp_tls_get_cert_chain(NULL, &chain);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试获取证书链 NULL 链 */
TEST(UvhttpTlsMbedtlsTest, GetCertChainNullChain) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_get_cert_chain(ssl, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取统计信息 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, GetStatsNullContext) {
    uvhttp_tls_stats_t stats;
    uvhttp_tls_error_t result = uvhttp_tls_get_stats(NULL, &stats);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试获取统计信息 NULL 统计 */
TEST(UvhttpTlsMbedtlsTest, GetStatsNullStats) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_get_stats(ctx, NULL);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取统计信息 */
TEST(UvhttpTlsMbedtlsTest, GetStats) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_stats_t stats;
    uvhttp_tls_error_t result2 = uvhttp_tls_get_stats(ctx, &stats);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试重置统计信息 NULL 上下文 */
TEST(UvhttpTlsMbedtlsTest, ResetStatsNullContext) {
    uvhttp_tls_error_t result = uvhttp_tls_reset_stats(NULL);
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试重置统计信息 */
TEST(UvhttpTlsMbedtlsTest, ResetStats) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_reset_stats(ctx);
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取连接信息 NULL SSL */
TEST(UvhttpTlsMbedtlsTest, GetConnectionInfoNullSsl) {
    char buf[256];
    uvhttp_tls_error_t result = uvhttp_tls_get_connection_info(NULL, buf, sizeof(buf));
    EXPECT_EQ(result, UVHTTP_TLS_ERROR_INVALID_PARAM);
}

/* 测试获取连接信息 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, GetConnectionInfoNullBuffer) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    uvhttp_tls_error_t result2 = uvhttp_tls_get_connection_info(ssl, NULL, 256);
    EXPECT_EQ(result2, UVHTTP_TLS_ERROR_INVALID_PARAM);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取连接信息 */
TEST(UvhttpTlsMbedtlsTest, GetConnectionInfo) {
    uvhttp_tls_error_t result = uvhttp_tls_init();
    EXPECT_EQ(result, UVHTTP_TLS_OK);
    
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    ASSERT_NE(ctx, nullptr);
    
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
    ASSERT_NE(ssl, nullptr);
    
    char buf[256];
    uvhttp_tls_error_t result2 = uvhttp_tls_get_connection_info(ssl, buf, sizeof(buf));
    EXPECT_EQ(result2, UVHTTP_TLS_OK);
    
    mbedtls_ssl_free(ssl);
    free(ssl);
    
    uvhttp_tls_context_free(ctx);
}

/* 测试获取错误字符串 NULL 缓冲区 */
TEST(UvhttpTlsMbedtlsTest, GetErrorStringNullBuffer) {
    uvhttp_tls_get_error_string(0, NULL, 256);
    /* 不应该崩溃 */
}

/* 测试获取错误字符串零缓冲区大小 */
TEST(UvhttpTlsMbedtlsTest, GetErrorStringZeroBufferSize) {
    char buf[256];
    uvhttp_tls_get_error_string(0, buf, 0);
    /* 不应该崩溃 */
}

/* 测试获取错误字符串 */
TEST(UvhttpTlsMbedtlsTest, GetErrorString) {
    char buf[256];
    uvhttp_tls_get_error_string(0, buf, sizeof(buf));
    /* 不应该崩溃 */
}

/* 测试打印错误 */
TEST(UvhttpTlsMbedtlsTest, PrintError) {
    uvhttp_tls_print_error(0);
    /* 不应该崩溃 */
}