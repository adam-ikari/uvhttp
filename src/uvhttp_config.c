/**
 * @file uvhttp_config.c
 * @brief 配置管理系统实现
 */

#include "uvhttp_config.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/resource.h>

/* 全局配置实例 */
static uvhttp_config_t* g_current_config = NULL;

/* 创建新配置 */
uvhttp_config_t* uvhttp_config_new(void) {
    uvhttp_config_t* config = UVHTTP_MALLOC(sizeof(uvhttp_config_t));
    if (!config) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_OUT_OF_MEMORY, "Failed to allocate config");
        return NULL;
    }
    
    uvhttp_config_set_defaults(config);
    return config;
}

/* 释放配置 */
void uvhttp_config_free(uvhttp_config_t* config) {
    if (config) {
        UVHTTP_FREE(config);
    }
}

/* 设置默认配置 */
void uvhttp_config_set_defaults(uvhttp_config_t* config) {
    if (!config) return;
    
    config->max_connections = UVHTTP_DEFAULT_MAX_CONNECTIONS;
    config->read_buffer_size = UVHTTP_DEFAULT_READ_BUFFER_SIZE;
    config->backlog = UVHTTP_DEFAULT_BACKLOG;
    config->keepalive_timeout = UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT;
    config->request_timeout = UVHTTP_DEFAULT_REQUEST_TIMEOUT;
    
    config->max_body_size = UVHTTP_DEFAULT_MAX_BODY_SIZE;
    config->max_header_size = UVHTTP_DEFAULT_MAX_HEADER_SIZE;
    config->max_url_size = UVHTTP_DEFAULT_MAX_URL_SIZE;
    
    config->max_requests_per_connection = UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN;
    config->rate_limit_window = UVHTTP_DEFAULT_RATE_LIMIT_WINDOW;
    config->enable_compression = UVHTTP_DEFAULT_ENABLE_COMPRESSION;
    config->enable_tls = UVHTTP_DEFAULT_ENABLE_TLS;
    
    config->memory_pool_size = UVHTTP_DEFAULT_MEMORY_POOL_SIZE;
    config->enable_memory_debug = UVHTTP_DEFAULT_ENABLE_MEMORY_DEBUG;
    config->memory_warning_threshold = UVHTTP_DEFAULT_MEMORY_WARNING_THRESHOLD;
    
    config->log_level = UVHTTP_DEFAULT_LOG_LEVEL;
    config->enable_access_log = UVHTTP_DEFAULT_ENABLE_ACCESS_LOG;
    strcpy(config->log_file_path, "");
}

/* 从文件加载配置 */
int uvhttp_config_load_file(uvhttp_config_t* config, const char* filename) {
    if (!config || !filename) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 配置解析时的限制 */
#define UVHTTP_MAX_BODY_SIZE_CONFIG (100 * 1024 * 1024)  /* 避免与头文件中的宏冲突 */
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        /* 跳过注释和空行 */
        char* ptr = line;
        while (isspace(*ptr)) ptr++;
        if (*ptr == '\0' || *ptr == '#') continue;
        
        /* 简单解析 key=value */
        char* eq = strchr(ptr, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char* key = ptr;
        char* value = eq + 1;
        
        /* 去除空格和换行符 */
        while (isspace(*key)) key++;
        while (isspace(*value)) value++;
        
        /* 去除value末尾的换行符和空格 */
        char* end = value + strlen(value) - 1;
        while (end >= value && isspace(*end)) {
            *end = '\0';
            end--;
        }
        
        /* 设置核心配置 - 使用安全的strtol()进行验证 */
        if (strcmp(key, "max_connections") == 0) {
            UVHTTP_LOG_DEBUG("Parsing max_connections, value='%s'", value);
            char* endptr;
            long val = strtol(value, &endptr, 10);
            UVHTTP_LOG_DEBUG("strtol result: val=%ld, endptr='%s' (char=%d)", val, endptr, *endptr);
            if (*endptr != '\0') {
                UVHTTP_LOG_ERROR("Invalid max_connections=%s: contains non-numeric characters", value);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            if (val < 1 || val > 65535) {
                UVHTTP_LOG_ERROR("Invalid max_connections=%ld: out of range [1-65535]", val);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            config->max_connections = (int)val;
        } else if (strcmp(key, "read_buffer_size") == 0) {
            char* endptr;
            long val = strtol(value, &endptr, 10);
            if (*endptr != '\0' || val < 1024 || val > 1024 * 1024) {
                UVHTTP_LOG_ERROR("Invalid read_buffer_size=%s in config file", value);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            config->read_buffer_size = (int)val;
        } else if (strcmp(key, "max_body_size") == 0) {
            char* endptr;
            unsigned long long val = strtoull(value, &endptr, 10);
            if (*endptr != '\0') {
                UVHTTP_LOG_ERROR("Invalid max_body_size=%s: non-numeric characters", value);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            if (val > UVHTTP_MAX_BODY_SIZE_CONFIG) {
                UVHTTP_LOG_ERROR("Invalid max_body_size=%llu: exceeds limit %llu", val, UVHTTP_MAX_BODY_SIZE_CONFIG);
                fclose(file);
                return UVHTTP_ERROR_INVALID_PARAM;
            }
            config->max_body_size = (size_t)val;
        }
    }
    
    fclose(file);
    return UVHTTP_OK;
}

/* 保存配置到文件 */
int uvhttp_config_save_file(const uvhttp_config_t* config, const char* filename) {
    if (!config || !filename) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    FILE* file = fopen(filename, "w");
    if (!file) {
        UVHTTP_ERROR_REPORT(UVHTTP_ERROR_NOT_FOUND, "Failed to create config file");
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    fprintf(file, "# UVHTTP Configuration File\n");
    fprintf(file, "# Generated automatically\n\n");
    
    /* 写入配置项 */
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
    fprintf(file, "max_requests_per_connection=%d\n", config->max_requests_per_connection);
    fprintf(file, "rate_limit_window=%d\n", config->rate_limit_window);
    fprintf(file, "enable_compression=%d\n", config->enable_compression);
    fprintf(file, "enable_tls=%d\n", config->enable_tls);
    
    fprintf(file, "\n# Memory Configuration\n");
    fprintf(file, "memory_pool_size=%zu\n", config->memory_pool_size);
    fprintf(file, "enable_memory_debug=%d\n", config->enable_memory_debug);
    fprintf(file, "memory_warning_threshold=%.2f\n", config->memory_warning_threshold);
    
    fprintf(file, "\n# Logging Configuration\n");
    fprintf(file, "log_level=%d\n", config->log_level);
    fprintf(file, "enable_access_log=%d\n", config->enable_access_log);
    fprintf(file, "log_file_path=%s\n", config->log_file_path);
    
    fclose(file);
    UVHTTP_LOG_INFO("Configuration saved to %s", filename);
    return UVHTTP_OK;
}

/* 从环境变量加载配置 */
int uvhttp_config_load_env(uvhttp_config_t* config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 检查环境变量并更新配置 - 使用安全验证 */
    const char* env_val;
    
    if ((env_val = getenv("UVHTTP_MAX_CONNECTIONS"))) {
        char* endptr;
        long val = strtol(env_val, &endptr, 10);
        if (*endptr == '\0' && val >= 1 && val <= 65535) {
            config->max_connections = (int)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_MAX_CONNECTIONS=%s, using default", env_val);
        }
    }
    if ((env_val = getenv("UVHTTP_READ_BUFFER_SIZE"))) {
        char* endptr;
        long val = strtol(env_val, &endptr, 10);
        if (*endptr == '\0' && val >= 1024 && val <= 1024 * 1024) {
            config->read_buffer_size = (int)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_READ_BUFFER_SIZE=%s, using default", env_val);
        }
    }
    if ((env_val = getenv("UVHTTP_MAX_BODY_SIZE"))) {
        char* endptr;
        unsigned long long val = strtoull(env_val, &endptr, 10);
        if (*endptr == '\0' && val <= UVHTTP_MAX_BODY_SIZE_CONFIG) {
            config->max_body_size = (size_t)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_MAX_BODY_SIZE=%s, using default", env_val);
        }
    }
    if ((env_val = getenv("UVHTTP_ENABLE_TLS"))) {
        char* endptr;
        long val = strtol(env_val, &endptr, 10);
        if (*endptr == '\0' && (val == 0 || val == 1)) {
            config->enable_tls = (int)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_ENABLE_TLS=%s, using default", env_val);
        }
    }
    if ((env_val = getenv("UVHTTP_LOG_LEVEL"))) {
        char* endptr;
        long val = strtol(env_val, &endptr, 10);
        if (*endptr == '\0' && val >= 0 && val <= 5) {
            config->log_level = (int)val;
        } else {
            UVHTTP_LOG_WARN("Invalid UVHTTP_LOG_LEVEL=%s, using default", env_val);
        }
    }
    
    return UVHTTP_OK;
}

/* 验证配置 */
int uvhttp_config_validate(const uvhttp_config_t* config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 
     * 配置范围定义 - 基于性能测试和系统限制
     * 
     * UVHTTP_MIN_CONNECTIONS: 最小连接数，确保服务器基本功能
     * UVHTTP_MAX_CONNECTIONS_HARD: 硬限制，基于系统文件描述符限制
     * UVHTTP_MIN_BUFFER_SIZE: 最小缓冲区，确保基本HTTP处理能力
     * UVHTTP_MAX_BUFFER_SIZE: 最大缓冲区，平衡内存使用和性能
     * UVHTTP_MAX_BODY_SIZE_CONFIG: 最大请求体，防止内存耗尽攻击
     * 
     * 性能考虑:
     * - 每个连接约消耗4KB内存（缓冲区+结构体）
     * - 65535连接约消耗256MB内存
     * - 建议生产环境根据服务器内存调整max_connections
     */
#define UVHTTP_MIN_CONNECTIONS 1
#define UVHTTP_MAX_CONNECTIONS_HARD 65535  /* 基于系统限制 */
#define UVHTTP_MIN_BUFFER_SIZE 1024
#define UVHTTP_MAX_BUFFER_SIZE (1024 * 1024)
#define UVHTTP_MIN_BODY_SIZE 1024
#define UVHTTP_MAX_BODY_SIZE_CONFIG (100 * 1024 * 1024)  /* 避免与头文件中的宏冲突 */
    
    /* 核心验证：连接数和缓冲区大小 */
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
        UVHTTP_LOG_ERROR("max_body_size=%zu exceeds valid range [%zu-%llu]",
                         config->max_body_size, (size_t)UVHTTP_MIN_BODY_SIZE, 
                         UVHTTP_MAX_BODY_SIZE_CONFIG);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 检查文件描述符限制 */
    struct rlimit rl;
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        /* 为系统保留一些文件描述符 */
        int reserved_fds = 10;
        if ((size_t)config->max_connections > (size_t)rl.rlim_cur - (size_t)reserved_fds) {
            UVHTTP_LOG_WARN("max_connections=%d may exceed file descriptor limit=%zu",
                           config->max_connections, (size_t)rl.rlim_cur - reserved_fds);
        }
    }
    
    return UVHTTP_OK;
}

/* 打印配置 */
void uvhttp_config_print(const uvhttp_config_t* config) {
    if (!config) return;
    
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
    printf("  Max Requests per Connection: %d\n", config->max_requests_per_connection);
    printf("  Rate Limit Window: %d seconds\n", config->rate_limit_window);
    printf("  Enable Compression: %s\n", config->enable_compression ? "Yes" : "No");
    printf("  Enable TLS: %s\n", config->enable_tls ? "Yes" : "No");
    
    printf("\nMemory:\n");
    printf("  Memory Pool Size: %zu bytes\n", config->memory_pool_size);
    printf("  Enable Memory Debug: %s\n", config->enable_memory_debug ? "Yes" : "No");
    printf("  Memory Warning Threshold: %.2f\n", config->memory_warning_threshold);
    
    printf("\nLogging:\n");
    printf("  Log Level: %d\n", config->log_level);
    printf("  Enable Access Log: %s\n", config->enable_access_log ? "Yes" : "No");
    printf("  Log File Path: %s\n", config->log_file_path);
    printf("==============================\n\n");
}

/* 获取当前配置 */
const uvhttp_config_t* uvhttp_config_get_current(void) {
    if (!g_current_config) {
        UVHTTP_LOG_WARN("Global configuration not initialized");
    }
    return g_current_config;
}

/* 动态更新最大连接数 */
int uvhttp_config_update_max_connections(int max_connections) {
    if (max_connections < 1 || max_connections > 10000) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (g_current_config) {
        int old_value = g_current_config->max_connections;
        g_current_config->max_connections = max_connections;
        
        UVHTTP_LOG_INFO("Max connections updated: %d -> %d", old_value, max_connections);
        return UVHTTP_OK;
    }
    
    return UVHTTP_ERROR_INVALID_PARAM;
}

/* 动态更新缓冲区大小 */
int uvhttp_config_update_buffer_size(int buffer_size) {
    if (buffer_size < 1024 || buffer_size > 1024 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (g_current_config) {
        int old_value = g_current_config->read_buffer_size;
        g_current_config->read_buffer_size = buffer_size;
        
        UVHTTP_LOG_INFO("Read buffer size updated: %d -> %d", old_value, buffer_size);
        return UVHTTP_OK;
    }
    
    return UVHTTP_ERROR_INVALID_PARAM;
}

/* 动态更新限制参数 */
int uvhttp_config_update_limits(size_t max_body_size, size_t max_header_size) {
    if (max_body_size < 1024 || max_body_size > 100 * 1024 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    if (max_header_size < 512 || max_header_size > 64 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (g_current_config) {
        size_t old_body = g_current_config->max_body_size;
        size_t old_header = g_current_config->max_header_size;
        
        g_current_config->max_body_size = max_body_size;
        g_current_config->max_header_size = max_header_size;
        
        UVHTTP_LOG_INFO("Limits updated - Body: %zu -> %zu, Header: %zu -> %zu", 
                       old_body, max_body_size, old_header, max_header_size);
        return UVHTTP_OK;
    }
    
    return UVHTTP_ERROR_INVALID_PARAM;
}



/* 配置变更回调 */
static uvhttp_config_change_callback_t g_config_callback = NULL;

int uvhttp_config_monitor_changes(uvhttp_config_change_callback_t callback) {
    g_config_callback = callback;
    return UVHTTP_OK;
}

/* 热重载配置 */
int uvhttp_config_reload(void) {
    if (!g_current_config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 保存当前配置作为备份 */
    uvhttp_config_t backup = *g_current_config;
    
    /* 尝试重新加载配置文件 */
    int result = uvhttp_config_load_file(g_current_config, "uvhttp.conf");
    if (result != UVHTTP_OK) {
        /* 恢复备份 */
        *g_current_config = backup;
        return result;
    }
    
    /* 验证新配置 */
    if (uvhttp_config_validate(g_current_config) != UVHTTP_OK) {
        /* 恢复备份 */
        *g_current_config = backup;
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 触发配置变更回调 */
    if (g_config_callback) {
        g_config_callback("config_reload", &backup, g_current_config);
    }
    
    UVHTTP_LOG_INFO("Configuration reloaded successfully");
    return UVHTTP_OK;
}

/* 初始化全局配置 */
void uvhttp_config_init_global(uvhttp_config_t* config) {
    g_current_config = config;
}
