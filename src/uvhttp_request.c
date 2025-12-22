#include "uvhttp_request.h"
#include "uvhttp_utils.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>



int uvhttp_request_init(uvhttp_request_t* request, void* client) {
    if (!request || !client) {
        return -1;
    }
    
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->client = client;
    request->method = UVHTTP_GET; // 默认方法
    
    // 初始化HTTP解析器
    request->parser_settings = uvhttp_malloc(sizeof(llhttp_settings_t));
    if (!request->parser_settings) {
        return -1;
    }
    llhttp_settings_init(request->parser_settings);
    
    request->parser = uvhttp_malloc(sizeof(struct llhttp__internal_s));
    if (!request->parser) {
        uvhttp_free(request->parser_settings);
        return -1;
    }
    llhttp_init(request->parser, HTTP_REQUEST, request->parser_settings);
    
    // 初始化body缓冲区
    request->body_capacity = UVHTTP_INITIAL_BUFFER_SIZE;
    request->body = uvhttp_malloc(request->body_capacity);
    if (!request->body) {
        uvhttp_free(request->parser);
        uvhttp_free(request->parser_settings);
        return -1;
    }
    request->body_length = 0;
    
    return 0;
}

void uvhttp_request_cleanup(uvhttp_request_t* request) {
    if (request->body) {
        uvhttp_free(request->body);
    }
    if (request->parser) {
        uvhttp_free(request->parser);
    }
    if (request->parser_settings) {
        uvhttp_free(request->parser_settings);
    }
}

const char* uvhttp_request_get_method(uvhttp_request_t* request) {
    switch (request->method) {
        case UVHTTP_GET: return "GET";
        case UVHTTP_POST: return "POST";
        case UVHTTP_PUT: return "PUT";
        case UVHTTP_DELETE: return "DELETE";
        case UVHTTP_HEAD: return "HEAD";
        case UVHTTP_OPTIONS: return "OPTIONS";
        case UVHTTP_PATCH: return "PATCH";
        default: return "UNKNOWN";
    }
}

const char* uvhttp_request_get_url(uvhttp_request_t* request) {
    return request->url;
}

const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name) {
    /* 输入验证 */
    if (!request || !name) {
        return NULL;
    }
    
    /* 检查请求是否已初始化 */
    if (!request->headers || request->header_count == 0) {
        return NULL;
    }
    
    /* 验证 header 名称长度和内容 */
    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > UVHTTP_MAX_HEADER_NAME_LENGTH) {
        return NULL;
    }
    
    /* 检查 header 名称是否包含非法字符 */
    for (size_t i = 0; i < name_len; i++) {
        char c = name[i];
        /* HTTP header 名称只能包含特定字符 */
        if (!((c >= 'A' && c <= 'Z') || 
              (c >= 'a' && c <= 'z') || 
              (c >= '0' && c <= '9') || 
              c == '-' || c == '_')) {
            return NULL;
        }
    }
    
    /* 查找 header（不区分大小写） */
    for (size_t i = 0; i < request->header_count; i++) {
        if (request->headers[i].name && 
            strcasecmp(request->headers[i].name, name) == 0) {
            /* 验证 header 值 */
            if (request->headers[i].value && 
                strlen(request->headers[i].value) <= UVHTTP_MAX_HEADER_VALUE_LENGTH) {
                return request->headers[i].value;
            }
        }
    }
    
    return NULL;
}

const char* uvhttp_request_get_body(uvhttp_request_t* request) {
    return request->body;
}

size_t uvhttp_request_get_body_length(uvhttp_request_t* request) {
    return request->body_length;
}

const char* uvhttp_request_get_path(uvhttp_request_t* request) {
    if (!request || !request->url) {
        return "/";
    }
    
    const char* url = request->url;
    const char* query_start = strchr(url, '?');
    
    if (query_start) {
        // 返回路径部分（不包含查询参数）
        static char path_buffer[MAX_URL_LEN];
        size_t path_len = query_start - url;
        if (path_len >= sizeof(path_buffer)) {
            path_len = sizeof(path_buffer) - 1;
        }
        strncpy(path_buffer, url, path_len);
        path_buffer[path_len] = '\0';
        return path_buffer;
    }
    
    return url;
}

const char* uvhttp_request_get_query_string(uvhttp_request_t* request) {
    if (!request || !request->url) {
        return NULL;
    }
    
    const char* query_start = strchr(request->url, '?');
    return query_start ? query_start + 1 : NULL;
}

const char* uvhttp_request_get_query_param(uvhttp_request_t* request, const char* name) {
    if (!request || !name) {
        return NULL;
    }
    
    const char* query_string = uvhttp_request_get_query_string(request);
    if (!query_string) {
        return NULL;
    }
    
    // 简单的查询参数解析
    size_t name_len = strlen(name);
    const char* p = query_string;
    
    while (*p) {
        if (strncmp(p, name, name_len) == 0 && p[name_len] == '=') {
            const char* value = p + name_len + 1;
            const char* end = strchr(value, '&');
            
            static char param_value[MAX_URL_LEN];
            size_t value_len;
            
            if (end) {
                value_len = end - value;
            } else {
                value_len = strlen(value);
            }
            
            if (value_len >= sizeof(param_value)) {
                value_len = sizeof(param_value) - 1;
            }
            
            strncpy(param_value, value, value_len);
            param_value[value_len] = '\0';
            
            return param_value;
        }
        
        p = strchr(p, '&');
        if (!p) break;
        p++;
    }
    
    return NULL;
}

const char* uvhttp_request_get_client_ip(uvhttp_request_t* request) {
    if (!request) {
        return NULL;
    }
    
    // 尝试从X-Forwarded-For头部获取（代理/负载均衡器）
    const char* forwarded_for = uvhttp_request_get_header(request, "X-Forwarded-For");
    if (forwarded_for) {
        // X-Forwarded-For可能包含多个IP，取第一个
        static char client_ip[46]; // IPv6最大长度
        const char* comma = strchr(forwarded_for, ',');
        size_t ip_len;
        
        if (comma) {
            ip_len = comma - forwarded_for;
        } else {
            ip_len = strlen(forwarded_for);
        }
        
        if (ip_len >= sizeof(client_ip)) {
            ip_len = sizeof(client_ip) - 1;
        }
        
        strncpy(client_ip, forwarded_for, ip_len);
        client_ip[ip_len] = '\0';
        return client_ip;
    }
    
    // 尝试从X-Real-IP头部获取
    const char* real_ip = uvhttp_request_get_header(request, "X-Real-IP");
    if (real_ip) {
        return real_ip;
    }
    
    // 从TCP连接获取真实IP（需要访问底层socket）
    if (request->client) {
        struct sockaddr_storage addr;
        int addr_len = sizeof(addr);
        
        if (uv_tcp_getpeername(request->client, (struct sockaddr*)&addr, &addr_len) == 0) {
            static char ip_string[46];
            
            if (addr.ss_family == AF_INET) {
                struct sockaddr_in* addr_in = (struct sockaddr_in*)&addr;
                uv_ip4_name(addr_in, ip_string, sizeof(ip_string));
                return ip_string;
            } else if (addr.ss_family == AF_INET6) {
                struct sockaddr_in6* addr_in6 = (struct sockaddr_in6*)&addr;
                uv_ip6_name(addr_in6, ip_string, sizeof(ip_string));
                return ip_string;
            }
        }
    }
    
    return "127.0.0.1"; // 默认值
}