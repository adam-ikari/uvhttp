/**
 * @file uvhttp_config.h
 * @brief Configuration management system
 *
 * Provides dynamic runtime adjustment
 */

#ifndef UVHTTP_CONFIG_H
#define UVHTTP_CONFIG_H

#include "uvhttp_constants.h"
#include "uvhttp_defaults.h"
#include "uvhttp_error.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct uvhttp_context;
typedef struct uvhttp_context uvhttp_context_t;

/* Configuration structure */
typedef struct {
    /* Server configuration */
    int max_connections; /* Maximum connections, default 1000, based on system
                            resource limits */
    int backlog; /* Listen queue length, default 128, Linux kernel default */
    int connection_timeout; /* Connection timeout (seconds), default 60 seconds,
                               prevent slow connection attacks */

    /* Performance configuration */
    size_t max_file_size; /* Maximum file size for file response, default 100MB,
                             balance memory and bandwidth */

    /* Security configuration */
    int trust_proxy_headers; /* Trust X-Forwarded-For/X-Real-IP when resolving
                              * client IP. Default 0 (spoof-safe): always use
                              * the TCP peer address. Enable only behind a
                              * trusted reverse proxy. */

    /* WebSocket configuration */
    int websocket_max_frame_size; /* Maximum frame size, default 16MB, RFC 6455
                                     recommended value */
    int websocket_max_message_size; /* Maximum message size, default 64MB, based
                                     * on actual application scenarios
                                     */
    int websocket_ping_interval; /* Ping interval, default 30 seconds, balance
                                    connection detection and overhead */
    int websocket_ping_timeout;  /* Ping timeout, default 10 seconds, 3x RTT
                                    estimated value */

    /* Network configuration */
    int tcp_keepalive_timeout; /* TCP Keep-Alive timeout, default 60 seconds,
                                  standard value */
    int sendfile_timeout_ms;   /* sendfile timeout, default 5000ms, balance
                                  performance and reliability */
    int sendfile_max_retry;    /* sendfile maximum retry count, default 3 times,
                                  empirical value */
} uvhttp_config_t;

/* Every field above is read by at least one execution path: these are the
 * compile-time defaults' runtime overrides. v2.10 deleted the 14 fields whose
 * values never reached the server (read_buffer_size, max_body_size,
 * max_header_size, max_url_size, max_requests_per_connection,
 * request_timeout, keepalive_timeout, the four rate_limit_* fields,
 * cache_default_max_entries, cache_default_ttl,
 * lru_cache_batch_eviction_size) - they were set, validated, printed, and then
 * ignored by every code path. Behaviour for those limits is owned by the
 * compile-time constants in uvhttp_constants.h (UVHTTP_MAX_BODY_SIZE,
 * UVHTTP_READ_BUFFER_SIZE, MAX_URL_LEN, UVHTTP_CACHE_DEFAULT_*). Request policy
 * is not a config concern at all: it runs in the server's request filter.
 * A field that is neither authoritative nor absent is noise: if you need one
 * of these knobs, add it back together with the code that reads it. */

/* Configuration management functions */
/**
 * @brief Create new configuration object
 * @param config Output parameter, used to receive the created object
 * @return UVHTTP_OK on success, other values represent failure
 * @note On success, *config is set to a valid object, must use
 * uvhttp_config_free to release
 * @note On failure, *config is set to NULL
 */
uvhttp_error_t uvhttp_config_new(uvhttp_config_t** config);
void uvhttp_config_free(uvhttp_config_t* config);
void uvhttp_config_set_defaults(uvhttp_config_t* config);

/* Configuration validation */
int uvhttp_config_validate(const uvhttp_config_t* config);
void uvhttp_config_print(const uvhttp_config_t* config);

/* Dynamic configuration adjustment */
int uvhttp_config_update_max_connections(uvhttp_context_t* context,
                                         int max_connections);

/* Get current configuration */
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context);

/* Set global configuration */
void uvhttp_config_set_current(uvhttp_context_t* context,
                               uvhttp_config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONFIG_H */