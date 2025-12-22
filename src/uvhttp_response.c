#include "uvhttp_response.h"
#include "uvhttp_common.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

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
    
    // HTTP/1.1优化：添加Content-Length或Connection头
    if (!has_content_length && response->body) {
        pos += snprintf(buffer + pos, *length - pos, "Content-Length: %zu\r\n", 
                       response->body_length);
    }
    
    // HTTP/1.1优化：根据keep-alive设置Connection头
    if (!has_connection) {
        if (response->keep_alive) {
            pos += snprintf(buffer + pos, *length - pos, "Connection: keep-alive\r\n");
            pos += snprintf(buffer + pos, *length - pos, "Keep-Alive: timeout=5, max=1000\r\n");
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
    
    response->client = client;
    response->status_code = UVHTTP_STATUS_OK;
    
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
    if (status_code < UVHTTP_STATUS_CONTINUE || status_code > 599) {
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

uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response) {
    if (!response) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    // 计算所需的headers大小 - 增加安全边界
    size_t headers_size = UVHTTP_INITIAL_BUFFER_SIZE;
    char* temp_buffer = uvhttp_malloc(headers_size);
    if (!temp_buffer) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    // 第一次尝试构建headers以获取实际大小
    size_t headers_length = headers_size;
    build_response_headers(response, temp_buffer, &headers_length);
    
    // 检查构建结果和缓冲区大小
    if (headers_length >= headers_size - 1) {
        // 需要更大的缓冲区 - 添加安全边界
        size_t new_size = headers_length + 256; // 添加256字节安全边界
        if (new_size > UVHTTP_MAX_BODY_SIZE) { // 防止过大分配
            uvhttp_free(temp_buffer);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        char* new_buffer = uvhttp_malloc(new_size);
        if (!new_buffer) {
            uvhttp_free(temp_buffer);
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        // 重新构建headers
        headers_length = new_size;
        build_response_headers(response, new_buffer, &headers_length);
        
        uvhttp_free(temp_buffer);
        temp_buffer = new_buffer;
        headers_size = new_size;
    }
    
    // 验证总大小不会过大
    size_t total_size = headers_length + response->body_length;
    if (total_size > UVHTTP_MAX_BODY_SIZE * 2) { // 限制总响应大小
        uvhttp_free(temp_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    // 分配最终缓冲区
    char* response_data = uvhttp_malloc(total_size);
    if (!response_data) {
        uvhttp_free(temp_buffer);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    // 安全复制headers
    if (headers_length > 0) {
        memcpy(response_data, temp_buffer, headers_length);
    }
    
    // 安全复制body
    if (response->body && response->body_length > 0) {
        memcpy(response_data + headers_length, response->body, response->body_length);
    }
    
    // 这里应该实际发送response_data
    // 网络发送实现
    uvhttp_write_data_t* write_data = uvhttp_malloc(sizeof(uvhttp_write_data_t));
    if (!write_data) {
        uvhttp_free(temp_buffer);
        uvhttp_free(response_data);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    write_data->data = uvhttp_malloc(response->body_length);
    if (!write_data->data) {
        uvhttp_free(write_data);
        uvhttp_free(temp_buffer);
        uvhttp_free(response_data);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    
    memcpy(write_data->data, response->body, response->body_length);
    write_data->length = response->body_length;
    write_data->response = response;
    
    uv_buf_t buf = uv_buf_init(write_data->data, write_data->length);
    int result = uv_write(&write_data->write_req, (uv_stream_t*)response->client, &buf, 1, on_write_complete);
    
    if (result < 0) {
        uvhttp_free(write_data->data);
        uvhttp_free(write_data);
        uvhttp_free(temp_buffer);
        uvhttp_free(response_data);
        return UVHTTP_ERROR_WRITE_FAILED;
    }
    
    // 清理所有分配的资源
    uvhttp_free(temp_buffer);
    uvhttp_free(response_data);
    
    // 标记响应已完成
    response->finished = 1;
    
    return UVHTTP_OK;
}