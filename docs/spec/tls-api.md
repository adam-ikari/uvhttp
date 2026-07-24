# TLS API Spec

## Overview

The TLS module provides Transport Layer Security (TLS) for HTTP connections, implemented on top of mbedTLS 3.x. It manages TLS context creation, certificate/key loading, cipher suite configuration, TLS 1.3 support, mutual TLS (mTLS), session caching, and TLS handshake lifecycle. The module is guarded by the `UVHTTP_FEATURE_TLS` compile-time feature gate.

## Interfaces

### uvhttp_tls_context_new
- **Signature**: `uvhttp_error_t uvhttp_tls_context_new(uvhttp_tls_context_t** ctx)`
- **Purpose**: Create a new TLS context with default mbedTLS server configuration
- **Preconditions**: `ctx` must be a non-NULL pointer to `uvhttp_tls_context_t*`.
- **Postconditions**: On success, `*ctx` points to a valid TLS context with initialized mbedTLS config, entropy, DRBG, certificate store, and SSL cache. Defaults to server mode with stream transport and preset defaults. On failure, `*ctx` is set to NULL.
- **Error conditions**:
  - `UVHTTP_ERROR_INVALID_PARAM`: `ctx` is NULL
  - `UVHTTP_ERROR_OUT_OF_MEMORY`: allocation failure
  - `UVHTTP_ERROR_TLS_INIT`: DRBG seeding failed
  - `UVHTTP_ERROR_TLS_CONTEXT`: mbedtls_ssl_config_defaults failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_free
- **Signature**: `void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx)`
- **Purpose**: Free a TLS context and all associated mbedTLS resources
- **Preconditions**: `ctx` can be NULL (no-op). Must have been created by `uvhttp_tls_context_new`.
- **Postconditions**: All mbedTLS contexts (SSL config, certificates, private key, CA cert, CRL, entropy, DRBG, cache) are freed. The context memory is released. The pointer is invalid after return.
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_load_cert
- **Signature**: `uvhttp_error_t uvhttp_tls_context_load_cert_chain(uvhttp_tls_context_t* ctx, const char* cert_file)`
- **Purpose**: Load a certificate (chain) from a PEM file
- **Preconditions**: `ctx` must be valid. `cert_file` must be non-NULL and point to a readable PEM file.
- **Postconditions**: The certificate is parsed and configured as the server's own certificate.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` or `cert_file` is NULL
  - `UVHTTP_ERROR_TLS_CERT`: mbedTLS cert parse or config failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_load_key
- **Signature**: `uvhttp_error_t uvhttp_tls_context_load_private_key(uvhttp_tls_context_t* ctx, const char* key_file)`
- **Purpose**: Load a private key from a PEM file
- **Preconditions**: `ctx` must be valid. `key_file` must be non-NULL and point to a readable PEM key file. The certificate should have been loaded first (`uvhttp_tls_context_load_cert_chain`).
- **Postconditions**: The private key is parsed and associated with the context.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` or `key_file` is NULL
  - `UVHTTP_ERROR_TLS_KEY`: mbedTLS key parse failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_load_ca_file
- **Signature**: `uvhttp_error_t uvhttp_tls_context_load_ca_file(uvhttp_tls_context_t* ctx, const char* ca_file)`
- **Purpose**: Load a CA certificate file for client certificate verification
- **Preconditions**: `ctx` must be valid. `ca_file` must be non-NULL.
- **Postconditions**: The CA certificate is parsed and is set as the trusted CA chain for verification.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` or `ca_file` is NULL
  - `UVHTTP_ERROR_TLS_CA`: mbedTLS CA cert parse failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_enable_tls13
- **Signature**: `uvhttp_error_t uvhttp_tls_context_enable_tls13(uvhttp_tls_context_t* ctx, int enable)`
- **Purpose**: Enable or disable TLS 1.3 (minimum version)
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: When enabled, the minimum TLS version is set to TLS 1.3 (MBEDTLS_SSL_MINOR_VERSION_4). When disabled, the minimum is set to TLS 1.2 (MBEDTLS_SSL_MINOR_VERSION_3).
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_set_cipher_suites
- **Signature**: `uvhttp_error_t uvhttp_tls_context_set_cipher_suites(uvhttp_tls_context_t* ctx, const int* cipher_suites)`
- **Purpose**: Set the preferred cipher suites for the TLS context
- **Preconditions**: `ctx` must be valid. `cipher_suites` must be non-NULL and point to a zero-terminated array of mbedTLS cipher suite identifiers.
- **Postconditions**: The TLS configuration uses the specified cipher suites in preference order.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` or `cipher_suites` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_enable_client_auth
- **Signature**: `uvhttp_error_t uvhttp_tls_context_enable_client_auth(uvhttp_tls_context_t* ctx, int require_cert)`
- **Purpose**: Enable or disable client certificate authentication (mTLS)
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: When `require_cert` is non-zero, auth mode is set to `MBEDTLS_SSL_VERIFY_REQUIRED` (client must present a valid certificate). When zero, auth mode is `MBEDTLS_SSL_VERIFY_NONE`.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_set_verify_depth
- **Signature**: `uvhttp_error_t uvhttp_tls_context_set_verify_depth(uvhttp_tls_context_t* ctx, int depth)`
- **Purpose**: Set the certificate chain verification depth (currently a no-op placeholder)
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Currently returns UVHTTP_OK without modification (reserved for future use).
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_enable_session_tickets
- **Signature**: `uvhttp_error_t uvhttp_tls_context_enable_session_tickets(uvhttp_tls_context_t* ctx, int enable)`
- **Purpose**: Enable or disable TLS session tickets (currently a no-op placeholder)
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Returns UVHTTP_OK. Session ticket support is temporarily disabled in the mbedTLS configuration.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_context_set_session_cache
- **Signature**: `uvhttp_error_t uvhttp_tls_context_set_session_cache(uvhttp_tls_context_t* ctx, int max_sessions)`
- **Purpose**: Set the maximum number of entries in the TLS session cache
- **Preconditions**: `ctx` must be valid. `max_sessions` must be >= 0.
- **Postconditions**: The session cache entry limit is updated.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_create_ssl
- **Signature**: `mbedtls_ssl_context* uvhttp_tls_create_ssl(uvhttp_tls_context_t* ctx)`
- **Purpose**: Create an SSL session context from a TLS context
- **Preconditions**: `ctx` must be valid.
- **Postconditions**: Returns a valid mbedTLS SSL context on success, NULL on failure.
- **Error conditions**:
  - Returns NULL: `ctx` is NULL, allocation failure, or mbedtls_ssl_setup failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_setup_ssl
- **Signature**: `uvhttp_error_t uvhttp_tls_setup_ssl(mbedtls_ssl_context* ssl, int fd)`
- **Purpose**: Attach a socket file descriptor to an SSL context for BIO
- **Preconditions**: `ssl` must be non-NULL. `fd` must be a valid socket descriptor.
- **Postconditions**: Custom BIO callbacks (`mbedtls_net_send`/`mbedtls_net_recv`) are set on the SSL context, using the provided file descriptor.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ssl` is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_handshake
- **Signature**: `uvhttp_error_t uvhttp_tls_handshake(mbedtls_ssl_context* ssl)`
- **Purpose**: Perform the TLS handshake (non-blocking)
- **Preconditions**: `ssl` must be non-NULL and must have been set up with a socket via `uvhttp_tls_setup_ssl`.
- **Postconditions**: On success, the TLS handshake is complete. Non-blocking states return specific error codes.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ssl` is NULL
  - `UVHTTP_ERROR_TLS_WANT_READ`: handshake needs more data from socket (non-blocking)
  - `UVHTTP_ERROR_TLS_WANT_WRITE`: handshake needs to write to socket (non-blocking)
  - `UVHTTP_ERROR_TLS_HANDSHAKE`: handshake failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_read
- **Signature**: `uvhttp_error_t uvhttp_tls_read(mbedtls_ssl_context* ssl, void* buf, size_t len)`
- **Purpose**: Read decrypted data from a TLS connection
- **Preconditions**: `ssl` must be non-NULL and handshake must be complete. `buf` must be non-NULL.
- **Postconditions**: On success, returns the number of bytes read (positive value). On non-blocking WANT_READ/WANT_WRITE, returns the corresponding error code.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ssl` or `buf` is NULL
  - `UVHTTP_ERROR_TLS_WANT_READ`: non-blocking, need more data
  - `UVHTTP_ERROR_TLS_WANT_WRITE`: non-blocking, need to write
  - `UVHTTP_ERROR_TLS_READ`: read error
- **Thread safety**: Not thread-safe.

### uvhttp_tls_write
- **Signature**: `uvhttp_error_t uvhttp_tls_write(mbedtls_ssl_context* ssl, const void* buf, size_t len)`
- **Purpose**: Write encrypted data to a TLS connection
- **Preconditions**: `ssl` must be non-NULL and handshake must be complete. `buf` must be non-NULL.
- **Postconditions**: On success, returns the number of bytes written (positive value). On non-blocking WANT_READ/WANT_WRITE, returns the corresponding error code.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ssl` or `buf` is NULL
  - `UVHTTP_ERROR_TLS_WANT_READ`: non-blocking, need to read
  - `UVHTTP_ERROR_TLS_WANT_WRITE`: non-blocking, need to write
  - `UVHTTP_ERROR_TLS_WRITE`: write error
- **Thread safety**: Not thread-safe.

### uvhttp_tls_init / uvhttp_tls_cleanup
- **Signature**: `uvhttp_error_t uvhttp_tls_init(uvhttp_context_t* context)` / `void uvhttp_tls_cleanup(uvhttp_context_t* context)`
- **Purpose**: Initialize/cleanup the global TLS subsystem (entropy source, DRBG)
- **Preconditions**: `context` must be non-NULL. For `uvhttp_tls_cleanup`, the context must have been initialized.
- **Postconditions**: `context->tls_initialized` is set to 1 on success, 0 after cleanup.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `context` is NULL
  - `UVHTTP_ERROR_TLS_INIT`: DRBG seeding failed
- **Thread safety**: Not thread-safe.

### uvhttp_tls_verify_peer_cert
- **Signature**: `int uvhttp_tls_verify_peer_cert(mbedtls_ssl_context* ssl)`
- **Purpose**: Verify the peer's certificate chain
- **Preconditions**: `ssl` must be non-NULL and handshake must be complete.
- **Postconditions**: Returns 1 if verification succeeded (flags=0), 0 otherwise.
- **Thread safety**: Thread-safe for reads after handshake.

### uvhttp_tls_verify_hostname
- **Signature**: `int uvhttp_tls_verify_hostname(mbedtls_x509_crt* cert, const char* hostname)`
- **Purpose**: Verify a hostname against a certificate (CN and SAN)
- **Preconditions**: `cert` and `hostname` must be non-NULL.
- **Postconditions**: Returns 1 if the hostname matches the certificate's CN or any SAN entry, 0 otherwise.
- **Thread safety**: Thread-safe for reads.

### uvhttp_tls_check_cert_validity
- **Signature**: `int uvhttp_tls_check_cert_validity(mbedtls_x509_crt* cert)`
- **Purpose**: Check if a certificate is within its validity period
- **Preconditions**: `cert` must be non-NULL.
- **Postconditions**: Returns 1 if the certificate is valid (not yet expired and not before its valid_from date), 0 otherwise.
- **Thread safety**: Thread-safe for reads.

### uvhttp_tls_get_stats / uvhttp_tls_reset_stats
- **Signature**: `uvhttp_error_t uvhttp_tls_get_stats(uvhttp_tls_context_t* ctx, uvhttp_tls_stats_t* stats)` / `uvhttp_error_t uvhttp_tls_reset_stats(uvhttp_tls_context_t* ctx)`
- **Purpose**: Get or reset TLS performance statistics
- **Preconditions**: `ctx` must be valid. For `uvhttp_tls_get_stats`, `stats` must be non-NULL.
- **Postconditions**: Statistics are copied to the output struct or zeroed.
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ctx` or `stats` (for get) is NULL
- **Thread safety**: Not thread-safe.

### uvhttp_tls_get_connection_info
- **Signature**: `uvhttp_error_t uvhttp_tls_get_connection_info(mbedtls_ssl_context* ssl, char* buf, size_t buf_size)`
- **Purpose**: Get a human-readable string with TLS version and cipher suite
- **Preconditions**: `ssl` and `buf` must be non-NULL.
- **Postconditions**: `buf` is filled with a string like "Version: TLSv1.3, Cipher: TLS-AES-256-GCM-SHA384".
- **Error conditions**:
  - `UVHTTP_ERROR_TLS_INVALID_PARAM`: `ssl` or `buf` is NULL
- **Thread safety**: Thread-safe for reads after handshake.

### uvhttp_tls_get_error_string / uvhttp_tls_print_error
- **Signature**: `void uvhttp_tls_get_error_string(int ret, char* buf, size_t buf_size)` / `void uvhttp_tls_print_error(int ret)`
- **Purpose**: Convert an mbedTLS error code to a human-readable string
- **Preconditions**: `buf` must be non-NULL and `buf_size` > 0 for `uvhttp_tls_get_error_string`.
- **Postconditions**: The error string is written to the buffer or logged.
- **Thread safety**: Thread-safe.

## TLS Handshake Lifecycle

1. **Context creation**: `uvhttp_tls_context_new` creates a server-side TLS context with default configuration.
2. **Certificate loading**: `uvhttp_tls_context_load_cert_chain` loads the server certificate. `uvhttp_tls_context_load_private_key` loads the corresponding private key.
3. **Optional CA loading**: `uvhttp_tls_context_load_ca_file` loads CA certificates for client verification.
4. **Optional configuration**: Cipher suites, TLS 1.3, mTLS, session cache, and CRL are configured as needed.
5. **SSL session creation**: `uvhttp_tls_create_ssl` creates an SSL session from the context.
6. **BIO setup**: `uvhttp_tls_setup_ssl` attaches the socket file descriptor to the SSL session.
7. **Handshake**: `uvhttp_tls_handshake` performs the TLS handshake. This is non-blocking and may return `WANT_READ`/`WANT_WRITE` to be called again after the socket is ready.
8. **Data transfer**: `uvhttp_tls_read` and `uvhttp_tls_write` handle encrypted data transfer.
9. **Cleanup**: The SSL session is freed by the caller. The TLS context is freed via `uvhttp_tls_context_free`.

## Behavior Rules

1. **mbedTLS backend**: All TLS operations are delegated to mbedTLS 3.x. The library provides entropy, DRBG, certificate parsing, SSL handshake, and cipher operations.

2. **Non-blocking handshake**: The handshake uses custom BIO callbacks (`mbedtls_net_send`/`mbedtls_net_recv`) that translate EAGAIN/EWOULDBLOCK to `MBEDTLS_ERR_SSL_WANT_READ`/`WANT_WRITE`. These are propagated to the caller as `UVHTTP_ERROR_TLS_WANT_READ`/`UVHTTP_ERROR_TLS_WANT_WRITE` for integration with libuv's event loop.

3. **Session cache**: The session cache is initialized but currently disconnected from the SSL config (pending thread-safety verification). Sessions are not cached in the current version.

4. **TLS 1.3 support**: When enabled, the minimum TLS version is set to TLS 1.3 (MBEDTLS_SSL_MINOR_VERSION_4). When disabled, minimum is TLS 1.2.

5. **mTLS support**: Client certificate authentication is configurable via `uvhttp_tls_context_enable_client_auth`. Requires CA certificates to be loaded first.

6. **Certificate validation**: The `uvhttp_tls_check_cert_validity` checks both `valid_from` and `valid_to` dates. The `uvhttp_tls_verify_peer_cert` checks the verification result flags.

7. **Hostname verification**: CN (Common Name) and SAN (Subject Alternative Name) are both checked. Uses exact string matching (wildcard support is not yet implemented).

8. **CRL checking**: Certificate Revocation List checking can be enabled via `uvhttp_tls_context_enable_crl_checking`. CRL files are loaded via `uvhttp_tls_load_crl_file`.

9. **Double-free protection**: `uvhttp_tls_context_free` handles NULL gracefully. All mbedTLS sub-contexts are freed in order.

10. **Statistics**: The TLS context tracks handshake count, errors, bytes sent/received, session hits/misses, and average handshake time.

## Performance Requirements

- TLS context creation: O(1) with mbedTLS initialization
- SSL session creation: O(1)
- Handshake: O(n) where n is the certificate chain length; typically 2-3 RTT for TLS 1.2, 1-2 RTT for TLS 1.3
- Data transfer: O(n) where n is the data size; encryption overhead ~5-15% CPU
- Memory: ~1KB per TLS context, ~512 bytes per SSL session
- Cipher suite selection: O(1) via mbedTLS internal lookup

## Test Requirements

- TLS context creation and destruction
- Certificate and key loading (valid and invalid files)
- TLS 1.3 enable/disable
- Cipher suite configuration
- mTLS client auth enable/disable
- SSL session creation, setup, and handshake
- Non-blocking handshake with WANT_READ/WANT_WRITE handling
- TLS read/write operations
- Peer certificate verification (valid and invalid)
- Hostname verification (CN match, SAN match, no match)
- Certificate validity checking (valid, expired, not-yet-valid)
- Certificate subject/issuer/serial retrieval
- CRL enable and file loading
- Session cache configuration
- TLS statistics get and reset
- Connection info string formatting
- NULL parameter handling for all public functions
- Memory cleanup (no leaks on free)
- Feature gate: builds without UVHTTP_FEATURE_TLS