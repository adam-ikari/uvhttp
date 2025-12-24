/* UVHTTP 静态文件服务模块 - V2版本，集成LRU缓存 */

#ifndef UVHTTP_STATIC_V2_H
#define UVHTTP_STATIC_V2_H

#include <stddef.h>
#include <time.h>
#include "uvhttp_constants.h"

/* 前向声明 */
typedef struct cache_manager cache_manager_t;
typedef struct cache_entry cache_entry_t;

#ifdef __cplusplus
extern "C" {
#endif

/* 静态文件配置结构 */
typedef struct uvhttp_static_config {
    char root_directory[UVHTTP_MAX_FILE_PATH_SIZE];  /* 根目录路径 */
    char index_file[UVHTTP_MAX_PATH_SIZE];           /* 默认索引文件 */
    int enable_directory_listing;                    /* 是否启用目录列表 */
    int enable_etag;                                 /* 是否启用ETag */
    int enable_last_modified;                        /* 是否启用Last-Modified */
    size_t max_cache_size;                           /* 最大缓存大小（字节） */
    int cache_ttl;                                   /* 缓存TTL（秒） */
    int max_cache_entries;                           /* 最大缓存条目数 */
    char custom_headers[UVHTTP_MAX_HEADER_VALUE_SIZE]; /* 自定义响应头 */
} uvhttp_static_config_t;

/* 静态文件服务上下文 */
typedef struct uvhttp_static_context {
    uvhttp_static_config_t config;                   /* 配置 */
    cache_manager_t* cache;                          /* LRU缓存管理器 */
} uvhttp_static_context_t;

/* MIME类型映射条目 */
typedef struct uvhttp_mime_mapping {
    const char* extension;                           /* 文件扩展名 */
    const char* mime_type;                           /* MIME类型 */
} uvhttp_mime_mapping_t;

/**
 * 创建静态文件服务上下文
 * 
 * @param config 静态文件配置
 * @return 静态文件服务上下文指针，失败返回NULL
 */
uvhttp_static_context_t* uvhttp_static_create(const uvhttp_static_config_t* config);

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
 * @return 0成功，-1失败
 */
int uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                void* request,
                                void* response);

/**
 * 根据文件扩展名获取MIME类型
 * 
 * @param file_path 文件路径
 * @param mime_type 输出MIME类型缓冲区
 * @param buffer_size 缓冲区大小
 * @return 0成功，-1失败
 */
int uvhttp_static_get_mime_type(const char* file_path,
                               char* mime_type,
                               size_t buffer_size);

/**
 * 清理文件缓存
 * 
 * @param ctx 静态文件服务上下文
 */
void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx);

/**
 * 检查文件路径是否安全（防止路径遍历攻击）
 * 
 * @param root_dir 根目录
 * @param file_path 请求的文件路径
 * @param resolved_path 解析后的安全路径
 * @param buffer_size 缓冲区大小
 * @return 1安全，0不安全
 */
int uvhttp_static_resolve_safe_path(const char* root_dir,
                                   const char* file_path,
                                   char* resolved_path,
                                   size_t buffer_size);

/**
 * 生成ETag值
 * 
 * @param file_path 文件路径
 * @param last_modified 最后修改时间
 * @param file_size 文件大小
 * @param etag 输出ETag缓冲区
 * @param buffer_size 缓冲区大小
 * @return 0成功，-1失败
 */
int uvhttp_static_generate_etag(const char* file_path,
                               time_t last_modified,
                               size_t file_size,
                               char* etag,
                               size_t buffer_size);

/**
 * 检查条件请求（If-None-Match, If-Modified-Since）
 * 
 * @param request HTTP请求
 * @param etag 文件ETag
 * @param last_modified 最后修改时间
 * @return 1需要返回304，0需要返回完整内容
 */
int uvhttp_static_check_conditional_request(void* request,
                                           const char* etag,
                                           time_t last_modified);

/**
 * 设置静态文件相关的响应头
 * 
 * @param response HTTP响应
 * @param file_path 文件路径
 * @param file_size 文件大小
 * @param last_modified 最后修改时间
 * @param etag ETag值
 * @return 0成功，-1失败
 */
int uvhttp_static_set_response_headers(void* response,
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
                                  size_t* total_memory_usage,
                                  int* entry_count,
                                  int* hit_count,
                                  int* miss_count,
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

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_STATIC_V2_H */