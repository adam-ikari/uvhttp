#include "uvhttp_request.h"
#include "uvhttp_connection.h"
#include "uvhttp_router.h"
#include "uvhttp_middleware.h"
#include "uvhttp_utils.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_validation.h"
#include "uvhttp_features.h"
#include "uvhttp_error_handler.h"
#include <stdlib.h>
#include "uthash.h"
#include <string.h>
#include <strings.h>
#include <stdio.h>

#if UVHTTP_FEATURE_WEBSOCKET
#include "uvhttp_websocket_impl.h"
#endif

// WebSocket握手检测函数
static int is_websocket_handshake(uvhttp_request_t* request);

// HTTP解析器回调函数声明
static int on_message_begin(llhttp_t* parser);
static int on_url(llhttp_t* parser, const char* at, size_t length);
static int on_header_field(llhttp_t* parser, const char* at, size_t length);
static int on_header_value(llhttp_t* parser, const char* at, size_t length);
static int on_body(llhttp_t* parser, const char* at, size_t length);
static int on_message_complete(llhttp_t* parser);



int uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client) {
    if (!request || !client) {
        return -1;
    }
    
    memset(request, 0, sizeof(uvhttp_request_t));
    
    request->client = client;
    request->method = UVHTTP_GET; // 默认方法
    
    // 初始化HTTP解析器
    request->parser_settings = uvhttp_alloc(sizeof(llhttp_settings_t));
    if (!request->parser_settings) {
        return -1;
    }
    llhttp_settings_init(request->parser_settings);
    
    request->parser = uvhttp_alloc(sizeof(llhttp_t));
    if (!request->parser) {
        uvhttp_free(request->parser_settings);
        return -1;
    }
    
    // 设置回调函数
    request->parser_settings->on_message_begin = on_message_begin;
    request->parser_settings->on_url = on_url;
    request->parser_settings->on_header_field = on_header_field;
    request->parser_settings->on_header_value = on_header_value;
    request->parser_settings->on_body = on_body;
    request->parser_settings->on_message_complete = on_message_complete;
    
    llhttp_init(request->parser, HTTP_REQUEST, request->parser_settings);
    
    // 启用 lenient keep-alive 模式以正确处理 Connection: close 后的数据
    llhttp_set_lenient_keep_alive(request->parser, 1);
    
    // 初始化body缓冲区
    request->body_capacity = UVHTTP_INITIAL_BUFFER_SIZE;
    request->body = uvhttp_alloc(request->body_capacity);
    if (!request->body) {
        uvhttp_free(request->parser);
        uvhttp_free(request->parser_settings);
        return -1;
    }
    request->body_length = 0;
    
    return 0;
}

void uvhttp_request_cleanup(uvhttp_request_t* request) {
    if (!request) {
        return;
    }
    
    if (request->body) {
        uvhttp_free(request->body);
    }
    if (request->parser) {
        uvhttp_free(request->parser);
    }
    if (request->parser_settings) {
        uvhttp_free(request->parser_settings);
    }
    if (request->headers_extra) {
        uvhttp_free(request->headers_extra);
        request->headers_extra = NULL;
        request->headers_extra_count = 0;
    }
}

// HTTP解析器回调函数实现
static int on_message_begin(llhttp_t* parser) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 重置解析状态
    conn->parsing_complete = 0;
    conn->content_length = 0;
    conn->body_received = 0;
    
    return 0;
}

static int on_url(llhttp_t* parser, const char* at, size_t length) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 确保URL长度不超过限制
    if (length >= MAX_URL_LEN) {
        return -1;
    }
    
    // 检查是否超出目标缓冲区大小，确保安全性
    if (length >= sizeof(conn->request->url)) {
        return -1;
    }
    
    memcpy(conn->request->url, at, length);
    conn->request->url[length] = '\0';
    
    return 0;
}

static int on_header_field(llhttp_t* parser, const char* at, size_t length) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    /* 性能优化：只设置长度标记，避免清零整个缓冲区（256字节） */
    conn->current_header_field_len = 0;
    conn->parsing_header_field = UVHTTP_TRUE;
    
    /* 检查header字段名长度限制 */
    if (length >= UVHTTP_MAX_HEADER_NAME_SIZE) {
        return -1;  /* 字段名太长 */
    }
    
    /* 复制header字段名 */
    memcpy(conn->current_header_field, at, length);
    conn->current_header_field_len = length;
    
    return 0;
}

static int on_header_value(llhttp_t* parser, const char* at, size_t length) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 检查header值长度限制
    if (length >= UVHTTP_MAX_HEADER_VALUE_SIZE) {
        return -1;  // 值太长
    }
    
    // 检查是否还有空间存储header
    if (conn->request->header_count >= UVHTTP_MAX_HEADERS_MAX) {
        return -1;  // header数量达到最大限制
    }
    
    // 检查是否需要动态分配额外头部
    if (conn->request->header_count >= MAX_HEADERS_INLINE && !conn->request->headers_extra) {
        // 动态分配额外头部数组
        size_t extra_capacity = UVHTTP_MAX_HEADERS_MAX - MAX_HEADERS_INLINE;
        conn->request->headers_extra = (uvhttp_header_t*)uvhttp_alloc(
            extra_capacity * sizeof(uvhttp_header_t)
        );
        if (!conn->request->headers_extra) {
            return -1;  // 内存分配失败
        }
        conn->request->headers_extra_count = 0;
    }
    
    // 检查当前header字段名是否存在
    if (conn->current_header_field_len == 0) {
        return -1;  // 没有对应的header字段名
    }
    
    // 确定使用哪个头部数组
    uvhttp_header_t* header;
    if (conn->request->header_count < MAX_HEADERS_INLINE) {
        header = &conn->request->headers[conn->request->header_count];
    } else {
        header = &conn->request->headers_extra[conn->request->header_count - MAX_HEADERS_INLINE];
        conn->request->headers_extra_count++;
    }
    
    // 复制header字段名
    size_t field_len = conn->current_header_field_len;
    if (field_len >= sizeof(header->name)) {
        field_len = sizeof(header->name) - 1;
    }
    memcpy(header->name, conn->current_header_field, field_len);
    header->name[field_len] = '\0';
    
    // 复制header值
    if (length >= sizeof(header->value)) {
        length = sizeof(header->value) - 1;
    }
    memcpy(header->value, at, length);
    header->value[length] = '\0';
    
    /* 增加header计数 */
    conn->request->header_count++;
    
    /* 性能优化：只设置长度标记，避免清零整个缓冲区（256字节） */
    conn->current_header_field_len = 0;
    conn->parsing_header_field = 0;
    
    return 0;
}

static int on_body(llhttp_t* parser, const char* at, size_t length) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }
    
    // 检查是否需要扩容body缓冲区
    if (conn->request->body_length + length > conn->request->body_capacity) {
        // 计算新的容量（至少扩容到之前的两倍或满足所需大小）
        size_t new_capacity = conn->request->body_capacity * 2;
        if (new_capacity < conn->request->body_length + length) {
            new_capacity = conn->request->body_length + length;
        }
        
        // 检查是否超过最大限制
        if (new_capacity > UVHTTP_MAX_BODY_SIZE) {
            return -1;  // body太大
        }
        
        // 重新分配内存
        char* new_body = uvhttp_realloc(conn->request->body, new_capacity);
        if (!new_body) {
            return -1;  // 内存分配失败
        }
        
        conn->request->body = new_body;
        conn->request->body_capacity = new_capacity;
    }
    
    // 复制body数据
    memcpy(conn->request->body + conn->request->body_length, at, length);
    conn->request->body_length += length;
    
    return 0;
}

/* 单线程事件驱动的HTTP请求完成处理
 * 在libuv事件循环线程中执行，处理完整的HTTP请求
 * 单线程优势：无竞态条件，请求处理顺序可预测
 */
static int on_message_complete(llhttp_t* parser) {
    if (!parser) {
        return -1;
    }
    
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request || !conn->response) {
        return -1;
    }
    
    /* 防止重复处理：单线程中简单的状态检查就足够 */
    if (conn->parsing_complete) {
        return 0;
    }
    
    /* 设置HTTP方法 - 单线程安全 */
    conn->request->method = (uvhttp_method_t)llhttp_get_method(parser);
    
    /* 标记解析完成 - 无需原子操作 */
    conn->parsing_complete = UVHTTP_TRUE;
    
    /* 重置读缓冲区使用量，为下一个请求做准备 */
    conn->read_buffer_used = 0;
    
#if UVHTTP_FEATURE_RATE_LIMIT
    /* 限流检查 - 在中间件之前执行 */
    if (conn->server && conn->server->rate_limit_enabled) {
        /* 检查客户端IP是否在白名单中 */
        int is_whitelisted = UVHTTP_FALSE;
        if (conn->server->rate_limit_whitelist && conn->server->rate_limit_whitelist_count > 0) {
            /* 获取客户端IP地址 */
            struct sockaddr_in client_addr;
            int addr_len = sizeof(client_addr);
            if (uv_tcp_getpeername(&conn->tcp_handle, (struct sockaddr*)&client_addr, &addr_len) == 0) {
                char client_ip[INET_ADDRSTRLEN];
                uv_inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
                
                /* 检查是否在白名单中 */
                /* 使用哈希表优化白名单查找（O(1) 复杂度） */
                struct whitelist_item *item;
                HASH_FIND_STR(conn->server->rate_limit_whitelist_hash, client_ip, item);
                if (item) {
                    is_whitelisted = UVHTTP_TRUE;
                }
            }
        }
        
        /* 如果不在白名单中，进行限流检查 */
        if (!is_whitelisted && uvhttp_server_check_rate_limit(conn->server) != UVHTTP_OK) {
            /* 超过限流，返回429 Too Many Requests */
            uvhttp_response_set_status(conn->response, 429);
            uvhttp_response_set_header(conn->response, "Content-Type", "text/plain");
            uvhttp_response_set_header(conn->response, "Retry-After", "60");
            uvhttp_response_set_body(conn->response, "Too Many Requests", 18);
            uvhttp_response_send(conn->response);
            return 0;
        }
    }
#endif /* UVHTTP_FEATURE_RATE_LIMIT */
    
    /* 检查是否为WebSocket握手请求 */
    if (is_websocket_handshake(conn->request)) {
        // WebSocket握手需要特殊处理
        // 获取WebSocket Key
        const char* ws_key = uvhttp_request_get_header(conn->request, "Sec-WebSocket-Key");
        if (!ws_key) {
            // 没有WebSocket Key，返回错误响应
            uvhttp_response_set_status(conn->response, 400);
            uvhttp_response_set_header(conn->response, "Content-Type", "text/plain");
            uvhttp_response_set_body(conn->response, "Missing Sec-WebSocket-Key header", 32);
            uvhttp_response_send(conn->response);
            return 0;
        }

        // 发送101 Switching Protocols响应
        uvhttp_response_set_status(conn->response, 101);
        uvhttp_response_set_header(conn->response, "Upgrade", "websocket");
        uvhttp_response_set_header(conn->response, "Connection", "Upgrade");

        // 使用正确的API生成Accept值
        char accept[64];
        if (uvhttp_ws_generate_accept(ws_key, accept, sizeof(accept)) != 0) {
            // 如果生成失败，返回错误响应
            uvhttp_response_set_status(conn->response, 500);
            uvhttp_response_set_header(conn->response, "Content-Type", "text/plain");
            uvhttp_response_set_body(conn->response, "WebSocket handshake failed", 24);
            uvhttp_response_send(conn->response);
            return 0;
        }
        uvhttp_response_set_header(conn->response, "Sec-WebSocket-Accept", accept);

        // 发送握手响应
        uvhttp_response_send(conn->response);

        // 握手成功后，处理WebSocket连接
        int ws_result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
        if (ws_result != 0) {
            UVHTTP_LOG_ERROR("Failed to handle WebSocket handshake: %d\n", ws_result);
            uvhttp_connection_close(conn);
            return 0;
        }

        return 0;
    }
    
    #if UVHTTP_FEATURE_MIDDLEWARE
    /* 执行中间件链 - 零开销设计 */
    if (conn->server && conn->server->middleware_chain) {
        int middleware_result = uvhttp_http_middleware_execute(
            conn->server->middleware_chain,
            conn->request,
            conn->response
        );
        
        /* 如果中间件返回非零，停止执行（中间件已处理响应） */
        if (middleware_result != 0) {
            return 0;
        }
    }
#endif
    
    /* 单线程请求处理 - 无需锁机制 */
    if (conn->server && conn->server->router) {
        // 额外检查request->url是否有效
        if (!conn->request->url[0]) {
            // 如果URL为空，设置为"/"
            uvhttp_safe_strncpy(conn->request->url, "/", sizeof(conn->request->url));
        }
        
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(
            conn->server->router, conn->request->url, 
            uvhttp_method_to_string(conn->request->method));
        
        if (handler) {
            /* 同步执行用户处理器 - 在事件循环线程中 */
            handler(conn->request, conn->response);
        } else {
            /* 未找到路由，发送404响应 */
            uvhttp_response_set_status(conn->response, 404);
            uvhttp_response_set_header(conn->response, "Content-Type", "text/plain");
            uvhttp_response_set_body(conn->response, "Not Found", 9);
            uvhttp_response_send(conn->response);
        }
    } else {
        /* 没有路由器，发送默认响应 */
        uvhttp_response_set_status(conn->response, 200);
        uvhttp_response_set_header(conn->response, "Content-Type", "text/plain");
        uvhttp_response_set_body(conn->response, "OK", 2);
        uvhttp_response_send(conn->response);
    }
    
    return 0;
}

// 检查是否为WebSocket握手请求
static int is_websocket_handshake(uvhttp_request_t* request) {
    const char* upgrade = uvhttp_request_get_header(request, "Upgrade");
    const char* connection = uvhttp_request_get_header(request, "Connection");
    const char* ws_key = uvhttp_request_get_header(request, "Sec-WebSocket-Key");

    // 检查必需的头部
    if (!upgrade || !connection || !ws_key) {
        return UVHTTP_FALSE;
    }

    // 检查Upgrade头部（不区分大小写）
    if (strcasecmp(upgrade, "websocket") != 0) {
        return UVHTTP_FALSE;
    }

    // 检查Connection头部（可能包含多个值）
    if (strstr(connection, "Upgrade") == NULL) {
        return UVHTTP_FALSE;
    }

    return UVHTTP_TRUE;
}

const char* uvhttp_request_get_method(uvhttp_request_t* request) {
    if (!request) return NULL;
    switch (request->method) {
        case UVHTTP_GET: return "GET";
        case UVHTTP_POST: return "POST";
        case UVHTTP_PUT: return "PUT";
        case UVHTTP_DELETE: return "DELETE";
        case UVHTTP_HEAD: return "HEAD";
        case UVHTTP_OPTIONS: return "OPTIONS";
        case UVHTTP_PATCH: return "PATCH";
        case UVHTTP_ANY: return "ANY";
        default: return "UNKNOWN";
    }
}

const char* uvhttp_request_get_url(uvhttp_request_t* request) {
    if (!request) return NULL;
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
    for (size_t index = 0; index < name_len; index++) {
        char char_value = name[index];
        /* HTTP header 名称只能包含特定字符 */
        if (!((char_value >= 'A' && char_value <= 'Z') || 
              (char_value >= 'a' && char_value <= 'z') || 
              (char_value >= '0' && char_value <= '9') || 
              char_value == '-' || char_value == '_')) {
            return NULL;
        }
    }
    
    /* 查找 header（不区分大小写） */
    for (size_t index = 0; index < request->header_count; index++) {
        uvhttp_header_t* header;
        if (index < MAX_HEADERS_INLINE) {
            header = &request->headers[index];
        } else {
            header = &request->headers_extra[index - MAX_HEADERS_INLINE];
        }
        
        if (header->name && 
            strcasecmp(header->name, name) == 0) {
            /* 验证 header 值 */
            if (header->value && 
                strlen(header->value) <= UVHTTP_MAX_HEADER_VALUE_LENGTH) {
                return header->value;
            }
        }
    }
    
    return NULL;
}

const char* uvhttp_request_get_body(uvhttp_request_t* request) {
    if (!request) return NULL;
    return request->body;
}

size_t uvhttp_request_get_body_length(uvhttp_request_t* request) {
    if (!request) return 0;
    return request->body_length;
}

const char* uvhttp_request_get_path(uvhttp_request_t* request) {
    if (!request) {
        return NULL;
    }
    if (!request->url) {
        return "/";
    }
    
    const char* url = request->url;
    const char* query_start = strchr(url, '?');
    
    if (query_start) {
        // 返回路径部分（不包含查询参数）
        static char path_buffer[UVHTTP_MAX_PATH_SIZE];
        size_t path_len = query_start - url;
        if (path_len >= sizeof(path_buffer)) {
            path_len = sizeof(path_buffer) - 1;
        }
        uvhttp_safe_strncpy(path_buffer, url, path_len + 1);
        
        // 验证路径安全性
        if (!uvhttp_validate_url_path(path_buffer)) {
            return "/";
        }
        
        return path_buffer;
    }
    
    return url;
}

const char* uvhttp_request_get_query_string(uvhttp_request_t* request) {
    if (!request || !request->url) {
        return NULL;
    }
    
    const char* query_start = strchr(request->url, '?');
    const char* query_string = query_start ? query_start + 1 : NULL;
    
    // 验证查询字符串安全性
    if (query_string && !uvhttp_validate_query_string(query_string)) {
        return NULL;
    }
    
    return query_string;
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
            
            static char param_value[UVHTTP_MAX_URL_SIZE];
            size_t value_len;
            
            if (end) {
                value_len = end - value;
            } else {
                value_len = strlen(value);
            }
            
            if (value_len >= sizeof(param_value)) {
                value_len = sizeof(param_value) - 1;
            }
            
            uvhttp_safe_strncpy(param_value, value, value_len + 1);
            
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
        static char client_ip[UVHTTP_IPV6_MAX_STRING_LENGTH];
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
        
        uvhttp_safe_strncpy(client_ip, forwarded_for, ip_len + 1);
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
            static char ip_string[UVHTTP_IPV6_MAX_STRING_LENGTH];
            
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

void uvhttp_request_free(uvhttp_request_t* request) {
    if (!request) {
        return;
    }
    
    uvhttp_request_cleanup(request);
    uvhttp_free(request);
}