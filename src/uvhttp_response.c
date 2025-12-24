#include "uvhttp_response.h"
#include "uvhttp_common.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_connection.h"
#include "uvhttp_error_handler.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

// 函数声明
static void uvhttp_free_write_data(uv_write_t* req, int status);

static const char* get_status_text(int status_code) {
    switch (status_code) {
        case UVHTTP_STATUS_OK: return "OK";
        case UVHTTP_STATUS_CREATED: return "Created";
        case UVHTTP_STATUS_NO_CONTENT: return "No Content";
        case UVHTTP_STATUS_BAD_REQUEST: return "Bad Request";
        case UVHTTP_STATUS_UNAUTHORIZED: return "Unauthorized";
        case UVHTTP_STATUS_FORBIDDEN: return "Forbidden";
        case UVHTTP_STATUS_NOT_FOUND: return "Not Found";
        case UVHTTP_STATUS_METHOD_NOT_ALLOWED: return "Method Not Allowed";
        case UVHTTP_STATUS_INTERNAL_ERROR: return "Internal Server Error";
        case UVHTTP_STATUS_NOT_IMPLEMENTED: return "Not Implemented";
        case UVHTTP_STATUS_BAD_GATEWAY: return "Bad Gateway";
        case UVHTTP_STATUS_SERVICE_UNAVAILABLE: return "Service Unavailable";
        default: return "Unknown";
    }
}

// 辅助函数：检查字符串中是否包含控制字符（包括换行符）
static int contains_control_chars(const char* str) {
    if (!str) return 0;
    
    for (const char* p = str; *p; p++) {
        unsigned char c = (unsigned char)*p;
        // 检查是否包含控制字符 (0-31) 但排除制表符 (9) 和空格 (32)
        if (c < UVHTTP_SPACE_CHARACTER && c != UVHTTP_TAB_CHARACTER) {
            return 1;  // 包含控制字符
        }
        // 检查删除字符
        if (c == UVHTTP_DELETE_CHARACTER) {
            return 1;  // 包含删除字符
        }
    }
    return 0;
}

static void build_response_headers(uvhttp_response_t* response, char* buffer, size_t* length) {
    size_t pos = 0;
    
    // 状态行
    pos += snprintf(buffer + pos, *length - pos, UVHTTP_VERSION_1_1 " %d %s\r\n", 
                   response->status_code, get_status_text(response->status_code));
    
    // 默认headers检查
    int has_content_type = 0;
    int has_content_length = 0;
    int has_connection = 0;
    
    // 遍历现有headers
    for (size_t i = 0; i < response->header_count; i++) {
        // 安全检查：验证header值不包含控制字符，防止响应拆分
        if (contains_control_chars(response->headers[i].value)) {
            // 如果header值包含控制字符，跳过该header
            UVHTTP_LOG_ERROR("Invalid header value detected: header '%s' contains control characters\n", 
                           response->headers[i].name);
            continue;
        }
        
        pos += snprintf(buffer + pos, *length - pos, "%s: %s\r\n",
                       response->headers[i].name, response->headers[i].value);
        
        if (strcasecmp(response->headers[i].name, "Content-Type") == 0) {
            has_content_type = 1;
        }
        if (strcasecmp(response->headers[i].name, "Content-Length") == 0) {
            has_content_length = 1;
        }
        if (strcasecmp(response->headers[i].name, "Connection") == 0) {
            has_connection = 1;
        }
    }
    
    // 添加默认Content-Type
    if (!has_content_type) {
        pos += snprintf(buffer + pos, *length - pos, "Content-Type: text/plain\r\n");
    }
    
    // HTTP/1.1要求：必须有Content-Length或使用chunked编码
    // 这里我们总是添加Content-Length以确保兼容性
    if (!has_content_length) {
        if (response->body && response->body_length > 0) {
            pos += snprintf(buffer + pos, *length - pos, "Content-Length: %zu\r\n", 
                           response->body_length);
        } else {
            // 即使没有body也要设置Content-Length: 0
            pos += snprintf(buffer + pos, *length - pos, "Content-Length: 0\r\n");
        }
        has_content_length = 1;
    }
    
    // HTTP/1.1优化：根据keep-alive设置Connection头
    if (!has_connection) {
        if (response->keep_alive) {
            pos += snprintf(buffer + pos, *length - pos, "Connection: keep-alive\r\n");
            pos += snprintf(buffer + pos, *length - pos, "Keep-Alive: timeout=%d, max=%d\r\n", 
                   UVHTTP_DEFAULT_KEEP_ALIVE_TIMEOUT, UVHTTP_DEFAULT_KEEP_ALIVE_MAX);
        } else {
            pos += snprintf(buffer + pos, *length - pos, "Connection: close\r\n");
        }
    }
    
    // 结束headers
    pos += snprintf(buffer + pos, *length - pos, "\r\n");
    
    *length = pos;
}

uvhttp_error_t uvhttp_response_init(uvhttp_response_t* response, void* client) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (!client) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    memset(response, 0, sizeof(uvhttp_response_t));
    
    // HTTP/1.1优化：设置默认值
    response->keep_alive = 1;    // HTTP/1.1默认保持连接
    response->status_code = UVHTTP_STATUS_OK;
    response->sent = 0;          // 未发送
    response->finished = 0;       // 未完成
    
    response->client = client;
    
    return UVHTTP_OK;
}

void uvhttp_response_cleanup(uvhttp_response_t* response) {
    if (!response) {
        return;
    }
    
    if (response->body) {
        uvhttp_free(response->body);
        response->body = NULL;
    }
    
    response->body_length = 0;
}

uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证状态码范围
    if (status_code < UVHTTP_STATUS_MIN_CONTINUE || status_code > UVHTTP_STATUS_MAX) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->status_code = status_code;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value) {
    if (!response || !name || !value) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (response->header_count >= MAX_HEADERS) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    // 验证header名称和值
    if (uvhttp_validate_header_value(name, value) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 额外验证：检查header值是否包含控制字符，防止响应拆分
    if (contains_control_chars(value)) {
        UVHTTP_LOG_ERROR("Invalid header value: contains control characters\n");
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    uvhttp_header_t* header = &response->headers[response->header_count];
    
    // 使用安全的字符串复制函数
    if (uvhttp_safe_strcpy(header->name, UVHTTP_MAX_HEADER_NAME_SIZE, name) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (uvhttp_safe_strcpy(header->value, UVHTTP_MAX_HEADER_VALUE_SIZE, value) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    response->header_count++;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (!body) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    if (length == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 检查长度限制 - 简化版本使用1MB限制
    if (length > UVHTTP_MAX_BODY_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 验证body内容 - 检查无效字符
    for (size_t i = 0; i < length; i++) {
        // 允许所有二进制数据，但记录警告
        if (i < length - 1 && body[i] == 0) {
            // NULL字节是有效的，不需要处理
        }
    }
    
    if (response->body) {
        uvhttp_free(response->body);
        response->body = NULL;
    }
    
    response->body = uvhttp_malloc(length);
    if (!response->body) {
        response->body_length = 0;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(response->body, body, length);
    response->body_length = length;
    
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_send_response_data(uvhttp_response_t* response, const char* data, size_t length) {
    if (!response || !data || length == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    uvhttp_write_data_t* write_data = uvhttp_malloc(sizeof(uvhttp_write_data_t));
    if (!write_data) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    write_data->data = uvhttp_malloc(length);
    if (!write_data->data) {
        uvhttp_free(write_data);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(write_data->data, data, length);
    write_data->length = length;
    write_data->response = response;
    
    uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);
    write_data->write_req.data = write_data;
    int result = uv_write(&write_data->write_req, (uv_stream_t*)response->client, &buf, 1, 
                         (uv_write_cb)uvhttp_free_write_data);
    
    if (result < 0) {
        uvhttp_free(write_data->data);
        uvhttp_free(write_data);
        return UVHTTP_ERROR_RESPONSE_SEND;
    }
    
    return UVHTTP_OK;
}

/* 单线程安全的写入完成回调
 * 在libuv事件循环线程中执行，安全释放写入相关资源
 * 单线程优势：无需锁，资源释放顺序可预测
 */
static void uvhttp_free_write_data(uv_write_t* req, int status) {
    (void)status; // 避免未使用参数警告
    uvhttp_write_data_t* write_data = (uvhttp_write_data_t*)req->data;
    if (write_data) {
        /* 检查是否需要关闭连接 */
        if (write_data->response && !write_data->response->keep_alive) {
            /* 获取连接对象并关闭连接 */
            uv_tcp_t* client = (uv_tcp_t*)write_data->response->client;
            if (client) {
                uvhttp_connection_t* conn = (uvhttp_connection_t*)client->data;
                if (conn) {
                    uvhttp_connection_close(conn);
                }
            }
        }
        
        /* 单线程安全的资源释放 */
        if (write_data->data) {
            uvhttp_free(write_data->data);
        }
        uvhttp_free(write_data);
    }
}

/* 单线程事件驱动的HTTP响应发送
 * 修复ab兼容性，确保HTTP响应格式正确
 */
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    /* 单线程安全的重复发送检查 */
    if (response->sent) {
        return UVHTTP_OK;
    }
    response->sent = 1;
    
    /* 构建完整的HTTP响应 - 单线程内存操作 */
    size_t headers_size = UVHTTP_INITIAL_BUFFER_SIZE;
    char* headers_buffer = uvhttp_malloc(headers_size);
    if (!headers_buffer) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    size_t headers_length = headers_size;
    build_response_headers(response, headers_buffer, &headers_length);
    
    /* 计算总大小 */
    size_t total_size = headers_length + response->body_length;
    if (total_size > UVHTTP_MAX_BODY_SIZE * 2) {
        uvhttp_free(headers_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    /* 分配完整响应数据 - 这个内存将在异步回调中安全释放 */
    char* response_data = uvhttp_malloc(total_size + 1);  /* +1 for null terminator */
    if (!response_data) {
        uvhttp_free(headers_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    /* 复制headers */
    memcpy(response_data, headers_buffer, headers_length);
    
    /* 复制body */
    if (response->body && response->body_length > 0) {
        memcpy(response_data + headers_length, response->body, response->body_length);
    }
    
    /* 确保以null结尾（虽然HTTP不需要，但为了安全） */
    response_data[total_size] = '\0';
    
    /* 立即释放headers_buffer，不再需要 */
    uvhttp_free(headers_buffer);
    
    /* 创建写数据结构，确保response_data在异步发送完成后被正确释放 */
    uvhttp_write_data_t* write_data = uvhttp_malloc(sizeof(uvhttp_write_data_t));
    if (!write_data) {
        uvhttp_free(response_data);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    write_data->data = response_data;  /* response_data将在异步回调中释放 */
    write_data->length = total_size;
    write_data->response = response;
    
    uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);
    write_data->write_req.data = write_data;
    
    /* 异步写入 - libuv在事件循环线程中执行，完成后调用回调释放资源 */
    int result = uv_write(&write_data->write_req, (uv_stream_t*)response->client, &buf, 1, 
                         (uv_write_cb)uvhttp_free_write_data);
    
    if (result < 0) {
        /* 写入失败，立即清理资源 */
        uvhttp_free(write_data->data);
        uvhttp_free(write_data);
        return UVHTTP_ERROR_RESPONSE_SEND;
    }
    
    response->finished = 1;
    
    /* 如果响应设置了 Connection: close，需要在发送完成后关闭连接 */
    if (!response->keep_alive) {
        /* 获取连接对象 */
        uvhttp_connection_t* conn = (uvhttp_connection_t*)((uv_tcp_t*)response->client)->data;
        if (conn) {
            /* 设置连接不保持活跃，确保在响应发送完成后关闭 */
            conn->keep_alive = 0;
        }
    }
    
    return UVHTTP_OK;
}

void uvhttp_response_free(uvhttp_response_t* response) {
    if (!response) {
        return;
    }
    
    uvhttp_response_cleanup(response);
    uvhttp_free(response);
}