#include "uvhttp_request.h"

#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_features.h"
#include "uvhttp_logging.h"
#include "uvhttp_middleware.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"
#include "uvhttp_validation.h"

#include "uthash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if UVHTTP_FEATURE_WEBSOCKET
#    include "uvhttp_websocket.h"
#endif

/* HTTP 响应相关字符串常量 */
#define HTTP_CONTENT_TYPE_TEXT_PLAIN "text/plain"
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_UPGRADE "Upgrade"
#define HTTP_HEADER_CONNECTION "Connection"
#define HTTP_HEADER_SEC_WEBSOCKET_KEY "Sec-WebSocket-Key"
#define HTTP_HEADER_SEC_WEBSOCKET_ACCEPT "Sec-WebSocket-Accept"
#define HTTP_HEADER_RETRY_AFTER "Retry-After"
#define HTTP_HEADER_X_FORWARDED_FOR "X-Forwarded-For"
#define HTTP_HEADER_X_REAL_IP "X-Real-IP"
#define HTTP_VALUE_WEBSOCKET "websocket"
#define HTTP_VALUE_ROOT_PATH "/"
#define HTTP_VALUE_RETRY_AFTER_SECONDS "60"
#define HTTP_VALUE_DEFAULT_IP "127.0.0.1"
#define HTTP_RESPONSE_OK "OK"
#define HTTP_RESPONSE_NOT_FOUND "Not Found"
#define HTTP_RESPONSE_TOO_MANY_REQUESTS "Too Many Requests"
#define HTTP_RESPONSE_WS_HANDSHAKE_FAILED "WebSocket handshake failed"
#define HTTP_RESPONSE_WS_KEY_MISSING "Missing Sec-WebSocket-Key header"

/* WebSocket握手检测函数 */
static int is_websocket_handshake(uvhttp_request_t* request);

// HTTP解析器回调函数声明
static int on_message_begin(llhttp_t* parser);
static int on_url(llhttp_t* parser, const char* at, size_t length);
static int on_header_field(llhttp_t* parser, const char* at, size_t length);
static int on_header_value(llhttp_t* parser, const char* at, size_t length);
static int on_body(llhttp_t* parser, const char* at, size_t length);
static int on_message_complete(llhttp_t* parser);

#if UVHTTP_FEATURE_RATE_LIMIT
static int check_rate_limit_whitelist(uvhttp_connection_t* conn);
static int is_client_whitelisted(uvhttp_connection_t* conn);
#endif
static int handle_websocket_handshake_request(uvhttp_connection_t* conn);
static void ensure_valid_url(uvhttp_request_t* request);

uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request,
                                   uv_tcp_t* client) {
    if (!request || !client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(request, 0, sizeof(uvhttp_request_t));

    request->client = client;
    request->method = UVHTTP_GET;   // 默认方法
    request->headers_capacity = 32; /* 初始容量：32个内联 headers */

    // 初始化HTTP解析器
    request->parser_settings = uvhttp_alloc(sizeof(llhttp_settings_t));
    if (!request->parser_settings) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    llhttp_settings_init(request->parser_settings);

    request->parser = uvhttp_alloc(sizeof(llhttp_t));
    if (!request->parser) {
        uvhttp_free(request->parser_settings);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
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
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    request->body_length = 0;

    return UVHTTP_OK;
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
    }
}

// HTTP解析器回调函数实现
static int on_message_begin(llhttp_t* parser) {

    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        UVHTTP_LOG_ERROR("on_message_begin: conn or request is NULL\n");
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
        UVHTTP_LOG_ERROR("on_url: conn or request is NULL\n");
        return -1;
    }

    // 确保URL长度不超过限制
    if (length >= MAX_URL_LEN) {
        UVHTTP_LOG_ERROR("on_url: URL too long: %zu\n", length);
        return -1;
    }

    // 检查是否超出目标缓冲区大小，确保安全性
    if (length >= sizeof(conn->request->url)) {
        UVHTTP_LOG_ERROR("on_url: URL exceeds buffer size: %zu\n", length);
        return -1;
    }

    memcpy(conn->request->url, at, length);
    conn->request->url[length] = '\0';

    return 0;
}

static int on_header_field(llhttp_t* parser, const char* at, size_t length) {

    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        UVHTTP_LOG_ERROR("on_header_field: conn or request is NULL\n");
        return -1;
    }

    /* 性能优化：只设置长度标记，避免清零整个缓冲区（256字节） */
    conn->current_header_field_len = 0;
    conn->parsing_header_field = 1;

    /* 检查header字段名长度限制 */
    if (length >= UVHTTP_MAX_HEADER_NAME_SIZE) {
        UVHTTP_LOG_ERROR("on_header_field: header name too long: %zu\n",
                         length);
        return -1; /* 字段名太长 */
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

    // 检查当前header字段名是否存在
    if (conn->current_header_field_len == 0) {
        return -1;  // 没有对应的header字段名
    }

    // 构造 header 名称和值
    char header_name[UVHTTP_MAX_HEADER_NAME_SIZE];
    size_t field_len = conn->current_header_field_len;
    if (field_len >= sizeof(header_name)) {
        field_len = sizeof(header_name) - 1;
    }
    memcpy(header_name, conn->current_header_field, field_len);
    header_name[field_len] = '\0';

    char header_value[UVHTTP_MAX_HEADER_VALUE_SIZE];
    size_t value_len = length;
    if (value_len >= sizeof(header_value)) {
        value_len = sizeof(header_value) - 1;
    }
    memcpy(header_value, at, value_len);
    header_value[value_len] = '\0';

    // 使用新的 API 添加 header

    if (uvhttp_request_add_header(conn->request, header_name, header_value) !=
        0) {
        return -1;  // 添加失败
    }

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

#if UVHTTP_FEATURE_RATE_LIMIT
/* 检查客户端是否在白名单中 */
static int is_client_whitelisted(uvhttp_connection_t* conn) {
    if (!conn->server->rate_limit_whitelist ||
        conn->server->rate_limit_whitelist_count == 0) {
        return 0;
    }

    struct sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    if (uv_tcp_getpeername(&conn->tcp_handle, (struct sockaddr*)&client_addr,
                           &addr_len) != 0) {
        return 0;
    }

    char client_ip[INET_ADDRSTRLEN];
    uv_inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    struct whitelist_item* item;
    HASH_FIND_STR(conn->server->rate_limit_whitelist_hash, client_ip, item);
    return item != NULL;
}

/* 检查并执行限流 */
static int check_rate_limit_whitelist(uvhttp_connection_t* conn) {
    if (!conn->server || !conn->server->rate_limit_enabled) {
        return 0;
    }

    if (is_client_whitelisted(conn)) {
        return 0;
    }

    if (uvhttp_server_check_rate_limit(conn->server) != UVHTTP_OK) {
        uvhttp_response_set_status(conn->response, 429);
        uvhttp_response_set_header(conn->response, HTTP_HEADER_CONTENT_TYPE,
                                   HTTP_CONTENT_TYPE_TEXT_PLAIN);
        uvhttp_response_set_header(conn->response, HTTP_HEADER_RETRY_AFTER,
                                   HTTP_VALUE_RETRY_AFTER_SECONDS);
        uvhttp_response_set_body(conn->response,
                                 HTTP_RESPONSE_TOO_MANY_REQUESTS,
                                 strlen(HTTP_RESPONSE_TOO_MANY_REQUESTS));
        uvhttp_response_send(conn->response);
        return -1;
    }

    return 0;
}
#endif

/* 处理 WebSocket 握手请求 */
static int handle_websocket_handshake_request(uvhttp_connection_t* conn) {
    const char* ws_key =
        uvhttp_request_get_header(conn->request, HTTP_HEADER_SEC_WEBSOCKET_KEY);
    if (!ws_key) {
        uvhttp_response_set_status(conn->response, 400);
        uvhttp_response_set_header(conn->response, HTTP_HEADER_CONTENT_TYPE,
                                   HTTP_CONTENT_TYPE_TEXT_PLAIN);
        uvhttp_response_set_body(conn->response, HTTP_RESPONSE_WS_KEY_MISSING,
                                 strlen(HTTP_RESPONSE_WS_KEY_MISSING));
        uvhttp_response_send(conn->response);
        return 0;
    }

    uvhttp_response_set_status(conn->response, 101);
    uvhttp_response_set_header(conn->response, HTTP_HEADER_UPGRADE,
                               HTTP_VALUE_WEBSOCKET);
    uvhttp_response_set_header(conn->response, HTTP_HEADER_CONNECTION,
                               HTTP_HEADER_UPGRADE);

    char accept[64];
    if (uvhttp_ws_generate_accept(ws_key, accept, sizeof(accept)) != 0) {
        uvhttp_response_set_status(conn->response, 500);
        uvhttp_response_set_header(conn->response, HTTP_HEADER_CONTENT_TYPE,
                                   HTTP_CONTENT_TYPE_TEXT_PLAIN);
        uvhttp_response_set_body(conn->response,
                                 HTTP_RESPONSE_WS_HANDSHAKE_FAILED,
                                 strlen(HTTP_RESPONSE_WS_HANDSHAKE_FAILED));
        uvhttp_response_send(conn->response);
        return 0;
    }

    uvhttp_response_set_header(conn->response, HTTP_HEADER_SEC_WEBSOCKET_ACCEPT,
                               accept);
    uvhttp_response_send(conn->response);

    int ws_result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    if (ws_result != 0) {
        UVHTTP_LOG_ERROR("Failed to handle WebSocket handshake: %d\n",
                         ws_result);
        uvhttp_connection_close(conn);
    }

    return ws_result;
}

/* 确保 URL 有效，如果为空则设置为 "/" */
static void ensure_valid_url(uvhttp_request_t* request) {
    if (!request->url[0]) {
        strncpy(request->url, HTTP_VALUE_ROOT_PATH, sizeof(request->url) - 1);
        request->url[sizeof(request->url) - 1] = '\0';
    }
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

    /* 防止重复处理 */
    if (conn->parsing_complete) {
        return 0;
    }

    /* 设置HTTP方法 */
    conn->request->method = (uvhttp_method_t)llhttp_get_method(parser);
    conn->parsing_complete = 1;
    conn->read_buffer_used = 0;

#if UVHTTP_FEATURE_RATE_LIMIT
    /* 限流检查 */
    if (check_rate_limit_whitelist(conn) != 0) {
        return 0;
    }
#endif

    /* WebSocket 握手 */
    if (is_websocket_handshake(conn->request)) {
        return handle_websocket_handshake_request(conn);
    }

    /* 路由处理 */
    if (conn->server && conn->server->router) {
        ensure_valid_url(conn->request);

        uvhttp_request_handler_t handler = uvhttp_router_find_handler(
            conn->server->router, conn->request->url,
            uvhttp_method_to_string(conn->request->method));

        if (handler) {
            handler(conn->request, conn->response);
        } else {
            uvhttp_response_set_status(conn->response, 404);
            uvhttp_response_set_header(conn->response, HTTP_HEADER_CONTENT_TYPE,
                                       HTTP_CONTENT_TYPE_TEXT_PLAIN);
            uvhttp_response_set_body(conn->response, HTTP_RESPONSE_NOT_FOUND,
                                     strlen(HTTP_RESPONSE_NOT_FOUND));
            uvhttp_response_send(conn->response);
        }
    } else {
        /* 没有路由器，发送默认响应 */
        uvhttp_response_set_status(conn->response, 200);
        uvhttp_response_set_header(conn->response, HTTP_HEADER_CONTENT_TYPE,
                                   HTTP_CONTENT_TYPE_TEXT_PLAIN);
        uvhttp_response_set_body(conn->response, HTTP_RESPONSE_OK,
                                 strlen(HTTP_RESPONSE_OK));
        uvhttp_response_send(conn->response);
    }

    return 0;
}

// 检查是否为WebSocket握手请求
static int is_websocket_handshake(uvhttp_request_t* request) {
    const char* upgrade =
        uvhttp_request_get_header(request, HTTP_HEADER_UPGRADE);
    const char* connection =
        uvhttp_request_get_header(request, HTTP_HEADER_CONNECTION);
    const char* ws_key =
        uvhttp_request_get_header(request, HTTP_HEADER_SEC_WEBSOCKET_KEY);

    // 检查必需的头部
    if (!upgrade || !connection || !ws_key) {
        return FALSE;
    }

    /* 检查Upgrade头部（不区分大小写） */
    if (strcasecmp(upgrade, HTTP_VALUE_WEBSOCKET) != 0) {
        return FALSE;
    }

    /* 检查Connection头部（可能包含多个值） */
    if (strstr(connection, HTTP_HEADER_UPGRADE) == NULL) {
        return FALSE;
    }

    return TRUE;
}

const char* uvhttp_request_get_url(uvhttp_request_t* request) {
    if (!request)
        return NULL;
    return request->url;
}

const char* uvhttp_request_get_header(uvhttp_request_t* request,
                                      const char* name) {
    /* 输入验证 */
    if (!request || !name) {
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
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) {
            return NULL;
        }
    }

    /* 查找 header（不区分大小写） */
    for (size_t i = 0; i < request->header_count; i++) {
        uvhttp_header_t* header = uvhttp_request_get_header_at(request, i);
        if (header && strcasecmp(header->name, name) == 0) {
            /* 验证 header 值 */
            if (strlen(header->value) <= UVHTTP_MAX_HEADER_VALUE_LENGTH) {
                return header->value;
            }
        }
    }

    return NULL;
}

const char* uvhttp_request_get_body(uvhttp_request_t* request) {
    if (!request)
        return NULL;
    return request->body;
}

size_t uvhttp_request_get_body_length(uvhttp_request_t* request) {
    if (!request)
        return 0;
    return request->body_length;
}

const char* uvhttp_request_get_path(uvhttp_request_t* request) {
    if (!request) {
        return NULL;
    }

    const char* url = request->url;
    const char* query_start = strchr(url, '?');

    if (query_start) {
        // 返回路径部分（不包含查询参数）
        static char path_buffer[UVHTTP_MAX_PATH_SIZE];
        size_t path_length = query_start - url;

        // 确保路径长度不超过缓冲区大小
        if (path_length >= sizeof(path_buffer)) {
            // 路径太长，返回根路径
            return "/";
        }

        // 复制路径部分（不包含查询参数）
        memcpy(path_buffer, url, path_length);
        path_buffer[path_length] = '\0';

        // 验证路径安全性
        if (!uvhttp_validate_url_path(path_buffer)) {
            return "/";
        }

        return path_buffer;
    }

    return url;
}

const char* uvhttp_request_get_query_string(uvhttp_request_t* request) {
    if (!request) {
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

const char* uvhttp_request_get_query_param(uvhttp_request_t* request,
                                           const char* name) {
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

            strncpy(param_value, value, value_len);
            param_value[value_len] = '\0';

            return param_value;
        }

        p = strchr(p, '&');
        if (!p)
            break;
        p++;
    }

    return NULL;
}

const char* uvhttp_request_get_client_ip(uvhttp_request_t* request) {
    if (!request) {
        return NULL;
    }

    // 尝试从X-Forwarded-For头部获取（代理/负载均衡器）
    const char* forwarded_for =
        uvhttp_request_get_header(request, HTTP_HEADER_X_FORWARDED_FOR);
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

        strncpy(client_ip, forwarded_for, ip_len);
        client_ip[ip_len] = '\0';
        return client_ip;
    }

    // 尝试从X-Real-IP头部获取
    const char* real_ip = uvhttp_request_get_header(request, HTTP_HEADER_X_REAL_IP);
    if (real_ip) {
        return real_ip;
    }

    // 从TCP连接获取真实IP（需要访问底层socket）
    if (request->client) {
        struct sockaddr_storage addr;
        int addr_len = sizeof(addr);

        if (uv_tcp_getpeername(request->client, (struct sockaddr*)&addr,
                               &addr_len) == 0) {
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

    return HTTP_VALUE_DEFAULT_IP;
}

void uvhttp_request_free(uvhttp_request_t* request) {
    if (!request) {
        return;
    }

    uvhttp_request_cleanup(request);
    uvhttp_free(request);
}

/* ========== Headers 操作 API 实现 ========== */

/* 获取 header 数量 */
size_t uvhttp_request_get_header_count(uvhttp_request_t* request) {
    if (!request) {
        return 0;
    }
    return request->header_count;
}

/* 获取指定索引的 header（内部使用） */
uvhttp_header_t* uvhttp_request_get_header_at(uvhttp_request_t* request,
                                              size_t index) {
    if (!request || index >= request->headers_capacity) {
        return NULL;
    }

    /* 检查是否在内联数组中 */
    if (index < UVHTTP_INLINE_HEADERS_CAPACITY) {
        return &request->headers[index];
    }

    /* 在动态扩容数组中 */
    if (request->headers_extra) {
        return &request->headers_extra[index - UVHTTP_INLINE_HEADERS_CAPACITY];
    }

    return NULL;
}

/* 添加 header（内部使用，自动扩容） */
uvhttp_error_t uvhttp_request_add_header(uvhttp_request_t* request,
                                         const char* name, const char* value) {

    if (!request || !name || !value) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    /* 检查是否需要扩容 */

    if (request->header_count >= request->headers_capacity) {

        /* 计算新容量（最多 MAX_HEADERS） */
        size_t new_capacity = request->headers_capacity * 2;
        if (new_capacity == 0) {
            new_capacity = UVHTTP_INLINE_HEADERS_CAPACITY; /* 初始容量 */
        }
        if (new_capacity > MAX_HEADERS) {
            new_capacity = MAX_HEADERS;
        }

        /* 如果新容量等于当前容量，说明已达到最大值 */
        if (new_capacity == request->headers_capacity) {
            return UVHTTP_ERROR_BUFFER_TOO_SMALL; /* 已满 */
        }

        /* 分配或重新分配动态数组 */
        size_t extra_count = new_capacity - UVHTTP_INLINE_HEADERS_CAPACITY;
        int is_first_alloc = (request->headers_extra == NULL);

        uvhttp_header_t* new_extra = uvhttp_realloc(
            request->headers_extra, extra_count * sizeof(uvhttp_header_t));
        if (!new_extra) {
            return UVHTTP_ERROR_OUT_OF_MEMORY; /* 内存分配失败 */
        }

        /* 如果是首次分配，清零新分配的内存 */
        if (is_first_alloc) {
            memset(new_extra, 0, extra_count * sizeof(uvhttp_header_t));
        }

        request->headers_extra = new_extra;
        request->headers_capacity = new_capacity;
    }

    /* 获取 header 指针 */
    uvhttp_header_t* header =
        uvhttp_request_get_header_at(request, request->header_count);
    if (!header) {
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* 复制 header 名称 */
    size_t name_len = strlen(name);
    if (name_len >= sizeof(header->name)) {
        name_len = sizeof(header->name) - 1;
    }
    memcpy(header->name, name, name_len);
    header->name[name_len] = '\0';

    /* 复制 header 值 */
    size_t value_len = strlen(value);
    if (value_len >= sizeof(header->value)) {
        value_len = sizeof(header->value) - 1;
    }
    memcpy(header->value, value, value_len);
    header->value[value_len] = '\0';

    /* 增加计数 */
    request->header_count++;

    return UVHTTP_OK;
}

/* 遍历所有 headers */
void uvhttp_request_foreach_header(uvhttp_request_t* request,
                                   uvhttp_header_callback_t callback,
                                   void* user_data) {
    if (!request || !callback) {
        return;
    }

    for (size_t i = 0; i < request->header_count; i++) {
        uvhttp_header_t* header = uvhttp_request_get_header_at(request, i);
        if (header) {
            callback(header->name, header->value, user_data);
        }
    }
}