/* UVHTTP 静态文件服务模块实现 */

#if UVHTTP_FEATURE_STATIC_FILES

#include "uvhttp_static.h"
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
    {".ogg", "video/ogg"},
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
 * 清理缓存条目
 */
static void free_cache_entry(uvhttp_static_cache_entry_t* entry) {
    if (!entry) return;
    
    if (entry->content) {
        uvhttp_free(entry->content);
    }
    uvhttp_free(entry);
}

/**
 * 从缓存中查找文件
 */
static uvhttp_static_cache_entry_t* find_in_cache(uvhttp_static_context_t* ctx,
                                                 const char* file_path) {
    if (!ctx || !file_path) return NULL;
    
    uvhttp_static_cache_entry_t* entry = ctx->cache_head;
    while (entry) {
        if (strcmp(entry->file_path, file_path) == 0) {
            /* 检查缓存是否过期 */
            if (ctx->config.cache_ttl > 0) {
                time_t now = time(NULL);
                if (now - entry->cache_time > ctx->config.cache_ttl) {
                    /* 缓存过期，移除并返回NULL */
                    /* TODO: 实现LRU缓存清理 */
                    return NULL;
                }
            }
            return entry;
        }
        entry = entry->next;
    }
    return NULL;
}

/**
 * 添加文件到缓存
 */
static int add_to_cache(uvhttp_static_context_t* ctx,
                        const char* file_path,
                        const char* content,
                        size_t content_length,
                        const char* mime_type,
                        time_t last_modified,
                        const char* etag) {
    if (!ctx || !file_path || !content) return -1;
    
    /* 检查缓存大小限制 */
    if (ctx->config.max_cache_size > 0 &&
        ctx->current_cache_size + content_length > ctx->config.max_cache_size) {
        /* TODO: 实现LRU缓存清理 */
        uvhttp_log_safe_error(0, "static_cache", "Cache size limit reached");
        return -1;
    }
    
    uvhttp_static_cache_entry_t* entry = uvhttp_malloc(sizeof(uvhttp_static_cache_entry_t));
    if (!entry) {
        uvhttp_handle_memory_failure("cache_entry", NULL, NULL);
        return -1;
    }
    
    memset(entry, 0, sizeof(uvhttp_static_cache_entry_t));
    
    /* 复制文件路径 */
    strncpy(entry->file_path, file_path, sizeof(entry->file_path) - 1);
    entry->file_path[sizeof(entry->file_path) - 1] = '\0';
    
    /* 复制文件内容 */
    entry->content = uvhttp_malloc(content_length);
    if (!entry->content) {
        uvhttp_handle_memory_failure("cache_content", NULL, NULL);
        uvhttp_free(entry);
        return -1;
    }
    memcpy(entry->content, content, content_length);
    entry->content_length = content_length;
    
    /* 复制MIME类型 */
    if (mime_type) {
        strncpy(entry->mime_type, mime_type, sizeof(entry->mime_type) - 1);
        entry->mime_type[sizeof(entry->mime_type) - 1] = '\0';
    }
    
    /* 设置其他属性 */
    entry->last_modified = last_modified;
    entry->cache_time = time(NULL);
    if (etag) {
        strncpy(entry->etag, etag, sizeof(entry->etag) - 1);
        entry->etag[sizeof(entry->etag) - 1] = '\0';
    }
    
    /* 添加到链表头部 */
    entry->next = ctx->cache_head;
    ctx->cache_head = entry;
    
    /* 更新缓存统计 */
    ctx->current_cache_size += content_length;
    ctx->cache_count++;
    
    return 0;
}

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
    
    /* 验证根目录 */
    if (!uvhttp_validate_file_path(config->root_directory)) {
        uvhttp_log_safe_error(0, "static_create", "Invalid root directory");
        uvhttp_free(ctx);
        return NULL;
    }
    
    /* 设置默认值 */
    if (ctx->config.index_file[0] == '\0') {
        strcpy(ctx->config.index_file, "index.html");
    }
    
    return ctx;
}

void uvhttp_static_free(uvhttp_static_context_t* ctx) {
    if (!ctx) return;
    
    /* 清理缓存 */
    uvhttp_static_clear_cache(ctx);
    
    uvhttp_free(ctx);
}

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

int uvhttp_static_generate_etag(const char* file_path,
                               time_t last_modified,
                               size_t file_size,
                               char* etag,
                               size_t buffer_size) {
    if (!file_path || !etag || buffer_size == 0) return -1;
    
    /* 简单的ETag生成：文件大小-修改时间 */
    snprintf(etag, buffer_size, "\"%zu-%ld\"", file_size, last_modified);
    
    return 0;
}

int uvhttp_static_check_conditional_request(void* request,
                                           const char* etag,
                                           time_t last_modified) {
    if (!request || !etag) return 0;
    
    (void)last_modified; /* 避免未使用参数警告 */
    
    /* 检查If-None-Match */
    const char* if_none_match = uvhttp_request_get_header(request, "If-None-Match");
    if (if_none_match && strcmp(if_none_match, etag) == 0) {
        return 1; /* 返回304 Not Modified */
    }
    
    /* 检查If-Modified-Since */
    const char* if_modified_since = uvhttp_request_get_header(request, "If-Modified-Since");
    if (if_modified_since) {
        /* 简单的时间比较（实际实现需要解析HTTP日期格式） */
        /* TODO: 实现HTTP日期解析 */
    }
    
    return 0; /* 需要返回完整内容 */
}

int uvhttp_static_set_response_headers(void* response,
                                      const char* file_path,
                                      size_t file_size,
                                      time_t last_modified,
                                      const char* etag) {
    if (!response || !file_path) return -1;
    
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
        /* TODO: 实现HTTP日期格式化 */
        strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gmtime(&last_modified));
        uvhttp_response_set_header(response, "Last-Modified", time_str);
    }
    
    /* 设置ETag */
    if (etag && etag[0] != '\0') {
        uvhttp_response_set_header(response, "ETag", etag);
    }
    
    /* 设置缓存控制 */
    uvhttp_response_set_header(response, "Cache-Control", "public, max-age=3600");
    
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
    
    /* 回到文件开头 */
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
    uvhttp_static_cache_entry_t* cache_entry = find_in_cache(ctx, safe_path);
    
    /* 如果不在缓存中，读取文件 */
    if (!cache_entry) {
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
        add_to_cache(ctx, safe_path, file_content, file_size, 
                    mime_type, last_modified, etag);
        
        /* 发送响应 */
        uvhttp_static_set_response_headers(response, safe_path, file_size, 
                                          last_modified, etag);
        uvhttp_response_set_body(response, file_content, file_size);
        uvhttp_response_set_status(response, 200);
        
        uvhttp_free(file_content);
    } else {
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
    }
    
    return 0;
}

void uvhttp_static_clear_cache(uvhttp_static_context_t* ctx) {
    if (!ctx) return;
    
    uvhttp_static_cache_entry_t* entry = ctx->cache_head;
    while (entry) {
        uvhttp_static_cache_entry_t* next = entry->next;
        free_cache_entry(entry);
        entry = next;
    }
    
    ctx->cache_head = NULL;
    ctx->current_cache_size = 0;
    ctx->cache_count = 0;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */