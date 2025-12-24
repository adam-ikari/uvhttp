/* UVHTTP 静态文件服务模块实现 */

#if UVHTTP_FEATURE_STATIC_FILES

#define _XOPEN_SOURCE 600 /* 启用 strptime, timegm */
#define _DEFAULT_SOURCE /* 启用 strcasecmp */

#include "uvhttp_static.h"
#include "uvhttp_async_file.h"
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
#include <time.h>
#include <uv.h>

#include "uvhttp_error_handler.h"

/* 前向声明 */
static void on_async_file_read_complete(uvhttp_async_file_request_t* req, int status);

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
    uvhttp_static_cache_entry_t* prev = NULL;
    while (entry) {
        if (strcmp(entry->file_path, file_path) == 0) {
            /* 检查缓存是否过期 */
            if (ctx->config.cache_ttl > 0) {
                time_t now = time(NULL);
                if (now - entry->cache_time > ctx->config.cache_ttl) {
                    /* 缓存过期，移除并返回NULL */
                    UVHTTP_LOG_DEBUG("Cache entry expired: %s", entry->file_path);
                    
                    /* 从链表中移除过期条目 */
                    if (prev) {
                        prev->next = entry->next;
                    } else {
                        ctx->cache_head = entry->next;
                    }
                    
                    /* 更新缓存大小 */
                    ctx->current_cache_size -= entry->content_length;
                    
                    /* 释放过期条目 */
                    free_cache_entry(entry);
                    UVHTTP_LOG_DEBUG("Expired cache entry removed");
                    
                    return NULL;
                }
            }
            return entry;
        }
        prev = entry;
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
        /* 实现LRU缓存清理 - 移除最旧的条目直到有足够空间 */
        UVHTTP_LOG_DEBUG("Cache size limit reached, starting LRU cleanup");
        
        while (ctx->cache_head && 
               ctx->current_cache_size + content_length > ctx->config.max_cache_size) {
            uvhttp_static_cache_entry_t* oldest = ctx->cache_head;
            
            /* 移除最旧的条目（链表头部） */
            ctx->cache_head = oldest->next;
            ctx->current_cache_size -= oldest->content_length;
            
            UVHTTP_LOG_DEBUG("LRU cleanup: removed entry %s (freed %zu bytes)", 
                           oldest->file_path, oldest->content_length);
            
            free_cache_entry(oldest);
        }
        
        UVHTTP_LOG_DEBUG("LRU cleanup completed, current cache size: %zu bytes", 
                       ctx->current_cache_size);
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
    
    /* 初始化统计信息 */
    memset(&ctx->stats, 0, sizeof(uvhttp_static_stats_t));
    
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
    
    /* 初始化异步读取设置 */
    ctx->enable_async_read = 1;  /* 默认启用异步读取 */
    ctx->async_file_threshold = 64 * 1024;  /* 默认64KB阈值 */
    ctx->async_manager = NULL;
    
    return ctx;
}

void uvhttp_static_free(uvhttp_static_context_t* ctx) {
    if (!ctx) return;
    
    /* 清理缓存 */
    uvhttp_static_clear_cache(ctx);
    
    /* 清理异步文件管理器 */
    uvhttp_static_cleanup_async(ctx);
    
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
        /* 解析HTTP日期格式 (RFC 1123): Wed, 21 Oct 2015 07:28:00 GMT */
        struct tm tm_mod_since = {0};
        
        /* 使用更安全的日期解析方式 */
        char* result = strptime(if_modified_since, "%a, %d %b %Y %H:%M:%S GMT", &tm_mod_since);
        if (result) {
            time_t mod_since_time = timegm(&tm_mod_since);
            if (last_modified <= mod_since_time) {
                /* 文件未修改，返回304 Not Modified */
                return 1; /* 调用者需要设置304状态 */
            }
        } else {
            UVHTTP_LOG_WARN("Failed to parse If-Modified-Since header: %s", if_modified_since);
        }
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
        /* HTTP日期格式化 (RFC 1123): Wed, 21 Oct 2015 07:28:00 GMT */
        struct tm* gm_time = gmtime(&last_modified);
        if (gm_time) {
            strftime(time_str, sizeof(time_str), "%a, %d %b %Y %H:%M:%S GMT", gm_time);
            uvhttp_response_set_header(response, "Last-Modified", time_str);
        } else {
            UVHTTP_LOG_ERROR("Failed to format last modified time");
        }
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
    if (!dir_path || !request_path) {
        return NULL;
    }
    
    DIR* dir = opendir(dir_path);
    if (!dir) {
        UVHTTP_LOG_ERROR("Failed to open directory: %s", dir_path);
        return NULL;
    }
    
    /* 计算所需缓冲区大小 */
    size_t buffer_size = 4096; /* 基础HTML大小 */
    size_t entry_count = 0;
    struct dirent* entry;
    
    /* 第一次遍历：计算条目数量和所需缓冲区大小 */
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue; /* 跳过当前目录 */
        }
        
        buffer_size += strlen(entry->d_name) + 200; /* 每个条目的HTML开销 */
        entry_count++;
    }
    
    /* 分配缓冲区 */
    char* html = uvhttp_malloc(buffer_size);
    if (!html) {
        closedir(dir);
        return NULL;
    }
    
    /* 开始生成HTML */
    size_t offset = 0;
    offset += snprintf(html + offset, buffer_size - offset,
        "<!DOCTYPE html>\n"
        "<html>\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<title>Directory listing for %s</title>\n"
        "<style>\n"
        "body { font-family: Arial, sans-serif; margin: 20px; }\n"
        "h1 { color: #333; }\n"
        "table { border-collapse: collapse; width: 100%%; }\n"
        "th, td { text-align: left; padding: 8px; border-bottom: 1px solid #ddd; }\n"
        "th { background-color: #f2f2f2; }\n"
        "a { text-decoration: none; color: #0066cc; }\n"
        "a:hover { text-decoration: underline; }\n"
        ".dir { font-weight: bold; }\n"
        ".size { text-align: right; color: #666; }\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>Directory listing for %s</h1>\n"
        "<table>\n"
        "<tr><th>Name</th><th>Size</th><th>Modified</th></tr>\n",
        request_path, request_path);
    
    /* 添加父目录链接 */
    if (strcmp(request_path, "/") != 0) {
        offset += snprintf(html + offset, buffer_size - offset,
            "<tr><td><a href=\"../\">../</a></td><td class=\"dir\">-</td><td>-</td></tr>\n");
    }
    
    /* 重置目录位置 */
    rewinddir(dir);
    
    /* 收集目录条目信息 */
    typedef struct {
        char name[256];
        size_t size;
        time_t mtime;
        int is_dir;
    } dir_entry_t;
    
    dir_entry_t* entries = uvhttp_malloc(entry_count * sizeof(dir_entry_t));
    if (!entries) {
        uvhttp_free(html);
        closedir(dir);
        return NULL;
    }
    
    size_t actual_count = 0;
    
    /* 第二次遍历：收集条目信息 */
    while ((entry = readdir(dir)) != NULL && actual_count < entry_count) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        
        dir_entry_t* dir_entry = &entries[actual_count];
        strncpy(dir_entry->name, entry->d_name, sizeof(dir_entry->name) - 1);
        dir_entry->name[sizeof(dir_entry->name) - 1] = '\0';
        
        /* 获取文件信息 */
        char full_path[UVHTTP_MAX_FILE_PATH_SIZE];
        snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        
        struct stat st;
        if (stat(full_path, &st) == 0) {
            dir_entry->size = st.st_size;
            dir_entry->mtime = st.st_mtime;
            dir_entry->is_dir = S_ISDIR(st.st_mode);
        } else {
            dir_entry->size = 0;
            dir_entry->mtime = 0;
            dir_entry->is_dir = 0;
        }
        
        actual_count++;
    }
    closedir(dir);
    
    /* 排序：目录在前，然后按名称排序 */
    for (size_t i = 0; i < actual_count - 1; i++) {
        for (size_t j = i + 1; j < actual_count; j++) {
            /* 目录优先 */
            if (!entries[i].is_dir && entries[j].is_dir) {
                dir_entry_t temp = entries[i];
                entries[i] = entries[j];
                entries[j] = temp;
            }
            /* 同类型按名称排序 */
            else if (entries[i].is_dir == entries[j].is_dir) {
                if (strcmp(entries[i].name, entries[j].name) > 0) {
                    dir_entry_t temp = entries[i];
                    entries[i] = entries[j];
                    entries[j] = temp;
                }
            }
        }
    }
    
    /* 生成HTML表格行 */
    for (size_t i = 0; i < actual_count; i++) {
        dir_entry_t* dir_entry = &entries[i];
        
        /* 格式化修改时间 */
        char time_str[64];
        if (dir_entry->mtime > 0) {
            struct tm* tm_info = localtime(&dir_entry->mtime);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            strcpy(time_str, "-");
        }
        
        /* 生成表格行 */
        if (dir_entry->is_dir) {
            offset += snprintf(html + offset, buffer_size - offset,
                "<tr><td><a href=\"%s/\" class=\"dir\">%s/</a></td><td class=\"dir\">-</td><td>%s</td></tr>\n",
                dir_entry->name, dir_entry->name, time_str);
        } else {
            offset += snprintf(html + offset, buffer_size - offset,
                "<tr><td><a href=\"%s\">%s</a></td><td class=\"size\">%zu</td><td>%s</td></tr>\n",
                dir_entry->name, dir_entry->name, dir_entry->size, time_str);
        }
    }
    
    /* 完成HTML */
    offset += snprintf(html + offset, buffer_size - offset,
        "</table>\n"
        "<p style=\"margin-top: 20px; color: #666; font-size: small;\">"
        "%zu entries total"
        "</p>\n"
        "</body>\n"
        "</html>",
        actual_count);
    
    /* 清理 */
    uvhttp_free(entries);
    
    UVHTTP_LOG_DEBUG("Generated directory listing for %s (%zu entries)", request_path, actual_count);
    
    return html;
}

/**
 * 处理静态文件请求的主要函数
 */
int uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                void* request,
                                void* response) {
    if (!ctx || !request || !response) return -1;
    
    /* 更新请求统计 */
    ctx->stats.total_requests++;
    
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
    
    if (cache_entry) {
        /* 缓存命中 */
        ctx->stats.cache_hits++;
    } else {
        /* 缓存未命中 */
        ctx->stats.cache_misses++;
    }
    
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
        
        /* 生成ETag */
        char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_generate_etag(safe_path, last_modified, file_size, 
                                   etag, sizeof(etag));
        
        /* 检查条件请求 */
        if (uvhttp_static_check_conditional_request(request, etag, last_modified)) {
            uvhttp_response_set_status(response, 304); /* Not Modified */
            return 0;
        }
        
        /* 智能选择读取策略 */
        int use_async = 0;
        
        if (ctx->enable_async_read && ctx->async_manager) {
            /* 检查文件大小是否超过异步阈值 */
            if (file_size >= ctx->async_file_threshold) {
                /* 获取异步管理器统计信息 */
                int current_reads, max_concurrent;
                if (uvhttp_async_file_get_stats(ctx->async_manager, &current_reads, &max_concurrent) == 0) {
                    /* 如果当前并发数低于最大值的80%，使用异步读取 */
                    if (current_reads < (max_concurrent * 8 / 10)) {
                        use_async = 1;
                    }
                }
            }
            
            /* 对于大文件（>1MB），强制使用异步或流式传输 */
            if (file_size >= 1024 * 1024) {
                use_async = 1;
            }
        }
        
        if (use_async) {
            /* 检查是否需要流式传输（超大文件） */
            if (file_size >= UVHTTP_STATIC_MAX_FILE_SIZE / 2) {
                /* 使用流式传输 */
                ctx->stats.stream_transfers++;
                
                int ret = uvhttp_async_file_stream(ctx->async_manager, safe_path, 
                                                  response, 64 * 1024); /* 64KB chunks */
                if (ret == 0) {
                    /* 设置流式传输响应头 */
                    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
                    uvhttp_static_generate_etag(safe_path, last_modified, 
                                               file_size, etag, sizeof(etag));
                    
                    uvhttp_static_set_response_headers(response, safe_path, 
                                                      file_size, last_modified, etag);
                    uvhttp_response_set_header(response, "Transfer-Encoding", "chunked");
                    uvhttp_response_set_status(response, 200);
                    uvhttp_response_send(response);
                    
                    /* 更新统计信息 */
                    ctx->stats.total_bytes_served += file_size;
                    
                    return 0;
                } else {
                    uvhttp_log_safe_error(ret, "static_stream_fallback", safe_path);
                }
            } else {
                /* 异步读取文件 */
                ctx->stats.async_reads++;
                
                int ret = uvhttp_async_file_read(ctx->async_manager, safe_path, 
                                               request, response, ctx, 
                                               on_async_file_read_complete);
                if (ret == 0) {
                    return 0;  /* 异步处理中 */
                } else {
                    /* 异步读取失败，回退到同步读取 */
                    uvhttp_log_safe_error(ret, "static_async_fallback", safe_path);
                }
            }
        }
        
        /* 同步读取文件（回退方案） */
        ctx->stats.sync_reads++;
        
        char* file_content = read_file_content(safe_path, &file_size);
        if (!file_content) {
            uvhttp_response_set_status(response, 500); /* Internal Server Error */
            return -1;
        }
        
        /* 获取MIME类型 */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(safe_path, mime_type, sizeof(mime_type));
        
        /* 添加到缓存 */
        add_to_cache(ctx, safe_path, file_content, file_size, 
                    mime_type, last_modified, etag);
        
        /* 发送响应 */
        uvhttp_static_set_response_headers(response, safe_path, file_size, 
                                          last_modified, etag);
        uvhttp_response_set_body(response, file_content, file_size);
        uvhttp_response_set_status(response, 200);
        
        /* 更新统计信息 */
        ctx->stats.total_bytes_served += file_size;
        
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
            
            /* 更新统计信息 */
            ctx->stats.total_bytes_served += cache_entry->content_length;
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

/**
 * 异步文件读取完成回调
 */
static void on_async_file_read_complete(uvhttp_async_file_request_t* req, int status) {
    if (!req) return;
    
    void* response = req->response;
    uvhttp_static_context_t* ctx = (uvhttp_static_context_t*)req->static_context;
    
    /* 计算读取时间（简化实现，实际应该使用高精度计时器） */
    double read_time = 0.0; /* 这里应该从请求开始时间计算 */
    
    if (status == 0 && req->state == UVHTTP_ASYNC_FILE_STATE_COMPLETED && req->buffer) {
        /* 读取成功 */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(req->file_path, mime_type, sizeof(mime_type));
        
        char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_generate_etag(req->file_path, req->last_modified, 
                                   req->file_size, etag, sizeof(etag));
        
        /* 添加到缓存 */
        if (ctx) {
            add_to_cache(ctx, req->file_path, req->buffer, req->file_size, 
                        mime_type, req->last_modified, etag);
        }
        
        /* 设置响应头 */
        uvhttp_static_set_response_headers(response, req->file_path, 
                                          req->file_size, req->last_modified, etag);
        
        /* 发送响应 */
        uvhttp_response_set_body(response, req->buffer, req->file_size);
        uvhttp_response_set_status(response, 200);
        
        /* 更新统计信息 */
        if (ctx) {
            ctx->stats.total_bytes_served += req->file_size;
            uvhttp_static_update_read_time_stats(ctx, read_time);
        }
    } else {
        /* 读取失败，回退到同步读取 */
        if (ctx) {
            size_t file_size;
            time_t last_modified;
            
            if (get_file_info(req->file_path, &file_size, &last_modified) == 0) {
                char* file_content = read_file_content(req->file_path, &file_size);
                if (file_content) {
                    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
                    uvhttp_static_get_mime_type(req->file_path, mime_type, sizeof(mime_type));
                    
                    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
                    uvhttp_static_generate_etag(req->file_path, last_modified, 
                                               file_size, etag, sizeof(etag));
                    
                    /* 添加到缓存 */
                    add_to_cache(ctx, req->file_path, file_content, file_size, 
                                mime_type, last_modified, etag);
                    
                    /* 设置响应头 */
                    uvhttp_static_set_response_headers(response, req->file_path, 
                                                      file_size, last_modified, etag);
                    
                    /* 发送响应 */
                    uvhttp_response_set_body(response, file_content, file_size);
                    uvhttp_response_set_status(response, 200);
                    
                    uvhttp_free(file_content);
                } else {
                    uvhttp_response_set_status(response, 500);
                }
            } else {
                uvhttp_response_set_status(response, 404);
            }
        } else {
            uvhttp_response_set_status(response, 500);
        }
    }
    
    /* 发送响应 */
    uvhttp_response_send(response);
}

/**
 * 初始化异步文件读取功能
 */
int uvhttp_static_init_async(uvhttp_static_context_t* ctx,
                            uv_loop_t* loop,
                            int max_concurrent,
                            size_t buffer_size,
                            size_t file_threshold) {
    if (!ctx || !loop) {
        return -1;
    }
    
    /* 创建异步文件管理器 */
    ctx->async_manager = uvhttp_async_file_manager_create(loop, max_concurrent, 
                                                         buffer_size, 
                                                         UVHTTP_STATIC_MAX_FILE_SIZE);
    if (!ctx->async_manager) {
        uvhttp_log_safe_error(0, "static_async_init", "Failed to create async manager");
        return -1;
    }
    
    ctx->enable_async_read = 1;
    ctx->async_file_threshold = file_threshold;
    
    return 0;
}

/**
 * 清理异步文件读取功能
 */
void uvhttp_static_cleanup_async(uvhttp_static_context_t* ctx) {
    if (!ctx) return;
    
    if (ctx->async_manager) {
        uvhttp_async_file_manager_free(ctx->async_manager);
        ctx->async_manager = NULL;
    }
    
    ctx->enable_async_read = 0;
}

/**
 * 获取静态文件服务统计信息
 */
int uvhttp_static_get_stats(uvhttp_static_context_t* ctx, uvhttp_static_stats_t* stats) {
    if (!ctx || !stats) return -1;
    
    memcpy(stats, &ctx->stats, sizeof(uvhttp_static_stats_t));
    return 0;
}

/**
 * 重置静态文件服务统计信息
 */
int uvhttp_static_reset_stats(uvhttp_static_context_t* ctx) {
    if (!ctx) return -1;
    
    memset(&ctx->stats, 0, sizeof(uvhttp_static_stats_t));
    return 0;
}

/**
 * 更新读取时间统计
 */
int uvhttp_static_update_read_time_stats(uvhttp_static_context_t* ctx, double read_time) {
    if (!ctx) return -1;
    
    /* 更新平均读取时间 */
    if (ctx->stats.sync_reads + ctx->stats.async_reads > 0) {
        size_t total_reads = ctx->stats.sync_reads + ctx->stats.async_reads;
        ctx->stats.avg_read_time = (ctx->stats.avg_read_time * (total_reads - 1) + read_time) / total_reads;
    } else {
        ctx->stats.avg_read_time = read_time;
    }
    
    /* 更新最大读取时间 */
    if (read_time > ctx->stats.max_read_time) {
        ctx->stats.max_read_time = read_time;
    }
    
    return 0;
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */