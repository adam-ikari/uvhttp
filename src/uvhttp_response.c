#include "uvhttp_response.h"
#include "uvhttp_common.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>

static const char* get_status_text(int status_code) {
    switch (status_code) {
        case 200: return "OK";
        case 201: return "Created";
        case 204: return "No Content";
        case 400: return "Bad Request";
        case 401: return "Unauthorized";
        case 403: return "Forbidden";
        case 404: return "Not Found";
        case 405: return "Method Not Allowed";
        case 500: return "Internal Server Error";
        case 501: return "Not Implemented";
        case 502: return "Bad Gateway";
        case 503: return "Service Unavailable";
        default: return "Unknown";
    }
}

static void build_response_headers(uvhttp_response_t* response, char* buffer, size_t* length) {
    size_t pos = 0;
    
    // 状态行
    pos += snprintf(buffer + pos, *length - pos, "HTTP/1.1 %d %s\r\n", 
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
    response->status_code = 200;
    
    return 0;
}

void uvhttp_response_cleanup(uvhttp_response_t* response) {
    if (!response) {
        return;
    }
    
    if (response->body) {
        free(response->body);
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
    if (status_code < 100 || status_code > 599) {
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
    if (uvhttp_safe_strcpy(header->name, sizeof(header->name), name) != 0) {
        fprintf(stderr, "Failed to copy header name: %s\n", name);
        return;
    }
    
    if (uvhttp_safe_strcpy(header->value, sizeof(header->value), value) != 0) {
        fprintf(stderr, "Failed to copy header value for: %s\n", name);
        return;
    }
    
    response->header_count++;
}

int uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length) {
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
    if (length > 1024 * 1024) {
        fprintf(stderr, "Body too large: %zu bytes (max 1MB)\n", length);
        return -1;
    }
    
    // 验证body内容 - 检查无效字符
    for (size_t i = 0; i < length; i++) {
        // 允许所有二进制数据，但记录警告
        if (body[i] == 0 && i < length - 1) {
            fprintf(stderr, "Warning: NULL byte found in body at position %zu\n", i);
        }
    }
    
    if (response->body) {
        free(response->body);
        response->body = NULL;
    }
    
    response->body = malloc(length);
    if (!response->body) {
        fprintf(stderr, "Failed to allocate memory for body (%zu bytes)\n", length);
        response->body_length = 0;
        return -1;
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
    
    // 计算所需的headers大小
    size_t headers_size = 1024; // 初始预估
    char* temp_buffer = malloc(headers_size);
    if (!temp_buffer) {
        fprintf(stderr, "Failed to allocate temporary buffer\n");
        return;
    }
    
    // 第一次尝试构建headers以获取实际大小
    size_t headers_length = headers_size;
    build_response_headers(response, temp_buffer, &headers_length);
    
    // 如果需要更多空间，重新分配
    if (headers_length >= headers_size) {
        size_t new_size = headers_length + 1;
        char* new_buffer = realloc(temp_buffer, new_size);
        if (!new_buffer) {
            free(temp_buffer);
            fprintf(stderr, "Failed to reallocate buffer\n");
            return;
        }
        temp_buffer = new_buffer;
        headers_size = new_size;
        
        // 重新构建headers
        headers_length = headers_size;
        build_response_headers(response, temp_buffer, &headers_length);
    }
    
    // 计算总大小并分配最终缓冲区
    size_t total_size = headers_length + response->body_length;
    char* response_data = malloc(total_size);
    if (!response_data) {
        free(temp_buffer);
        fprintf(stderr, "Failed to allocate response buffer\n");
        return;
    }
    
    // 确保在所有错误路径上清理资源
    int success = 0;
    
    // 复制headers
    memcpy(response_data, temp_buffer, headers_length);
    
    // 复制body
    if (response->body && response->body_length > 0) {
        memcpy(response_data + headers_length, response->body, response->body_length);
    }
    
    // 这里应该实际发送response_data
    // 简化版本模拟发送 - 在实际实现中应该调用网络发送函数
    printf("Sending response (%zu bytes):\n%.*s", total_size, 
           (int)headers_length, response_data);
    
    success = 1; // 标记成功
    
    // 清理临时缓冲区 - 确保在所有路径上都执行
    free(temp_buffer);
    if (response_data) {
        free(response_data);
    }
    
    // 只有在成功时才标记响应已完成
    if (success) {
        response->finished = 1;
    }
}