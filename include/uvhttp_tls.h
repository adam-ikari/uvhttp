#ifndef UVHTTP_TLS_H
#define UVHTTP_TLS_H

#include <uv.h>
#include <mbedtls/ssl.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/x509.h>
#include <mbedtls/pk.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/error.h>
#include <mbedtls/debug.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uvhttp_tls_context uvhttp_tls_context_t;

// TLS模块管理
int uvhttp_tls_init(void);
void uvhttp_tls_cleanup(void);

// TLS上下文管理
uvhttp_tls_context_t* uvhttp_tls_context_new(void);
void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx);

// 证书配置
int uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t* ctx, const char* cert_file);
int uvhttp_tls_context_load_private_key(uvhttp_tls_context_t* ctx, const char* key_file);
int uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t* ctx, const char* ca_file);

// mTLS配置
int uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t* ctx, int require_cert);
int uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t* ctx, int depth);

// TLS安全配置
int uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t* ctx, const int* cipher_suites);
int uvhttp_tls_context_enable_session_tickets(uvhttp_tls_context_t* ctx, int enable);
int uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t* ctx, int max_sessions);
int uvhttp_tls_context_enable_ocsp_stapling(uvhttp_tls_context_t* ctx, int enable);
int uvhttp_tls_context_set_dh_parameters(uvhttp_tls_context_t* ctx, const char* dh_file);

// TLS连接管理
mbedtls_ssl_context* uvhttp_tls_create_ssl(uvhttp_tls_context_t* ctx);
int uvhttp_tls_setup_ssl(mbedtls_ssl_context* ssl, mbedtls_net_context* net_ctx);
int uvhttp_tls_handshake(mbedtls_ssl_context* ssl);
int uvhttp_tls_read(mbedtls_ssl_context* ssl, void* buf, size_t len);
int uvhttp_tls_write(mbedtls_ssl_context* ssl, const void* buf, size_t len);

// 证书验证
int uvhttp_tls_verify_peer_cert(mbedtls_ssl_context* ssl);
int uvhttp_tls_verify_hostname(const mbedtls_x509_crt* cert, const char* hostname);
int uvhttp_tls_check_cert_validity(const mbedtls_x509_crt* cert);
const mbedtls_x509_crt* uvhttp_tls_get_peer_cert(mbedtls_ssl_context* ssl);
int uvhttp_tls_get_cert_subject(const mbedtls_x509_crt* cert, char* buf, size_t buf_size);
int uvhttp_tls_get_cert_issuer(const mbedtls_x509_crt* cert, char* buf, size_t buf_size);
int uvhttp_tls_get_cert_serial(const mbedtls_x509_crt* cert, char* buf, size_t buf_size);

// 错误处理
void uvhttp_tls_get_error_string(int ret, char* buf, size_t buf_size);
void uvhttp_tls_print_error(int ret);

#ifdef __cplusplus
}
#endif

#endif