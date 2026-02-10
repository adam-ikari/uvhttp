#ifndef UVHTTP_TLS_H
#define UVHTTP_TLS_H

#include "uvhttp_error.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/debug.h>
#include <mbedtls/entropy.h>
#include <mbedtls/error.h>
#include <mbedtls/ssl.h>
#include <mbedtls/ssl_cache.h>
#include <mbedtls/timing.h>
#include <mbedtls/x509.h>
#include <mbedtls/x509_crt.h>
#include <uv.h>

/* Forward declarations */
typedef struct uvhttp_context uvhttp_context_t;

#ifdef __cplusplus
extern "C" {
#endif

/* TLS context type definition */
typedef struct uvhttp_tls_context uvhttp_tls_context_t;

/* TLSblockmanage */
uvhttp_error_t uvhttp_tls_init(uvhttp_context_t* context);
void uvhttp_tls_cleanup(uvhttp_context_t* context);

/* TLSmanage */
/**
 * @brief createnew TLS context
 * @param ctx outputParameter, Used forreceive TLS context
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 * @note Successwhen, *ctx issettovalid TLS contextobject, mustUse
 * uvhttp_tls_context_free release
 * @note Failurewhen, *ctx issetto NULL
 */
uvhttp_error_t uvhttp_tls_context_new(uvhttp_tls_context_t** ctx);
void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx);

/*  */
uvhttp_error_t uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t* ctx,
                                                  const char* cert_file);
uvhttp_error_t uvhttp_tls_context_load_private_key(uvhttp_tls_context_t* ctx,
                                                   const char* key_file);
uvhttp_error_t uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t* ctx,
                                               const char* ca_file);

/* mTLS */
uvhttp_error_t uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t* ctx,
                                                     int require_cert);
uvhttp_error_t uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t* ctx,
                                                   int depth);

/* TLS */
uvhttp_error_t uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t* ctx,
                                                    const int* cipher_suites);
uvhttp_error_t uvhttp_tls_context_enable_session_tickets(
    uvhttp_tls_context_t* ctx, int enable);
uvhttp_error_t uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t* ctx,
                                                    int max_sessions);
uvhttp_error_t uvhttp_tls_context_enable_ocsp_stapling(
    uvhttp_tls_context_t* ctx, int enable);
uvhttp_error_t uvhttp_tls_context_set_dh_parameters(uvhttp_tls_context_t* ctx,
                                                    const char* dh_file);

/* TLSConnectionmanage */
mbedtls_ssl_context* uvhttp_tls_create_ssl(uvhttp_tls_context_t* ctx);
uvhttp_error_t uvhttp_tls_setup_ssl(mbedtls_ssl_context* ssl, int fd);
uvhttp_error_t uvhttp_tls_handshake(mbedtls_ssl_context* ssl);
uvhttp_error_t uvhttp_tls_read(mbedtls_ssl_context* ssl, void* buf, size_t len);
uvhttp_error_t uvhttp_tls_write(mbedtls_ssl_context* ssl, const void* buf,
                                size_t len);

/* validate */
int uvhttp_tls_verify_peer_cert(mbedtls_ssl_context* ssl);
int uvhttp_tls_verify_hostname(mbedtls_x509_crt* cert, const char* hostname);
int uvhttp_tls_check_cert_validity(mbedtls_x509_crt* cert);
mbedtls_x509_crt* uvhttp_tls_get_peer_cert(mbedtls_ssl_context* ssl);
int uvhttp_tls_get_cert_subject(mbedtls_x509_crt* cert, char* buf,
                                size_t buf_size);
int uvhttp_tls_get_cert_issuer(mbedtls_x509_crt* cert, char* buf,
                               size_t buf_size);
int uvhttp_tls_get_cert_serial(mbedtls_x509_crt* cert, char* buf,
                               size_t buf_size);

/*  */
uvhttp_error_t uvhttp_tls_context_enable_crl_checking(uvhttp_tls_context_t* ctx,
                                                      int enable);
uvhttp_error_t uvhttp_tls_load_crl_file(uvhttp_tls_context_t* ctx,
                                        const char* crl_file);

/* OCSP */
uvhttp_error_t uvhttp_tls_get_ocsp_response(mbedtls_ssl_context* ssl,
                                            unsigned char** ocsp_response,
                                            size_t* response_len);
uvhttp_error_t uvhttp_tls_verify_ocsp_response(
    mbedtls_x509_crt* cert, const unsigned char* ocsp_response,
    size_t response_len);

/* TLS 1.3Support */
uvhttp_error_t uvhttp_tls_context_enable_tls13(uvhttp_tls_context_t* ctx,
                                               int enable);
uvhttp_error_t uvhttp_tls_context_set_tls13_cipher_suites(
    uvhttp_tls_context_t* ctx, const char* cipher_suites);
uvhttp_error_t uvhttp_tls_context_enable_early_data(uvhttp_tls_context_t* ctx,
                                                    int enable);

/* Sessionoptimization */
uvhttp_error_t uvhttp_tls_context_set_ticket_key(uvhttp_tls_context_t* ctx,
                                                 const unsigned char* key,
                                                 size_t key_len);
uvhttp_error_t uvhttp_tls_context_rotate_ticket_key(uvhttp_tls_context_t* ctx);
uvhttp_error_t uvhttp_tls_context_set_ticket_lifetime(uvhttp_tls_context_t* ctx,
                                                      int lifetime_seconds);

/* validate */
uvhttp_error_t uvhttp_tls_verify_cert_chain(mbedtls_ssl_context* ssl);
uvhttp_error_t uvhttp_tls_context_add_extra_chain_cert(
    uvhttp_tls_context_t* ctx, const char* cert_file);
uvhttp_error_t uvhttp_tls_get_cert_chain(mbedtls_ssl_context* ssl,
                                         mbedtls_x509_crt** chain);

/* TLS */
typedef struct uvhttp_tls_stats {
    unsigned long long handshake_count;
    unsigned long long handshake_errors;
    unsigned long long bytes_sent;
    unsigned long long bytes_received;
    unsigned long long session_hits;
    unsigned long long session_misses;
    double avg_handshake_time_ms;
} uvhttp_tls_stats_t;

uvhttp_error_t uvhttp_tls_get_stats(uvhttp_tls_context_t* ctx,
                                    uvhttp_tls_stats_t* stats);
uvhttp_error_t uvhttp_tls_reset_stats(uvhttp_tls_context_t* ctx);
uvhttp_error_t uvhttp_tls_get_connection_info(mbedtls_ssl_context* ssl,
                                              char* buf, size_t buf_size);

/* Error handling */
void uvhttp_tls_get_error_string(int ret, char* buf, size_t buf_size);
void uvhttp_tls_print_error(int ret);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_TLS_H */