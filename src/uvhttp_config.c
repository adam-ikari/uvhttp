/**
 * @file uvhttp_config.c
 * @brief 配置管理系统实现
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

/* 创建新配置 */
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

/* 释放配置 */
void uvhttp_config_free(uvhttp_config_t* config) {
    if (config) {
        uvhttp_free(config);
    }
}

/* 设置默认配置 */
void uvhttp_config_set_defaults(uvhttp_config_t* config) {
    if (!config)
        return;

    config->max_connections = UVHTTP_DEFAULT_MAX_CONNECTIONS;
    config->read_buffer_size = UVHTTP_DEFAULT_READ_BUFFER_SIZE;
    config->backlog = UVHTTP_DEFAULT_BACKLOG;
    config->keepalive_timeout = UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT;
    config->request_timeout = UVHTTP_DEFAULT_REQUEST_TIMEOUT;
    config->connection_timeout = UVHTTP_CONNECTION_TIMEOUT_DEFAULT;

    config->max_body_size = UVHTTP_DEFAULT_MAX_BODY_SIZE;
    config->max_header_size = UVHTTP_DEFAULT_MAX_HEADER_SIZE;
    config->max_url_size = UVHTTP_DEFAULT_MAX_URL_SIZE;
    config->max_file_size = UVHTTP_DEFAULT_MAX_FILE_SIZE;

    config->max_requests_per_connection = UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN;
    config->rate_limit_window = UVHTTP_DEFAULT_RATE_LIMIT_WINDOW;

    /* WebSocket 配置 */
    config->websocket_max_frame_size = UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE;
    config->websocket_max_message_size =
        UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE;
    config->websocket_ping_interval = UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL;
    config->websocket_ping_timeout = UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT;

    /* 网络配置 */
    config->tcp_keepalive_timeout = UVHTTP_TCP_KEEPALIVE_TIMEOUT;
    config->sendfile_timeout_ms = UVHTTP_SENDFILE_TIMEOUT_MS;
    config->sendfile_max_retry = UVHTTP_SENDFILE_DEFAULT_MAX_RETRY;

    /* 缓存配置 */
    config->cache_default_max_entries = UVHTTP_CACHE_DEFAULT_MAX_ENTRIES;
    config->cache_default_ttl = UVHTTP_CACHE_DEFAULT_TTL;
    config->lru_cache_batch_eviction_size =
        UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE;

    /* 限流配置 */
    config->rate_limit_max_requests = UVHTTP_RATE_LIMIT_MAX_REQUESTS;
    config->rate_limit_max_window_seconds =
        UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS;
    config->rate_limit_min_timeout_seconds =
        UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS;
}

/* 从文件加载配置 */
int uvhttp_config_load_file(uvhttp_config_t* config, const char* filename) {
    if (!config || !filename) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 配置解析时的限制 */
#define UVHTTP_MAX_BODY_SIZE_CONFIG \
    (100 * 1024 * 1024) /* 避免与头文件中的宏冲突 */

    FILE* file = fopen(filename, "r");
    if (!file) {
        return UVHTTP_ERROR_NOT_FOUND;
    }

    char line[512];
    while (fgets(line, sizeof(line), file)) {
        /* Skip comments and blank lines */
        char* ptr = line;
        while (isspace(*ptr))
            ptr++;
        if (*ptr == '\0' || *ptr == '#')
            continue;

        /* Simple key=value parsing */
        char* eq = strchr(ptr, '=');
        if (!eq)
            continue;

        *eq = '\0';
        char* key = ptr;
        char* value = eq + 1;

        /* Remove spaces and newlines */
        while (isspace(*key))
            key++;
        while (isspace(*value))
            value++;

        /* Remove trailing newlines and spaces from value */
        char* end = value + strlen(value) - 1;
        while (end >= value && isspace(*end)) {
            *end = '\0';
            end--;
        }

        /* Set core configuration - use safe strtol() for validation */
        if (strcmp(key, "max_connections") == 0) {
            UVHTTP_LOG_DEBUG("Parsing max_connections, value='%s'", value);
            char* endptr;
            long val = strtol(value, &endptr, 10);
            UVHTTP_LOG_DEBUG("strtol result: val=%ld, endptr='%s' (char=%d)",
                             val, endptr, *endptr);
            if (*endptr != '\0') {
                UVHTTP_LOG_ERROR("Invalid max_connections=%s: contains "
                                 "non-numeric characters",
                                 value);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            if (val < 1 || val > 65535) {
                UVHTTP_LOG_ERROR(
                    "Invalid max_connections=%ld: out of range [1-65535]", val);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            config->max_connections = (int)val;
        } else if (strcmp(key, "read_buffer_size") == 0) {
            char* endptr;
            long val = strtol(value, &endptr, 10);
            if (*endptr != '\0' || val < 1024 || val > 1024 * 1024) {
                UVHTTP_LOG_ERROR("Invalid read_buffer_size=%s in config file",
                                 value);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            config->read_buffer_size = (int)val;
        } else if (strcmp(key, "max_body_size") == 0) {
            char* endptr;
            unsigned long long val = strtoull(value, &endptr, 10);
            if (*endptr != '\0') {
                UVHTTP_LOG_ERROR(
                    "Invalid max_body_size=%s: non-numeric characters", value);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            if (val > UVHTTP_MAX_BODY_SIZE_CONFIG) {
                UVHTTP_LOG_ERROR("Invalid max_body_size=%zu: exceeds limit %zu",
                                 (size_t)val,
                                 (size_t)UVHTTP_MAX_BODY_SIZE_CONFIG);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            config->max_body_size = (size_t)val;
        }
    }

    fclose(file);
    return UVHTTP_OK;
}

/* Save configuration to file */
int uvhttp_config_save_file(const uvhttp_config_t* config,
                            const char* filename) {
    if (!config || !filename) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    FILE* file = fopen(filename, "w");
    if (!file) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_NOT_FOUND,
                            "Failed to create config file");
        return UVHTTP_ERROR_NOT_FOUND;
    }

    fprintf(file, "# UVHTTP Configuration File\n");
    fprintf(file, "# Generated automatically\n\n");

    /* Write configuration items */
    fprintf(file, "# Server Configuration\n");
    fprintf(file, "max_connections=%d\n", config->max_connections);
    fprintf(file, "read_buffer_size=%d\n", config->read_buffer_size);
    fprintf(file, "backlog=%d\n", config->backlog);
    fprintf(file, "keepalive_timeout=%d\n", config->keepalive_timeout);
    fprintf(file, "request_timeout=%d\n", config->request_timeout);

    fprintf(file, "\n# Performance Configuration\n");
    fprintf(file, "max_body_size=%zu\n", config->max_body_size);
    fprintf(file, "max_header_size=%zu\n", config->max_header_size);
    fprintf(file, "max_url_size=%zu\n", config->max_url_size);

    fprintf(file, "\n# Security Configuration\n");
    fprintf(file, "max_requests_per_connection=%d\n",
            config->max_requests_per_connection);
    fprintf(file, "rate_limit_window=%d\n", config->rate_limit_window);

    fclose(file);
    UVHTTP_LOG_INFO("Configuration saved to %s", filename);
    return UVHTTP_OK;
}

/* Load configuration from environment variables */
int uvhttp_config_load_env(uvhttp_config_t* config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Check environment variables and update configuration - use safe validation */
    const char* env_val;

    if ((env_val = getenv("UVHTTP_MAX_CONNECTIONS"))) {
        char* endptr;
        long val = strtol(env_val, &endptr, 10);
        if (*endptr == '\0' && val >= 1 && val <= 65535) {
            config->max_connections = (int)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_MAX_CONNECTIONS=%s, using default",
                            env_val);
        }
    }
    if ((env_val = getenv("UVHTTP_READ_BUFFER_SIZE"))) {
        char* endptr;
        long val = strtol(env_val, &endptr, 10);
        if (*endptr == '\0' && val >= 1024 && val <= 1024 * 1024) {
            config->read_buffer_size = (int)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_READ_BUFFER_SIZE=%s, using default",
                            env_val);
        }
    }
    if ((env_val = getenv("UVHTTP_MAX_BODY_SIZE"))) {
        char* endptr;
        unsigned long long val = strtoull(env_val, &endptr, 10);
        if (*endptr == '\0' && val <= UVHTTP_MAX_BODY_SIZE_CONFIG) {
            config->max_body_size = (size_t)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_MAX_BODY_SIZE=%s, using default",
                            env_val);
        }
    }

    return UVHTTP_OK;
}

/* Validate configuration */
int uvhttp_config_validate(const uvhttp_config_t* config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /*
     * Configuration range definitions - based on performance testing and system limits
     *
     * UVHTTP_MIN_CONNECTIONS: Minimum connections, ensure basic server functionality
     * UVHTTP_MAX_CONNECTIONS_HARD: Hard limit, based on system file descriptor limit
     * UVHTTP_MIN_BUFFER_SIZE: Minimum buffer, ensure basic HTTP processing capability
     * UVHTTP_MAX_BUFFER_SIZE: Maximum buffer, balance memory usage and performance
     * UVHTTP_MAX_BODY_SIZE_CONFIG: Maximum request body, prevent memory exhaustion attack
     *
     * Performance considerations:
     * - Each connection consumes about 4KB memory (buffer + struct)
     * - 65535 connections consume about 256MB memory
     * - Recommended: adjust max_connections based on server memory in production
     */
#define UVHTTP_MIN_CONNECTIONS 1
#define UVHTTP_MAX_CONNECTIONS_HARD 65535 /* Based on system limits */
#define UVHTTP_MIN_BUFFER_SIZE 1024
#define UVHTTP_MAX_BUFFER_SIZE (1024 * 1024)
#define UVHTTP_MIN_BODY_SIZE 1024
#define UVHTTP_MAX_BODY_SIZE_CONFIG \
    (100 * 1024 * 1024) /* Avoid conflict with macros in header file */

    /* Core validation: connection count and buffer size */
    if (config->max_connections < UVHTTP_MIN_CONNECTIONS ||
        config->max_connections > UVHTTP_MAX_CONNECTIONS_HARD) {
        UVHTTP_LOG_ERROR("max_connections=%d exceeds valid range [%d-%d]",
                         config->max_connections, UVHTTP_MIN_CONNECTIONS,
                         UVHTTP_MAX_CONNECTIONS_HARD);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->read_buffer_size < UVHTTP_MIN_BUFFER_SIZE ||
        config->read_buffer_size > UVHTTP_MAX_BUFFER_SIZE) {
        UVHTTP_LOG_ERROR("read_buffer_size=%d exceeds valid range [%d-%d]",
                         config->read_buffer_size, UVHTTP_MIN_BUFFER_SIZE,
                         UVHTTP_MAX_BUFFER_SIZE);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->max_body_size < UVHTTP_MIN_BODY_SIZE ||
        config->max_body_size > UVHTTP_MAX_BODY_SIZE_CONFIG) {
        UVHTTP_LOG_ERROR("max_body_size=%zu exceeds valid range [%zu-%zu]",
                         config->max_body_size, (size_t)UVHTTP_MIN_BODY_SIZE,
                         (size_t)UVHTTP_MAX_BODY_SIZE_CONFIG);
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

    if (config->cache_default_max_entries < UVHTTP_CACHE_MIN_MAX_ENTRIES ||
        config->cache_default_max_entries > UVHTTP_CACHE_MAX_MAX_ENTRIES) {

        UVHTTP_LOG_ERROR(
            "cache_default_max_entries=%d exceeds valid range [%d-%d]",

            config->cache_default_max_entries, UVHTTP_CACHE_MIN_MAX_ENTRIES,

            UVHTTP_CACHE_MAX_MAX_ENTRIES);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->cache_default_ttl < UVHTTP_CACHE_MIN_TTL ||

        config->cache_default_ttl > UVHTTP_CACHE_MAX_TTL) {

        UVHTTP_LOG_ERROR("cache_default_ttl=%d exceeds valid range [%d-%d]",

                         config->cache_default_ttl, UVHTTP_CACHE_MIN_TTL,

                         UVHTTP_CACHE_MAX_TTL);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->lru_cache_batch_eviction_size <
            UVHTTP_LRU_CACHE_MIN_BATCH_EVICTION_SIZE ||

        config->lru_cache_batch_eviction_size >
            UVHTTP_LRU_CACHE_MAX_BATCH_EVICTION_SIZE) {

        UVHTTP_LOG_ERROR(
            "lru_cache_batch_eviction_size=%d exceeds valid range [%d-%d]",

            config->lru_cache_batch_eviction_size,

            UVHTTP_LRU_CACHE_MIN_BATCH_EVICTION_SIZE,

            UVHTTP_LRU_CACHE_MAX_BATCH_EVICTION_SIZE);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->rate_limit_max_requests < UVHTTP_RATE_LIMIT_MIN_MAX_REQUESTS ||

        config->rate_limit_max_requests > UVHTTP_RATE_LIMIT_MAX_MAX_REQUESTS) {

        UVHTTP_LOG_ERROR(
            "rate_limit_max_requests=%d exceeds valid range [%d-%d]",

            config->rate_limit_max_requests, UVHTTP_RATE_LIMIT_MIN_MAX_REQUESTS,

            UVHTTP_RATE_LIMIT_MAX_MAX_REQUESTS);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->rate_limit_max_window_seconds <
            UVHTTP_RATE_LIMIT_MIN_WINDOW_SECONDS ||

        config->rate_limit_max_window_seconds >
            UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS) {

        UVHTTP_LOG_ERROR(
            "rate_limit_max_window_seconds=%d exceeds valid range [%d-%d]",

            config->rate_limit_max_window_seconds,

            UVHTTP_RATE_LIMIT_MIN_WINDOW_SECONDS,

            UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS);

        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (config->rate_limit_min_timeout_seconds <
            UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS ||

        config->rate_limit_min_timeout_seconds >
            UVHTTP_RATE_LIMIT_MAX_TIMEOUT_SECONDS) {

        UVHTTP_LOG_ERROR(
            "rate_limit_min_timeout_seconds=%d exceeds valid range [%d-%d]",

            config->rate_limit_min_timeout_seconds,

            UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS,

            UVHTTP_RATE_LIMIT_MAX_TIMEOUT_SECONDS);

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
    printf("  Read Buffer Size: %d bytes\n", config->read_buffer_size);
    printf("  Backlog: %d\n", config->backlog);
    printf("  Keepalive Timeout: %d seconds\n", config->keepalive_timeout);
    printf("  Request Timeout: %d seconds\n", config->request_timeout);

    printf("\nPerformance:\n");
    printf("  Max Body Size: %zu bytes\n", config->max_body_size);
    printf("  Max Header Size: %zu bytes\n", config->max_header_size);
    printf("  Max URL Size: %zu bytes\n", config->max_url_size);
    printf("\nSecurity:\n");
    printf("  Max Requests per Connection: %d\n",
           config->max_requests_per_connection);
    printf("  Rate Limit Window: %d seconds\n", config->rate_limit_window);
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
    if (max_connections < 1 || max_connections > 10000) {
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

/* Dynamically update read buffer size */
int uvhttp_config_update_read_buffer_size(uvhttp_context_t* context,
                                          int buffer_size) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (buffer_size < 1024 || buffer_size > 1048576) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (context->current_config) {
        int old_value =
            ((uvhttp_config_t*)context->current_config)->read_buffer_size;
        ((uvhttp_config_t*)context->current_config)->read_buffer_size =
            buffer_size;

        UVHTTP_LOG_INFO("Read buffer size updated: %d -> %d", old_value,
                        buffer_size);
        (void)old_value;
        return UVHTTP_OK;
    }

    return UVHTTP_ERROR_INVALID_PARAM;
}

/* Dynamically update size limits */
int uvhttp_config_update_size_limits(uvhttp_context_t* context,
                                     size_t max_body_size,
                                     size_t max_header_size) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (max_body_size < 1024 || max_body_size > 100 * 1024 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (max_header_size < 512 || max_header_size > 64 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (context->current_config) {
        size_t old_body =
            ((uvhttp_config_t*)context->current_config)->max_body_size;
        size_t old_header =
            ((uvhttp_config_t*)context->current_config)->max_header_size;

        ((uvhttp_config_t*)context->current_config)->max_body_size =
            max_body_size;
        ((uvhttp_config_t*)context->current_config)->max_header_size =
            max_header_size;
        UVHTTP_LOG_INFO("Limits updated - Body: %zu -> %zu, Header: %zu -> %zu",
                        old_body, max_body_size, old_header, max_header_size);
        (void)old_body;
        (void)old_header;
        return UVHTTP_OK;
    }

    return UVHTTP_ERROR_INVALID_PARAM;
}

/* Set global configuration */
void uvhttp_config_set_current(uvhttp_context_t* context,
                               uvhttp_config_t* config) {
    if (context) {
        context->current_config = config;
    }
}
