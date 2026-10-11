/**
 * @file uvhttp_config.c
 * @brief configmanagesystemimplement
 */

#include "uvhttp_config.h"

#include "uvhttp_allocator.h"
#include "uvhttp_context.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_logging.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>

/* create new config */
uvhttp_error_t uvhttp_config_new(uvhttp_config_t** config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *config = uvhttp_alloc(sizeof(uvhttp_config_t));
    if (!*config) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_OUT_OF_MEMORY,
                            "Failed to allocate config");
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    uvhttp_config_set_defaults(*config);
    return UVHTTP_OK;
}

/* releaseconfig */
void uvhttp_config_free(uvhttp_config_t* config) {
    if (config) {
        uvhttp_free(config);
    }
}

/* setdefaultconfig */
void uvhttp_config_set_defaults(uvhttp_config_t* config) {
    if (!config)
        return;

    config->max_connections = UVHTTP_DEFAULT_MAX_CONNECTIONS;
    config->backlog = UVHTTP_DEFAULT_BACKLOG;
    config->connection_timeout = UVHTTP_CONNECTION_TIMEOUT_DEFAULT;

    config->max_file_size = UVHTTP_DEFAULT_MAX_FILE_SIZE;

    config->trust_proxy_headers = 0; /* default: never trust proxy headers */

    /* WebSocket config */
    config->websocket_max_frame_size = UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE;
    config->websocket_max_message_size =
        UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE;
    config->websocket_ping_interval = UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL;
    config->websocket_ping_timeout = UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT;

    /* networkconfig */
    config->tcp_keepalive_timeout = UVHTTP_TCP_KEEPALIVE_TIMEOUT;
    config->sendfile_timeout_ms = UVHTTP_SENDFILE_TIMEOUT_MS;
    config->sendfile_max_retry = UVHTTP_SENDFILE_DEFAULT_MAX_RETRY;
}

/* Validate configuration */
int uvhttp_config_validate(const uvhttp_config_t* config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /*
     * Configuration range definitions - based on performance testing and system
     * limits
     *
     * UVHTTP_MIN_CONNECTIONS: Minimum connections, ensure basic server
     * functionality UVHTTP_MAX_CONNECTIONS_HARD: Hard limit, based on system
     * file descriptor limit
     *
     * Only fields that an execution path actually reads are validated here;
     * v2.10 removed the 14 fields whose values never reached the server (see
     * the "fields are either authoritative or absent" rule in docs/spec/
     * config-api.md).
     *
     * Performance considerations:
     * - Each connection consumes about 4KB memory (buffer + struct)
     * - 65535 connections consume about 256MB memory
     * - Recommended: adjust max_connections based on server memory in
     * production
     */
#define UVHTTP_MIN_CONNECTIONS 1
#define UVHTTP_MAX_CONNECTIONS_HARD 65535 /* Based on system limits */

    /* Core validation: connection count */
    if (config->max_connections < UVHTTP_MIN_CONNECTIONS ||
        config->max_connections > UVHTTP_MAX_CONNECTIONS_HARD) {
        UVHTTP_LOG_ERROR("max_connections=%d exceeds valid range [%d-%d]",
                         config->max_connections, UVHTTP_MIN_CONNECTIONS,
                         UVHTTP_MAX_CONNECTIONS_HARD);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Validate new runtime configuration */
    if (config->websocket_max_frame_size <
            UVHTTP_WEBSOCKET_CONFIG_MIN_FRAME_SIZE ||
        config->websocket_max_frame_size >
            UVHTTP_WEBSOCKET_CONFIG_MAX_FRAME_SIZE) {
        UVHTTP_LOG_ERROR(
            "websocket_max_frame_size=%d exceeds valid range [%d-%d]",
            config->websocket_max_frame_size,
            UVHTTP_WEBSOCKET_CONFIG_MIN_FRAME_SIZE,
            UVHTTP_WEBSOCKET_CONFIG_MAX_FRAME_SIZE);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->websocket_max_message_size <
            UVHTTP_WEBSOCKET_CONFIG_MIN_MESSAGE_SIZE ||
        config->websocket_max_message_size >
            UVHTTP_WEBSOCKET_CONFIG_MAX_MESSAGE_SIZE) {
        UVHTTP_LOG_ERROR(
            "websocket_max_message_size=%d exceeds valid range [%d-%d]",
            config->websocket_max_message_size,
            UVHTTP_WEBSOCKET_CONFIG_MIN_MESSAGE_SIZE,
            UVHTTP_WEBSOCKET_CONFIG_MAX_MESSAGE_SIZE);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->websocket_ping_interval <
            UVHTTP_WEBSOCKET_CONFIG_MIN_PING_INTERVAL ||
        config->websocket_ping_interval >
            UVHTTP_WEBSOCKET_CONFIG_MAX_PING_INTERVAL) {
        UVHTTP_LOG_ERROR(
            "websocket_ping_interval=%d exceeds valid range [%d-%d]",
            config->websocket_ping_interval,
            UVHTTP_WEBSOCKET_CONFIG_MIN_PING_INTERVAL,
            UVHTTP_WEBSOCKET_CONFIG_MAX_PING_INTERVAL);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->websocket_ping_timeout <
            UVHTTP_WEBSOCKET_CONFIG_MIN_PING_TIMEOUT ||

        config->websocket_ping_timeout >
            UVHTTP_WEBSOCKET_CONFIG_MAX_PING_TIMEOUT) {

        UVHTTP_LOG_ERROR(
            "websocket_ping_timeout=%d exceeds valid range [%d-%d]",

            config->websocket_ping_timeout,
            UVHTTP_WEBSOCKET_CONFIG_MIN_PING_TIMEOUT,

            UVHTTP_WEBSOCKET_CONFIG_MAX_PING_TIMEOUT);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->tcp_keepalive_timeout < UVHTTP_TCP_KEEPALIVE_MIN_TIMEOUT ||

        config->tcp_keepalive_timeout > UVHTTP_TCP_KEEPALIVE_MAX_TIMEOUT) {

        UVHTTP_LOG_ERROR("tcp_keepalive_timeout=%d exceeds valid range [%d-%d]",

                         config->tcp_keepalive_timeout,
                         UVHTTP_TCP_KEEPALIVE_MIN_TIMEOUT,

                         UVHTTP_TCP_KEEPALIVE_MAX_TIMEOUT);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->sendfile_timeout_ms < UVHTTP_SENDFILE_MIN_TIMEOUT_MS ||

        config->sendfile_timeout_ms > UVHTTP_SENDFILE_MAX_TIMEOUT_MS) {

        UVHTTP_LOG_ERROR("sendfile_timeout_ms=%d exceeds valid range [%d-%d]",

                         config->sendfile_timeout_ms,
                         UVHTTP_SENDFILE_MIN_TIMEOUT_MS,

                         UVHTTP_SENDFILE_MAX_TIMEOUT_MS);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Configuration dependency validation */
    if (config->websocket_max_message_size < config->websocket_max_frame_size) {
        UVHTTP_LOG_ERROR("websocket_max_message_size=%d < "
                         "websocket_max_frame_size=%d, invalid configuration",
                         config->websocket_max_message_size,
                         config->websocket_max_frame_size);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Validate fields that are actually consumed at runtime.
     * connection_timeout is multiplied by 1000 for uv_timer_start
     * (connection.c): 0 means an immediate timeout, a negative value makes
     * uv_timer_start fail with UV_EINVAL. */
    if (config->connection_timeout < UVHTTP_CONNECTION_TIMEOUT_MIN ||
        config->connection_timeout > UVHTTP_CONNECTION_TIMEOUT_MAX) {
        UVHTTP_LOG_ERROR("connection_timeout=%d exceeds valid range [%d-%d]",
                         config->connection_timeout,
                         UVHTTP_CONNECTION_TIMEOUT_MIN,
                         UVHTTP_CONNECTION_TIMEOUT_MAX);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->sendfile_max_retry < UVHTTP_SENDFILE_MIN_RETRY) {
        UVHTTP_LOG_ERROR("sendfile_max_retry=%d below minimum %d",
                         config->sendfile_max_retry, UVHTTP_SENDFILE_MIN_RETRY);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->trust_proxy_headers != 0 && config->trust_proxy_headers != 1) {
        UVHTTP_LOG_ERROR("trust_proxy_headers=%d must be 0 or 1",
                         config->trust_proxy_headers);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->backlog > config->max_connections) {
        UVHTTP_LOG_WARN("backlog=%d > max_connections=%d, this may cause "
                        "connection rejection",
                        config->backlog, config->max_connections);
    }

    /* Check file descriptor limits */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        /* Reserve some file descriptors for system */
        int reserved_fds = 10;
        if ((size_t)config->max_connections >
            (size_t)rl.rlim_cur - (size_t)reserved_fds) {
            UVHTTP_LOG_WARN(
                "max_connections=%d may exceed file descriptor limit=%zu",
                config->max_connections, (size_t)rl.rlim_cur - reserved_fds);
        }
    }

    return UVHTTP_OK;
}

/* Print configuration */
void uvhttp_config_print(const uvhttp_config_t* config) {
    if (!config)
        return;

    printf("\n=== UVHTTP Configuration ===\n");
    printf("Server:\n");
    printf("  Max Connections: %d\n", config->max_connections);
    printf("  Backlog: %d\n", config->backlog);
    printf("  Connection Timeout: %d seconds\n", config->connection_timeout);
    printf("  Max File Size: %zu bytes\n", config->max_file_size);
    printf("  Trust Proxy Headers: %d\n", config->trust_proxy_headers);
    printf("==============================\n\n");
}

/* Get current configuration */
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context) {
    if (!context) {
        UVHTTP_LOG_WARN("Context is NULL");
        return NULL;
    }
    if (!context->current_config) {
        UVHTTP_LOG_WARN("Configuration not initialized");
    }
    return context->current_config;
}

/* Dynamically update max connections */
int uvhttp_config_update_max_connections(uvhttp_context_t* context,
                                         int max_connections) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (max_connections < UVHTTP_MIN_CONNECTIONS ||
        max_connections > UVHTTP_MAX_CONNECTIONS_HARD) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (context->current_config) {
        int old_value =
            ((uvhttp_config_t*)context->current_config)->max_connections;
        ((uvhttp_config_t*)context->current_config)->max_connections =
            max_connections;

        UVHTTP_LOG_INFO("Max connections updated: %d -> %d", old_value,
                        max_connections);
        (void)old_value;
        return UVHTTP_OK;
    }

    return UVHTTP_ERROR_INVALID_PARAM;
}

/* Set global configuration */
void uvhttp_config_set_current(uvhttp_context_t* context,
                               uvhttp_config_t* config) {
    if (!context) {
        return;
    }

    /* Installing a configuration does NOT transfer ownership to the
     * context: the context only borrows the pointer and will not free it in
     * uvhttp_context_cleanup_config. The caller retains ownership and stays
     * responsible for releasing it (directly via uvhttp_config_free, or via
     * the server once the same object is assigned to server->config -
     * uvhttp_server_free frees it). */
    if (context->config_owned && context->current_config &&
        context->current_config != (void*)config) {
        /* The context created its own default config earlier: release it so
         * it does not leak once replaced. */
        uvhttp_config_free((uvhttp_config_t*)context->current_config);
    }
    context->current_config = config;
    context->config_owned = 0;
}
