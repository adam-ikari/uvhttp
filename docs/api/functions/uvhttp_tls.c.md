# uvhttp_tls.c Functions

**Defined at:** `undefined:undefined`


## Functions

### `mbedtls_net_send`

**Signature:**
```c
int mbedtls_net_send(void * ctx, const unsigned char * buf, size_t len)
```

**Parameters:**
- `ctx` (void *): TBD
- `buf` (const unsigned char *): TBD
- `len` (size_t): TBD


**Returns:**
`int`



---
### `mbedtls_net_recv`

**Signature:**
```c
int mbedtls_net_recv(void * ctx, unsigned char * buf, size_t len)
```

**Parameters:**
- `ctx` (void *): TBD
- `buf` (unsigned char *): TBD
- `len` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_tls_init`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_init(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_cleanup`

**Signature:**
```c
void uvhttp_tls_cleanup(uvhttp_context_t context)
```

**Parameters:**
- `context` (uvhttp_context_t): TBD


**Returns:**
`void`



---
### `uvhttp_tls_context_new`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_new(uvhttp_tls_context_t ctx)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_free`

**Signature:**
```c
void uvhttp_tls_context_free(uvhttp_tls_context_t ctx)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD


**Returns:**
`void`



---
### `uvhttp_tls_context_load_cert_chain`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t ctx, const char * cert_file)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `cert_file` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_load_private_key`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_load_private_key(uvhttp_tls_context_t ctx, const char * key_file)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `key_file` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_load_ca_file`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t ctx, const char * ca_file)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `ca_file` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_enable_client_auth`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t ctx, int require_cert)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `require_cert` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_verify_depth`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t ctx, int depth)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `depth` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_cipher_suites`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t ctx, const int * cipher_suites)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `cipher_suites` (const int *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_enable_session_tickets`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_enable_session_tickets(uvhttp_tls_context_t ctx, int enable)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `enable` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_session_cache`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t ctx, int max_sessions)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `max_sessions` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_enable_ocsp_stapling`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_enable_ocsp_stapling(uvhttp_tls_context_t ctx, int enable)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `enable` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_dh_parameters`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_dh_parameters(uvhttp_tls_context_t ctx, const char * dh_file)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `dh_file` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_create_ssl`

**Signature:**
```c
mbedtls_ssl_context * uvhttp_tls_create_ssl(uvhttp_tls_context_t ctx)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD


**Returns:**
`mbedtls_ssl_context *`



---
### `uvhttp_tls_setup_ssl`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_setup_ssl(mbedtls_ssl_context * ssl, int fd)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD
- `fd` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_handshake`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_handshake(mbedtls_ssl_context * ssl)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_read`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_read(mbedtls_ssl_context * ssl, void * buf, size_t len)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD
- `buf` (void *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_write`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_write(mbedtls_ssl_context * ssl, const void * buf, size_t len)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD
- `buf` (const void *): TBD
- `len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_verify_peer_cert`

**Signature:**
```c
int uvhttp_tls_verify_peer_cert(mbedtls_ssl_context * ssl)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD


**Returns:**
`int`



---
### `uvhttp_tls_verify_hostname`

**Signature:**
```c
int uvhttp_tls_verify_hostname(mbedtls_x509_crt * cert, const char * hostname)
```

**Parameters:**
- `cert` (mbedtls_x509_crt *): TBD
- `hostname` (const char *): TBD


**Returns:**
`int`



---
### `uvhttp_tls_check_cert_validity`

**Signature:**
```c
int uvhttp_tls_check_cert_validity(mbedtls_x509_crt * cert)
```

**Parameters:**
- `cert` (mbedtls_x509_crt *): TBD


**Returns:**
`int`



---
### `uvhttp_tls_get_peer_cert`

**Signature:**
```c
mbedtls_x509_crt * uvhttp_tls_get_peer_cert(mbedtls_ssl_context * ssl)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD


**Returns:**
`mbedtls_x509_crt *`



---
### `uvhttp_tls_get_cert_subject`

**Signature:**
```c
int uvhttp_tls_get_cert_subject(mbedtls_x509_crt * cert, char * buf, size_t buf_size)
```

**Parameters:**
- `cert` (mbedtls_x509_crt *): TBD
- `buf` (char *): TBD
- `buf_size` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_tls_get_cert_issuer`

**Signature:**
```c
int uvhttp_tls_get_cert_issuer(mbedtls_x509_crt * cert, char * buf, size_t buf_size)
```

**Parameters:**
- `cert` (mbedtls_x509_crt *): TBD
- `buf` (char *): TBD
- `buf_size` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_tls_get_cert_serial`

**Signature:**
```c
int uvhttp_tls_get_cert_serial(mbedtls_x509_crt * cert, char * buf, size_t buf_size)
```

**Parameters:**
- `cert` (mbedtls_x509_crt *): TBD
- `buf` (char *): TBD
- `buf_size` (size_t): TBD


**Returns:**
`int`



---
### `uvhttp_tls_context_enable_crl_checking`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_enable_crl_checking(uvhttp_tls_context_t ctx, int enable)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `enable` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_load_crl_file`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_load_crl_file(uvhttp_tls_context_t ctx, const char * crl_file)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `crl_file` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_get_ocsp_response`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_get_ocsp_response(mbedtls_ssl_context * ssl, unsigned char ** ocsp_response, size_t * response_len)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD
- `ocsp_response` (unsigned char **): TBD
- `response_len` (size_t *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_verify_ocsp_response`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_verify_ocsp_response(mbedtls_x509_crt * cert, const unsigned char * ocsp_response, size_t response_len)
```

**Parameters:**
- `cert` (mbedtls_x509_crt *): TBD
- `ocsp_response` (const unsigned char *): TBD
- `response_len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_enable_tls13`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_enable_tls13(uvhttp_tls_context_t ctx, int enable)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `enable` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_tls13_cipher_suites`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_tls13_cipher_suites(uvhttp_tls_context_t ctx, const char * cipher_suites)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `cipher_suites` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_enable_early_data`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_enable_early_data(uvhttp_tls_context_t ctx, int enable)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `enable` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_ticket_key`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_ticket_key(uvhttp_tls_context_t ctx, const unsigned char * key, size_t key_len)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `key` (const unsigned char *): TBD
- `key_len` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_rotate_ticket_key`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_rotate_ticket_key(uvhttp_tls_context_t ctx)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_set_ticket_lifetime`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_set_ticket_lifetime(uvhttp_tls_context_t ctx, int lifetime_seconds)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `lifetime_seconds` (int): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_verify_cert_chain`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_verify_cert_chain(mbedtls_ssl_context * ssl)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_context_add_extra_chain_cert`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_context_add_extra_chain_cert(uvhttp_tls_context_t ctx, const char * cert_file)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `cert_file` (const char *): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_get_cert_chain`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_get_cert_chain(mbedtls_ssl_context * ssl, mbedtls_x509_crt ** chain)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD
- `chain` (mbedtls_x509_crt **): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_get_stats`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_get_stats(uvhttp_tls_context_t ctx, uvhttp_tls_stats_t stats)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD
- `stats` (uvhttp_tls_stats_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_reset_stats`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_reset_stats(uvhttp_tls_context_t ctx)
```

**Parameters:**
- `ctx` (uvhttp_tls_context_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_get_connection_info`

**Signature:**
```c
uvhttp_error_t uvhttp_tls_get_connection_info(mbedtls_ssl_context * ssl, char * buf, size_t buf_size)
```

**Parameters:**
- `ssl` (mbedtls_ssl_context *): TBD
- `buf` (char *): TBD
- `buf_size` (size_t): TBD


**Returns:**
`uvhttp_error_t`



---
### `uvhttp_tls_get_error_string`

**Signature:**
```c
void uvhttp_tls_get_error_string(int ret, char * buf, size_t buf_size)
```

**Parameters:**
- `ret` (int): TBD
- `buf` (char *): TBD
- `buf_size` (size_t): TBD


**Returns:**
`void`



---
### `uvhttp_tls_print_error`

**Signature:**
```c
void uvhttp_tls_print_error(int ret)
```

**Parameters:**
- `ret` (int): TBD


**Returns:**
`void`



---
