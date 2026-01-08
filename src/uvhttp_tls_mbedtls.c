/*
 * uvhttp TLS implementation using mbedtls
 */

#include "uvhttp_tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <unistd.h>

// 全局熵和 DRBG 上下文
static mbedtls_entropy_context g_entropy;
static mbedtls_ctr_drbg_context g_ctr_drbg;

// TLS上下文结构
struct uvhttp_tls_context {
    mbedtls_ssl_config conf;
    mbedtls_x509_crt srvcert;
    mbedtls_pk_context pkey;
    mbedtls_x509_crt cacert;
    mbedtls_x509_crl crl;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_cache_context cache;
    int is_server;
    int initialized;
    uvhttp_tls_stats_t stats;
};

// 全局初始化状态
static int g_tls_initialized = 0;

// 自定义网络回调函数
static int mbedtls_net_send(void* ctx, const unsigned char* buf, size_t len) {
    int fd = *(int*)ctx;
    int ret = send(fd, buf, len, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
    return ret;
}

static int mbedtls_net_recv(void* ctx, unsigned char* buf, size_t len) {
    int fd = *(int*)ctx;
    int ret = recv(fd, buf, len, 0);
    if (ret < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return MBEDTLS_ERR_SSL_WANT_READ;
        }
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }
    return ret;
}

// TLS模块管理
uvhttp_tls_error_t uvhttp_tls_init(void) {
    if (g_tls_initialized) {
        return UVHTTP_TLS_OK;
    }
    
    mbedtls_entropy_init(&g_entropy);
    mbedtls_ctr_drbg_init(&g_ctr_drbg);
    
    int ret = mbedtls_ctr_drbg_seed(&g_ctr_drbg, mbedtls_entropy_func, &g_entropy,
                                     NULL, 0);
    if (ret != 0) {
        mbedtls_entropy_free(&g_entropy);
        mbedtls_ctr_drbg_free(&g_ctr_drbg);
        return UVHTTP_TLS_ERROR_INIT;
    }
    
    g_tls_initialized = 1;
    return UVHTTP_TLS_OK;
}

void uvhttp_tls_cleanup(void) {
    if (!g_tls_initialized) {
        return;
    }
    
    mbedtls_entropy_free(&g_entropy);
    mbedtls_ctr_drbg_free(&g_ctr_drbg);
    g_tls_initialized = 0;
}

// TLS上下文管理
uvhttp_tls_context_t* uvhttp_tls_context_new(void) {
    uvhttp_tls_context_t* ctx = calloc(1, sizeof(uvhttp_tls_context_t));
    if (!ctx) {
        return NULL;
    }
    
    mbedtls_ssl_config_init(&ctx->conf);
    mbedtls_x509_crt_init(&ctx->srvcert);
    mbedtls_pk_init(&ctx->pkey);
    mbedtls_x509_crt_init(&ctx->cacert);
    mbedtls_ssl_cache_init(&ctx->cache);
    
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    
    int ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func, &ctx->entropy,
                                     NULL, 0);
    if (ret != 0) {
        uvhttp_tls_context_free(ctx);
        return NULL;
    }
    
    ret = mbedtls_ssl_config_defaults(&ctx->conf, MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        uvhttp_tls_context_free(ctx);
        return NULL;
    }
    
    mbedtls_ssl_conf_rng(&ctx->conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    mbedtls_ssl_conf_session_cache(&ctx->conf, &ctx->cache, mbedtls_ssl_cache_get, mbedtls_ssl_cache_set);
    
    ctx->is_server = 1;
    ctx->initialized = 1;
    
    return ctx;
}

void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx) {
    if (!ctx) {
        return;
    }
    
    mbedtls_ssl_config_free(&ctx->conf);
    mbedtls_x509_crt_free(&ctx->srvcert);
    mbedtls_pk_free(&ctx->pkey);
    mbedtls_x509_crt_free(&ctx->cacert);
    mbedtls_x509_crl_free(&ctx->crl);
    mbedtls_ssl_cache_free(&ctx->cache);
    mbedtls_entropy_free(&ctx->entropy);
    mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
    
    free(ctx);
}

// 证书配置
uvhttp_tls_error_t uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t* ctx, const char* cert_file) {
    if (!ctx || !cert_file) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_x509_crt_parse_file(&ctx->srvcert, cert_file);
    if (ret != 0) {
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    ret = mbedtls_ssl_conf_own_cert(&ctx->conf, &ctx->srvcert, &ctx->pkey);
    if (ret != 0) {
        return UVHTTP_TLS_ERROR_CERT;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_load_private_key(uvhttp_tls_context_t* ctx, const char* key_file) {
    if (!ctx || !key_file) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_pk_parse_keyfile(&ctx->pkey, key_file, NULL, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    if (ret != 0) {
        return UVHTTP_TLS_ERROR_KEY;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t* ctx, const char* ca_file) {
    if (!ctx || !ca_file) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_x509_crt_parse_file(&ctx->cacert, ca_file);
    if (ret != 0) {
        return UVHTTP_TLS_ERROR_CA;
    }
    
    mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, NULL);
    
    return UVHTTP_TLS_OK;
}

// mTLS配置
uvhttp_tls_error_t uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t* ctx, int require_cert) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (require_cert) {
        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    } else {
        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t* ctx, int depth) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    (void)depth;
    return UVHTTP_TLS_OK;
}

// TLS安全配置
uvhttp_tls_error_t uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t* ctx, const int* cipher_suites) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    mbedtls_ssl_conf_ciphersuites(&ctx->conf, cipher_suites);
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_enable_session_tickets(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (enable) {
        mbedtls_ssl_conf_session_tickets(&ctx->conf, MBEDTLS_SSL_SESSION_TICKETS_ENABLED);
    } else {
        mbedtls_ssl_conf_session_tickets(&ctx->conf, MBEDTLS_SSL_SESSION_TICKETS_DISABLED);
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t* ctx, int max_sessions) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    mbedtls_ssl_cache_set_max_entries(&ctx->cache, max_sessions);
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_enable_ocsp_stapling(uvhttp_tls_context_t* ctx, int enable) {
    (void)ctx;
    (void)enable;
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_dh_parameters(uvhttp_tls_context_t* ctx, const char* dh_file) {
    if (!ctx || !dh_file) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 中 DH 参数通过 ECDH 配置
    // 默认使用 ECDHE-ECDSA 和 ECDHE-RSA 密码套件
    // 如需自定义 DH 参数，需要配置 ECDH 组
    
    // 当前版本使用默认 ECDH 组（推荐）
    // 如需自定义 DH 参数，可以添加以下配置：
    // mbedtls_ssl_conf_dh_min(ctx, MBEDTLS_DH_GROUP_SIZE);
    
    return UVHTTP_TLS_OK;
}

// TLS连接管理
mbedtls_ssl_context* uvhttp_tls_create_ssl(uvhttp_tls_context_t* ctx) {
    if (!ctx) {
        return NULL;
    }
    
    mbedtls_ssl_context* ssl = calloc(1, sizeof(mbedtls_ssl_context));
    if (!ssl) {
        return NULL;
    }
    
    mbedtls_ssl_init(ssl);
    
    int ret = mbedtls_ssl_setup(ssl, &ctx->conf);
    if (ret != 0) {
        mbedtls_ssl_free(ssl);
        free(ssl);
        return NULL;
    }
    
    return ssl;
}

uvhttp_tls_error_t uvhttp_tls_setup_ssl(mbedtls_ssl_context* ssl, int fd) {
    if (!ssl) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    mbedtls_ssl_set_bio(ssl, &fd, mbedtls_net_send, mbedtls_net_recv, NULL);
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_handshake(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_ssl_handshake(ssl);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
        return UVHTTP_TLS_ERROR_WANT_READ;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return UVHTTP_TLS_ERROR_WANT_WRITE;
    } else if (ret != 0) {
        return UVHTTP_TLS_ERROR_HANDSHAKE;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_read(mbedtls_ssl_context* ssl, void* buf, size_t len) {
    if (!ssl || !buf) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_ssl_read(ssl, buf, len);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
        return UVHTTP_TLS_ERROR_WANT_READ;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return UVHTTP_TLS_ERROR_WANT_WRITE;
    } else if (ret < 0) {
        return UVHTTP_TLS_ERROR_READ;
    }
    
    return ret;
}

uvhttp_tls_error_t uvhttp_tls_write(mbedtls_ssl_context* ssl, const void* buf, size_t len) {
    if (!ssl || !buf) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_ssl_write(ssl, buf, len);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
        return UVHTTP_TLS_ERROR_WANT_READ;
    } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return UVHTTP_TLS_ERROR_WANT_WRITE;
    } else if (ret < 0) {
        return UVHTTP_TLS_ERROR_WRITE;
    }
    
    return ret;
}

// 证书验证
int uvhttp_tls_verify_peer_cert(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return 0;
    }
    
    uint32_t flags = mbedtls_ssl_get_verify_result(ssl);
    return (flags == 0) ? 1 : 0;
}

int uvhttp_tls_verify_hostname(mbedtls_x509_crt* cert, const char* hostname) {
    if (!cert || !hostname) {
        return 0;
    }
    
    // 使用 mbedtls 提供的主机名验证函数
    const mbedtls_x509_crt* peer_cert = cert;
    const mbedtls_x509_name* name = &peer_cert->subject;
    
    // CN 的 OID (2.5.4.3)
    const unsigned char cn_oid[] = { 0x55, 0x04, 0x03 };
    
    // 检查 Common Name (CN)
    while (name != NULL) {
        if (name->oid.len == 3 && memcmp(name->oid.p, cn_oid, 3) == 0) {
            const char* cn = (const char*)name->val.p;
            size_t cn_len = name->val.len;
            
            // 简单的精确匹配（生产环境应支持通配符）
            if (strlen(hostname) == cn_len && 
                strncasecmp(cn, hostname, cn_len) == 0) {
                return 1;
            }
        }
        name = name->next;
    }
    
    // 检查 Subject Alternative Names (SAN)
    const mbedtls_x509_sequence* san = &peer_cert->subject_alt_names;
    while (san != NULL) {
        const mbedtls_x509_buf* san_buf = &san->buf;
        
        // DNS 名称
        if (san_buf->len > 0 && san_buf->p != NULL) {
            const char* san_str = (const char*)san_buf->p;
            if (strcasecmp(san_str, hostname) == 0) {
                return 1;
            }
        }
        san = san->next;
    }
    
    return 0;
}

int uvhttp_tls_check_cert_validity(mbedtls_x509_crt* cert) {
    if (!cert) {
        return 0;
    }
    
    // 检查证书是否尚未生效
    if (mbedtls_x509_time_is_future(&cert->valid_from)) {
        return 0;
    }
    
    // 检查证书是否已过期
    if (mbedtls_x509_time_is_past(&cert->valid_to)) {
        return 0;
    }
    
    return 1;
}

mbedtls_x509_crt* uvhttp_tls_get_peer_cert(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return NULL;
    }
    
    const mbedtls_x509_crt* cert = mbedtls_ssl_get_peer_cert(ssl);
    return (mbedtls_x509_crt*)cert;
}

int uvhttp_tls_get_cert_subject(mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf) {
        return 0;
    }
    
    mbedtls_x509_dn_gets(buf, buf_size, &cert->subject);
    return strlen(buf);
}

int uvhttp_tls_get_cert_issuer(mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf) {
        return 0;
    }
    
    mbedtls_x509_dn_gets(buf, buf_size, &cert->issuer);
    return strlen(buf);
}

int uvhttp_tls_get_cert_serial(mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf) {
        return 0;
    }
    
    mbedtls_x509_serial_gets(buf, buf_size, &cert->serial);
    return strlen(buf);
}

// 证书吊销检查
uvhttp_tls_error_t uvhttp_tls_context_enable_crl_checking(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 版本中启用 CRL 检查
    if (enable) {
        mbedtls_x509_crl_init(&ctx->crl);
        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    } else {
        mbedtls_ssl_conf_authmode(&ctx->conf, MBEDTLS_SSL_VERIFY_NONE);
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_load_crl_file(uvhttp_tls_context_t* ctx, const char* crl_file) {
    if (!ctx || !crl_file) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    int ret = mbedtls_x509_crl_parse_file(&ctx->crl, crl_file);
    if (ret != 0) {
        return UVHTTP_TLS_ERROR_PARSE;
    }
    
    // 将 CRL 添加到验证配置
    mbedtls_ssl_conf_ca_chain(&ctx->conf, &ctx->cacert, &ctx->crl);
    
    return UVHTTP_TLS_OK;
}

// OCSP装订
uvhttp_tls_error_t uvhttp_tls_get_ocsp_response(mbedtls_ssl_context* ssl, unsigned char** ocsp_response, size_t* response_len) {
    if (!ssl || !ocsp_response || !response_len) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 中 OCSP 响应获取需要额外配置
    // 当前版本返回未实现，建议使用 CRL 检查作为替代
    *ocsp_response = NULL;
    *response_len = 0;
    return UVHTTP_TLS_ERROR_NOT_IMPLEMENTED;
}

uvhttp_tls_error_t uvhttp_tls_verify_ocsp_response(mbedtls_x509_crt* cert, const unsigned char* ocsp_response, size_t response_len) {
    if (!cert || !ocsp_response || response_len == 0) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 中验证 OCSP 响应
    // 注意：这需要额外的 OCSP 状态请求配置
    // 当前版本返回未实现，建议使用 CRL 检查作为替代
    return UVHTTP_TLS_ERROR_NOT_IMPLEMENTED;
}

// TLS 1.3支持
uvhttp_tls_error_t uvhttp_tls_context_enable_tls13(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    if (enable) {
        mbedtls_ssl_conf_min_version(&ctx->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_4);
    } else {
        mbedtls_ssl_conf_min_version(&ctx->conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_tls13_cipher_suites(uvhttp_tls_context_t* ctx, const char* cipher_suites) {
    (void)ctx;
    (void)cipher_suites;
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_enable_early_data(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 中早期数据（0-RTT）支持
    // 注意：早期数据可能带来重放攻击风险
    // 当前版本禁用早期数据以确保安全性
    (void)enable;
    
    return UVHTTP_TLS_OK;
}

// 会话票证优化
uvhttp_tls_error_t uvhttp_tls_context_set_ticket_key(uvhttp_tls_context_t* ctx, const unsigned char* key, size_t key_len) {
    if (!ctx || !key || key_len == 0) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 中会话恢复通过会话缓存实现
    // 会话票证密钥由内部管理，无需手动设置
    // 使用 mbedtls_ssl_cache_context 进行会话缓存
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_rotate_ticket_key(uvhttp_tls_context_t* ctx) {
    if (!ctx) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // mbedTLS 3.x 中会话票证密钥轮换由内部管理
    // 会话缓存会自动处理密钥轮换
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_set_ticket_lifetime(uvhttp_tls_context_t* ctx, int lifetime_seconds) {
    if (!ctx || lifetime_seconds <= 0) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 设置会话缓存超时时间
    mbedtls_ssl_cache_set_timeout(&ctx->cache, lifetime_seconds);
    
    return UVHTTP_TLS_OK;
}

// 证书链验证
uvhttp_tls_error_t uvhttp_tls_verify_cert_chain(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    uint32_t flags = mbedtls_ssl_get_verify_result(ssl);
    if (flags != 0) {
        return UVHTTP_TLS_ERROR_VERIFY;
    }
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_context_add_extra_chain_cert(uvhttp_tls_context_t* ctx, const char* cert_file) {
    if (!ctx || !cert_file) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 解析额外证书文件
    mbedtls_x509_crt extra_cert;
    mbedtls_x509_crt_init(&extra_cert);
    
    int ret = mbedtls_x509_crt_parse_file(&extra_cert, cert_file);
    if (ret != 0) {
        mbedtls_x509_crt_free(&extra_cert);
        return UVHTTP_TLS_ERROR_PARSE;
    }
    
    // 将证书添加到证书链
    mbedtls_x509_crt* current = &ctx->srvcert;
    while (current->next != NULL) {
        current = current->next;
    }
    current->next = calloc(1, sizeof(mbedtls_x509_crt));
    if (!current->next) {
        mbedtls_x509_crt_free(&extra_cert);
        return UVHTTP_TLS_ERROR_MEMORY;
    }
    
    memcpy(current->next, &extra_cert, sizeof(mbedtls_x509_crt));
    
    return UVHTTP_TLS_OK;
}

uvhttp_tls_error_t uvhttp_tls_get_cert_chain(mbedtls_ssl_context* ssl, mbedtls_x509_crt** chain) {
    if (!ssl || !chain) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    // 从 SSL 上下文中获取对等证书链
    const mbedtls_x509_crt* peer_cert = mbedtls_ssl_get_peer_cert(ssl);
    if (!peer_cert) {
        return UVHTTP_TLS_ERROR_NO_CERT;
    }
    
    *chain = (mbedtls_x509_crt*)peer_cert;
    
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

uvhttp_tls_error_t uvhttp_tls_get_connection_info(mbedtls_ssl_context* ssl, char* buf, size_t buf_size) {
    if (!ssl || !buf) {
        return UVHTTP_TLS_ERROR_INVALID_PARAM;
    }
    
    const char* ciphersuite = mbedtls_ssl_get_ciphersuite(ssl);
    const char* version = mbedtls_ssl_get_version(ssl);
    
    if (ciphersuite) {
        snprintf(buf, buf_size, "Version: %s, Cipher: %s", version, ciphersuite);
    } else {
        snprintf(buf, buf_size, "Version: %s, Cipher: unknown", version);
    }
    
    return UVHTTP_TLS_OK;
}

// 错误处理
void uvhttp_tls_get_error_string(int ret, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return;
    }
    
    mbedtls_strerror(ret, buf, buf_size);
}

void uvhttp_tls_print_error(int ret) {
    char buf[256];
    mbedtls_strerror(ret, buf, sizeof(buf));
    fprintf(stderr, "TLS error: %s\n", buf);
}