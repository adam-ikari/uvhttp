#include "uvhttp_middleware.h"
#include "uvhttp_log.h"
#include "uvhttp_json.h"
#include "uvhttp_constants.h"
#include <stdio.h>
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>

// 中间件链实现
uvhttp_middleware_chain_t* uvhttp_middleware_chain_new(void) {
    uvhttp_middleware_chain_t* chain = uvhttp_malloc(sizeof(uvhttp_middleware_chain_t));
    if (!chain) return NULL;
    
    chain->head = NULL;
    chain->tail = NULL;
    chain->count = 0;
    return chain;
}

void uvhttp_middleware_chain_free(uvhttp_middleware_chain_t* chain) {
    if (!chain) return;
    
    uvhttp_middleware_t* current = chain->head;
    while (current) {
        uvhttp_middleware_t* next = current->next;
        uvhttp_free(current);
        current = next;
    }
    
    uvhttp_free(chain);
}

int uvhttp_middleware_chain_add(uvhttp_middleware_chain_t* chain, 
                               uvhttp_middleware_func_t func, 
                               void* data) {
    if (!chain || !func) return -1;
    
    uvhttp_middleware_t* middleware = uvhttp_malloc(sizeof(uvhttp_middleware_t));
    if (!middleware) return -1;
    
    middleware->func = func;
    middleware->data = data;
    middleware->next = NULL;
    
    if (chain->tail) {
        chain->tail->next = middleware;
    } else {
        chain->head = middleware;
    }
    chain->tail = middleware;
    chain->count++;
    
    return 0;
}

int uvhttp_middleware_chain_execute(uvhttp_middleware_chain_t* chain,
                                   uvhttp_request_t* request,
                                   uvhttp_response_t* response) {
    if (!chain || !request || !response) return -1;
    
    uvhttp_middleware_t* current = chain->head;
    while (current) {
        int result = current->func(request, response, current->data);
        if (result != 0) {
            return result; // 中间件返回错误，停止执行
        }
        current = current->next;
    }
    
    return 0;
}



// CORS中间件实现
uvhttp_cors_middleware_data_t* uvhttp_cors_middleware_data_new(const char* allow_origin,
                                                             const char* allow_methods,
                                                             const char* allow_headers,
                                                             const char* max_age) {
    uvhttp_cors_middleware_data_t* data = uvhttp_malloc(sizeof(uvhttp_cors_middleware_data_t));
    if (!data) return NULL;
    
    data->allow_origin = allow_origin ? allow_origin : "*";
    data->allow_methods = allow_methods ? allow_methods : "GET, POST, PUT, DELETE, OPTIONS";
    data->allow_headers = allow_headers ? allow_headers : "Content-Type, Authorization";
    data->max_age = max_age ? max_age : UVHTTP_CORS_MAX_AGE_DEFAULT;
    
    return data;
}

void uvhttp_cors_middleware_data_free(uvhttp_cors_middleware_data_t* data) {
    uvhttp_free(data);
}

int uvhttp_cors_middleware(uvhttp_request_t* request,
                          uvhttp_response_t* response,
                          void* next_data) {
    uvhttp_cors_middleware_data_t* data = (uvhttp_cors_middleware_data_t*)next_data;
    if (!data) return 0;
    
    // 设置CORS头
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", data->allow_origin);
    uvhttp_response_set_header(response, "Access-Control-Allow-Methods", data->allow_methods);
    uvhttp_response_set_header(response, "Access-Control-Allow-Headers", data->allow_headers);
    uvhttp_response_set_header(response, "Access-Control-Max-Age", data->max_age);
    
    // 处理OPTIONS预检请求
    const char* method = uvhttp_request_get_method(request);
    if (method && strcmp(method, "OPTIONS") == 0) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_body(response, "", 0);
        return 0; // 直接返回，不继续执行
    }
    
    return 0;
}

// 限流中间件实现
uvhttp_rate_limit_middleware_data_t* uvhttp_rate_limit_middleware_data_new(int max_requests_per_minute) {
    uvhttp_rate_limit_middleware_data_t* data = uvhttp_malloc(sizeof(uvhttp_rate_limit_middleware_data_t));
    if (!data) return NULL;
    
    data->max_requests_per_minute = max_requests_per_minute;
    data->current_requests = 0;
    data->window_start = time(NULL);
    
    return data;
}

void uvhttp_rate_limit_middleware_data_free(uvhttp_rate_limit_middleware_data_t* data) {
    uvhttp_free(data);
}

int uvhttp_rate_limit_middleware(uvhttp_request_t* request,
                                uvhttp_response_t* response,
                                void* next_data) {
    (void)request; /* 避免未使用参数警告 */
    (void)response; /* 避免未使用参数警告 */
    uvhttp_rate_limit_middleware_data_t* data = (uvhttp_rate_limit_middleware_data_t*)next_data;
    if (!data) return 0;
    
    time_t current_time = time(NULL);
    if (current_time == (time_t)-1) {
        // time() 函数失败，记录错误但不阻塞请求
        return 0;
    }
    
    // 检查是否需要重置时间窗口
    if (current_time - data->window_start >= UVHTTP_RATE_LIMIT_WINDOW) {
        data->window_start = current_time;
        data->current_requests = 0;
    }
    
    // 检查是否超过限制
    if (data->current_requests >= data->max_requests_per_minute) {
        int result = uvhttp_response_json_error(response, 429, "Too many requests");
        if (result != 0) {
            // 如果JSON响应失败，返回简单的文本响应
            uvhttp_response_set_status(response, 429);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, "Too many requests", 17);
        }
        return -1; // 停止执行
    }
    
    data->current_requests++;
    return 0;
}

// 静态文件中间件实现
uvhttp_static_middleware_data_t* uvhttp_static_middleware_data_new(const char* root_directory,
                                                                  const char* url_prefix,
                                                                  int auto_index,
                                                                  int enable_cache,
                                                                  int max_cache_size,
                                                                  const char* index_file) {
    uvhttp_static_middleware_data_t* data = uvhttp_malloc(sizeof(uvhttp_static_middleware_data_t));
    if (!data) return NULL;
    
    // 初始化所有指针为NULL
    data->root_directory = NULL;
    data->url_prefix = NULL;
    data->index_file = NULL;
    
    // 复制字符串，检查每个操作
    data->root_directory = root_directory ? strdup(root_directory) : strdup(".");
    if (!data->root_directory) goto error;
    
    data->url_prefix = url_prefix ? strdup(url_prefix) : strdup("/static");
    if (!data->url_prefix) goto error;
    
    data->index_file = index_file ? strdup(index_file) : strdup("index.html");
    if (!data->index_file) goto error;
    
    data->auto_index = auto_index;
    data->enable_cache = enable_cache;
    data->max_cache_size = max_cache_size > 0 ? max_cache_size : UVHTTP_STATIC_MAX_CACHE_SIZE;
    
    return data;
    
error:
    // 清理已分配的内存
    if (data->root_directory) uvhttp_free(data->root_directory);
    if (data->url_prefix) uvhttp_free(data->url_prefix);
    if (data->index_file) uvhttp_free(data->index_file);
    uvhttp_free(data);
    return NULL;
}

void uvhttp_static_middleware_data_free(uvhttp_static_middleware_data_t* data) {
    if (!data) return;
    
    uvhttp_free(data->root_directory);
    uvhttp_free(data->url_prefix);
    uvhttp_free(data->index_file);
    uvhttp_free(data);
}

// 获取文件MIME类型
static const char* get_mime_type(const char* filename) {
    const char* ext = strrchr(filename, '.');
    if (!ext) return "application/octet-stream";
    
    if (strcmp(ext, ".html") == 0 || strcmp(ext, ".htm") == 0) return "text/html";
    if (strcmp(ext, ".css") == 0) return "text/css";
    if (strcmp(ext, ".js") == 0) return "application/javascript";
    if (strcmp(ext, ".json") == 0) return "application/json";
    if (strcmp(ext, ".xml") == 0) return "application/xml";
    if (strcmp(ext, ".txt") == 0) return "text/plain";
    if (strcmp(ext, ".jpg") == 0 || strcmp(ext, ".jpeg") == 0) return "image/jpeg";
    if (strcmp(ext, ".png") == 0) return "image/png";
    if (strcmp(ext, ".gif") == 0) return "image/gif";
    if (strcmp(ext, ".svg") == 0) return "image/svg+xml";
    if (strcmp(ext, ".ico") == 0) return "image/x-icon";
    if (strcmp(ext, ".pdf") == 0) return "application/pdf";
    if (strcmp(ext, ".zip") == 0) return "application/zip";
    
    return "application/octet-stream";
}

// URL路径解码
static void url_decode(char* dst, const char* src, size_t dst_size) {
    size_t i = 0, j = 0;
    while (src[i] && j < dst_size - 1) {
        if (src[i] == '%' && src[i+1] && src[i+2]) {
            char hex[3] = {src[i+1], src[i+2], '\0'};
            dst[j++] = (char)strtol(hex, NULL, 16);
            i += 3;
        } else if (src[i] == '+') {
            dst[j++] = ' ';
            i++;
        } else {
            dst[j++] = src[i++];
        }
    }
    dst[j] = '\0';
}

// 增强的安全路径检查，防止目录遍历攻击
static int is_safe_path(const char* path) {
    if (!path || !path[0]) return 0;
    
    size_t path_len = strlen(path);
    
    // 检查路径长度限制
    if (path_len > UVHTTP_STATIC_MAX_PATH_SIZE) return 0;
    
    // 检查是否包含 ".."
    if (strstr(path, "..") != NULL) return 0;
    
    // 检查是否以 / 开头（绝对路径）
    if (path[0] == '/') return 0;
    
    // 检查是否包含危险字符
    if (strpbrk(path, "<>|\"`") != NULL) return 0;
    
    // 检查是否包含连续的斜杠
    if (strstr(path, "//") != NULL) return 0;
    
    // 检查是否以点开头或结尾（隐藏文件）
    if (path[0] == '.' || (path_len > 0 && path[path_len-1] == '.')) {
        // 允许 . 开头的文件，但要进一步检查
        if (strcmp(path, ".") == 0 || strncmp(path, "./", 2) == 0) {
            return 0;
        }
    }
    
    // 检查是否包含空字节（NULL字节注入）
    if (memchr(path, '\0', path_len) != path + path_len) {
        return 0;
    }
    
    // 检查路径中的每个组件
    char* path_copy = strdup(path);
    if (!path_copy) return 0;
    
    char* token = strtok(path_copy, "/");
    int safe = 1;
    
    while (token && safe) {
        // 检查组件长度
        if (strlen(token) > 255) {
            safe = 0;
            break;
        }
        
        // 检查是否为特殊文件名
        if (strcmp(token, ".") == 0 || strcmp(token, "..") == 0) {
            safe = 0;
            break;
        }
        
        token = strtok(NULL, "/");
    }
    
    uvhttp_free(path_copy);
    return safe;
}

int uvhttp_static_middleware(uvhttp_request_t* request,
                            uvhttp_response_t* response,
                            void* next_data) {
    uvhttp_static_middleware_data_t* data = (uvhttp_static_middleware_data_t*)next_data;
    if (!data) return 0;
    
    const char* url = uvhttp_request_get_url(request);
    if (!url) return 0;
    
    // 检查URL是否匹配前缀
    size_t prefix_len = strlen(data->url_prefix);
    if (strncmp(url, data->url_prefix, prefix_len) != 0) {
        return 0; // 不匹配，继续下一个中间件
    }
    
    // 提取文件路径
    const char* file_path = url + prefix_len;
    if (file_path[0] == '/') file_path++; // 跳过开头的 /
    
    // URL解码
    char decoded_path[UVHTTP_DECODED_PATH_SIZE];
    url_decode(decoded_path, file_path, sizeof(decoded_path));
    
    // 安全检查
    if (!is_safe_path(decoded_path)) {
        uvhttp_response_set_status(response, 403);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Forbidden", 9);
        return 0; // 停止处理
    }
    
    // 如果路径为空，使用索引文件
    if (strlen(decoded_path) == 0) {
        if (uvhttp_safe_strcpy(decoded_path, sizeof(decoded_path), data->index_file) != 0) {
            uvhttp_response_set_status(response, 500);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, "Internal Server Error", 21);
            return 0;
        }
    }
    
    // 构建完整文件路径
    char full_path[UVHTTP_MAX_FILE_PATH_SIZE];
    snprintf(full_path, sizeof(full_path), "%s/%s", data->root_directory, decoded_path);
    
    // 检查文件是否存在并可读
    FILE* file = fopen(full_path, "rb");
    if (!file) {
        uvhttp_response_set_status(response, 404);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "File not found", 14);
        return 0; // 停止处理
    }
    
    // 获取文件大小
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal server error", 21);
        return 0;
    }
    
    // 设置响应头
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", get_mime_type(full_path));
    
    // 设置文件大小头
    char content_length[UVHTTP_STATIC_MAX_CONTENT_LENGTH];
    snprintf(content_length, sizeof(content_length), "%ld", file_size);
    uvhttp_response_set_header(response, "Content-Length", content_length);
    
    // 设置缓存头
    if (data->enable_cache) {
        char cache_header[64];
        snprintf(cache_header, sizeof(cache_header), "public, max-age=%d", UVHTTP_CACHE_MAX_AGE);
        uvhttp_response_set_header(response, "Cache-Control", cache_header);
    }
    
    // 读取文件内容并发送
    if (file_size > 0) {
        // 检查文件大小限制（防止内存耗尽）
        if (file_size > UVHTTP_STATIC_MAX_FILE_SIZE) {
            fclose(file);
            uvhttp_response_set_status(response, UVHTTP_STATUS_REQUEST_ENTITY_TOO_LARGE);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, UVHTTP_MESSAGE_FILE_TOO_LARGE, UVHTTP_ERROR_MESSAGE_LENGTH);
            return 0;
        }
        
        char* buffer = uvhttp_malloc(file_size);
        if (!buffer) {
            fclose(file);
            uvhttp_response_set_status(response, 500);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, "Memory allocation failed", 25);
            return 0;
        }
        
        size_t read_size = fread(buffer, 1, file_size, file);
        if (read_size != (size_t)file_size) {
            uvhttp_free(buffer);
            fclose(file);
            uvhttp_response_set_status(response, 500);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, "File read error", 16);
            return 0;
        }
        
        int result = uvhttp_response_set_body(response, buffer, read_size);
        uvhttp_free(buffer);
        
        if (result != 0) {
            fclose(file);
            uvhttp_response_set_status(response, 500);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_body(response, "Response body error", 20);
            return 0;
        }
    }
    
    fclose(file);
    return 0; // 停止处理，文件已发送
}