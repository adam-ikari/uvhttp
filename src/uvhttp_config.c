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

/* 全局配置实例 */
static uvhttp_config_t* g_current_config = NULL;
static uvhttp_config_change_callback_t g_config_callback = NULL;

/* 配置键值对 */
typedef struct {
    const char* key;
    void* value;
    size_t size;
    int type; /* 0: int, 1: size_t, 2: double, 3: char* */
} config_key_t;

/* 配置键定义 */
static config_key_t config_keys[] = {
    {"max_connections", &g_current_config->max_connections, sizeof(int), 0},
    {"read_buffer_size", &g_current_config->read_buffer_size, sizeof(int), 0},
    {"backlog", &g_current_config->backlog, sizeof(int), 0},
    {"keepalive_timeout", &g_current_config->keepalive_timeout, sizeof(int), 0},
    {"request_timeout", &g_current_config->request_timeout, sizeof(int), 0},
    {"max_body_size", &g_current_config->max_body_size, sizeof(size_t), 1},
    {"max_header_size", &g_current_config->max_header_size, sizeof(size_t), 1},
    {"max_url_size", &g_current_config->max_url_size, sizeof(size_t), 1},
    {"worker_threads", &g_current_config->worker_threads, sizeof(int), 0},
    {"max_requests_per_connection", &g_current_config->max_requests_per_connection, sizeof(int), 0},
    {"rate_limit_window", &g_current_config->rate_limit_window, sizeof(int), 0},
    {"enable_compression", &g_current_config->enable_compression, sizeof(int), 0},
    {"enable_tls", &g_current_config->enable_tls, sizeof(int), 0},
    {"memory_pool_size", &g_current_config->memory_pool_size, sizeof(size_t), 1},
    {"enable_memory_debug", &g_current_config->enable_memory_debug, sizeof(int), 0},
    {"memory_warning_threshold", &g_current_config->memory_warning_threshold, sizeof(double), 2},
    {"log_level", &g_current_config->log_level, sizeof(int), 0},
    {"enable_access_log", &g_current_config->enable_access_log, sizeof(int), 0},
    {NULL, NULL, 0, 0}
};

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
    config->worker_threads = UVHTTP_DEFAULT_WORKER_THREADS;
    
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
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        UVHTTP_LOG_WARN("Config file not found: %s, using defaults", filename);
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), file)) {
        /* 跳过注释和空行 */
        char* ptr = line;
        while (isspace(*ptr)) ptr++;
        if (*ptr == '\0' || *ptr == '#') continue;
        
        /* 解析 key=value */
        char* eq = strchr(ptr, '=');
        if (!eq) continue;
        
        *eq = '\0';
        char* key = ptr;
        char* value = eq + 1;
        
        /* 去除空格 */
        while (isspace(*key)) key++;
        char* key_end = key + strlen(key) - 1;
        while (key_end > key && isspace(*key_end)) *key_end = '\0';
        
        while (isspace(*value)) value++;
        char* value_end = value + strlen(value) - 1;
        while (value_end > value && isspace(*value_end)) *value_end = '\0';
        
        /* 查找并设置配置 */
        for (int i = 0; config_keys[i].key; i++) {
            if (strcmp(key, config_keys[i].key) == 0) {
                void* old_value = UVHTTP_MALLOC(config_keys[i].size);
                if (old_value) {
                    memcpy(old_value, config_keys[i].value, config_keys[i].size);
                }
                
                switch (config_keys[i].type) {
                    case 0: /* int */
                        *(int*)config_keys[i].value = atoi(value);
                        break;
                    case 1: /* size_t */
                        *(size_t*)config_keys[i].value = (size_t)strtoull(value, NULL, 10);
                        break;
                    case 2: /* double */
                        *(double*)config_keys[i].value = atof(value);
                        break;
                    case 3: /* char* */
                        strncpy((char*)config_keys[i].value, value, 255);
                        ((char*)config_keys[i].value)[255] = '\0';
                        break;
                }
                
                /* 调用回调 */
                if (g_config_callback && old_value) {
                    g_config_callback(key, old_value, config_keys[i].value);
                    UVHTTP_FREE(old_value);
                }
                
                UVHTTP_LOG_INFO("Config updated: %s = %s", key, value);
                break;
            }
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
    fprintf(file, "worker_threads=%d\n", config->worker_threads);
    
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
    
    /* 检查环境变量并更新配置 */
    const char* env_val;
    
    if ((env_val = getenv("UVHTTP_MAX_CONNECTIONS"))) {
        config->max_connections = atoi(env_val);
    }
    if ((env_val = getenv("UVHTTP_READ_BUFFER_SIZE"))) {
        config->read_buffer_size = atoi(env_val);
    }
    if ((env_val = getenv("UVHTTP_MAX_BODY_SIZE"))) {
        config->max_body_size = (size_t)strtoull(env_val, NULL, 10);
    }
    if ((env_val = getenv("UVHTTP_ENABLE_TLS"))) {
        config->enable_tls = atoi(env_val);
    }
    if ((env_val = getenv("UVHTTP_LOG_LEVEL"))) {
        config->log_level = atoi(env_val);
    }
    
    return UVHTTP_OK;
}

/* 验证配置 */
int uvhttp_config_validate(const uvhttp_config_t* config) {
    if (!config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 验证连接数限制 */
    if (config->max_connections < 1 || config->max_connections > 10000) {
        UVHTTP_LOG_ERROR("Invalid max_connections: %d (must be 1-10000)", 
                        config->max_connections);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 验证缓冲区大小 */
    if (config->read_buffer_size < 1024 || config->read_buffer_size > 1024 * 1024) {
        UVHTTP_LOG_ERROR("Invalid read_buffer_size: %d (must be 1KB-1MB)", 
                        config->read_buffer_size);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 验证请求体大小 */
    if (config->max_body_size < 1024 || config->max_body_size > 100 * 1024 * 1024) {
        UVHTTP_LOG_ERROR("Invalid max_body_size: %zu (must be 1KB-100MB)", 
                        config->max_body_size);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 验证内存池大小 */
    if (config->memory_pool_size < 1024 * 1024 || 
        config->memory_pool_size > 1024 * 1024 * 1024) {
        UVHTTP_LOG_ERROR("Invalid memory_pool_size: %zu (must be 1MB-1GB)", 
                        config->memory_pool_size);
        return UVHTTP_ERROR_INVALID_PARAM;
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
    printf("  Worker Threads: %d\n", config->worker_threads);
    
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

/* 更新最大连接数 */
int uvhttp_config_update_max_connections(int max_connections) {
    if (!g_current_config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (max_connections < 1 || max_connections > 10000) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    int old_value = g_current_config->max_connections;
    g_current_config->max_connections = max_connections;
    
    if (g_config_callback) {
        g_config_callback("max_connections", &old_value, &max_connections);
    }
    
    UVHTTP_LOG_INFO("Max connections updated: %d -> %d", old_value, max_connections);
    return UVHTTP_OK;
}

/* 更新缓冲区大小 */
int uvhttp_config_update_buffer_size(int buffer_size) {
    if (!g_current_config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (buffer_size < 1024 || buffer_size > 1024 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    int old_value = g_current_config->read_buffer_size;
    g_current_config->read_buffer_size = buffer_size;
    
    if (g_config_callback) {
        g_config_callback("read_buffer_size", &old_value, &buffer_size);
    }
    
    UVHTTP_LOG_INFO("Read buffer size updated: %d -> %d", old_value, buffer_size);
    return UVHTTP_OK;
}

/* 更新限制参数 */
int uvhttp_config_update_limits(size_t max_body_size, size_t max_header_size) {
    if (!g_current_config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (max_body_size < 1024 || max_body_size > 100 * 1024 * 1024 ||
        max_header_size < 512 || max_header_size > 64 * 1024) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    size_t old_body = g_current_config->max_body_size;
    size_t old_header = g_current_config->max_header_size;
    
    g_current_config->max_body_size = max_body_size;
    g_current_config->max_header_size = max_header_size;
    
    if (g_config_callback) {
        g_config_callback("max_body_size", &old_body, &max_body_size);
        g_config_callback("max_header_size", &old_header, &max_header_size);
    }
    
    UVHTTP_LOG_INFO("Limits updated - Body: %zu -> %zu, Header: %zu -> %zu", 
                    old_body, max_body_size, old_header, max_header_size);
    return UVHTTP_OK;
}

/* 监控配置变化 */
int uvhttp_config_monitor_changes(uvhttp_config_change_callback_t callback) {
    g_config_callback = callback;
    return UVHTTP_OK;
}

/* 获取当前配置 */
const uvhttp_config_t* uvhttp_config_get_current(void) {
    return g_current_config;
}

/* 配置热重载 */
int uvhttp_config_reload(void) {
    if (!g_current_config) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 尝试从默认位置加载配置 */
    int result = uvhttp_config_load_file(g_current_config, "uvhttp.conf");
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_WARN("Failed to reload configuration");
        return result;
    }
    
    /* 验证新配置 */
    result = uvhttp_config_validate(g_current_config);
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Invalid configuration after reload");
        return result;
    }
    
    UVHTTP_LOG_INFO("Configuration reloaded successfully");
    return UVHTTP_OK;
}

/* 初始化全局配置 */
void uvhttp_config_init_global(uvhttp_config_t* config) {
    g_current_config = config;
}
