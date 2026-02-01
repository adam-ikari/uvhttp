/**
 * @file uvhttp_config.h
 * @brief 配置管理系统
 *
 * 提供动态配置管理，支持运行时调整参数
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

/* 前向声明 */
struct uvhttp_context;
typedef struct uvhttp_context uvhttp_context_t;

/* 配置结构 */
typedef struct {
    /* 服务器配置 */
    int max_connections; /* 最大连接数，默认1000，基于系统资源限制 */
    int read_buffer_size; /* 读取缓冲区大小，默认8KB，平衡内存和性能 */
    int backlog; /* 监听队列长度，默认128，Linux内核默认值 */
    int keepalive_timeout; /* Keep-Alive超时，默认30秒，平衡连接复用和资源释放
                            */
    int request_timeout; /* 请求超时，默认30秒，基于Web应用常见需求 */
    int connection_timeout; /* 连接超时时间（秒），默认60秒，防止慢连接攻击 */

    /* 性能配置 */
    size_t max_body_size; /* 最大请求体大小，默认10MB，防止DoS攻击 */
    size_t max_header_size; /* 最大请求头大小，默认8KB，HTTP/1.1推荐值 */
    size_t max_url_size; /* 最大URL长度，默认2KB，浏览器限制 */
    size_t
        max_file_size; /* 文件响应的最大文件大小，默认100MB，平衡内存和带宽 */

    /* 安全配置 */
    int max_requests_per_connection; /* 每个连接最大请求数，默认100，防止长连接滥用
                                      */
    int rate_limit_window; /* 限流时间窗口，默认60秒，平衡精确性和性能 */

    /* WebSocket 配置 */
    int websocket_max_frame_size; /* 最大帧大小，默认16MB，RFC 6455建议值 */
    int websocket_max_message_size; /* 最大消息大小，默认64MB，基于实际应用场景
                                     */
    int websocket_ping_interval; /* Ping间隔，默认30秒，平衡连接检测和开销 */
    int websocket_ping_timeout; /* Ping超时，默认10秒，3倍RTT估算值 */

    /* 网络配置 */
    int tcp_keepalive_timeout; /* TCP Keep-Alive超时，默认60秒，标准值 */
    int sendfile_timeout_ms; /* sendfile超时，默认5000ms，平衡性能和可靠性 */
    int sendfile_max_retry; /* sendfile最大重试次数，默认3次，经验值 */

    /* 缓存配置 */
    int cache_default_max_entries; /* 缓存默认最大条目数，默认1000，基于内存和性能平衡
                                    */
    int cache_default_ttl; /* 缓存默认TTL，默认3600秒（1小时），常见Web缓存策略
                            */
    int lru_cache_batch_eviction_size; /* LRU缓存批量驱逐大小，默认10，性能优化
                                        */

    /* 限流配置 */
    int rate_limit_max_requests; /* 限流最大请求数，默认100，防止突发流量 */
    int rate_limit_max_window_seconds; /* 限流最大时间窗口，默认60秒，与rate_limit_window一致
                                        */
    int rate_limit_min_timeout_seconds; /* 限流最小超时时间，默认1秒，防止过于频繁的请求
                                         */
} uvhttp_config_t;

/* 配置管理函数 */
/**
 * @brief 创建新的配置对象
 * @param config 输出参数，用于接收配置对象指针
 * @return UVHTTP_OK 成功，其他值表示失败
 * @note 成功时，*config 被设置为有效的配置对象，必须使用 uvhttp_config_free
 * 释放
 * @note 失败时，*config 被设置为 NULL
 */
uvhttp_error_t uvhttp_config_new(uvhttp_config_t** config);
void uvhttp_config_free(uvhttp_config_t* config);
void uvhttp_config_set_defaults(uvhttp_config_t* config);

/* 配置加载和保存 */
int uvhttp_config_load_file(uvhttp_config_t* config, const char* filename);
int uvhttp_config_save_file(const uvhttp_config_t* config,
                            const char* filename);
int uvhttp_config_load_env(uvhttp_config_t* config);

/* 配置验证 */
int uvhttp_config_validate(const uvhttp_config_t* config);
void uvhttp_config_print(const uvhttp_config_t* config);

/* 动态配置调整 */
int uvhttp_config_update_max_connections(uvhttp_context_t* context,
                                         int max_connections);
int uvhttp_config_update_read_buffer_size(uvhttp_context_t* context,
                                          int buffer_size);
int uvhttp_config_update_size_limits(uvhttp_context_t* context,
                                     size_t max_body_size,
                                     size_t max_header_size);

/* 获取当前配置 */
const uvhttp_config_t* uvhttp_config_get_current(uvhttp_context_t* context);

/* 设置全局配置 */
void uvhttp_config_set_current(uvhttp_context_t* context,
                               uvhttp_config_t* config);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONFIG_H */