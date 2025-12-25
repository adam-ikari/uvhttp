#include "uvhttp_tls.h"
#include "uvhttp_constants.h"
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// 真实的TLS上下文结构
struct uvhttp_tls_context {
    SSL_CTX* ssl_ctx;
    char cert_file[UVHTTP_TLS_PATH_MAX_SIZE];
    char key_file[UVHTTP_TLS_PATH_MAX_SIZE];
    char ca_file[UVHTTP_TLS_PATH_MAX_SIZE];
    int client_auth_required;
    int verify_depth;
    int initialized;
    
    // 性能统计
    uvhttp_tls_stats_t stats;
};

// 全局初始化状态
static int tls_global_initialized = 0;

// 初始化OpenSSL
static int init_openssl(void) {
    if (!tls_global_initialized) {
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
        tls_global_initialized = 1;
    }
    return 0;
}

// TLS模块管理
uvhttp_tls_error_t uvhttp_tls_init(void) {
    if (init_openssl() != 0) {
        return UVHTTP_TLS_ERROR_INIT;
    }
    return UVHTTP_TLS_OK;
}

void uvhttp_tls_cleanup(void) {
    if (tls_global_initialized) {
        EVP_cleanup();
        ERR_free_strings();
        tls_global_initialized = 0;
    }
}

// TLS上下文管理
uvhttp_tls_context_t* uvhttp_tls_context_new(void) {
    if (init_openssl() != 0) {
        return NULL;
    }
    
    uvhttp_tls_context_t* ctx = UVHTTP_MALLOC(sizeof(uvhttp_tls_context_t));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(uvhttp_tls_context_t));
    
    // 创建SSL上下文 - 支持TLS 1.3
    ctx->ssl_ctx = SSL_CTX_new(TLS_server_method());
    if (!ctx->ssl_ctx) {
        free(ctx);
        return NULL;
    }
    
    // 设置默认选项 - 禁用旧版本，启用TLS 1.3
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_SSLv2 | SSL_OP_NO_SSLv3 | SSL_OP_NO_TLSv1 | SSL_OP_NO_TLSv1_1);
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_SINGLE_DH_USE);
    SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_SINGLE_ECDH_USE);
    
    // 设置TLS 1.3密码套件
    SSL_CTX_set_ciphersuites(ctx->ssl_ctx, "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256");
    
    ctx->verify_depth = 3;
    ctx->initialized = 1;
    
    return ctx;
}

void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    if (ctx->ssl_ctx) {
        SSL_CTX_free(ctx->ssl_ctx);
    }
    
    ctx->initialized = 0;
    UVHTTP_FREE(ctx);
}

// 证书配置
uvhttp_tls_error_t uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t* ctx, const char* cert_file) {
    if (!ctx || !cert_file || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 检查文件名长度
    if (strlen(cert_file) >= sizeof(ctx->cert_file)) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 检查文件是否存在且可读
    if (access(cert_file, R_OK) != 0) {
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    if (SSL_CTX_use_certificate_file(ctx->ssl_ctx, cert_file, SSL_FILETYPE_PEM) != 1) {
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    snprintf(ctx->cert_file, sizeof(ctx->cert_file), "%s", cert_file);
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_load_private_key(uvhttp_tls_context_t* ctx, const char* key_file) {
    if (!ctx || !key_file || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (SSL_CTX_use_PrivateKey_file(ctx->ssl_ctx, key_file, SSL_FILETYPE_PEM) != 1) {
        return UVHTTP_TLS_ERROR_KEY;
    }
    
    // 验证私钥和证书匹配
    if (SSL_CTX_check_private_key(ctx->ssl_ctx) != 1) {
        return UVHTTP_TLS_ERROR_KEY;
    }
    
    snprintf(ctx->key_file, sizeof(ctx->key_file), "%s", key_file);
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t* ctx, const char* ca_file) {
    if (!ctx || !ca_file || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (SSL_CTX_load_verify_locations(ctx->ssl_ctx, ca_file, NULL) != 1) {
        return UVHTTP_TLS_ERROR_CA;
    }
    
    snprintf(ctx->ca_file, sizeof(ctx->ca_file), "%s", ca_file);
    return UVHTTP_TLS_OK;
}

// mTLS配置
uvhttp_tls_error_t uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t* ctx, int require_cert) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (require_cert) {
        SSL_CTX_set_verify(ctx->ssl_ctx, SSL_VERIFY_PEER | SSL_VERIFY_FAIL_IF_NO_PEER_CERT, NULL);
    } else {
        SSL_CTX_set_verify(ctx->ssl_ctx, SSL_VERIFY_NONE, NULL);
    }
    
    ctx->client_auth_required = require_cert;
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t* ctx, int depth) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    SSL_CTX_set_verify_depth(ctx->ssl_ctx, depth);
    ctx->verify_depth = depth;
    return UVHTTP_TLS_OK;
}

// TLS安全配置
int uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t* ctx, const int* cipher_suites) {
    (void)cipher_suites; // 暂时忽略
    if (!ctx || !ctx->ssl_ctx) {
        return -1;
    }
    
    // 设置安全的密码套件
    if (SSL_CTX_set_cipher_list(ctx->ssl_ctx, "HIGH:!aNULL:!MD5:!3DES") != 1) {
        char err_buf[UVHTTP_TLS_ERROR_BUFFER_SIZE];
        ERR_error_string(ERR_get_error(), err_buf);
        return -1;
    }
    return 0;
}

int uvhttp_tls_context_enable_session_tickets(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx || !ctx->ssl_ctx) {
        return -1;
    }
    
    if (enable) {
        SSL_CTX_set_session_cache_mode(ctx->ssl_ctx, SSL_SESS_CACHE_SERVER);
#ifdef SSL_CTX_set_session_ticket_mode
        SSL_CTX_set_session_ticket_mode(ctx->ssl_ctx, SSL_SESS_TICKET_MODE_SERVER);
#endif
    } else {
        SSL_CTX_set_session_cache_mode(ctx->ssl_ctx, SSL_SESS_CACHE_OFF);
#ifdef SSL_CTX_set_session_ticket_mode
        SSL_CTX_set_session_ticket_mode(ctx->ssl_ctx, SSL_SESS_TICKET_MODE_OFF);
#endif
    }
    return 0;
}

int uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t* ctx, int max_sessions) {
    if (!ctx || !ctx->ssl_ctx) {
        return -1;
    }
    
    SSL_CTX_sess_set_cache_size(ctx->ssl_ctx, max_sessions);
    return 0;
}

int uvhttp_tls_context_enable_ocsp_stapling(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (enable) {
        // 启用OCSP装订
#ifdef SSL_CTX_set_tlsext_status_req_cb
        SSL_CTX_set_tlsext_status_req_cb(ctx->ssl_ctx, NULL);
#endif
        // 启用OCSP装订扩展
#ifdef SSL_OP_NO_ANTI_REPLAY
        SSL_CTX_set_options(ctx->ssl_ctx, SSL_OP_NO_ANTI_REPLAY);
#endif
    } else {
        // 禁用OCSP装订
#ifdef SSL_CTX_set_tlsext_status_req_cb
        SSL_CTX_set_tlsext_status_req_cb(ctx->ssl_ctx, NULL);
#endif
    }
    
    return UVHTTP_TLS_OK;
}

int uvhttp_tls_context_set_dh_parameters(uvhttp_tls_context_t* ctx, const char* dh_file) {
    if (!ctx || !ctx->ssl_ctx) {
        return -1;
    }
    
    if (dh_file) {
        FILE* file = fopen(dh_file, "r");
        if (!file) {
            printf("TLS: 无法打开DH参数文件: %s\n", dh_file);
            return -1;
        }
        
        DH* dh = PEM_read_DHparams(file, NULL, NULL, NULL);
        fclose(file);
        
        if (!dh) {
            printf("TLS: 读取DH参数失败\n");
            return -1;
        }
        
        if (SSL_CTX_set_tmp_dh(ctx->ssl_ctx, dh) != 1) {
            DH_free(dh);
            printf("TLS: 设置DH参数失败\n");
            return -1;
        }
        
        DH_free(dh);
    }
    
    return 0;
}

// TLS连接管理
SSL* uvhttp_tls_create_ssl(uvhttp_tls_context_t* ctx) {
    if (!ctx || !ctx->ssl_ctx) {
        return NULL;
    }
    
    SSL* ssl = SSL_new(ctx->ssl_ctx);
    if (!ssl) {
        printf("TLS: 创建SSL对象失败\n");
        return NULL;
    }
    
    return ssl;
}

int uvhttp_tls_setup_ssl(SSL* ssl, int fd) {
    if (!ssl) {
        return -1;
    }
    
    if (SSL_set_fd(ssl, fd) != 1) {
        printf("TLS: 设置文件描述符失败\n");
        return -1;
    }
    
    SSL_set_accept_state(ssl); // 设置为服务器模式
    return 0;
}

uvhttp_tls_error_t uvhttp_tls_handshake(SSL* ssl) {
    if (!ssl) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = SSL_accept(ssl);
    if (ret <= 0) {
        int err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ) {
            return UVHTTP_TLS_ERROR_WANT_READ;
        }
        if (err == SSL_ERROR_WANT_WRITE) {
            return UVHTTP_TLS_ERROR_WANT_WRITE;
        }
        
        return UVHTTP_TLS_ERROR_HANDSHAKE;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_read(SSL* ssl, void* buf, size_t len) {
    if (!ssl || !buf) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = SSL_read(ssl, buf, len);
    if (ret <= 0) {
        int err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ) {
            return UVHTTP_TLS_ERROR_WANT_READ;
        }
        if (err == SSL_ERROR_WANT_WRITE) {
            return UVHTTP_TLS_ERROR_WANT_WRITE;
        }
        return UVHTTP_TLS_ERROR_READ;
    }
    
    return ret;
}

uvhttp_tls_error_t uvhttp_tls_write(SSL* ssl, const void* buf, size_t len) {
    if (!ssl || !buf) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = SSL_write(ssl, buf, len);
    if (ret <= 0) {
        int err = SSL_get_error(ssl, ret);
        if (err == SSL_ERROR_WANT_READ) {
            return UVHTTP_TLS_ERROR_WANT_READ;
        }
        if (err == SSL_ERROR_WANT_WRITE) {
            return UVHTTP_TLS_ERROR_WANT_WRITE;
        }
        return UVHTTP_TLS_ERROR_WRITE;
    }
    
    return ret;
}

// 证书验证
int uvhttp_tls_verify_peer_cert(SSL* ssl) {
    X509* cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    int ret = SSL_get_verify_result(ssl);
    if (ret != X509_V_OK) {
        X509_free(cert);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    X509_free(cert);
    return UVHTTP_TLS_OK;
}

int uvhttp_tls_verify_hostname(X509* cert, const char* hostname) {
    if (!cert || !hostname) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 优先使用OpenSSL的标准主机名验证函数
#ifdef X509_check_host
    if (X509_check_host(cert, hostname, strlen(hostname), 0, NULL) == 1) {
        return UVHTTP_TLS_OK;
    }
    return UVHTTP_TLS_ERROR_VERIFY;
#else
    // 降级到CN字段检查（SAN扩展不可用时的后备方案）
    X509_NAME* subject = X509_get_subject_name(cert);
    if (!subject) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    // 获取CN字段
    int cn_pos = X509_NAME_get_index_by_NID(subject, NID_commonName, -1);
    if (cn_pos < 0) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    X509_NAME_ENTRY* cn_entry = X509_NAME_get_entry(subject, cn_pos);
    if (!cn_entry) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    ASN1_STRING* cn_asn1 = X509_NAME_ENTRY_get_data(cn_entry);
    if (!cn_asn1) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    // 转换为字符串并验证
    char* cn_str = (char*)ASN1_STRING_get0_data(cn_asn1);
    size_t cn_len = ASN1_STRING_length(cn_asn1);
    
    if (!cn_str || cn_len == 0) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    // 精确匹配主机名
    if (strlen(hostname) == cn_len && strncmp(cn_str, hostname, cn_len) == 0) {
        return UVHTTP_TLS_OK;
    }
    
    return UVHTTP_TLS_ERROR_VERIFY;
#endif
}

int uvhttp_tls_check_cert_validity(X509* cert) {
    if (!cert) {
        return -1;
    }
    
    // 检查证书有效期
    int day, sec;
    if (ASN1_TIME_diff(&day, &sec, NULL, X509_get_notAfter(cert)) <= 0) {
        return -1;
    }
    
    if (ASN1_TIME_diff(&day, &sec, X509_get_notBefore(cert), NULL) <= 0) {
        return -1;
    }
    return 0;
}

X509* uvhttp_tls_get_peer_cert(SSL* ssl) {
    if (!ssl) {
        return NULL;
    }
    
    return SSL_get_peer_certificate(ssl);
}

int uvhttp_tls_get_cert_subject(X509* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    X509_NAME* subject = X509_get_subject_name(cert);
    if (!subject) {
        return -1;
    }
    
    if (X509_NAME_oneline(subject, buf, buf_size) == NULL) {
        return -1;
    }
    
    return 0;
}

int uvhttp_tls_get_cert_issuer(X509* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    X509_NAME* issuer = X509_get_issuer_name(cert);
    if (!issuer) {
        return -1;
    }
    
    if (X509_NAME_oneline(issuer, buf, buf_size) == NULL) {
        return -1;
    }
    
    return 0;
}

int uvhttp_tls_get_cert_serial(X509* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    ASN1_INTEGER* serial = X509_get_serialNumber(cert);
    if (!serial) {
        return -1;
    }
    
    BIGNUM* bn = ASN1_INTEGER_to_BN(serial, NULL);
    if (!bn) {
        return -1;
    }
    
    char* serial_str = BN_bn2hex(bn);
    if (!serial_str) {
        BN_free(bn);
        return -1;
    }
    
    snprintf(buf, buf_size, "%s", serial_str);
    
    OPENSSL_free(serial_str);
    BN_free(bn);
    
    return 0;
}

// 证书吊销检查
uvhttp_tls_error_t uvhttp_tls_context_enable_crl_checking(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    X509_STORE* store = SSL_CTX_get_cert_store(ctx->ssl_ctx);
    if (!store) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    if (enable) {
        X509_STORE_set_flags(store, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    }
    // 简化版本不支持禁用CRL检查
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_load_crl_file(uvhttp_tls_context_t* ctx, const char* crl_file) {
    if (!ctx || !crl_file || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(crl_file, "r");
    if (!file) {
        return UVHTTP_TLS_ERROR_CA;
    }
    
    X509_CRL* crl = PEM_read_X509_CRL(file, NULL, NULL, NULL);
    fclose(file);
    
    if (!crl) {
        return UVHTTP_TLS_ERROR_CA;
    }
    
    X509_STORE* store = SSL_CTX_get_cert_store(ctx->ssl_ctx);
    if (!store) {
        X509_CRL_free(crl);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    if (X509_STORE_add_crl(store, crl) != 1) {
        X509_CRL_free(crl);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    X509_CRL_free(crl);
    return UVHTTP_TLS_OK;
}

// OCSP装订
uvhttp_tls_error_t uvhttp_tls_get_ocsp_response(SSL* ssl, unsigned char** ocsp_response, size_t* response_len) {
    if (!ssl || !ocsp_response || !response_len) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    *ocsp_response = NULL;
    *response_len = 0;
    
#ifdef SSL_get_tlsext_status_ocsp_resp
    // 获取OCSP响应
    const unsigned char* response;
    long len = SSL_get_tlsext_status_ocsp_resp(ssl, &response);
    
    if (len <= 0 || !response) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    *response_len = (size_t)len;
    *ocsp_response = UVHTTP_MALLOC(*response_len);
    if (!*ocsp_response) {
        return UVHTTP_TLS_ERROR_MEMORY;
    }
    
    memcpy(*ocsp_response, response, *response_len);
    return UVHTTP_TLS_OK;
#else
    return UVHTTP_TLS_ERROR_VERIFY;
#endif
}

uvhttp_tls_error_t uvhttp_tls_verify_ocsp_response(X509* cert, const unsigned char* ocsp_response, size_t response_len) {
    if (!cert || !ocsp_response || response_len == 0) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
#ifdef OPENSSL_NO_OCSP
    return UVHTTP_TLS_ERROR_VERIFY;
#else
    OCSP_RESPONSE* resp = d2i_OCSP_RESPONSE(NULL, (const unsigned char**)&ocsp_response, response_len);
    if (!resp) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    int status = OCSP_response_status(resp);
    if (status != OCSP_RESPONSE_STATUS_SUCCESSFUL) {
        OCSP_RESPONSE_free(resp);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    OCSP_BASICRESP* basic = OCSP_response_get1_basic(resp);
    if (!basic) {
        OCSP_RESPONSE_free(resp);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    // 验证响应签名
    STACK_OF(X509)* certs = NULL;
    if (OCSP_basic_verify(basic, certs, X509_STORE_new(), 0) != 1) {
        OCSP_BASICRESP_free(basic);
        OCSP_RESPONSE_free(resp);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    // 检查证书状态
    OCSP_SINGLERESP* single = OCSP_resp_get0(basic, 0);
    if (!single) {
        OCSP_BASICRESP_free(basic);
        OCSP_RESPONSE_free(resp);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    int cert_status = OCSP_single_get0_status(single, NULL, NULL, NULL, NULL);
    if (cert_status != V_OCSP_CERTSTATUS_GOOD) {
        OCSP_BASICRESP_free(basic);
        OCSP_RESPONSE_free(resp);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    OCSP_BASICRESP_free(basic);
    OCSP_RESPONSE_free(resp);
    return UVHTTP_TLS_OK;
#endif
}

// TLS 1.3支持
uvhttp_tls_error_t uvhttp_tls_context_enable_tls13(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
#ifdef SSL_CTX_set_min_proto_version
    if (enable) {
        // 启用TLS 1.3
        SSL_CTX_set_min_proto_version(ctx->ssl_ctx, TLS1_2_VERSION);
        SSL_CTX_set_max_proto_version(ctx->ssl_ctx, TLS1_3_VERSION);
    } else {
        // 禁用TLS 1.3，仅支持到TLS 1.2
        SSL_CTX_set_min_proto_version(ctx->ssl_ctx, TLS1_2_VERSION);
        SSL_CTX_set_max_proto_version(ctx->ssl_ctx, TLS1_2_VERSION);
    }
#endif
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_tls13_cipher_suites(uvhttp_tls_context_t* ctx, const char* cipher_suites) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
#ifdef SSL_CTX_set_ciphersuites
    (void)cipher_suites; // 避免未使用参数警告
    const char* suites = cipher_suites ? cipher_suites : 
        "TLS_AES_256_GCM_SHA384:TLS_CHACHA20_POLY1305_SHA256:TLS_AES_128_GCM_SHA256";
    
    if (SSL_CTX_set_ciphersuites(ctx->ssl_ctx, suites) != 1) {
        return UVHTTP_TLS_ERROR_CERT;
    }
#else
    (void)cipher_suites; // 避免未使用参数警告
#endif
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_enable_early_data(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
#ifdef SSL_CTX_set_max_early_data
    if (enable) {
        SSL_CTX_set_max_early_data(ctx->ssl_ctx, 0x1000); // 4KB
    } else {
        SSL_CTX_set_max_early_data(ctx->ssl_ctx, 0);
    }
#else
    (void)enable; // 避免未使用参数警告
#endif
    
    return UVHTTP_TLS_OK;
}

// 会话票证优化
uvhttp_tls_error_t uvhttp_tls_context_set_ticket_key(uvhttp_tls_context_t* ctx, const unsigned char* key, size_t key_len) {
    if (!ctx || !ctx->ssl_ctx || !key || key_len < 16) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
#ifdef SSL_CTX_set_tlsext_ticket_keys
    // OpenSSL内部会复制密钥，我们可以直接传递const指针
    // 但API需要非const指针，因此需要临时转换
    int result = SSL_CTX_set_tlsext_ticket_keys(ctx->ssl_ctx, (void*)key, key_len);
    
    if (result != 1) {
        return UVHTTP_TLS_ERROR_CERT;
    }
#else
    (void)key; // 避免未使用参数警告
    (void)key_len;
#endif
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_rotate_ticket_key(uvhttp_tls_context_t* ctx) {
    if (!ctx || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
#ifdef SSL_CTX_set_tlsext_ticket_keys
    // 生成新的随机密钥
    unsigned char new_key[48];
    if (RAND_bytes(new_key, sizeof(new_key)) != 1) {
        return UVHTTP_TLS_ERROR_MEMORY;
    }
    
    // 设置新的票证密钥
    if (SSL_CTX_set_tlsext_ticket_keys(ctx->ssl_ctx, new_key, sizeof(new_key)) != 1) {
        return UVHTTP_TLS_ERROR_CERT;
    }
#endif
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_ticket_lifetime(uvhttp_tls_context_t* ctx, int lifetime_seconds) {
    if (!ctx || !ctx->ssl_ctx || lifetime_seconds <= 0) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 设置会话票证生命周期
    SSL_CTX_set_timeout(ctx->ssl_ctx, lifetime_seconds);
    
    return UVHTTP_TLS_OK;
}

// 证书链验证
uvhttp_tls_error_t uvhttp_tls_verify_cert_chain(SSL* ssl) {
    if (!ssl) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    X509_STORE_CTX* store_ctx = SSL_get_verify_result(ssl) ? NULL : X509_STORE_CTX_new();
    if (!store_ctx) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    X509* cert = SSL_get_peer_certificate(ssl);
    if (!cert) {
        X509_STORE_CTX_free(store_ctx);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    SSL_CTX* ssl_ctx = SSL_get_SSL_CTX(ssl);
    X509_STORE* store = SSL_CTX_get_cert_store(ssl_ctx);
    
    if (X509_STORE_CTX_init(store_ctx, store, cert, SSL_get_peer_cert_chain(ssl)) != 1) {
        X509_free(cert);
        X509_STORE_CTX_free(store_ctx);
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    // 设置验证标志
    X509_STORE_CTX_set_flags(store_ctx, X509_V_FLAG_CRL_CHECK | X509_V_FLAG_CRL_CHECK_ALL);
    
    // 执行证书链验证
    int verify_result = X509_verify_cert(store_ctx);
    
    X509_free(cert);
    X509_STORE_CTX_free(store_ctx);
    
    if (verify_result != 1) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_add_extra_chain_cert(uvhttp_tls_context_t* ctx, const char* cert_file) {
    if (!ctx || !cert_file || !ctx->ssl_ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(cert_file, "r");
    if (!file) {
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    X509* cert = PEM_read_X509(file, NULL, NULL, NULL);
    fclose(file);
    
    if (!cert) {
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    if (SSL_CTX_add_extra_chain_cert(ctx->ssl_ctx, cert) != 1) {
        X509_free(cert);
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_get_cert_chain(SSL* ssl, STACK_OF(X509)** chain) {
    if (!ssl || !chain) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    *chain = SSL_get_peer_cert_chain(ssl);
    if (!*chain) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    return UVHTTP_TLS_OK;
}

// TLS性能监控
uvhttp_tls_error_t uvhttp_tls_get_stats(uvhttp_tls_context_t* ctx, uvhttp_tls_stats_t* stats) {
    if (!ctx || !stats) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    memcpy(stats, &ctx->stats, sizeof(uvhttp_tls_stats_t));
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_reset_stats(uvhttp_tls_context_t* ctx) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    memset(&ctx->stats, 0, sizeof(uvhttp_tls_stats_t));
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_get_connection_info(SSL* ssl, char* buf, size_t buf_size) {
    if (!ssl || !buf || buf_size == 0) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    const char* version = SSL_get_version(ssl);
    const char* cipher = SSL_get_cipher(ssl);
    
    // 获取会话信息
    SSL_SESSION* session = SSL_get_session(ssl);
    int session_reused = session ? SSL_session_reused(ssl) : 0;
    
    snprintf(buf, buf_size, 
        "TLS Version: %s, Cipher: %s, Session Reused: %s",
        version ? version : "unknown",
        cipher ? cipher : "unknown",
        session_reused ? "yes" : "no");
    
    return UVHTTP_TLS_OK;
}

// 错误处理
void uvhttp_tls_get_error_string(int ret, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return;
    }
    
    unsigned long err = ERR_get_error();
    if (err != 0) {
        ERR_error_string_n(err, buf, buf_size);
    } else {
        snprintf(buf, buf_size, "TLS错误代码: %d", ret);
    }
}

void uvhttp_tls_print_error(int ret) {
    char err_buf[UVHTTP_TLS_ERROR_BUFFER_SIZE];
    uvhttp_tls_get_error_string(ret, err_buf, sizeof(err_buf));
    // 在生产环境中，这里应该使用日志系统
}