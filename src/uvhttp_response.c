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
    
    // 默认headers
    int has_content_type = 0;
    int has_content_length = 0;
    
    for (size_t i = 0; i < response->header_count; i++) {
        pos += snprintf(buffer + pos, *length - pos, "%s: %s\r\n",
                       response->headers[i].name, response->headers[i].value);
        
        if (strcasecmp(response->headers[i].name, "Content-Type") == 0) {
            has_content_type = 1;
        }
        if (strcasecmp(response->headers[i].name, "Content-Length") == 0) {
            has_content_length = 1;
        }
    }
    
    // 添加默认Content-Type
    if (!has_content_type) {
        pos += snprintf(buffer + pos, *length - pos, "Content-Type: text/plain\r\n");
    }
    
    // 添加Content-Length
    if (!has_content_length && response->body) {
        pos += snprintf(buffer + pos, *length - pos, "Content-Length: %zu\r\n", 
                       response->body_length);
    }
    
    // 结束headers
    pos += snprintf(buffer + pos, *length - pos, "\r\n");
    
    *length = pos;
}

int uvhttp_response_init(uvhttp_response_t* response, void* client) {
    if (!response) {
        fprintf(stderr, "Response object is NULL\n");
        return -1;
    }
    
    if (!client) {
        fprintf(stderr, "Client handle is NULL\n");
        return -1;
    }
    
    memset(response, 0, sizeof(uvhttp_response_t));
    
    response->client = client;
    response->status_code = UVHTTP_STATUS_OK;
    
    return 0;
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

void uvhttp_response_set_status(uvhttp_response_t* response, int status_code) {
    if (!response) {
        fprintf(stderr, "Response object is NULL\n");
        return;
    }
    
    // 验证状态码范围
    if (status_code < UVHTTP_STATUS_CONTINUE || status_code > 599) {
        fprintf(stderr, "Invalid status code: %d\n", status_code);
        return;
    }
    
    response->status_code = status_code;
}

void uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value) {
    if (!response || !name || !value) {
        return;
    }
    
    if (response->header_count >= MAX_HEADERS) {
        return;
    }
    
    // 验证header名称和值
    if (uvhttp_validate_header_value(name, value) != 0) {
        fprintf(stderr, "Invalid header: %s\n", name);
        return;
    }
    
    uvhttp_header_t* header = &response->headers[response->header_count];
    
    // 使用安全的字符串复制函数
    if (uvhttp_safe_strcpy(header->name, UVHTTP_MAX_HEADER_NAME_SIZE, name) != 0) {
        fprintf(stderr, "Failed to copy header name: %s\n", name);
        return;
    }
    
    if (uvhttp_safe_strcpy(header->value, UVHTTP_MAX_HEADER_VALUE_SIZE, value) != 0) {
        fprintf(stderr, "Failed to copy header value for: %s\n", name);
        return;
    }
    
    response->header_count++;
}

uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length) {
    if (!response) {
        fprintf(stderr, "Response object is NULL\n");
        return -1;
    }
    
    if (!body) {
        fprintf(stderr, "Body data is NULL\n");
        return -1;
    }
    
    if (length == 0) {
        fprintf(stderr, "Body length is zero\n");
        return -1;
    }
    
    // 检查长度限制 - 简化版本使用1MB限制
    if (length > UVHTTP_MAX_BODY_SIZE) {
        fprintf(stderr, "Body too large: %zu bytes (max 1MB)\n", length);
        return -1;
    }
    
    // 验证body内容 - 检查无效字符
    for (size_t i = 0; i < length; i++) {
        // 允许所有二进制数据，但记录警告
        if (i < length - 1 && body[i] == 0) {
            fprintf(stderr, "Warning: NULL byte found in body at position %zu\n", i);
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
    
    return 0;
}

void uvhttp_response_send(uvhttp_response_t* response) {
    if (!response) {
        fprintf(stderr, "Invalid response object\n");
        return;
    }
    
    // 计算所需的headers大小 - 增加安全边界
    size_t headers_size = UVHTTP_INITIAL_BUFFER_SIZE;
    char* temp_buffer = uvhttp_malloc(headers_size);
    if (!temp_buffer) {
        fprintf(stderr, "Failed to allocate temporary buffer\n");
        return;
    }
    
    // 第一次尝试构建headers以获取实际大小
    size_t headers_length = headers_size;
    int build_result = build_response_headers(response, temp_buffer, &headers_length);
    
    // 检查构建结果和缓冲区大小
    if (build_result != 0 || headers_length >= headers_size - 1) {
        // 需要更大的缓冲区 - 添加安全边界
        size_t new_size = headers_length + 256; // 添加256字节安全边界
        if (new_size > UVHTTP_MAX_BODY_SIZE) { // 防止过大分配
            fprintf(stderr, "Response headers too large: %zu bytes\n", headers_length);
            uvhttp_free(temp_buffer);
            return;
        }
        
        char* new_buffer = uvhttp_malloc(new_size);
        if (!new_buffer) {
            fprintf(stderr, "Failed to allocate larger buffer\n");
            uvhttp_free(temp_buffer);
            return;
        }
        
        // 重新构建headers
        headers_length = new_size;
        build_result = build_response_headers(response, new_buffer, &headers_length);
        if (build_result != 0) {
            fprintf(stderr, "Failed to build response headers\n");
            uvhttp_free(temp_buffer);
            uvhttp_free(new_buffer);
            return;
        }
        
        uvhttp_free(temp_buffer);
        temp_buffer = new_buffer;
        headers_size = new_size;
    }
    
    // 验证总大小不会过大
    size_t total_size = headers_length + response->body_length;
    if (total_size > UVHTTP_MAX_BODY_SIZE * 2) { // 限制总响应大小
        fprintf(stderr, "Response too large: %zu bytes\n", total_size);
        uvhttp_free(temp_buffer);
        return;
    }
    
    // 分配最终缓冲区
    char* response_data = uvhttp_malloc(total_size);
    if (!response_data) {
        fprintf(stderr, "Failed to allocate response buffer\n");
        uvhttp_free(temp_buffer);
        return;
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
    // 简化版本模拟发送 - 在实际实现中应该调用网络发送函数
    printf("Sending response (%zu bytes):\n%.*s", total_size, 
           (int)headers_length, response_data);
    
    // 清理所有分配的资源
    uvhttp_free(temp_buffer);
    uvhttp_free(response_data);
    
    // 标记响应已完成
    response->finished = 1;
}