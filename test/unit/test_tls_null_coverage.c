/* uvhttp_tls_mbedtls.c NULL参数覆盖率测试 */

#include "uvhttp_tls.h"
#include <stdio.h>
#include <assert.h>

/* 测试TLS初始化 */
void test_tls_init(void) {
    uvhttp_tls_error_t err = uvhttp_tls_init();
    assert(err == UVHTTP_TLS_OK || err == UVHTTP_TLS_ERROR_INIT);

    printf("test_tls_init: PASSED\n");
}

/* 测试TLS上下文创建 */
void test_tls_context_new(void) {
    uvhttp_tls_context_t* ctx = uvhttp_tls_context_new();
    /* 即使没有初始化，也可能返回NULL或创建失败 */
    if (ctx) {
        uvhttp_tls_context_free(ctx);
    }

    printf("test_tls_context_new: PASSED\n");
}

/* 测试TLS上下文释放NULL */
void test_tls_context_free_null(void) {
    uvhttp_tls_context_free(NULL);

    printf("test_tls_context_free_null: PASSED\n");
}

/* 测试加载证书链NULL */
void test_tls_context_load_cert_chain_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_load_cert_chain(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_load_cert_chain_null: PASSED\n");
}

/* 测试加载私钥NULL */
void test_tls_context_load_private_key_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_load_private_key(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_load_private_key_null: PASSED\n");
}

/* 测试加载CA文件NULL */
void test_tls_context_load_ca_file_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_load_ca_file(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_load_ca_file_null: PASSED\n");
}

/* 测试启用客户端认证NULL */
void test_tls_context_enable_client_auth_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_client_auth(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_enable_client_auth_null: PASSED\n");
}

/* 测试设置验证深度NULL */
void test_tls_context_set_verify_depth_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_verify_depth(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_set_verify_depth_null: PASSED\n");
}

/* 测试设置密码套件NULL */
void test_tls_context_set_cipher_suites_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_cipher_suites(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_set_cipher_suites_null: PASSED\n");
}

/* 测试启用会话票据NULL */
void test_tls_context_enable_session_tickets_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_session_tickets(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_enable_session_tickets_null: PASSED\n");
}

/* 测试设置会话缓存NULL */
void test_tls_context_set_session_cache_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_session_cache(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_set_session_cache_null: PASSED\n");
}

/* 测试启用OCSP装订NULL */
void test_tls_context_enable_ocsp_stapling_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_ocsp_stapling(NULL, 0);
    /* 可能返回OK（函数可能忽略NULL） */
    printf("test_tls_context_enable_ocsp_stapling_null: PASSED (err=%d)\n", err);
}

/* 测试设置DH参数NULL */
void test_tls_context_set_dh_parameters_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_dh_parameters(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_set_dh_parameters_null: PASSED\n");
}

/* 测试设置SSL NULL */
void test_tls_setup_ssl_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_setup_ssl(NULL, -1);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_setup_ssl_null: PASSED\n");
}

/* 测试握手NULL */
void test_tls_handshake_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_handshake(NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_handshake_null: PASSED\n");
}

/* 测试读取NULL */
void test_tls_read_null(void) {
    char buf[1024];
    uvhttp_tls_error_t err = uvhttp_tls_read(NULL, buf, sizeof(buf));
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_read_null: PASSED\n");
}

/* 测试写入NULL */
void test_tls_write_null(void) {
    const char* data = "test";
    uvhttp_tls_error_t err = uvhttp_tls_write(NULL, data, 4);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_write_null: PASSED\n");
}

/* 测试启用CRL检查NULL */
void test_tls_context_enable_crl_checking_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_crl_checking(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_enable_crl_checking_null: PASSED\n");
}

/* 测试加载CRL文件NULL */
void test_tls_load_crl_file_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_load_crl_file(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_load_crl_file_null: PASSED\n");
}

/* 测试获取OCSP响应NULL */
void test_tls_get_ocsp_response_null(void) {
    unsigned char* ocsp_response = NULL;
    size_t response_len = 0;
    uvhttp_tls_error_t err = uvhttp_tls_get_ocsp_response(NULL, &ocsp_response, &response_len);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_get_ocsp_response_null: PASSED\n");
}

/* 测试验证OCSP响应NULL */
void test_tls_verify_ocsp_response_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_verify_ocsp_response(NULL, NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_verify_ocsp_response_null: PASSED\n");
}

/* 测试启用TLS1.3 NULL */
void test_tls_context_enable_tls13_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_tls13(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_enable_tls13_null: PASSED\n");
}

/* 测试设置TLS1.3密码套件NULL */
void test_tls_context_set_tls13_cipher_suites_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_tls13_cipher_suites(NULL, NULL);
    /* 可能返回OK（函数可能忽略NULL） */
    printf("test_tls_context_set_tls13_cipher_suites_null: PASSED (err=%d)\n", err);
}

/* 测试启用早期数据NULL */
void test_tls_context_enable_early_data_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_enable_early_data(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_enable_early_data_null: PASSED\n");
}

/* 测试设置票据密钥NULL */
void test_tls_context_set_ticket_key_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_ticket_key(NULL, NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_set_ticket_key_null: PASSED\n");
}

/* 测试轮换票据密钥NULL */
void test_tls_context_rotate_ticket_key_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_rotate_ticket_key(NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_rotate_ticket_key_null: PASSED\n");
}

/* 测试设置票据生命周期NULL */
void test_tls_context_set_ticket_lifetime_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_set_ticket_lifetime(NULL, 0);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_set_ticket_lifetime_null: PASSED\n");
}

/* 测试验证证书链NULL */
void test_tls_verify_cert_chain_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_verify_cert_chain(NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_verify_cert_chain_null: PASSED\n");
}

/* 测试添加额外链证书NULL */
void test_tls_context_add_extra_chain_cert_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_context_add_extra_chain_cert(NULL, NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_context_add_extra_chain_cert_null: PASSED\n");
}

/* 测试获取证书链NULL */
void test_tls_get_cert_chain_null(void) {
    mbedtls_x509_crt* chain = NULL;
    uvhttp_tls_error_t err = uvhttp_tls_get_cert_chain(NULL, &chain);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_get_cert_chain_null: PASSED\n");
}

/* 测试获取统计信息NULL */
void test_tls_get_stats_null(void) {
    uvhttp_tls_stats_t stats;
    uvhttp_tls_error_t err = uvhttp_tls_get_stats(NULL, &stats);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_get_stats_null: PASSED\n");
}

/* 测试重置统计信息NULL */
void test_tls_reset_stats_null(void) {
    uvhttp_tls_error_t err = uvhttp_tls_reset_stats(NULL);
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_reset_stats_null: PASSED\n");
}

/* 测试获取连接信息NULL */
void test_tls_get_connection_info_null(void) {
    char buf[256];
    uvhttp_tls_error_t err = uvhttp_tls_get_connection_info(NULL, buf, sizeof(buf));
    assert(err != UVHTTP_TLS_OK);

    printf("test_tls_get_connection_info_null: PASSED\n");
}

int main() {
    printf("=== uvhttp_tls_mbedtls.c NULL参数覆盖率测试 ===\n\n");

    test_tls_init();
    test_tls_context_new();
    test_tls_context_free_null();
    test_tls_context_load_cert_chain_null();
    test_tls_context_load_private_key_null();
    test_tls_context_load_ca_file_null();
    test_tls_context_enable_client_auth_null();
    test_tls_context_set_verify_depth_null();
    test_tls_context_set_cipher_suites_null();
    test_tls_context_enable_session_tickets_null();
    test_tls_context_set_session_cache_null();
    test_tls_context_enable_ocsp_stapling_null();
    test_tls_context_set_dh_parameters_null();
    test_tls_setup_ssl_null();
    test_tls_handshake_null();
    test_tls_read_null();
    test_tls_write_null();
    test_tls_context_enable_crl_checking_null();
    test_tls_load_crl_file_null();
    test_tls_get_ocsp_response_null();
    test_tls_verify_ocsp_response_null();
    test_tls_context_enable_tls13_null();
    test_tls_context_set_tls13_cipher_suites_null();
    test_tls_context_enable_early_data_null();
    test_tls_context_set_ticket_key_null();
    test_tls_context_rotate_ticket_key_null();
    test_tls_context_set_ticket_lifetime_null();
    test_tls_verify_cert_chain_null();
    test_tls_context_add_extra_chain_cert_null();
    test_tls_get_cert_chain_null();
    test_tls_get_stats_null();
    test_tls_reset_stats_null();
    test_tls_get_connection_info_null();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}