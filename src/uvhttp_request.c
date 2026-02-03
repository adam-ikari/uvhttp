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
#include "uvhttp_static.h"
#include "uvhttp_utils.h"
#include "uvhttp_validation.h"

#if UVHTTP_FEATURE_PROTOCOL_UPGRADE
#    include "uvhttp_protocol_upgrade.h"
#endif

#include "uthash.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#if UVHTTP_FEATURE_WEBSOCKET
#    include "uvhttp_websocket.h"
#endif

/* HTTP response related string constants */

// HTTP parser callback function declarations
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
static void ensure_valid_url(uvhttp_request_t* request);

uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request,
                                   uv_tcp_t* client) {
    if (!request || !client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(request, 0, sizeof(uvhttp_request_t));

    request->client = client;
    request->method = UVHTTP_GET;   // defaultmethod
    request->headers_capacity = 32; /* initial capacity: 32 inline headers */

    // initialize HTTP parser
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

    // setcallbackfunction
    request->parser_settings->on_message_begin = on_message_begin;
    request->parser_settings->on_url = on_url;
    request->parser_settings->on_header_field = on_header_field;
    request->parser_settings->on_header_value = on_header_value;
    request->parser_settings->on_body = on_body;
    request->parser_settings->on_message_complete = on_message_complete;

    llhttp_init(request->parser, HTTP_REQUEST, request->parser_settings);

    // enable lenient keep-alive pattern to correctly process data after
    // Connection: close
    llhttp_set_lenient_keep_alive(request->parser, 1);

    // initializebodybuffer
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

// HTTP parser callback function implementation
static int on_message_begin(llhttp_t* parser) {

    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        UVHTTP_LOG_ERROR("on_message_begin: conn or request is NULL\n");
        return -1;
    }

    // resetparsestate
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

    // ensure URL length does not exceed limit
    if (length >= MAX_URL_LEN) {
        UVHTTP_LOG_ERROR("on_url: URL too long: %zu\n", length);
        return -1;
    }

    // check if exceeds target buffer size, ensure safety
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

    /* performance optimization: only set length marker, avoid zeroing entire
     * buffer (256 bytes) */
    conn->current_header_field_len = 0;
    conn->parsing_header_field = 1;

    /* check header field name length limit */
    if (length >= UVHTTP_MAX_HEADER_NAME_SIZE) {
        UVHTTP_LOG_ERROR("on_header_field: header name too long: %zu\n",
                         length);
        return -1; /* field name too long */
    }

    /* copy header field name */
    memcpy(conn->current_header_field, at, length);
    conn->current_header_field_len = length;

    return 0;
}

static int on_header_value(llhttp_t* parser, const char* at, size_t length) {

    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }

    // checkheadervaluelengthlimit
    if (length >= UVHTTP_MAX_HEADER_VALUE_SIZE) {
        return -1;  // value too long
    }

    // check if current header field name exists
    if (conn->current_header_field_len == 0) {
        return -1;  // no corresponding header field name
    }

    // construct header name and value
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

    // use new API to add header

    if (uvhttp_request_add_header(conn->request, header_name, header_value) !=
        0) {
        return -1;  // addfailure
    }

    /* performance optimization: only set length marker, avoid zeroing entire
     * buffer (256 bytes) */
    conn->current_header_field_len = 0;
    conn->parsing_header_field = 0;

    return 0;
}

static int on_body(llhttp_t* parser, const char* at, size_t length) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request) {
        return -1;
    }

    // check if need to expand body buffer
    if (conn->request->body_length + length > conn->request->body_capacity) {
        // calculate new capacity (at least double previous size or meet
        // required size)
        size_t new_capacity = conn->request->body_capacity * 2;
        if (new_capacity < conn->request->body_length + length) {
            new_capacity = conn->request->body_length + length;
        }

        // check if exceeds maximum limit
        if (new_capacity > UVHTTP_MAX_BODY_SIZE) {
            return -1;  // body too large
        }

        // reallocate memory
        char* new_body = uvhttp_realloc(conn->request->body, new_capacity);
        if (!new_body) {
            return -1;  // memoryallocatefailure
        }

        conn->request->body = new_body;
        conn->request->body_capacity = new_capacity;
    }

    // copybodydata
    memcpy(conn->request->body + conn->request->body_length, at, length);
    conn->request->body_length += length;

    return 0;
}

#if UVHTTP_FEATURE_RATE_LIMIT
/* check if client is in whitelist */
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

/* check and execute rate limiting */
static int check_rate_limit_whitelist(uvhttp_connection_t* conn) {
    if (!conn->server || !conn->server->rate_limit_enabled) {
        return 0;
    }

    if (is_client_whitelisted(conn)) {
        return 0;
    }

    if (uvhttp_server_check_rate_limit(conn->server) != UVHTTP_OK) {
        uvhttp_response_set_status(conn->response, 429);
        uvhttp_response_set_header(conn->response, UVHTTP_HEADER_CONTENT_TYPE,
                                   UVHTTP_CONTENT_TYPE_TEXT);
        uvhttp_response_set_header(conn->response, UVHTTP_HEADER_RETRY_AFTER,
                                   UVHTTP_VALUE_RETRY_AFTER_SECONDS);
        uvhttp_response_set_body(conn->response,
                                 UVHTTP_MESSAGE_TOO_MANY_REQUESTS,
                                 strlen(UVHTTP_MESSAGE_TOO_MANY_REQUESTS));
        uvhttp_response_send(conn->response);
        return -1;
    }

    return 0;
}
#endif

/* process WebSocket handshakerequest */

/* ensure URL is valid, if null then set to "/" */
static void ensure_valid_url(uvhttp_request_t* request) {
    if (!request->url[0]) {
        strncpy(request->url, UVHTTP_VALUE_ROOT_PATH, sizeof(request->url) - 1);
        request->url[sizeof(request->url) - 1] = '\0';
    }
}

/* single-threaded event-driven HTTP request complete processing
 * executed in libuv event loop thread, process complete HTTP request
 * single-thread advantage: no race condition, request processing order is
 * predictable
 */
static int on_message_complete(llhttp_t* parser) {

    if (!parser) {
        return -1;
    }

    uvhttp_connection_t* conn = (uvhttp_connection_t*)parser->data;
    if (!conn || !conn->request || !conn->response) {
        return -1;
    }

    /* prevent duplicate processing */
    if (conn->parsing_complete) {
        return 0;
    }

    /* setHTTPmethod */
    conn->request->method = (uvhttp_method_t)llhttp_get_method(parser);
    conn->parsing_complete = 1;
    conn->read_buffer_used = 0;

#if UVHTTP_FEATURE_RATE_LIMIT
    /* rate limiting check */
    if (check_rate_limit_whitelist(conn) != 0) {
        return 0;
    }
#endif

#if UVHTTP_FEATURE_PROTOCOL_UPGRADE
    /* Fast path: check if Upgrade header is present */
    const char* upgrade_header =
        uvhttp_request_get_header(conn->request, UVHTTP_HEADER_UPGRADE);
    if (upgrade_header) {
        /* Protocol upgrade detection */
        if (conn->server && conn->server->protocol_registry) {
            char protocol_name[32];

            uvhttp_protocol_registry_t* registry =
                (uvhttp_protocol_registry_t*)conn->server->protocol_registry;

            /* Iterate through registered protocols */
            for (uvhttp_protocol_info_t* proto = registry->protocols;
                 proto != NULL; proto = proto->next) {
                /* Call protocol detector */
                if (proto->detector(conn->request, protocol_name,
                                    sizeof(protocol_name))) {
                    /* Call upgrade handler */
                    uvhttp_error_t result =
                        proto->handler(conn, protocol_name, proto->user_data);

                    if (result == UVHTTP_OK) {
                        /* Upgrade successful, stop HTTP processing */
                        return 0;
                    } else {
                        /* Upgrade failed, return error response */
                        UVHTTP_LOG_ERROR("Protocol upgrade failed: %s",
                                         uvhttp_error_string(result));
                        uvhttp_response_set_status(conn->response, 400);
                        uvhttp_response_set_header(conn->response,
                                                   UVHTTP_HEADER_CONTENT_TYPE,
                                                   UVHTTP_CONTENT_TYPE_TEXT);
                        uvhttp_response_set_body(conn->response,
                                                 "Protocol upgrade failed",
                                                 strlen("Protocol upgrade failed"));
                        uvhttp_response_send(conn->response);
                        return 0;
                    }
                }
            }
        }
    }
#endif

    /* routerprocess */
    if (conn->server && conn->server->router) {
        ensure_valid_url(conn->request);

        uvhttp_request_handler_t handler = uvhttp_router_find_handler(
            conn->server->router, conn->request->url,
            uvhttp_method_to_string(conn->request->method));

        if (handler) {
            handler(conn->request, conn->response);
        } else if (conn->server->router->static_context) {
            /* if no handler found but have static file context, attempt static
             * file processing */
            uvhttp_result_t result = uvhttp_static_handle_request(
                (uvhttp_static_context_t*)conn->server->router->static_context,
                conn->request, conn->response);

            if (result != UVHTTP_OK) {
                uvhttp_response_set_status(conn->response, 404);
                uvhttp_response_set_header(conn->response,
                                           UVHTTP_HEADER_CONTENT_TYPE,
                                           UVHTTP_CONTENT_TYPE_TEXT);
                uvhttp_response_set_body(conn->response,
                                         UVHTTP_MESSAGE_NOT_FOUND,
                                         strlen(UVHTTP_MESSAGE_NOT_FOUND));
                uvhttp_response_send(conn->response);
            }
        } else {
            uvhttp_response_set_status(conn->response, 404);
            uvhttp_response_set_header(conn->response,
                                       UVHTTP_HEADER_CONTENT_TYPE,
                                       UVHTTP_CONTENT_TYPE_TEXT);
            uvhttp_response_set_body(conn->response, UVHTTP_MESSAGE_NOT_FOUND,
                                     strlen(UVHTTP_MESSAGE_NOT_FOUND));
            uvhttp_response_send(conn->response);
        }
    } else {
        /* no router, send default response */
        uvhttp_response_set_status(conn->response, 200);
        uvhttp_response_set_header(conn->response, UVHTTP_HEADER_CONTENT_TYPE,
                                   UVHTTP_CONTENT_TYPE_TEXT);
        uvhttp_response_set_body(conn->response, UVHTTP_MESSAGE_OK,
                                 strlen(UVHTTP_MESSAGE_OK));
        uvhttp_response_send(conn->response);
    }

    return 0;
}

const char* uvhttp_request_get_method(uvhttp_request_t* request) {
    if (!request)
        return NULL;

    /* map uvhttp_method_t to llhttp_method_t */
    llhttp_method_t method;
    switch (request->method) {
    case UVHTTP_GET:
        method = HTTP_GET;
        break;
    case UVHTTP_POST:
        method = HTTP_POST;
        break;
    case UVHTTP_PUT:
        method = HTTP_PUT;
        break;
    case UVHTTP_DELETE:
        method = HTTP_DELETE;
        break;
    case UVHTTP_HEAD:
        method = HTTP_HEAD;
        break;
    case UVHTTP_OPTIONS:
        method = HTTP_OPTIONS;
        break;
    case UVHTTP_PATCH:
        method = HTTP_PATCH;
        break;
    case UVHTTP_ANY:
        return "ANY";
    default:
        return "UNKNOWN";
    }

    return llhttp_method_name(method);
}

const char* uvhttp_request_get_url(uvhttp_request_t* request) {
    if (!request)
        return NULL;
    return request->url;
}

const char* uvhttp_request_get_header(uvhttp_request_t* request,
                                      const char* name) {
    /* inputverify */
    if (!request || !name) {
        return NULL;
    }

    /* verify header name length and content */
    size_t name_len = strlen(name);
    if (name_len == 0 || name_len > UVHTTP_MAX_HEADER_NAME_LENGTH) {
        return NULL;
    }

    /* check if header name contains illegal characters */
    for (size_t i = 0; i < name_len; i++) {
        char c = name[i];
        /* HTTP header name can only contain specific characters */
        if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
              (c >= '0' && c <= '9') || c == '-' || c == '_')) {
            return NULL;
        }
    }

    /* find header (case-insensitive) */
    for (size_t i = 0; i < request->header_count; i++) {
        uvhttp_header_t* header = uvhttp_request_get_header_at(request, i);
        if (header && strcasecmp(header->name, name) == 0) {
            /* verify header value */
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
        // return path part (without query parameters)
        static char path_buffer[UVHTTP_MAX_PATH_SIZE];
        size_t path_length = query_start - url;

        // ensure path length does not exceed buffer size
        if (path_length >= sizeof(path_buffer)) {
            // path too long, return root path
            return "/";
        }

        // copy path part (without query parameters)
        memcpy(path_buffer, url, path_length);
        path_buffer[path_length] = '\0';

        // verify path safety
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

    // verify query string safety
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

    // simple query parameter parsing
    size_t name_len = strlen(name);
    const char* p = query_string;

    while (*p) {
        if (strncmp(p, name, name_len) == 0 && p[name_len] == '=') {
            const char* value = p + name_len + 1;
            const char* end = strchr(value, '&');

            /* Static buffer is safe in single-threaded event loop architecture.
             * UVHTTP uses single-threaded design, so this buffer is only
             * accessed by one thread at a time. The returned pointer is only
             * valid until the next call to this function. */
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

    // attempt to get from X-Forwarded-For header (proxy/load balancer)
    const char* forwarded_for =
        uvhttp_request_get_header(request, UVHTTP_HEADER_X_FORWARDED_FOR);
    if (forwarded_for) {
        // X-Forwarded-For may contain multiple IPs, take the first one
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

    // attempt to get from X-Real-IP header
    const char* real_ip =
        uvhttp_request_get_header(request, UVHTTP_HEADER_X_REAL_IP);
    if (real_ip) {
        return real_ip;
    }

    // get real IP from TCP connection (need to access underlying socket)
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

    return UVHTTP_VALUE_DEFAULT_IP;
}

void uvhttp_request_free(uvhttp_request_t* request) {
    if (!request) {
        return;
    }

    uvhttp_request_cleanup(request);
    uvhttp_free(request);
}

/* ========== Headers operation API implement ========== */

/* get header count */
size_t uvhttp_request_get_header_count(uvhttp_request_t* request) {
    if (!request) {
        return 0;
    }
    return request->header_count;
}

/* get header at specified index (internal use) */
uvhttp_header_t* uvhttp_request_get_header_at(uvhttp_request_t* request,
                                              size_t index) {
    if (!request || index >= request->headers_capacity) {
        return NULL;
    }

    /* check if in inline array */
    if (index < UVHTTP_INLINE_HEADERS_CAPACITY) {
        return &request->headers[index];
    }

    /* in dynamically expanded array */
    if (request->headers_extra) {
        return &request->headers_extra[index - UVHTTP_INLINE_HEADERS_CAPACITY];
    }

    return NULL;
}

/* add header (internal use, auto-expand) */
uvhttp_error_t uvhttp_request_add_header(uvhttp_request_t* request,
                                         const char* name, const char* value) {

    if (!request || !name || !value) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    /* check if need to expand */

    if (request->header_count >= request->headers_capacity) {

        /* calculate new capacity (maximum MAX_HEADERS) */
        size_t new_capacity = request->headers_capacity * 2;
        if (new_capacity == 0) {
            new_capacity =
                UVHTTP_INLINE_HEADERS_CAPACITY; /* initial capacity */
        }
        if (new_capacity > MAX_HEADERS) {
            new_capacity = MAX_HEADERS;
        }

        /* if new capacity equals current capacity, it means maximum value
         * reached */
        if (new_capacity == request->headers_capacity) {
            return UVHTTP_ERROR_BUFFER_TOO_SMALL; /* full */
        }

        /* allocate or reallocate dynamic array */
        size_t extra_count = new_capacity - UVHTTP_INLINE_HEADERS_CAPACITY;
        int is_first_alloc = (request->headers_extra == NULL);

        uvhttp_header_t* new_extra = uvhttp_realloc(
            request->headers_extra, extra_count * sizeof(uvhttp_header_t));
        if (!new_extra) {
            return UVHTTP_ERROR_OUT_OF_MEMORY; /* memoryallocatefailure */
        }

        /* if first allocation, zero out newly allocated memory */
        if (is_first_alloc) {
            memset(new_extra, 0, extra_count * sizeof(uvhttp_header_t));
        }

        request->headers_extra = new_extra;
        request->headers_capacity = new_capacity;
    }

    /* get header pointer */
    uvhttp_header_t* header =
        uvhttp_request_get_header_at(request, request->header_count);
    if (!header) {
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* copy header name */
    size_t name_len = strlen(name);
    if (name_len >= sizeof(header->name)) {
        name_len = sizeof(header->name) - 1;
    }
    memcpy(header->name, name, name_len);
    header->name[name_len] = '\0';

    /* copy header value */
    size_t value_len = strlen(value);
    if (value_len >= sizeof(header->value)) {
        value_len = sizeof(header->value) - 1;
    }
    memcpy(header->value, value, value_len);
    header->value[value_len] = '\0';

    /* increase count */
    request->header_count++;

    return UVHTTP_OK;
}

/* traverse all headers */
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