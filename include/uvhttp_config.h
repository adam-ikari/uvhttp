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
    int max_connections;  /* Maximum connections, default 1000, based on system
                             resource limits */
    int read_buffer_size; /* Read buffer size, default 8KB, balance memory and
                             performance */
    int backlog; /* Listen queue length, default 128, Linux kernel default */
    int keepalive_timeout; /* Keep-Alive timeout, default 30 seconds, balance
                            * connection reuse and resource release
                            */
    int request_timeout; /* Request timeout, default 30 seconds, based on common
                            web application requirements */
    int connection_timeout; /* Connection timeout (seconds), default 60 seconds,
                               prevent slow connection attacks */

    /* Performance configuration */
    size_t max_body_size;   /* Maximum request body size, default 10MB, prevent
                               DoS attacks */
    size_t max_header_size; /* Maximum request header size, default 8KB,
                               HTTP/1.1 recommended value */
    size_t max_url_size;    /* Maximum URL length, default 2KB, browser limit */
    size_t max_file_size; /* Maximum file size for file response, default 100MB,
                             balance memory and bandwidth */

    /* Security configuration */
    int max_requests_per_connection; /* Maximum requests per connection, default
                                      * 100, prevent long connection abuse
                                      */
    int rate_limit_window; /* Rate limiting time window, default 60 seconds,
                              balance accuracy and performance */

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

    /* Cache configuration */
    int cache_default_max_entries; /* Cache default maximum entries, default
                                    * 1000, based on memory and performance
                                    * balance
                                    */
    int cache_default_ttl; /* Cache default TTL, default 3600 seconds (1 hour),
                            * common web cache strategy
                            */
    int lru_cache_batch_eviction_size; /* LRU cache batch eviction size, default
                                        * 10, performance optimization
                                        */

    /* Rate limiting configuration */
    int rate_limit_max_requests; /* Rate limit max requests, default 100, prevent abuse */
    int rate_limit_max_window_seconds;  /* Rate limit window, default 60
                                           seconds, same as rate_limit_window */
    int rate_limit_min_timeout_seconds; /* Rate limit timeout when limit exceeded,
                                         * Default 1 seconds, prevent request spam
                                         */
} uvhttp_config_t;

/* Configuration management functions */
/**
 * @brief Create new configuration object
 * @param config Output parameter, used to receive the created object
 * @return UVHTTP_OK on success, other values represent failure
 * @note On success, *config is set to a valid object, must use uvhttp_config_free to release
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
int uvhttp_config_update_read_buffer_size(uvhttp_context_t* context,
                                          int buffer_size);
int uvhttp_config_update_size_limits(uvhttp_context_t* context,
                                     size_t max_body_size,
                                     size_t max_header_size);

/* Get current configuration */
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context);

/* Set global configuration */
void uvhttp_config_set_current(uvhttp_context_t* context,
                               uvhttp_config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONFIG_H */