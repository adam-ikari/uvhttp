/* UVHTTP 静态文件服务模块实现 - V2版本，集成LRU缓存 */

#if UVHTTP_FEATURE_STATIC_FILES

#define _XOPEN_SOURCE 600 /* 启用 strptime, timegm */
#define _DEFAULT_SOURCE /* 启用 strcasecmp */

#include "uvhttp_static.h"
#include "uvhttp_middleware.h"
#include "uvhttp_lru_cache.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_validation.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_utils.h"
#include "uvhttp_constants.h"
#include "uvhttp_platform.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <errno.h>
#include <time.h>

#include "uvhttp_error_handler.h"

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

/* 前向声明 */
static uvhttp_result_t uvhttp_static_sendfile_with_config(const char* file_path, 
                                                          void* response,
                                                          const uvhttp_static_config_t* config);

/**
 * HTML转义函数 - 防止XSS攻击
 */
static void html_escape(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0) {
        return;
    }

    size_t i = 0;
    size_t j = 0;

    while (src[i] != '\0' && j < dest_size - 1) {
        switch (src[i]) {
            case '<':
                if (j + 4 < dest_size) {
                    dest[j++] = '&';
                    dest[j++] = 'l';
                    dest[j++] = 't';
                    dest[j++] = ';';
                }
                break;
            case '>':
                if (j + 4 < dest_size) {
                    dest[j++] = '&';
                    dest[j++] = 'g';
                    dest[j++] = 't';
                    dest[j++] = ';';
                }
                break;
            case '&':
                if (j + 5 < dest_size) {
                    dest[j++] = '&';
                    dest[j++] = 'a';
                    dest[j++] = 'm';
                    dest[j++] = 'p';
                    dest[j++] = ';';
                }
                break;
            case '"':
                if (j + 6 < dest_size) {
                    dest[j++] = '&';
                    dest[j++] = 'q';
                    dest[j++] = 'u';
                    dest[j++] = 'o';
                    dest[j++] = 't';
                    dest[j++] = ';';
                }
                break;
            case '\'':
                if (j + 6 < dest_size) {
                    dest[j++] = '&';
                    dest[j++] = '#';
                    dest[j++] = '3';
                    dest[j++] = '9';
                    dest[j++] = ';';
                }
                break;
            default:
                dest[j++] = src[i];
                break;
        }
        i++;
    }

    dest[j] = '\0';
}

/**
 * 根据文件扩展名获取MIME类型
 */
uvhttp_result_t uvhttp_static_get_mime_type(const char* file_path,
                               char* mime_type,
                               size_t buffer_size) {
    if (!file_path || !mime_type || buffer_size == 0) return UVHTTP_ERROR_INVALID_PARAM;

    const char* extension = get_file_extension(file_path);

    /* 查找MIME类型 */
    for (int i = 0; mime_types[i].extension; i++) {
        if (strcasecmp(extension, mime_types[i].extension) == 0) {
            if (uvhttp_safe_strncpy(mime_type, mime_types[i].mime_type, buffer_size) != 0) {
                UVHTTP_LOG_ERROR("Failed to copy MIME type: %s", mime_types[i].mime_type);
            }
            return UVHTTP_OK;
        }
    }

    /* 默认MIME类型 */
    if (uvhttp_safe_strncpy(mime_type, "application/octet-stream", buffer_size) != 0) {
        UVHTTP_LOG_ERROR("Failed to copy default MIME type");
    }
    
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
    char* content = uvhttp_alloc(*file_size);
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
 * 计算目录列表所需的缓冲区大小
 */
static size_t calculate_dir_listing_buffer_size(const char* dir_path, size_t* entry_count) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return 0;
    }
    
    size_t buffer_size = UVHTTP_DIR_LISTING_BUFFER_SIZE;
    *entry_count = 0;
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        
        buffer_size += strlen(entry->d_name) + UVHTTP_DIR_ENTRY_HTML_OVERHEAD;
        (*entry_count)++;
    }
    
    closedir(dir);
    return buffer_size;
}

/**
 * 目录条目结构
 */
typedef struct {
    char name[256];
    size_t size;
    time_t mtime;
    int is_dir;
} dir_entry_t;

/**
 * 收集目录条目信息
 */
static dir_entry_t* collect_dir_entries(const char* dir_path, size_t* actual_count) {
    DIR* dir = opendir(dir_path);
    if (!dir) {
        return NULL;
    }
    
    /* 先计算条目数量 */
    size_t entry_count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        entry_count++;
    }
    
    /* 分配内存 */
    dir_entry_t* entries = uvhttp_alloc(entry_count * sizeof(dir_entry_t));
    if (!entries) {
        closedir(dir);
        return NULL;
    }
    
    /* 收集条目信息 */
    rewinddir(dir);
    *actual_count = 0;
    
    while ((entry = readdir(dir)) != NULL && *actual_count < entry_count) {
        if (strcmp(entry->d_name, ".") == 0) {
            continue;
        }
        
        dir_entry_t* dir_entry = &entries[*actual_count];
        /* 使用安全的字符串拷贝，文件名太长时自动截断 */
        uvhttp_safe_strncpy(dir_entry->name, entry->d_name, sizeof(dir_entry->name));
        
        /* 获取文件信息 */
        char full_path[UVHTTP_MAX_FILE_PATH_SIZE];
        int written = snprintf(full_path, sizeof(full_path), "%s/%s", dir_path, entry->d_name);
        if (written < 0 || (size_t)written >= sizeof(full_path)) {
            continue;
        }
        
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
        
        (*actual_count)++;
    }
    
    closedir(dir);
    return entries;
}

/**
 * 排序目录条目
 */
static void sort_dir_entries(dir_entry_t* entries, size_t count) {
    for (size_t i = 0; i < count - 1; i++) {
        for (size_t j = i + 1; j < count; j++) {
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
}

/**
 * 生成目录列表HTML
 */
static char* generate_directory_listing(const char* dir_path, const char* request_path) {
    if (!dir_path || !request_path) {
        return NULL;
    }
    
    /* 计算缓冲区大小 */
    size_t entry_count = 0;
    size_t buffer_size = calculate_dir_listing_buffer_size(dir_path, &entry_count);
    if (buffer_size == 0) {
        return NULL;
    }
    
    /* 分配缓冲区 */
    char* html = uvhttp_alloc(buffer_size);
    if (!html) {
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
    
    /* 收集目录条目 */
    size_t actual_count = 0;
    dir_entry_t* entries = collect_dir_entries(dir_path, &actual_count);
    if (!entries) {
        uvhttp_free(html);
        return NULL;
    }
    
    /* 排序条目 */
    sort_dir_entries(entries, actual_count);
    
    /* 生成HTML表格行 */
    for (size_t i = 0; i < actual_count; i++) {
        dir_entry_t* dir_entry = &entries[i];

        /* 格式化修改时间 */
        char time_str[64];
        if (dir_entry->mtime > 0) {
            struct tm* tm_info = localtime(&dir_entry->mtime);
            strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        } else {
            time_str[0] = '-';
            time_str[1] = '\0';
        }

        /* HTML转义文件名 */
        char escaped_name[UVHTTP_MAX_FILE_PATH_SIZE * 6];
        html_escape(escaped_name, dir_entry->name, sizeof(escaped_name));

        /* 生成表格行 */
        if (dir_entry->is_dir) {
            offset += snprintf(html + offset, buffer_size - offset,
                "<tr><td><a href=\"%s/\" class=\"dir\">%s/</a></td><td class=\"dir\">-</td><td>%s</td></tr>\n",
                dir_entry->name, escaped_name, time_str);
        } else {
            offset += snprintf(html + offset, buffer_size - offset,
                "<tr><td><a href=\"%s\">%s</a></td><td class=\"size\">%zu</td><td>%s</td></tr>\n",
                dir_entry->name, escaped_name, dir_entry->size, time_str);
        }
    }
    
    /* 完成HTML */
    snprintf(html + offset, buffer_size - offset,
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
 * 生成ETag值
 */
uvhttp_result_t uvhttp_static_generate_etag(const char* file_path,
                               time_t last_modified,
                               size_t file_size,
                               char* etag,
                               size_t buffer_size) {
    if (!file_path || !etag || buffer_size == 0) return UVHTTP_ERROR_INVALID_PARAM;

    /* 简单的ETag生成：文件大小-修改时间 */
    snprintf(etag, buffer_size, "\"%zu-%ld\"", file_size, (long)last_modified);

    return UVHTTP_OK;
}

/**
 * 设置静态文件相关的响应头
 */
uvhttp_result_t uvhttp_static_set_response_headers(void* response,
                                      const char* file_path,
                                      size_t file_size,
                                      time_t last_modified,
                                      const char* etag) {
    if (!response) return UVHTTP_ERROR_INVALID_PARAM;
    
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

    return UVHTTP_OK;
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
uvhttp_error_t uvhttp_static_create(const uvhttp_static_config_t* config, uvhttp_static_context_t** context) {
    if (!config || !context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_static_context_t* ctx = uvhttp_alloc(sizeof(uvhttp_static_context_t));
    if (!ctx) {
        uvhttp_handle_memory_failure("static_context", NULL, NULL);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(ctx, 0, sizeof(uvhttp_static_context_t));

    /* 复制配置 */
    memcpy(&ctx->config, config, sizeof(uvhttp_static_config_t));

    /* 创建LRU缓存 */
    uvhttp_error_t result = uvhttp_lru_cache_create(
        config->max_cache_size,    /* 最大内存使用量 */
        1000,                     /* 最大条目数 */
        config->cache_ttl,        /* 缓存TTL */
        &ctx->cache               /* 输出参数 */
    );

    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to create LRU cache: %s", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return result;
    }

    if (!ctx->cache) {
        uvhttp_free(ctx);
        return UVHTTP_ERROR_IO_ERROR;
    }

    *context = ctx;
    return UVHTTP_OK;
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
            resolved_path[current_len] = '/';
            resolved_path[current_len + 1] = '\0';
        } else {
            return 0;
        }
    }
    
    /* 处理文件路径 */
    const char* path_to_add = (file_path[0] == '/') ? file_path + 1 : file_path;
    size_t current_len = strlen(resolved_path);
    size_t add_len = strlen(path_to_add);
    
    if (current_len + add_len < buffer_size) {
        memcpy(resolved_path + current_len, path_to_add, add_len + 1);  // +1 for null terminator
    } else {
        return 0;
    }
    
    /* 使用 realpath 进行路径规范化，防止路径遍历攻击 */
    char realpath_buf[PATH_MAX];
    char* resolved = realpath(resolved_path, realpath_buf);

    if (!resolved) {
        /* 路径不存在或无效 */
        return 0;
    }

    /* 将根目录也转换为绝对路径，以便比较 */
    char root_realpath_buf[PATH_MAX];
    char* root_resolved = realpath(root_dir, root_realpath_buf);
    if (!root_resolved) {
        /* 根目录不存在或无效 */
        return 0;
    }

    /* 确保规范化后的路径在根目录内 */
    size_t root_dir_len = strlen(root_resolved);
    if (strncmp(resolved, root_resolved, root_dir_len) != 0) {
        /* 路径不在根目录内 */
        return 0;
    }

    /* 确保路径在根目录下（不是根目录本身或父目录） */
    if (strlen(resolved) > root_dir_len && resolved[root_dir_len] != '/') {
        return 0;
    }

    /* 将规范化后的路径复制回输出缓冲区 */
    if (strlen(resolved) >= buffer_size) {
        return 0;
    }
    /* 使用安全的字符串复制函数 */
    if (uvhttp_safe_strcpy(resolved_path, buffer_size, resolved) != 0) {
        return 0;
    }

    return 1;
}

/**
 * 处理静态文件请求的主要函数
 */
uvhttp_result_t uvhttp_static_handle_request(uvhttp_static_context_t* ctx,
                                             void* request,
                                             void* response) {
    if (!ctx || !request || !response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    const char* url = uvhttp_request_get_url(request);
    if (!url) {
        uvhttp_response_set_status(response, 400);
        return UVHTTP_ERROR_MALFORMED_REQUEST;
    }

    /* 解析URL路径，移除查询参数 */
    char clean_path[UVHTTP_MAX_PATH_SIZE];
    const char* query_start = strchr(url, '?');
    size_t path_len = query_start ? (size_t)(query_start - url) : strlen(url);

    if (path_len >= sizeof(clean_path)) {
        uvhttp_response_set_status(response, 414); /* URI Too Long */
        return UVHTTP_ERROR_HEADER_TOO_LARGE;
    }

    /* 使用安全的字符串复制 */
    memcpy(clean_path, url, path_len);
    clean_path[path_len] = '\0';
    
    /* 处理根路径 */
    if (strcmp(clean_path, "/") == 0) {
        if (uvhttp_safe_strncpy(clean_path, ctx->config.index_file, sizeof(clean_path)) != 0) {
            /* index_file 太长，使用默认值 */
            strncpy(clean_path, "index.html", sizeof(clean_path) - 1);
            clean_path[sizeof(clean_path) - 1] = '\0';
        }
    }
    
    /* 构建安全的文件路径 */
    char safe_path[UVHTTP_MAX_FILE_PATH_SIZE];
    if (!uvhttp_static_resolve_safe_path(ctx->config.root_directory,
                                        clean_path,
                                        safe_path,
                                        sizeof(safe_path))) {
        uvhttp_response_set_status(response, 403); /* Forbidden */
        return UVHTTP_ERROR_NOT_FOUND;
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
        uvhttp_response_send(response);
        return UVHTTP_OK;
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
                    return UVHTTP_OK;
                }
            }
            /* 尝试添加索引文件 */
            char index_path[UVHTTP_MAX_FILE_PATH_SIZE];
            int result = snprintf(index_path, sizeof(index_path), "%s/%s", safe_path, ctx->config.index_file);
            if (result >= (int)sizeof(index_path)) {
                uvhttp_response_set_status(response, 414); /* URI Too Long */
                return UVHTTP_ERROR_HEADER_TOO_LARGE;
            }

            if (get_file_info(index_path, &file_size, &last_modified) == 0) {
                /* 使用安全的字符串拷贝，路径太长时自动截断 */
                uvhttp_safe_strncpy(safe_path, index_path, sizeof(safe_path));
            } else {
                uvhttp_response_set_status(response, 404); /* Not Found */
                return UVHTTP_ERROR_NOT_FOUND;
            }
        } else {
            uvhttp_response_set_status(response, 404); /* Not Found */
            return UVHTTP_ERROR_NOT_FOUND;
        }
    }

    /* 检查文件大小限制 */
    if (file_size > UVHTTP_STATIC_MAX_FILE_SIZE) {
        uvhttp_response_set_status(response, 413); /* Payload Too Large */
        return UVHTTP_ERROR_FILE_TOO_LARGE;
    }
    
    /* 对于大文件（> 1MB），使用sendfile零拷贝优化 */
    if (file_size > 1024 * 1024) {
        /* 获取MIME类型 */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(safe_path, mime_type, sizeof(mime_type));
        
        /* 生成ETag */
        char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_generate_etag(safe_path, last_modified, file_size, 
                                   etag, sizeof(etag));
        
        /* 检查条件请求 */
        if (uvhttp_static_check_conditional_request(request, etag, last_modified)) {
            uvhttp_response_set_status(response, 304); /* Not Modified */
            return 0;
        }
        
        /* 使用sendfile发送（传递配置） */
        uvhttp_result_t sendfile_result = uvhttp_static_sendfile_with_config(safe_path, response, &ctx->config);
        if (sendfile_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("sendfile failed: %s", uvhttp_error_string(sendfile_result));
            /* 回退到传统方式 */
        } else {
            /* sendfile成功，返回 */
            return UVHTTP_OK;
        }
    }

    /* 读取文件内容（小文件或sendfile失败时的回退） */
    char* file_content = read_file_content(safe_path, &file_size);
    if (!file_content) {
        uvhttp_response_set_status(response, 500); /* Internal Server Error */
        return UVHTTP_ERROR_IO_ERROR;
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
        return UVHTTP_OK;
    }
    
    /* 添加到缓存 */
    if (uvhttp_lru_cache_put(ctx->cache, safe_path, file_content, file_size, 
                            mime_type, last_modified, etag) != 0) {
        /* 缓存添加失败，但仍要返回内容 */
        uvhttp_log_safe_error(0, "static_cache", "Failed to cache file");
    }
    
    /* 发送响应 */
    uvhttp_static_set_response_headers(response, safe_path, file_size, 
                                          last_modified, etag);
    uvhttp_response_set_body(response, file_content, file_size);
    uvhttp_response_set_status(response, 200);
    uvhttp_response_send(response);
    
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

/**
 * 获取缓存统计信息
 */
void uvhttp_static_get_cache_stats(uvhttp_static_context_t* ctx,
                                  size_t* total_memory_usage,
                                  int* entry_count,
                                  int* hit_count,
                                  int* miss_count,
                                  int* eviction_count) {
    if (!ctx || !ctx->cache) {
        if (total_memory_usage) *total_memory_usage = 0;
        if (entry_count) *entry_count = 0;
        if (hit_count) *hit_count = 0;
        if (miss_count) *miss_count = 0;
        if (eviction_count) *eviction_count = 0;
        return;
    }
    
    uvhttp_lru_cache_get_stats(ctx->cache, total_memory_usage, entry_count,
                               hit_count, miss_count, eviction_count);
}

/**
 * 获取缓存命中率
 */
double uvhttp_static_get_cache_hit_rate(uvhttp_static_context_t* ctx) {
    if (!ctx || !ctx->cache) {
        return 0.0;
    }
    
    size_t total_memory_usage;
    int entry_count, hit_count, miss_count, eviction_count;
    
    uvhttp_lru_cache_get_stats(ctx->cache, &total_memory_usage, &entry_count,
                               &hit_count, &miss_count, &eviction_count);
    
    if (hit_count + miss_count == 0) {
        return 0.0;
    }
    
    return (double)hit_count / (hit_count + miss_count) * 100.0;
}

/**
 * 清理过期缓存条目
 */
int uvhttp_static_cleanup_expired_cache(uvhttp_static_context_t* ctx) {
    if (!ctx || !ctx->cache) {
        return 0;
    }

    return uvhttp_lru_cache_cleanup_expired(ctx->cache);
}

/* ========== 静态文件中间件实现 ========== */

/**

 * 缓存预热：预加载指定的文件到缓存中

 */

uvhttp_result_t uvhttp_static_prewarm_cache(uvhttp_static_context_t* ctx,

                                            const char* file_path) {

    if (!ctx || !file_path) {

        return UVHTTP_ERROR_INVALID_PARAM;

    }

    

    /* 构建完整文件路径 */

    char full_path[UVHTTP_MAX_FILE_PATH_SIZE];

    int result = snprintf(full_path, sizeof(full_path), "%s/%s", 

                         ctx->config.root_directory, file_path);

    if (result >= (int)sizeof(full_path)) {

        return UVHTTP_ERROR_INVALID_PARAM;

    }
    
    /* 检查文件是否存在 */
    struct stat st;
    if (stat(full_path, &st) != 0 || S_ISDIR(st.st_mode)) {
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    /* 获取文件信息 */
    size_t file_size = (size_t)st.st_size;
    time_t last_modified = st.st_mtime;
    
    /* 检查文件大小限制 */
    if (file_size > UVHTTP_STATIC_MAX_FILE_SIZE) {
        UVHTTP_LOG_WARN("File too large for prewarming: %s (size: %zu)", 
                        file_path, file_size);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 读取文件内容 */
    char* file_content = read_file_content(full_path, &file_size);
    if (!file_content) {
        UVHTTP_LOG_ERROR("Failed to read file for prewarming: %s", file_path);
        return UVHTTP_ERROR_SERVER_INIT;
    }
    
    /* 获取MIME类型 */
    char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_get_mime_type(full_path, mime_type, sizeof(mime_type));
    
    /* 生成ETag */
    char etag[UVHTTP_MAX_HEADER_VALUE_SIZE];
    uvhttp_static_generate_etag(full_path, last_modified, file_size, 
                               etag, sizeof(etag));
    
    /* 添加到缓存 */
    uvhttp_error_t cache_result = uvhttp_lru_cache_put(ctx->cache, full_path, 
                                                       file_content, file_size,
                                                       mime_type, last_modified, etag);
    if (cache_result != UVHTTP_OK) {
        UVHTTP_LOG_WARN("Failed to cache file for prewarming: %s", file_path);
        uvhttp_free(file_content);
        return cache_result;
    }
    
    UVHTTP_LOG_INFO("Prewarmed cache: %s (size: %zu)", file_path, file_size);
    return UVHTTP_OK;
}

/**
 * 缓存预热：预加载目录中的所有文件
 */
int uvhttp_static_prewarm_directory(uvhttp_static_context_t* ctx,
                                    const char* dir_path,
                                    int max_files) {
    if (!ctx || !dir_path) {
        UVHTTP_LOG_ERROR("Invalid parameters for directory prewarming");
        return -1;
    }
    
    if (!ctx->cache) {
        UVHTTP_LOG_ERROR("Cache not initialized for prewarming");
        return -1;
    }
    
    /* 构建完整目录路径 */
    char full_dir_path[UVHTTP_MAX_FILE_PATH_SIZE];
    int result = snprintf(full_dir_path, sizeof(full_dir_path), "%s/%s", 
                         ctx->config.root_directory, dir_path);
    if (result >= (int)sizeof(full_dir_path)) {
        UVHTTP_LOG_ERROR("Directory path too long: %s", dir_path);
        return -1;
    }
    
    /* 检查目录是否存在 */
    struct stat st;
    if (stat(full_dir_path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        UVHTTP_LOG_ERROR("Directory not found: %s", dir_path);
        return -1;
    }
    
    /* 打开目录 */
    DIR* dir = opendir(full_dir_path);
    if (!dir) {
        UVHTTP_LOG_ERROR("Failed to open directory: %s (errno: %d)", dir_path, errno);
        return -1;
    }
    
    int prewarmed_count = 0;
    struct dirent* entry;
    
    while ((entry = readdir(dir)) != NULL) {
        /* 跳过 . 和 .. */
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }
        
        /* 跳过目录 */
        if (entry->d_type == DT_DIR) {
            continue;
        }
        
        /* 检查文件数量限制 */
        if (max_files > 0 && prewarmed_count >= max_files) {
            UVHTTP_LOG_INFO("Reached max files limit for prewarming: %d", max_files);
            break;
        }
        
        /* 构建相对路径 */
        char relative_path[UVHTTP_MAX_FILE_PATH_SIZE];
        result = snprintf(relative_path, sizeof(relative_path), "%s/%s", 
                         dir_path, entry->d_name);
        if (result < 0 || result >= (int)sizeof(relative_path)) {
            UVHTTP_LOG_WARN("File path too long, skipping: %s/%s", dir_path, entry->d_name);
            continue;
        }
        
        /* 预热文件 */
        if (uvhttp_static_prewarm_cache(ctx, relative_path) == UVHTTP_OK) {
            prewarmed_count++;
        } else {
            UVHTTP_LOG_WARN("Failed to prewarm file: %s", relative_path);
        }
    }
    
    closedir(dir);
    
    UVHTTP_LOG_INFO("Prewarmed directory: %s (%d files)", dir_path, prewarmed_count);
    return prewarmed_count;
}

/* ============ 零拷贝优化：sendfile 实现 ============ */

/* sendfile 上下文结构 */
typedef struct {
    uv_fs_t open_req;
    uv_fs_t sendfile_req;
    uv_fs_t close_req;
    uv_file in_fd;
    uvhttp_response_t* response;
    char* file_path;
    size_t file_size;
    size_t bytes_sent;
    int64_t offset;
    int completed;
    uv_os_fd_t out_fd;  /* 输出文件描述符 */
    uint64_t start_time;  /* 开始时间（用于超时检测） */
    int retry_count;  /* 重试次数 */
    int timeout_ms;  /* 超时时间（毫秒） */
    int max_retry;  /* 最大重试次数 */
    size_t chunk_size;  /* 分块大小 */
    uv_timer_t timeout_timer;  /* 超时定时器 */
} sendfile_context_t;

/* sendfile 默认配置（宏定义） */
#define SENDFILE_DEFAULT_TIMEOUT_MS  30000  /* 30秒超时，适应慢速网络环境 */
#define SENDFILE_DEFAULT_MAX_RETRY    2      /* 最大重试次数 */
#define SENDFILE_DEFAULT_CHUNK_SIZE   (64 * 1024)  /* 64KB 分块 */

/* sendfile 回调函数 */
/* 文件关闭回调 */
static void on_file_close(uv_fs_t* req);

/* 超时回调函数 */
static void on_sendfile_timeout(uv_timer_t* timer) {
    sendfile_context_t* ctx = (sendfile_context_t*)timer->data;
    
    if (!ctx || ctx->completed) {
        return;
    }
    
    UVHTTP_LOG_ERROR("sendfile timeout: %s (sent %zu/%zu bytes, elapsed %dms)", 
                    ctx->file_path, ctx->bytes_sent, ctx->file_size, ctx->timeout_ms);
    
    /* 标记为完成，防止重复处理 */
    ctx->completed = 1;
    
    /* 停止定时器 */
    uv_timer_stop(timer);
    uv_close((uv_handle_t*)timer, NULL);
    
    /* 关闭文件并清理资源 */
    uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)ctx->response->client);
    uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
}

/* sendfile 回调函数 */
/* 文件关闭回调 */
static void on_file_close(uv_fs_t* req) {
    sendfile_context_t* ctx = (sendfile_context_t*)req->data;
    
    if (req->result < 0) {
        UVHTTP_LOG_ERROR("Failed to close file: %s", uv_strerror(req->result));
    }
    
    uv_fs_req_cleanup(req);
    
    /* 释放上下文内存 */
    if (ctx) {
        if (ctx->file_path) {
            uvhttp_free(ctx->file_path);
            ctx->file_path = NULL;
        }
        uvhttp_free(ctx);
    }
}

/* sendfile 回调函数 */
static void on_sendfile_complete(uv_fs_t* req) {
    sendfile_context_t* ctx = (sendfile_context_t*)req->data;
    
    if (!ctx || ctx->completed) {
        return;
    }
    
    /* 获取event loop */
    uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)ctx->response->client);
    
    /* 停止超时定时器（如果仍在运行） */
    if (!uv_is_closing((uv_handle_t*)&ctx->timeout_timer)) {
        uv_timer_stop(&ctx->timeout_timer);
        uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
    }
    
    /* 检查是否发送完成或出错 */
    if (req->result < 0) {
        UVHTTP_LOG_ERROR("sendfile failed: %s", uv_strerror(req->result));
        
        /* 检查是否可以重试 */
        if (ctx->retry_count < ctx->max_retry && 
            (req->result == UV_EINTR || req->result == UV_EAGAIN)) {
            ctx->retry_count++;
            UVHTTP_LOG_INFO("Retrying sendfile: %s (attempt %d/%d)", 
                           ctx->file_path, ctx->retry_count, ctx->max_retry);
            
            /* 重试发送 */
            size_t remaining = ctx->file_size - ctx->offset;
            size_t chunk_size = (remaining > ctx->chunk_size) ? ctx->chunk_size : remaining;
            
            uv_fs_req_cleanup(req);
            uv_fs_sendfile(loop, &ctx->sendfile_req, 
                          ctx->out_fd, ctx->in_fd, ctx->offset, chunk_size, 
                          on_sendfile_complete);
            return;
        }
        
        /* 发送失败，关闭文件并清理 */
        uv_fs_req_cleanup(req);
        uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
        return;
    }
    
    ctx->bytes_sent += req->result;
    ctx->offset += req->result;
    
    /* 检查是否发送完成 */
    if (ctx->offset >= (int64_t)ctx->file_size) {
        /* 停止超时定时器 */
        if (!uv_is_closing((uv_handle_t*)&ctx->timeout_timer)) {
            uv_timer_stop(&ctx->timeout_timer);
            uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
        }
        
        /* 关闭文件 */
        uv_fs_req_cleanup(req);
        uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
        
        /* 标记完成 */
        ctx->completed = 1;
        UVHTTP_LOG_INFO("sendfile completed: %s (%zu bytes)", ctx->file_path, ctx->bytes_sent);
    } else {
        /* 继续发送剩余数据 */
        size_t remaining = ctx->file_size - ctx->offset;
        size_t chunk_size = (remaining > ctx->chunk_size) ? ctx->chunk_size : remaining;
        
        uv_fs_req_cleanup(req);
        uv_fs_sendfile(loop, &ctx->sendfile_req, 
                      ctx->out_fd,
                      ctx->in_fd, ctx->offset, chunk_size, on_sendfile_complete);
        
        /* 重启超时定时器 */
        if (!uv_is_closing((uv_handle_t*)&ctx->timeout_timer)) {
            uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout, ctx->timeout_ms, 0);
        }
    }
}

/**
 * 设置 sendfile 配置参数
 */
uvhttp_error_t uvhttp_static_set_sendfile_config(uvhttp_static_context_t* ctx,
                                                int timeout_ms,
                                                int max_retry,
                                                size_t chunk_size) {
    if (!ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 设置配置值（0 表示使用默认值） */
    if (timeout_ms > 0) {
        ctx->config.sendfile_timeout_ms = timeout_ms;
    }
    if (max_retry > 0) {
        ctx->config.sendfile_max_retry = max_retry;
    }
    if (chunk_size > 0) {
        ctx->config.sendfile_chunk_size = chunk_size;
    }
    
    return UVHTTP_OK;
}

/* 内部函数：带配置的 sendfile */
static uvhttp_result_t uvhttp_static_sendfile_with_config(const char* file_path, 
                                                          void* response,
                                                          const uvhttp_static_config_t* config) {
    uvhttp_response_t* resp = (uvhttp_response_t*)response;
    
    /* 获取文件大小 */
    struct stat st;
    if (stat(file_path, &st) != 0) {
        UVHTTP_LOG_ERROR("Failed to stat file: %s", file_path);
        return UVHTTP_ERROR_NOT_FOUND;
    }
    
    size_t file_size = (size_t)st.st_size;
    
    /* 策略选择 */
    if (file_size < 4096) {
        /* 小文件：使用优化的系统调用（open + read + close），避免 stdio 开销 */
        UVHTTP_LOG_DEBUG("Small file detected, using optimized I/O: %s (%zu bytes)", 
                        file_path, file_size);
        
        /* 使用 open() 代替 fopen()，减少系统调用开销 */
        int fd = open(file_path, O_RDONLY);
        if (fd < 0) {
            UVHTTP_LOG_ERROR("Failed to open file: %s", file_path);
            return UVHTTP_ERROR_NOT_FOUND;
        }
        
        /* 读取文件内容 */
        char* buffer = (char*)uvhttp_alloc(file_size + 1);
        if (!buffer) {
            close(fd);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        ssize_t bytes_read = read(fd, buffer, (size_t)file_size);
        close(fd);
        
        if (bytes_read < 0) {
            UVHTTP_LOG_ERROR("Failed to read file: %s", file_path);
            uvhttp_free(buffer);
            return UVHTTP_ERROR_RESPONSE_SEND;
        }
        
        /* 设置响应 */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type));
        
        uvhttp_response_set_status(resp, 200);
        uvhttp_response_set_header(resp, "Content-Type", mime_type);
        uvhttp_response_set_header(resp, "Content-Length", "");
        uvhttp_response_set_body(resp, buffer, (size_t)bytes_read);
        
        uvhttp_free(buffer);
        return UVHTTP_OK;
    }
    else if (file_size <= 10 * 1024 * 1024) {
        /* 中等文件：使用分块异步 sendfile（与 大文件一致） */
        UVHTTP_LOG_DEBUG("Medium file detected, using chunked async sendfile: %s (%zu bytes)", 
                        file_path, file_size);
        
        /* 获取event loop */
        uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)resp->client);
        
        /* 创建 sendfile 上下文 */
        sendfile_context_t* ctx = (sendfile_context_t*)uvhttp_alloc(sizeof(sendfile_context_t));
        if (!ctx) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        memset(ctx, 0, sizeof(sendfile_context_t));
        ctx->response = resp;
        ctx->file_size = file_size;
        ctx->offset = 0;
        ctx->bytes_sent = 0;
        ctx->completed = 0;
        ctx->start_time = uv_now(loop);
        ctx->retry_count = 0;
        
        /* 从配置中读取 sendfile 参数 */
        if (config && config->sendfile_timeout_ms > 0) {
            ctx->timeout_ms = config->sendfile_timeout_ms;
        } else {
            ctx->timeout_ms = SENDFILE_DEFAULT_TIMEOUT_MS;
        }
        
        if (config && config->sendfile_max_retry > 0) {
            ctx->max_retry = config->sendfile_max_retry;
        } else {
            ctx->max_retry = SENDFILE_DEFAULT_MAX_RETRY;
        }
        
        if (config && config->sendfile_chunk_size > 0) {
            ctx->chunk_size = config->sendfile_chunk_size;
        } else {
            ctx->chunk_size = SENDFILE_DEFAULT_CHUNK_SIZE;
        }
        
        /* 分配文件路径内存 */
        size_t path_len = strlen(file_path);
        ctx->file_path = (char*)uvhttp_alloc(path_len + 1);
        if (!ctx->file_path) {
            uvhttp_free(ctx);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(ctx->file_path, file_path, path_len);
        ctx->file_path[path_len] = '\0';
        
        /* 获取输出文件描述符 */
        int fd_result = uv_fileno((uv_handle_t*)resp->client, &ctx->out_fd);
        if (fd_result < 0) {
            UVHTTP_LOG_ERROR("Failed to get client fd: %s", uv_strerror(fd_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }
        
        /* 打开输入文件 */
        ctx->in_fd = open(file_path, O_RDONLY);
        if (ctx->in_fd < 0) {
            UVHTTP_LOG_ERROR("Failed to open file for sendfile: %s", file_path);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_NOT_FOUND;
        }
        
        /* 初始化超时定时器 */
        int timer_result = uv_timer_init(loop, &ctx->timeout_timer);
        if (timer_result != 0) {
            UVHTTP_LOG_ERROR("Failed to init timeout timer: %s", uv_strerror(timer_result));
            close(ctx->in_fd);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }
        ctx->timeout_timer.data = ctx;
        
        /* 开始分块 sendfile（每次发送配置的分块大小） */
        size_t chunk_size = ctx->chunk_size;
        
        uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout, ctx->timeout_ms, 0);
        
        int sendfile_result = uv_fs_sendfile(loop, &ctx->sendfile_req,
                                              ctx->out_fd,
                                              ctx->in_fd, 0, chunk_size, on_sendfile_complete);
        
        /* 检查 sendfile 是否同步失败 */
        if (sendfile_result < 0) {
            UVHTTP_LOG_ERROR("Failed to start sendfile: %s", uv_strerror(sendfile_result));
            /* 清理资源 */
            uv_timer_stop(&ctx->timeout_timer);
            uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_RESPONSE_SEND;
        }
        
        return UVHTTP_OK;
    }
    else {
        /* 大文件：使用 sendfile 零拷贝优化 */
        UVHTTP_LOG_DEBUG("Large file detected, using sendfile: %s (%zu bytes)", 
                        file_path, file_size);
        
        /* 获取event loop */
        uv_loop_t* loop = uv_handle_get_loop((uv_handle_t*)resp->client);
        
        /* 创建 sendfile 上下文 */
        sendfile_context_t* ctx = (sendfile_context_t*)uvhttp_alloc(sizeof(sendfile_context_t));
        if (!ctx) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        memset(ctx, 0, sizeof(sendfile_context_t));
        ctx->response = resp;
        ctx->file_size = file_size;
        ctx->offset = 0;
        ctx->bytes_sent = 0;
        ctx->completed = 0;
        ctx->start_time = uv_now(loop);
        ctx->retry_count = 0;
        
        /* 从配置中读取 sendfile 参数 */
        if (config && config->sendfile_timeout_ms > 0) {
            ctx->timeout_ms = config->sendfile_timeout_ms;
        } else {
            ctx->timeout_ms = SENDFILE_DEFAULT_TIMEOUT_MS;
        }
        
        if (config && config->sendfile_max_retry > 0) {
            ctx->max_retry = config->sendfile_max_retry;
        } else {
            ctx->max_retry = SENDFILE_DEFAULT_MAX_RETRY;
        }
        
        if (config && config->sendfile_chunk_size > 0) {
            ctx->chunk_size = config->sendfile_chunk_size;
        } else {
            ctx->chunk_size = SENDFILE_DEFAULT_CHUNK_SIZE;
        }
        
        /* 分配文件路径内存 */
        size_t path_len = strlen(file_path);
        ctx->file_path = (char*)uvhttp_alloc(path_len + 1);
        if (!ctx->file_path) {
            uvhttp_free(ctx);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        memcpy(ctx->file_path, file_path, path_len);
        ctx->file_path[path_len] = '\0';
        
        /* 获取输出文件描述符 */
        int fd_result = uv_fileno((uv_handle_t*)resp->client, &ctx->out_fd);
        if (fd_result < 0) {
            UVHTTP_LOG_ERROR("Failed to get client fd: %s", uv_strerror(fd_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }
        
        /* 打开输入文件 */
        ctx->in_fd = open(file_path, O_RDONLY);
        if (ctx->in_fd < 0) {
            UVHTTP_LOG_ERROR("Failed to open file for sendfile: %s", file_path);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_NOT_FOUND;
        }
        
        /* 获取MIME类型 */
        char mime_type[UVHTTP_MAX_HEADER_VALUE_SIZE];
        uvhttp_static_get_mime_type(file_path, mime_type, sizeof(mime_type));
        
        char content_length[64];
        snprintf(content_length, sizeof(content_length), "%zu", file_size);
        
        uvhttp_response_set_status(resp, 200);
        uvhttp_response_set_header(resp, "Content-Type", mime_type);
        uvhttp_response_set_header(resp, "Content-Length", content_length);
        
        /* 发送响应头 */
        uvhttp_error_t send_result = uvhttp_response_send(resp);
        if (send_result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("Failed to send response headers: %s", uvhttp_error_string(send_result));
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            return send_result;
        }
        
        /* 初始化超时定时器 */
        int timer_result = uv_timer_init(loop, &ctx->timeout_timer);
        if (timer_result != 0) {
            UVHTTP_LOG_ERROR("Failed to init timeout timer: %s", uv_strerror(timer_result));
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_SERVER_INIT;
        }
        ctx->timeout_timer.data = ctx;
        
        /* 开始分块 sendfile（每次发送配置的分块大小） */
        size_t chunk_size = ctx->chunk_size;
        
        uv_timer_start(&ctx->timeout_timer, on_sendfile_timeout, ctx->timeout_ms, 0);
        
        int sendfile_result = uv_fs_sendfile(loop, &ctx->sendfile_req,
                                              ctx->out_fd,
                                              ctx->in_fd, 0, chunk_size, on_sendfile_complete);
        
        /* 检查 sendfile 是否同步失败 */
        if (sendfile_result < 0) {
            UVHTTP_LOG_ERROR("Failed to start sendfile: %s", uv_strerror(sendfile_result));
            /* 清理资源 */
            uv_timer_stop(&ctx->timeout_timer);
            uv_close((uv_handle_t*)&ctx->timeout_timer, NULL);
            uv_fs_close(loop, &ctx->close_req, ctx->in_fd, on_file_close);
            uvhttp_free(ctx->file_path);
            uvhttp_free(ctx);
            return UVHTTP_ERROR_RESPONSE_SEND;
        }
        
        return UVHTTP_OK;
    }
}

/* 零拷贝发送静态文件（混合策略）- 使用默认配置 */
uvhttp_result_t uvhttp_static_sendfile(const char* file_path, void* response) {
    /* 调用内部函数，使用 NULL 配置（使用默认值） */
    return uvhttp_static_sendfile_with_config(file_path, response, NULL);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
