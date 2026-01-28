/**
 * @file uvhttp_config.h
 * @brief 配置管理系统
 * 
 * 提供动态配置管理，支持运行时调整参数
 */

#ifndef UVHTTP_CONFIG_H
#define UVHTTP_CONFIG_H

#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct uvhttp_context;
typedef struct uvhttp_context uvhttp_context_t;

/* 配置结构 */
typedef struct {
    /* 服务器配置 */
    int max_connections;
    int read_buffer_size;
    int backlog;
    int keepalive_timeout;
    int request_timeout;
    int connection_timeout;  /* 连接超时时间（秒），默认60秒 */
    
    /* 性能配置 */
    size_t max_body_size;
    size_t max_header_size;
    size_t max_url_size;
    size_t max_file_size;  /* 文件响应的最大文件大小 */
    
    /* 安全配置 */
    int max_requests_per_connection;
    int rate_limit_window;
    int enable_compression;
    int enable_tls;
    
    /* 内存配置 */
    size_t memory_pool_size;
    int enable_memory_debug;
    double memory_warning_threshold;
    
    /* 日志配置 - 已移除 */
    int log_level;
    int enable_access_log;
    char log_file_path[256];
} uvhttp_config_t;

/* 默认配置值 */
#define UVHTTP_DEFAULT_MAX_CONNECTIONS       2048   /* 使用默认值，适合大多数应用 */
#define UVHTTP_DEFAULT_READ_BUFFER_SIZE      8192
#define UVHTTP_DEFAULT_BACKLOG               256
#define UVHTTP_DEFAULT_KEEPALIVE_TIMEOUT     30
#define UVHTTP_DEFAULT_REQUEST_TIMEOUT       60
#define UVHTTP_DEFAULT_MAX_BODY_SIZE         (1024 * 1024)
#define UVHTTP_DEFAULT_MAX_HEADER_SIZE       8192
#define UVHTTP_DEFAULT_MAX_URL_SIZE          2048
#define UVHTTP_DEFAULT_MAX_FILE_SIZE         (10 * 1024 * 1024)  /* 10MB */
#define UVHTTP_DEFAULT_MAX_REQUESTS_PER_CONN 100
#define UVHTTP_DEFAULT_RATE_LIMIT_WINDOW     60
#define UVHTTP_DEFAULT_ENABLE_COMPRESSION     1
#define UVHTTP_DEFAULT_ENABLE_TLS            0
#define UVHTTP_DEFAULT_MEMORY_POOL_SIZE      (16 * 1024 * 1024)
#define UVHTTP_DEFAULT_ENABLE_MEMORY_DEBUG   0
#define UVHTTP_DEFAULT_MEMORY_WARNING_THRESHOLD 0.8
#define UVHTTP_DEFAULT_LOG_LEVEL             2  /* INFO */
#define UVHTTP_DEFAULT_ENABLE_ACCESS_LOG     0

/* 配置管理函数 */
/**
 * @brief 创建新的配置对象
 * @param config 输出参数，用于接收配置对象指针
 * @return UVHTTP_OK 成功，其他值表示失败
 * @note 成功时，*config 被设置为有效的配置对象，必须使用 uvhttp_config_free 释放
 * @note 失败时，*config 被设置为 NULL
 */
uvhttp_error_t uvhttp_config_new(uvhttp_config_t** config);
void uvhttp_config_free(uvhttp_config_t* config);
void uvhttp_config_set_defaults(uvhttp_config_t* config);

/* 配置加载和保存 */
int uvhttp_config_load_file(uvhttp_config_t* config, const char* filename);
int uvhttp_config_save_file(const uvhttp_config_t* config, const char* filename);
int uvhttp_config_load_env(uvhttp_config_t* config);

/* 配置验证 */
int uvhttp_config_validate(const uvhttp_config_t* config);
void uvhttp_config_print(const uvhttp_config_t* config);

/* 动态配置调整 */
int uvhttp_config_update_max_connections(uvhttp_context_t* context, int max_connections);
int uvhttp_config_update_read_buffer_size(uvhttp_context_t* context, int buffer_size);
int uvhttp_config_update_size_limits(uvhttp_context_t* context, size_t max_body_size, size_t max_header_size);

/* 配置监控 */
typedef void (*uvhttp_config_change_callback_t)(const char* key, const void* old_value, const void* new_value);
int uvhttp_config_monitor_changes(uvhttp_context_t* context, uvhttp_config_change_callback_t callback);

/* 获取当前配置 */
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context);

/* 设置全局配置 */
void uvhttp_config_set_current(uvhttp_context_t* context, uvhttp_config_t* config);

/* 配置热重载 */
int uvhttp_config_reload(uvhttp_context_t* context);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONFIG_H */