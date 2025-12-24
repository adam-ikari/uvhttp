/* UVHTTP 静态文件服务模块实现 - V2版本，集成LRU缓存 */

#include "uvhttp_static.h"
#include "uvhttp_lru_cache.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_validation.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

/* MIME类型映射表 */
static const uvhttp_mime_mapping_t mime_types[] = {
    /* 文本类型 */
    {".html", "text/html"},
    {".htm", "text/html"},
    {".css", "text/css"},
    {".js", "application/javascript"},
    {".json", "application/json"},
    {".xml", "application/xml"},
    {".txt", "text/plain"},
    {".md", "text/markdown"},
    {".csv", "text/csv"},
    
    /* 图片类型 */
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"},
    {".webp", "image/webp"},
    {".bmp", "image/bmp"},
    
    /* 音频类型 */
    {".mp3", "audio/mpeg"},
    {".wav", "audio/wav"},
    {".ogg", "audio/ogg"},
    {".aac", "audio/aac"},
    
    /* 视频类型 */
    {".mp4", "video/mp4"},
    {".webm", "video/webm"},
    {".avi", "video/x-msvideo"},
    
    /* 字体类型 */
    {".woff", "font/woff"},
    {".woff2", "font/woff2"},
    {".ttf", "font/ttf"},
    {".eot", "application/vnd.ms-fontobject"},
    
    /* 应用类型 */
    {".pdf", "application/pdf"},
    {".zip", "application/zip"},
    {".tar", "application/x-tar"},
    {".gz", "application/gzip"},
    
    /* 默认类型 */
    {".", "application/octet-stream"},
    {NULL, NULL}
};

/**
 * 获取文件扩展名
 */
static const char* get_file_extension(const char* file_path) {
    if (!file_path) return NULL;
    
    const char* last_dot = strrchr(file_path, '.');
    return last_dot ? last_dot : "";
}

/**
 * 根据文件扩展名获取MIME类型
 */
int uvhttp_static_get_mime_type(const char* file_path,
                               char* mime_type,
                               size_t buffer_size) {
    if (!file_path || !mime_type || buffer_size == 0) return -1;
    
    const char* extension = get_file_extension(file_path);
    
    /* 查找MIME类型 */
    for (int i = 0; mime_types[i].extension; i++) {
        if (strcasecmp(extension, mime_types[i].extension) == 0) {
            strncpy(mime_type, mime_types[i].mime_type, buffer_size - 1);
            mime_type[buffer_size - 1] = '\0';
            return 0;
        }
    }
    
    /* 默认MIME类型 */
    strncpy(mime_type, "application/octet-stream", buffer_size - 1);
    mime_type[buffer_size - 1] = '\0';
    
    return 0;
}

/**
 * 读取文件内容
 */
static char* read_file_content(const char* file_path, size_t* file_size) {
    if (!file_path || !file_size) return NULL;
    
    FILE* file = fopen(file_path, "rb");
    if (!file) {
        return NULL;
    }
    
    /* 获取文件大小 */
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    
    long size = ftell(file);
    if (size < 0) {
        fclose(file);
        return NULL;
    }
    
    *file_size = (size_t)size;
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    
    /* 分配内存并读取文件 */
    char* content = uvhttp_malloc(*file_size);
    if (!content) {
        fclose(file);
        uvhttp_handle_memory_failure("file_content", NULL, NULL);
        return NULL;
    }
    
    size_t bytes_read = fread(content, 1, *file_size, file);
    fclose(file);
    
    if (bytes_read != *file_size) {
        uvhttp_free(content);
        return NULL;
    }
    
    return content;
}

/**
 * 获取文件信息
 */
static int get_file_info(const char* file_path, size_t* file_size, time_t* last_modified) {
    if (!file_path) return -1;
    
    struct stat st;
    if (stat(file_path, &st) != 0) {
        return -1;
    }
    
    if (!S_ISREG(st.st_mode)) {
        return -1; /* 不是常规文件 */
    }
    
    if (file_size) *file_size = st.st_size;
    if (last_modified) *last_modified = st.st_mtime;
    
    return 0;
}

/**
 * 生成目录列表HTML
 */
static char* generate_directory_listing(const char* dir_path, const char* request_path) {
    (void)dir_path; /* 避免未使用参数警告 */
    
    /* TODO: 实现目录列表生成 */
    char* html = uvhttp_malloc(1024);
    if (!html) return NULL;
    
    snprintf(html, 1024,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head><title>Directory listing for %s</title></head>\n"
        "<body>\n"
        "<h1>Directory listing for %s</h1>\n"
        "<ul>\n"
        "<li><a href=\"../\">../</a></li>\n"
        "</ul>\n"
        "</body>\n"
        "</html>",
        request_path, request_path);
    
    return html;
}

/**
 * 生成ETag值
 */
int uvhttp_static_generate_etag(const char* file_path,
                               time_t last_modified,
                               size_t file_size,
                               char* etag,
                               size_t buffer_size) {
    if (!file_path || !etag || buffer_size == 0) return -1;
    
    /* 简单的ETag生成：文件大小-修改时间 */
    snprintf(etag, buffer_size, "\"%zu-%ld\"", file_size, (long)last_modified);
    
    return 0;
}

/**
 * 设置静态文件相关的响应头
 */
int uvhttp_static_set_response_headers(void* response,
                                      const char* file_path,
                                      size_t file_size,
                                      time_t last_modified,
                                      const char* etag) {
    if (!response) return -1;
    
    /* 设置Content-Type */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
    if (uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type)) == 0) {
        uvhttp_response_set_header(response, "Content-Type", mime_type);
    }
    
    /* 设置Content-Length */
    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%zu", file_size);
    uvhttp_response_set_header(response, "Content-Length", content_length);
    
    /* 设置Last-Modified */
    if (last_modified > 0) {
        char time_str[64];
        strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", 
                gmtime(&last_modified));
        uvhttp_response_set_header(response, "Last-Modified", time_str);
    }
    
    /* 设置ETag */
    if (etag && *etag) {
        uvhttp_response_set_header(response, "ETag", etag);
    }
    
    /* 设置Cache-Control */
    uvhttp_response_set_header(response, "Cache-Control", "public, max-age=3600");
    
    return 0;
}

/**
 * 检查条件请求（If-None-Match, If-Modified-Since）
 */
int uvhttp_static_check_conditional_request(void* request,
                                           const char* etag,
                                           time_t last_modified) {
    if (!request) return 0;
    
    /* 检查If-None-Match */
    const char* if_none_match = uvhttp_request_get_header(request, "If-None-Match");
    if (if_none_match && etag && strcmp(if_none_match, etag) == 0) {
        return 1; /* 返回304 */
    }
    
    /* 检查If-Modified-Since */
    const char* if_modified_since = uvhttp_request_get_header(request, "If-Modified-Since");
    if (if_modified_since && last_modified > 0) {
        struct tm tm = {0};
        if (strptime(if_modified_since, "%a, %d %b %Y %H:%M:%S GMT", &tm)) {
            time_t if_time = mktime(&tm);
            if (if_time >= last_modified) {
                return 1; /* 返回304 */
            }
        }
    }
    
    return 0; /* 需要返回完整内容 */
}

/**
 * 创建静态文件服务上下文
 */
uvhttp_static_context_t* uvhttp_static_create(const uvhttp_static_config_t* config) {
    if (!config) return NULL;
    
    uvhttp_static_context_t* ctx = uvhttp_malloc(sizeof(uvhttp_static_context_t));
    if (!ctx) {
        uvhttp_handle_memory_failure("static_context", NULL, NULL);
        return NULL;
    }
    
    memset(ctx, 0, sizeof(uvhttp_static_context_t));
    
    /* 复制配置 */
    memcpy(&ctx->config, config, sizeof(uvhttp_static_config_t));
    
    /* 创建LRU缓存 */
    ctx->cache = uvhttp_lru_cache_create(
        config->max_cache_size,    /* 最大内存使用量 */
        1000,                     /* 最大条目数 */
        config->cache_ttl         /* 缓存TTL */
    );
    
    if (!ctx->cache) {
        uvhttp_free(ctx);
        return NULL;
    }
    
    return ctx;
}

/**
 * 释放静态文件服务上下文
 */
void uvhttp_static_free(uvhttp_static_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->cache) {
        uvhttp_lru_cache_free(ctx->cache);
    }
    
    uvhttp_free(ctx);
}

/**
 * 检查文件路径是否安全（防止路径遍历攻击）
 */
int uvhttp_static_resolve_safe_path(const char* root_dir,
                                   const char* file_path,
                                   char* resolved_path,
                                   size_t buffer_size) {
    if (!root_dir || !file_path || !resolved_path || buffer_size == 0) {
        return 0;
    }
    
    /* 验证输入路径 */
    if (!uvhttp_validate_url_path(file_path)) {
        return 0;
    }
    
    /* 构建完整路径 */
    int root_len = strlen(root_dir);
    int path_len = strlen(file_path);
    
    /* 确保路径不会溢出缓冲区 */
    if (root_len + path_len + 2 >= (int)buffer_size) {
        return 0;
    }
    
    /* 使用安全的字符串操作复制根目录 */
    if (uvhttp_safe_strcpy(resolved_path, buffer_size, root_dir) != 0) {
        return 0;
    }
    
    /* 添加路径分隔符（如果需要） */
    if (root_len > 0 && root_dir[root_len - 1] != '/') {
        size_t current_len = strlen(resolved_path);
        if (current_len + 1 < buffer_size) {
            strcat(resolved_path, "/");
        } else {
            return 0;
        }
    }
    
    /* 处理文件路径 */
    const char* path_to_add = (file_path[0] == '/') ? file_path + 1 : file_path;
    size_t current_len = strlen(resolved_path);
    size_t add_len = strlen(path_to_add);
    
    if (current_len + add_len < buffer_size) {
        strcat(resolved_path, path_to_add);
    } else {
        return 0;
    }
    
    /* 检查路径遍历攻击 */
    if (strstr(resolved_path, "..") || strstr(resolved_path, "//")) {
        return 0;
    }
    
    /* 确保路径在根目录内 */
    if (strncmp(resolved_path, root_dir, strlen(root_dir)) != 0) {
        return 0;
    }
    
    return 1;
}

/**
 * 处理静态文件请求的主要函数
 */
int uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                void* request,
                                void* response) {
    if (!ctx || !request || !response) return -1;
    
    const char* url = uvhttp_request_get_url(request);
    if (!url) {
        uvhttp_response_set_status(response, 400);
        return -1;
    }
    
    /* 解析URL路径，移除查询参数 */
    char clean_path[UVHTTP_MAX_PATH_SIZE];
    const char* query_start = strchr(url, '?');
    size_t path_len = query_start ? (size_t)(query_start - url) : strlen(url);
    
    if (path_len >= sizeof(clean_path)) {
        uvhttp_response_set_status(response, 414); /* URI Too Long */
        return -1;
    }
    
    strncpy(clean_path, url, path_len);
    clean_path[path_len] = '\0';
    
    /* 处理根路径 */
    if (strcmp(clean_path, "/") == 0) {
        strncpy(clean_path, ctx->config.index_file, sizeof(clean_path) - 1);
        clean_path[sizeof(clean_path) - 1] = '\0';
    }
    
    /* 构建安全的文件路径 */
    char safe_path[UVHTTP_MAX_FILE_PATH_SIZE];
    if (!uvhttp_static_resolve_safe_path(ctx->config.root_directory, 
                                        clean_path, 
                                        safe_path, 
                                        sizeof(safe_path))) {
        uvhttp_response_set_status(response, 403); /* Forbidden */
        return -1;
    }
    
    /* 检查缓存 */
    cache_entry_t* cache_entry = uvhttp_lru_cache_find(ctx->cache, safe_path);
    
    if (cache_entry) {
        /* 从缓存发送响应 */
        if (uvhttp_static_check_conditional_request(request, cache_entry->etag, 
                                                   cache_entry->last_modified)) {
            uvhttp_response_set_status(response, 304); /* Not Modified */
        } else {
            uvhttp_static_set_response_headers(response, cache_entry->file_path, 
                                                  cache_entry->content_length,
                                                  cache_entry->last_modified,
                                                  cache_entry->etag);
            uvhttp_response_set_body(response, cache_entry->content,
                                      cache_entry->content_length);
            uvhttp_response_set_status(response, 200);
        }
        return 0;
    }
    
    /* 缓存未命中，读取文件 */
    size_t file_size;
    time_t last_modified;
    
    /* 获取文件信息 */
    if (get_file_info(safe_path, &file_size, &last_modified) != 0) {
        /* 检查是否为目录 */
        struct stat st;
        if (stat(safe_path, &st) == 0 && S_ISDIR(st.st_mode)) {
            if (ctx->config.enable_directory_listing) {
                /* 生成目录列表 */
                char* dir_html = generate_directory_listing(safe_path, clean_path);
                if (dir_html) {
                    uvhttp_response_set_status(response, 200);
                    uvhttp_response_set_header(response, "Content-Type", "text/html");
                    uvhttp_response_set_body(response, dir_html, strlen(dir_html));
                    uvhttp_free(dir_html);
                    return 0;
                }
            }
            /* 尝试添加索引文件 */
            char index_path[UVHTTP_MAX_FILE_PATH_SIZE];
            int result = snprintf(index_path, sizeof(index_path), "%s/%s", safe_path, ctx->config.index_file);
            if (result >= (int)sizeof(index_path)) {
                uvhttp_response_set_status(response, 414); /* URI Too Long */
                return -1;
            }
            
            if (get_file_info(index_path, &file_size, &last_modified) == 0) {
                strcpy(safe_path, index_path);
            } else {
                uvhttp_response_set_status(response, 404); /* Not Found */
                return -1;
            }
        } else {
            uvhttp_response_set_status(response, 404); /* Not Found */
            return -1;
        }
    }
    
    /* 检查文件大小限制 */
    if (file_size > UVHTTP_STATIC_MAX_FILE_SIZE) {
        uvhttp_response_set_status(response, 413); /* Payload Too Large */
        return -1;
    }
    
    /* 读取文件内容 */
    char* file_content = read_file_content(safe_path, &file_size);
    if (!file_content) {
        uvhttp_response_set_status(response, 500); /* Internal Server Error */
        return -1;
    }
    
    /* 获取MIME类型 */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_get_mime_type(safe_path, mime_type, sizeof(mime_type));
    
    /* 生成ETag */
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_generate_etag(safe_path, last_modified, file_size, 
                               etag, sizeof(etag));
    
    /* 检查条件请求 */
    if (uvhttp_static_check_conditional_request(request, etag, last_modified)) {
        uvhttp_free(file_content);
        uvhttp_response_set_status(response, 304); /* Not Modified */
        return 0;
    }
    
    /* 添加到缓存 */
    if (uvhttp_lru_cache_put(ctx->cache, safe_path, file_content, file_size, 
                            mime_type, last_modified, etag) != 0) {
        /* 缓存添加失败，但仍要返回内容 */
        uvhttp_log_safe_error(0, "static_cache", "Failed to cache file: %s", safe_path);
    }
    
    /* 发送响应 */
    uvhttp_static_set_response_headers(response, safe_path, file_size, 
                                          last_modified, etag);
    uvhttp_response_set_body(response, file_content, file_size);
    uvhttp_response_set_status(response, 200);
    
    /* 注意：文件内容的内存现在由缓存管理，不要在这里释放 */
    
    return 0;
}

/**
 * 清理文件缓存
 */
void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx) {
    if (!ctx || !ctx->cache) return;
    
    uvhttp_lru_cache_clear(ctx->cache);
}
