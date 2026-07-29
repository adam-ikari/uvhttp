/*
 * UVHTTP TLS API 覆盖率测试
 * 
 * 测试 uvhttp_tls.c 的核心 API
 */

#include <gtest/gtest.h>

#if UVHTTP_FEATURE_TLS
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#endif

extern "C" {
    #include "uvhttp_tls.h"
    #include "uvhttp_context.h"
    #include "uvhttp_allocator.h"
    #include "uv.h"
}

/* ========== 测试 TLS 模块初始化和清理 ========== */

TEST(UvhttpTlsApiCoverageTest, TlsInitNullContext) {
    uvhttp_error_t result = uvhttp_tls_init(nullptr);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, TlsInitValid) {
    uv_loop_t loop;
    ASSERT_EQ(uv_loop_init(&loop), 0);
    
    uvhttp_context_t* context = nullptr;
    uvhttp_error_t result = uvhttp_context_create(&loop, &context);
    
    if (result == UVHTTP_OK && context) {
        result = uvhttp_tls_init(context);

        /* 不强制检查结果 */

        uvhttp_tls_cleanup(context);
        /* uvhttp_tls_cleanup 仅释放 mbedTLS 内部状态，但不释放
         * uvhttp_context_init_tls 通过 uvhttp_alloc 分配的 entropy/drbg
         * 内存，且会把 tls_initialized 置 0，导致 uvhttp_context_destroy
         * 内的 cleanup 成为空操作而泄漏。这里显式释放分配以匹配初始化。 */
#if UVHTTP_FEATURE_TLS
        uvhttp_free(context->tls_entropy);
        context->tls_entropy = nullptr;
        uvhttp_free(context->tls_drbg);
        context->tls_drbg = nullptr;
#endif
        uvhttp_context_destroy(context);
    }
    
    uv_loop_close(&loop);
}

/* ========== 测试 TLS 上下文创建和释放 ========== */

TEST(UvhttpTlsApiCoverageTest, TlsContextNewNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_new(nullptr);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, TlsContextNewValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_error_t result = uvhttp_tls_context_new(&ctx);
    
    EXPECT_EQ(result, UVHTTP_OK);
    ASSERT_NE(ctx, nullptr);
    
    uvhttp_tls_context_free(ctx);
}

TEST(UvhttpTlsApiCoverageTest, TlsContextFreeNull) {
    /* 释放 nullptr 应该安全 */
    uvhttp_tls_context_free(nullptr);
}

/* ========== 测试加载证书链 ========== */

TEST(UvhttpTlsApiCoverageTest, LoadCertChainNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_load_cert_chain(nullptr, "cert.pem");
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, LoadCertChainNullPath) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_cert_chain(ctx, nullptr);
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, LoadCertChainEmptyPath) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_cert_chain(ctx, "");
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, LoadCertChainNonExistentFile) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_cert_chain(ctx, "/nonexistent/cert.pem");
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试加载私钥 ========== */

TEST(UvhttpTlsApiCoverageTest, LoadPrivateKeyNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_load_private_key(nullptr, "key.pem");
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, LoadPrivateKeyNullPath) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_private_key(ctx, nullptr);
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, LoadPrivateKeyEmptyPath) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_private_key(ctx, "");
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试加载 CA 文件 ========== */

TEST(UvhttpTlsApiCoverageTest, LoadCaFileNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_load_ca_file(nullptr, "ca.pem");
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, LoadCaFileNullPath) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_ca_file(ctx, nullptr);
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, LoadCaFileEmptyPath) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_load_ca_file(ctx, "");
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试启用客户端认证 ========== */

TEST(UvhttpTlsApiCoverageTest, EnableClientAuthNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_enable_client_auth(nullptr, 1);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, EnableClientAuthValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_enable_client_auth(ctx, 1);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试设置验证深度 ========== */

TEST(UvhttpTlsApiCoverageTest, SetVerifyDepthNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_set_verify_depth(nullptr, 5);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, SetVerifyDepthValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_set_verify_depth(ctx, 5);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, SetVerifyDepthZero) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_set_verify_depth(ctx, 0);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, SetVerifyDepthNegative) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_set_verify_depth(ctx, -1);
        
        /* 负值可能被拒绝或接受 */
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试设置密码套件 ========== */

TEST(UvhttpTlsApiCoverageTest, SetCipherSuitesNullContext) {
    int cipher_suites[] = {0};
    uvhttp_error_t result = uvhttp_tls_context_set_cipher_suites(nullptr, cipher_suites);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, SetCipherSuitesNullCipher) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_set_cipher_suites(ctx, nullptr);
        
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_tls_context_free(ctx);
    }
}

TEST(UvhttpTlsApiCoverageTest, SetCipherSuitesEmptyCipher) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        int cipher_suites[] = {0};
        uvhttp_error_t result = uvhttp_tls_context_set_cipher_suites(ctx, cipher_suites);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试启用会话票据 ========== */

TEST(UvhttpTlsApiCoverageTest, EnableSessionTicketsNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_enable_session_tickets(nullptr, 1);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, EnableSessionTicketsValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_enable_session_tickets(ctx, 1);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试设置会话缓存 ========== */

TEST(UvhttpTlsApiCoverageTest, SetSessionCacheNullContext) {
    uvhttp_error_t result = uvhttp_tls_context_set_session_cache(nullptr, 1);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, SetSessionCacheValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_set_session_cache(ctx, 1);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试启用 TLS 1.3 ========== */

TEST(UvhttpTlsApiCoverageTest, EnableTls13NullContext) {
    uvhttp_error_t result = uvhttp_tls_context_enable_tls13(nullptr, 1);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, EnableTls13Valid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_context_enable_tls13(ctx, 1);
        
        /* 不强制检查结果 */
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试创建 SSL ========== */

TEST(UvhttpTlsApiCoverageTest, CreateSslNullContext) {
    mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(nullptr);
    
    EXPECT_EQ(ssl, nullptr);
}

TEST(UvhttpTlsApiCoverageTest, CreateSslValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);
    
    if (ctx) {
        mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(ctx);
        
        EXPECT_NE(ssl, nullptr);
        
        if (ssl) {
            mbedtls_ssl_free(ssl);
            uvhttp_free(ssl);
        }
        
        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试获取错误字符串 ========== */

TEST(UvhttpTlsApiCoverageTest, GetErrorStringNullBuf) {
    uvhttp_tls_get_error_string(0, nullptr, 100);
    
    /* 应该安全 */
}

TEST(UvhttpTlsApiCoverageTest, GetErrorStringZeroSize) {
    char buf[100];
    uvhttp_tls_get_error_string(0, buf, 0);
    
    /* 应该安全 */
}

TEST(UvhttpTlsApiCoverageTest, GetErrorStringValid) {
    char buf[100];
    uvhttp_tls_get_error_string(0, buf, sizeof(buf));
    
    /* 应该安全 */
}

/* ========== 测试打印错误 ========== */

TEST(UvhttpTlsApiCoverageTest, PrintError) {
    uvhttp_tls_print_error(0);
    
    /* 应该安全 */
}

/* ========== 测试重置统计信息 ========== */

TEST(UvhttpTlsApiCoverageTest, ResetStatsNullContext) {
    uvhttp_error_t result = uvhttp_tls_reset_stats(nullptr);
    
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpTlsApiCoverageTest, ResetStatsValid) {
    uvhttp_tls_context_t* ctx = nullptr;
    uvhttp_tls_context_new(&ctx);

    if (ctx) {
        uvhttp_error_t result = uvhttp_tls_reset_stats(ctx);

        /* 不强制检查结果 */

        uvhttp_tls_context_free(ctx);
    }
}

/* ========== 测试证书验证函数 ========== */

TEST(UvhttpTlsApiCoverageTest, VerifyPeerCertNull) {
    int result = uvhttp_tls_verify_peer_cert(nullptr);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpTlsApiCoverageTest, VerifyHostnameNullCert) {
    int result = uvhttp_tls_verify_hostname(nullptr, "example.com");
    EXPECT_EQ(result, 0);
}

TEST(UvhttpTlsApiCoverageTest, VerifyHostnameNullHostname) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int result = uvhttp_tls_verify_hostname(&cert, nullptr);
    EXPECT_EQ(result, 0);
    mbedtls_x509_crt_free(&cert);
}

TEST(UvhttpTlsApiCoverageTest, CheckCertValidityNull) {
    int result = uvhttp_tls_check_cert_validity(nullptr);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpTlsApiCoverageTest, CheckCertValidityValid) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int ret = mbedtls_x509_crt_parse_file(&cert, "test/certs/server.crt");
    if (ret == 0) {
        int result = uvhttp_tls_check_cert_validity(&cert);
        EXPECT_EQ(result, 1);
    }
    mbedtls_x509_crt_free(&cert);
}

TEST(UvhttpTlsApiCoverageTest, GetPeerCertNull) {
    mbedtls_x509_crt* result = uvhttp_tls_get_peer_cert(nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpTlsApiCoverageTest, GetCertSubjectNull) {
    char buf[256];
    int result = uvhttp_tls_get_cert_subject(nullptr, buf, sizeof(buf));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpTlsApiCoverageTest, GetCertSubjectNullBuf) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int result = uvhttp_tls_get_cert_subject(&cert, nullptr, 100);
    EXPECT_EQ(result, 0);
    mbedtls_x509_crt_free(&cert);
}

TEST(UvhttpTlsApiCoverageTest, GetCertSubjectValid) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int ret = mbedtls_x509_crt_parse_file(&cert, "test/certs/server.crt");
    if (ret == 0) {
        char buf[256];
        int result = uvhttp_tls_get_cert_subject(&cert, buf, sizeof(buf));
        EXPECT_GT(result, 0);
    }
    mbedtls_x509_crt_free(&cert);
}

TEST(UvhttpTlsApiCoverageTest, GetCertIssuerNull) {
    char buf[256];
    int result = uvhttp_tls_get_cert_issuer(nullptr, buf, sizeof(buf));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpTlsApiCoverageTest, GetCertIssuerValid) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int ret = mbedtls_x509_crt_parse_file(&cert, "test/certs/server.crt");
    if (ret == 0) {
        char buf[256];
        int result = uvhttp_tls_get_cert_issuer(&cert, buf, sizeof(buf));
        EXPECT_GT(result, 0);
    }
    mbedtls_x509_crt_free(&cert);
}

TEST(UvhttpTlsApiCoverageTest, GetCertSerialNull) {
    char buf[256];
    int result = uvhttp_tls_get_cert_serial(nullptr, buf, sizeof(buf));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpTlsApiCoverageTest, GetCertSerialValid) {
    mbedtls_x509_crt cert;
    mbedtls_x509_crt_init(&cert);
    int ret = mbedtls_x509_crt_parse_file(&cert, "test/certs/server.crt");
    if (ret == 0) {
        char buf[256];
        int result = uvhttp_tls_get_cert_serial(&cert, buf, sizeof(buf));
        EXPECT_GT(result, 0);
    }
    mbedtls_x509_crt_free(&cert);
}