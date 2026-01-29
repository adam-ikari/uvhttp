/* UVHTTP 静态文件服务模块 - 统一版本，支持可选LRU缓存 */

#ifndef UVHTTP_STATIC_H
#define UVHTTP_STATIC_H

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_constants.h"
#    include "uvhttp_error.h"
#    include "uvhttp_middleware.h"

#    include <stddef.h>
#    include <time.h>

/* LRU缓存条件编译支持 */
#    if UVHTTP_FEATURE_LRU_CACHE
#        include "uvhttp_lru_cache.h"
#    endif

/* 前向声明 */
typedef struct cache_manager cache_manager_t;
typedef struct cache_entry cache_entry_t;

#    ifdef __cplusplus
extern "C" {
#    endif

/* 静态文件配置结构 - CPU 缓存优化布局 */
typedef struct uvhttp_static_config {
    /* 8字节对齐字段 - 热路径 */
    size_t max_cache_size;      /* 最大缓存大小（字节） */
    size_t sendfile_chunk_size; /* sendfile 分块大小（字节） */

    /* 4字节对齐字段 - 中等访问频率 */
    int cache_ttl;                /* 缓存TTL（秒） */
    int max_cache_entries;        /* 最大缓存条目数 */
    int sendfile_timeout_ms;      /* sendfile 超时时间（毫秒） */
    int sendfile_max_retry;       /* sendfile 最大重试次数 */
    int enable_directory_listing; /* 是否启用目录列表 */
    int enable_etag;              /* 是否启用ETag */
    int enable_last_modified;     /* 是否启用Last-Modified */
    int enable_sendfile;          /* 是否启用 sendfile 零拷贝优化 */

    /* 字符串字段 - 冷路径 */
    char root_directory[UVHTTP_MAX_FILE_PATH_SIZE];    /* 根目录路径 */
    char index_file[UVHTTP_MAX_PATH_SIZE];             /* 默认索引文件 */
    char custom_headers[UVHTTP_MAX_HEADER_VALUE_SIZE]; /* 自定义响应头 */
} uvhttp_static_config_t;

/* 静态文件服务上下文 */
typedef struct uvhttp_static_context {
    uvhttp_static_config_t config; /* 配置 */
    cache_manager_t* cache;        /* LRU缓存管理器 */
} uvhttp_static_context_t;

/* MIME类型映射条目 */
typedef struct uvhttp_mime_mapping {
    const char* extension; /* 文件扩展名 */
    const char* mime_type; /* MIME类型 */
} uvhttp_mime_mapping_t;

/**
 * 创建静态文件服务上下文
 *
 * @param config 静态文件配置
 * @param context 输出参数，返回创建的上下文
 * @return UVHTTP_OK 成功，其他值表示错误
 */
uvhttp_error_t uvhttp_static_create(const uvhttp_static_config_t* config,
                                    uvhttp_static_context_t** context);
/**
 * 设置 sendfile 配置参数
 *
 * @param ctx 静态文件服务上下文
 * @param timeout_ms 超时时间（毫秒），0 表示使用默认值
 * @param max_retry 最大重试次数，0 表示使用默认值
 * @param chunk_size 分块大小（字节），0 表示使用默认值
 * @return UVHTTP_OK 成功，其他值表示失败
 */
uvhttp_error_t uvhttp_static_set_sendfile_config(uvhttp_static_context_t* ctx,
                                                 int timeout_ms, int max_retry,
                                                 size_t chunk_size);

/**
 * 释放静态文件服务上下文
 *
 * @param ctx 静态文件服务上下文
 */
void uvhttp_static_free(uvhttp_static_context_t* ctx);

/**
 * 处理静态文件请求
 *
 * @param ctx 静态文件服务上下文
 * @param request HTTP请求
 * @param response HTTP响应
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                             void* request, void* response);

/**
 * Nginx 优化：使用 sendfile 零拷贝发送静态文件（混合策略）
 *
 * 根据文件大小自动选择最优策略：
 * - 小文件 (< 4KB): 使用传统方式（避免 sendfile 开销）
 * - 中等文件 (4KB - 10MB): 使用异步 sendfile
 * - 大文件 (> 10MB): 使用分块 sendfile
 *
 * @param file_path 文件路径
 * @param response HTTP响应
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_sendfile(const char* file_path, void* response);

/**
 * 根据文件扩展名获取MIME类型
 *
 * @param file_path 文件路径
 * @param mime_type 输出MIME类型缓冲区
 * @param buffer_size 缓冲区大小
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_get_mime_type(const char* file_path,
                                            char* mime_type,
                                            size_t buffer_size);

/**
 * 清理文件缓存
 *
 * @param ctx 静态文件服务上下文
 */
void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx);

/**
 * 缓存预热：预加载指定的文件到缓存中
 *
 * @param ctx 静态文件服务上下文
 * @param file_path 文件路径（相对于根目录）
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx,
                                            const char* file_path);

/**
 * 缓存预热：预加载目录中的所有文件
 *
 * @param ctx 静态文件服务上下文
 * @param dir_path 目录路径（相对于根目录）
 * @param max_files 最大文件数（0表示无限制）
 * @return 预热的文件数量，-1表示失败
 */
int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx,
                                    const char* dir_path, int max_files);

/**
 * 检查文件路径是否安全（防止路径遍历攻击）
 *
 * @param root_dir 根目录
 * @param file_path 请求的文件路径
 * @param resolved_path 解析后的安全路径
 * @param buffer_size 缓冲区大小
 * @return 1安全，0不安全
 */
int uvhttp_static_resolve_safe_path(const char* root_dir, const char* file_path,
                                    char* resolved_path, size_t buffer_size);

/**
 * 生成ETag值
 *
 * @param file_path 文件路径
 * @param last_modified 最后修改时间
 * @param file_size 文件大小
 * @param etag 输出ETag缓冲区
 * @param buffer_size 缓冲区大小
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_generate_etag(const char* file_path,
                                            time_t last_modified,
                                            size_t file_size, char* etag,
                                            size_t buffer_size);

/**
 * 检查条件请求（If-None-Match, If-Modified-Since）
 *
 * @param request HTTP请求
 * @param etag 文件ETag
 * @param last_modified 最后修改时间
 * @return 1需要返回304，0需要返回完整内容
 */
int uvhttp_static_check_conditional_request(void* request, const char* etag,
                                            time_t last_modified);

/**
 * 设置静态文件相关的响应头
 *
 * @param response HTTP响应
 * @param file_path 文件路径
 * @param file_size 文件大小
 * @param last_modified 最后修改时间
 * @param etag ETag值
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_set_response_headers(void* response,
                                                   const char* file_path,
                                                   size_t file_size,
                                                   time_t last_modified,
                                                   const char* etag);

/**
 * 获取缓存统计信息
 *
 * @param ctx 静态文件服务上下文
 * @param total_memory_usage 输出总内存使用量
 * @param entry_count 输出条目数量
 * @param hit_count 输出命中次数
 * @param miss_count 输出未命中次数
 * @param eviction_count 输出驱逐次数
 */
void uvhttp_static_get_cache_stats(uvhttp_static_context_t* ctx,
                                   size_t* total_memory_usage, int* entry_count,
                                   int* hit_count, int* miss_count,
                                   int* eviction_count);

/**
 * 获取缓存命中率
 *
 * @param ctx 静态文件服务上下文
 * @return 命中率（0.0-1.0）
 */
double uvhttp_static_get_cache_hit_rate(uvhttp_static_context_t* ctx);

/**
 * 清理过期缓存条目
 *
 * @param ctx 静态文件服务上下文
 * @return 清理的条目数量
 */
int uvhttp_static_cleanup_expired_cache(uvhttp_static_context_t* ctx);

/**
 * 启用缓存（如果之前被禁用）
 *
 * @param ctx 静态文件服务上下文
 * @param max_memory 最大内存使用量
 * @param max_entries 最大条目数
 * @param ttl 缓存TTL（秒）
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_result_t uvhttp_static_enable_cache(uvhttp_static_context_t* ctx,
                                           size_t max_memory, int max_entries,
                                           int ttl);

/**
 * 禁用缓存
 *
 * @param ctx 静态文件服务上下文
 */
void uvhttp_static_disable_cache(uvhttp_static_context_t* ctx);

/* ========== 静态文件中间件接口 ========== */

#    ifdef __cplusplus
}
#    endif

#endif /* UVHTTP_FEATURE_STATIC_FILES */

#endif /* UVHTTP_STATIC_H */