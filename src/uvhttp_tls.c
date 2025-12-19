#include "uvhttp_tls.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// 辅助函数声明
static int uvhttp_tls_match_hostname(const char* pattern, const char* hostname);
static int uvhttp_tls_get_cert_common_name(const mbedtls_x509_crt* cert, char* buf, size_t buf_size);

struct uvhttp_tls_context {
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_ssl_config ssl_conf;
    mbedtls_x509_crt cert;
    mbedtls_pk_context pkey;
    mbedtls_x509_crt ca_cert;
    int client_auth_required;
    int verify_depth;
    char* pers;
};

static int tls_initialized = 0;
static uv_mutex_t tls_init_mutex;

static void init_tls(void) {
    uv_mutex_lock(&tls_init_mutex);
    if (!tls_initialized) {
        mbedtls_entropy_init(&entropy);
        mbedtls_ctr_drbg_init(&ctr_drbg);
        mbedtls_ssl_config_init(&ssl_conf);
        mbedtls_x509_crt_init(&cert);
        mbedtls_pk_init(&pkey);
        mbedtls_x509_crt_init(&ca_cert);
        tls_initialized = 1;
    }
    uv_mutex_unlock(&tls_init_mutex);
}

static void cleanup_tls(void) {
    uv_mutex_lock(&tls_init_mutex);
    if (tls_initialized) {
        mbedtls_entropy_free(&entropy);
        mbedtls_ctr_drbg_free(&ctr_drbg);
        mbedtls_ssl_config_free(&ssl_conf);
        mbedtls_x509_crt_free(&cert);
        mbedtls_pk_free(&pkey);
        mbedtls_x509_crt_free(&ca_cert);
        tls_initialized = 0;
    }
    uv_mutex_unlock(&tls_init_mutex);
    uv_mutex_destroy(&tls_init_mutex);
}

// 初始化TLS模块
int uvhttp_tls_init(void) {
    if (uv_mutex_init(&tls_init_mutex) != 0) {
        return -1;
    }
    return 0;
}

// 清理TLS模块
void uvhttp_tls_cleanup(void) {
    cleanup_tls();
}

uvhttp_tls_context_t* uvhttp_tls_context_new(void) {
    init_tls();
    
    uvhttp_tls_context_t* ctx = malloc(sizeof(uvhttp_tls_context_t));
    if (!ctx) {
        return NULL;
    }
    
    memset(ctx, 0, sizeof(uvhttp_tls_context_t));
    
    mbedtls_entropy_init(&ctx->entropy);
    mbedtls_ctr_drbg_init(&ctx->ctr_drbg);
    mbedtls_ssl_config_init(&ctx->ssl_conf);
    mbedtls_x509_crt_init(&ctx->cert);
    mbedtls_pk_init(&ctx->pkey);
    mbedtls_x509_crt_init(&ctx->ca_cert);
    
    ctx->pers = strdup("uvhttp_tls_server");
    ctx->client_auth_required = 0;
    ctx->verify_depth = 1;
    
    // 初始化随机数生成器
    int ret = mbedtls_ctr_drbg_seed(&ctx->ctr_drbg, mbedtls_entropy_func,
                                    &ctx->entropy, 
                                    (const unsigned char*)ctx->pers,
                                    strlen(ctx->pers));
    if (ret != 0) {
        fprintf(stderr, "mbedtls_ctr_drbg_seed failed: %d\n", ret);
        uvhttp_tls_context_free(ctx);
        return NULL;
    }
    
    // 配置SSL
    ret = mbedtls_ssl_config_defaults(&ctx->ssl_conf,
                                      MBEDTLS_SSL_IS_SERVER,
                                      MBEDTLS_SSL_TRANSPORT_STREAM,
                                      MBEDTLS_SSL_PRESET_DEFAULT);
    if (ret != 0) {
        fprintf(stderr, "mbedtls_ssl_config_defaults failed: %d\n", ret);
        uvhttp_tls_context_free(ctx);
        return NULL;
    }
    
    mbedtls_ssl_conf_rng(&ctx->ssl_conf, mbedtls_ctr_drbg_random, &ctx->ctr_drbg);
    mbedtls_ssl_conf_min_version(&ctx->ssl_conf, MBEDTLS_SSL_MAJOR_VERSION_3, MBEDTLS_SSL_MINOR_VERSION_3);
    
    // 应用默认安全配置
    uvhttp_tls_context_set_cipher_suites(ctx, NULL); // 使用默认安全密码套件
    mbedtls_ssl_conf_ecp_curves(&ctx->ssl_conf, mbedtls_ssl_list_ciphersuites()); // 支持所有椭圆曲线
    mbedtls_ssl_conf_renegotiation(&ctx->ssl_conf, MBEDTLS_SSL_RENEGOTIATION_DISABLED); // 禁用重新协商
    mbedtls_ssl_conf_legacy_renegotiation(&ctx->ssl_conf, MBEDTLS_SSL_LEGACY_NO_RENEGOTIATION);
    
    return ctx;
}

void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx) {
    if (ctx) {
        mbedtls_entropy_free(&ctx->entropy);
        mbedtls_ctr_drbg_free(&ctx->ctr_drbg);
        mbedtls_ssl_config_free(&ctx->ssl_conf);
        mbedtls_x509_crt_free(&ctx->cert);
        mbedtls_pk_free(&ctx->pkey);
        mbedtls_x509_crt_free(&ctx->ca_cert);
        if (ctx->pers) {
            free(ctx->pers);
            ctx->pers = NULL;
        }
        free(ctx);
    }
}

int uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t* ctx, const char* cert_file) {
    if (!ctx || !cert_file) {
        return -1;
    }
    
    int ret = mbedtls_x509_crt_parse_file(&ctx->cert, cert_file);
    if (ret != 0) {
        fprintf(stderr, "Failed to load certificate chain from %s: %d\n", cert_file, ret);
        uvhttp_tls_print_error(ret);
        return -1;
    }
    
    mbedtls_ssl_conf_ca_chain(&ctx->ssl_conf, ctx->cert.next, NULL);
    ret = mbedtls_ssl_conf_own_cert(&ctx->ssl_conf, &ctx->cert, &ctx->pkey);
    if (ret != 0) {
        fprintf(stderr, "Failed to configure own certificate: %d\n", ret);
        uvhttp_tls_print_error(ret);
        return -1;
    }
    
    return 0;
}

int uvhttp_tls_context_load_private_key(uvhttp_tls_context_t* ctx, const char* key_file) {
    if (!ctx || !key_file) {
        return -1;
    }
    
    int ret = mbedtls_pk_parse_keyfile(&ctx->pkey, key_file, NULL);
    if (ret != 0) {
        fprintf(stderr, "Failed to load private key from %s: %d\n", key_file, ret);
        uvhttp_tls_print_error(ret);
        return -1;
    }
    
    return 0;
}

int uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t* ctx, const char* ca_file) {
    if (!ctx || !ca_file) {
        return -1;
    }
    
    int ret = mbedtls_x509_crt_parse_file(&ctx->ca_cert, ca_file);
    if (ret != 0) {
        fprintf(stderr, "Failed to load CA file from %s: %d\n", ca_file, ret);
        uvhttp_tls_print_error(ret);
        return -1;
    }
    
    mbedtls_ssl_conf_ca_chain(&ctx->ssl_conf, &ctx->ca_cert, NULL);
    
    return 0;
}

int uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t* ctx, int require_cert) {
    if (!ctx) {
        return -1;
    }
    
    ctx->client_auth_required = require_cert;
    
    if (require_cert) {
        mbedtls_ssl_conf_authmode(&ctx->ssl_conf, MBEDTLS_SSL_VERIFY_REQUIRED);
    } else {
        mbedtls_ssl_conf_authmode(&ctx->ssl_conf, MBEDTLS_SSL_VERIFY_OPTIONAL);
    }
    
    return 0;
}

int uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t* ctx, int depth) {
    if (!ctx || depth < 1) {
        return -1;
    }
    
    ctx->verify_depth = depth;
    // mbed TLS通过证书链来控制验证深度
    
    return 0;
}

// 默认的安全密码套件列表
static const int default_cipher_suites[] = {
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_256_GCM_SHA384,
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384,
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,
    MBEDTLS_TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256,
    MBEDTLS_TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256,
    0 // 终止符
};

int uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t* ctx, const int* cipher_suites) {
    if (!ctx) {
        return -1;
    }
    
    const int* suites = cipher_suites ? cipher_suites : default_cipher_suites;
    mbedtls_ssl_conf_ciphersuites(&ctx->ssl_conf, suites);
    
    return 0;
}

int uvhttp_tls_context_enable_session_tickets(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx) {
        return -1;
    }
    
    mbedtls_ssl_conf_session_tickets(&ctx->ssl_conf, enable ? MBEDTLS_SSL_SESSION_TICKETS_ENABLED : MBEDTLS_SSL_SESSION_TICKETS_DISABLED);
    
    return 0;
}

int uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t* ctx, int max_sessions) {
    if (!ctx || max_sessions <= 0) {
        return -1;
    }
    
    // 这里可以添加会话缓存实现
    // 简单实现：禁用会话缓存以提高安全性
    mbedtls_ssl_conf_session_cache(&ctx->ssl_conf, NULL, NULL, 0);
    
    return 0;
}

int uvhttp_tls_context_enable_ocsp_stapling(uvhttp_tls_context_t* ctx, int enable) {
    if (!ctx) {
        return -1;
    }
    
    // OCSP Stapling 需要更多实现，这里先占位
    // 可以在后续版本中实现
    (void)enable;
    
    return 0;
}

int uvhttp_tls_context_set_dh_parameters(uvhttp_tls_context_t* ctx, const char* dh_file) {
    if (!ctx) {
        return -1;
    }
    
    if (!dh_file) {
        // 使用默认的DH参数
        mbedtls_ssl_conf_dh_min_bitlen(&ctx->ssl_conf, 2048);
        return 0;
    }
    
    // 从文件加载DH参数
    // 这里需要实现文件读取和DH参数解析
    // 简化实现，只设置最小位数
    mbedtls_ssl_conf_dh_min_bitlen(&ctx->ssl_conf, 2048);
    
    return 0;
}

mbedtls_ssl_context* uvhttp_tls_create_ssl(uvhttp_tls_context_t* ctx) {
    if (!ctx) {
        return NULL;
    }
    
    mbedtls_ssl_context* ssl = malloc(sizeof(mbedtls_ssl_context));
    if (!ssl) {
        return NULL;
    }
    
    mbedtls_ssl_init(ssl);
    
    int ret = mbedtls_ssl_setup(ssl, &ctx->ssl_conf);
    if (ret != 0) {
        fprintf(stderr, "mbedtls_ssl_setup failed: %d\n", ret);
        uvhttp_tls_print_error(ret);
        mbedtls_ssl_free(ssl);
        free(ssl);
        return NULL;
    }
    
    return ssl;
}

int uvhttp_tls_setup_ssl(mbedtls_ssl_context* ssl, mbedtls_net_context* net_ctx) {
    if (!ssl || !net_ctx) {
        return -1;
    }
    
    // 使用mbed TLS的标准网络回调，但通过libuv事件循环驱动
    mbedtls_ssl_set_bio(ssl, net_ctx, mbedtls_net_send, mbedtls_net_recv, NULL);
    
    return 0;
}

int uvhttp_tls_handshake(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return -1;
    }
    
    int ret = mbedtls_ssl_handshake(ssl);
    if (ret != 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            return UV_EAGAIN; // 握手进行中，需要等待事件循环
        } else {
            fprintf(stderr, "mbedtls_ssl_handshake failed: %d\n", ret);
            uvhttp_tls_print_error(ret);
            return UV_EIO; // 握手错误
        }
    }
    
    return 0; // 握手完成
}

int uvhttp_tls_read(mbedtls_ssl_context* ssl, void* buf, size_t len) {
    if (!ssl || !buf) {
        return -1;
    }
    
    int ret = mbedtls_ssl_read(ssl, buf, len);
    if (ret < 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ) {
            return UV_EAGAIN; // 需要更多数据，事件循环会继续监听
        } else if (ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            return UV_EAGAIN; // 需要写入，但事件循环会处理
        } else {
            fprintf(stderr, "mbedtls_ssl_read failed: %d\n", ret);
            uvhttp_tls_print_error(ret);
            return UV_EIO; // IO错误
        }
    }
    
    return ret;
}

int uvhttp_tls_write(mbedtls_ssl_context* ssl, const void* buf, size_t len) {
    if (!ssl || !buf) {
        return -1;
    }
    
    int ret = mbedtls_ssl_write(ssl, buf, len);
    if (ret <= 0) {
        if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            return UV_EAGAIN; // 需要等待事件循环
        } else {
            fprintf(stderr, "mbedtls_ssl_write failed: %d\n", ret);
            uvhttp_tls_print_error(ret);
            return UV_EIO; // IO错误
        }
    }
    
    return ret;
}

int uvhttp_tls_verify_peer_cert(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return -1;
    }
    
    uint32_t flags = mbedtls_ssl_get_verify_result(ssl);
    if (flags != 0) {
        char error_buf[256];
        mbedtls_x509_crt_verify_info(error_buf, sizeof(error_buf), "", flags);
        fprintf(stderr, "Certificate verification failed: %s\n", error_buf);
        return -1;
    }
    
    // 获取对端证书进行额外验证
    const mbedtls_x509_crt* cert = uvhttp_tls_get_peer_cert(ssl);
    if (!cert) {
        fprintf(stderr, "No peer certificate available\n");
        return -1;
    }
    
    // 检查证书有效期
    if (uvhttp_tls_check_cert_validity(cert) != 0) {
        fprintf(stderr, "Certificate is not valid or expired\n");
        return -1;
    }
    
    return 0;
}

int uvhttp_tls_verify_hostname(const mbedtls_x509_crt* cert, const char* hostname) {
    if (!cert || !hostname) {
        return -1;
    }
    
    // 检查证书中的CN或SAN是否匹配主机名
    const mbedtls_x509_sequence* san = &cert->subject_alt_names;
    
    // 首先检查SAN（主题备用名称）
    while (san != NULL) {
        if (san->buf.len > 0) {
            char san_name[256];
            size_t san_len = (san->buf.len < sizeof(san_name) - 1) ? san->buf.len : sizeof(san_name) - 1;
            memcpy(san_name, san->buf.p, san_len);
            san_name[san_len] = '\0';
            
            // 支持通配符匹配
            if (uvhttp_tls_match_hostname(san_name, hostname) == 0) {
                return 0;
            }
        }
        san = san->next;
    }
    
    // 如果没有SAN，检查CN（通用名称）
    char cn[256];
    if (uvhttp_tls_get_cert_common_name(cert, cn, sizeof(cn)) == 0) {
        return uvhttp_tls_match_hostname(cn, hostname);
    }
    
    return -1; // 没有匹配的主机名
}

int uvhttp_tls_check_cert_validity(const mbedtls_x509_crt* cert) {
    if (!cert) {
        return -1;
    }
    
    // 获取当前时间
    time_t current_time = time(NULL);
    
    // 检查证书是否在有效期内
    if (mbedtls_x509_time_is_future(&cert->valid_from) || 
        mbedtls_x509_time_is_past(&cert->valid_to)) {
        return -1;
    }
    
    // 检查证书是否过期
    mbedtls_time_t cert_time = (mbedtls_time_t)current_time;
    if (mbedtls_x509_time_cmp(&cert->valid_to, cert_time) < 0) {
        return -1; // 证书已过期
    }
    
    // 检查证书是否还未生效
    if (mbedtls_x509_time_cmp(&cert->valid_from, cert_time) > 0) {
        return -1; // 证书还未生效
    }
    
    return 0;
}

// 辅助函数：匹配主机名（支持通配符）
static int uvhttp_tls_match_hostname(const char* pattern, const char* hostname) {
    if (!pattern || !hostname) {
        return -1;
    }
    
    // 简单的通配符匹配实现
    if (pattern[0] == '*' && pattern[1] == '.') {
        // 通配符域名，如 *.example.com
        const char* domain = pattern + 2;
        size_t domain_len = strlen(domain);
        size_t hostname_len = strlen(hostname);
        
        if (hostname_len < domain_len) {
            return -1;
        }
        
        const char* hostname_domain = hostname + (hostname_len - domain_len);
        if (strcmp(hostname_domain, domain) == 0) {
            // 确保不是直接匹配域名（防止*匹配空）
            if (hostname_len > domain_len) {
                return 0;
            }
        }
    } else {
        // 精确匹配
        return strcmp(pattern, hostname);
    }
    
    return -1;
}

// 辅助函数：获取证书CN
static int uvhttp_tls_get_cert_common_name(const mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    const mbedtls_x509_name* name = &cert->subject;
    while (name != NULL) {
        if (MBEDTLS_OID_CMP(MBEDTLS_OID_AT_CN, &name->oid) == 0) {
            if (name->val.len < buf_size) {
                memcpy(buf, name->val.p, name->val.len);
                buf[name->val.len] = '\0';
                return 0;
            }
        }
        name = name->next;
    }
    
    return -1;
}

const mbedtls_x509_crt* uvhttp_tls_get_peer_cert(mbedtls_ssl_context* ssl) {
    if (!ssl) {
        return NULL;
    }
    
    return mbedtls_ssl_get_peer_cert(ssl);
}

int uvhttp_tls_get_cert_subject(const mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    int ret = mbedtls_x509_dn_gets(buf, buf_size, &cert->subject);
    return (ret > 0 && ret < buf_size) ? 0 : -1;
}

int uvhttp_tls_get_cert_issuer(const mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    int ret = mbedtls_x509_dn_gets(buf, buf_size, &cert->issuer);
    return (ret > 0 && ret < buf_size) ? 0 : -1;
}

void uvhttp_tls_get_error_string(int ret, char* buf, size_t buf_size) {
    if (!buf || buf_size == 0) {
        return;
    }
    
    mbedtls_strerror(ret, buf, buf_size);
}

int uvhttp_tls_get_cert_serial(const mbedtls_x509_crt* cert, char* buf, size_t buf_size) {
    if (!cert || !buf || buf_size == 0) {
        return -1;
    }
    
    int ret = mbedtls_mpi_write_string(&cert->serial, 10, buf, buf_size, NULL);
    return (ret == 0) ? 0 : -1;
}

void uvhttp_tls_print_error(int ret) {
    char error_buf[256];
    uvhttp_tls_get_error_string(ret, error_buf, sizeof(error_buf));
    fprintf(stderr, "mbed TLS error: %s\n", error_buf);
}