#include "uvhttp_json.h"
#include "uvhttp_utils.h"
#include "uvhttp_constants.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* JSON字符串转义 */
static int json_escape_complete(const char* src, char* dst, size_t dst_size) {
    if (!src || !dst || dst_size == 0) return -1;
    
    size_t j = 0;
    for (size_t i = 0; src[i] && j < dst_size - 7; i++) {
        unsigned char c = (unsigned char)src[i];
        
        switch (c) {
            case '"':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = '"'; }
                else return -1;
                break;
            case '\\':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = '\\'; }
                else return -1;
                break;
            case '\b':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = 'b'; }
                else return -1;
                break;
            case '\f':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = 'f'; }
                else return -1;
                break;
            case '\n':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = 'n'; }
                else return -1;
                break;
            case '\r':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = 'r'; }
                else return -1;
                break;
            case '\t':
                if (j + 1 < dst_size) { dst[j++] = '\\'; dst[j++] = 't'; }
                else return -1;
                break;
            default:
                if (c < 0x20) {
                    if (j + 5 < dst_size) {
                        j += snprintf(dst + j, dst_size - j, "\\u%04x", c);
                    } else {
                        return -1;
                    }
                } else if (c >= 0x80) {
                    /* UTF-8字符处理 */
                    if ((c & UVHTTP_UTF8_2BYTE_MASK) == 0xC0 && i + 1 < strlen(src)) {
                        if (j + 1 < dst_size) { dst[j++] = src[i]; dst[j++] = src[i+1]; }
                        else return -1;
                        i++;
                    } else if ((c & 0xF0) == 0xE0 && i + 2 < strlen(src)) {
                        if (j + 2 < dst_size) { 
                            dst[j++] = src[i]; 
                            dst[j++] = src[i+1]; 
                            dst[j++] = src[i+2]; 
                        } else return -1;
                        i += 2;
                    } else if ((c & 0xF8) == 0xF0 && i + 3 < strlen(src)) {
                        if (j + 3 < dst_size) { 
                            dst[j++] = src[i]; 
                            dst[j++] = src[i+1]; 
                            dst[j++] = src[i+2]; 
                            dst[j++] = src[i+3]; 
                        } else return -1;
                        i += 3;
                    } else {
                        if (j + 5 < dst_size) {
                            j += snprintf(dst + j, dst_size - j, "\\u%04x", 0xFFFD);
                        } else {
                            return -1;
                        }
                    }
                } else {
                    if (j < dst_size) dst[j++] = c;
                    else return -1;
                }
                break;
        }
    }
    
    if (j < dst_size) {
        dst[j] = '\0';
        return 0;
    } else {
        return -1;
    }
}

int uvhttp_response_json(uvhttp_response_t* response, int status_code, const char* json_data) {
    if (!response || !json_data) {
        return -1;
    }
    
    uvhttp_response_set_status(response, status_code);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    return uvhttp_response_set_body(response, json_data, strlen(json_data));
}

int uvhttp_response_json_simple(uvhttp_response_t* response, int status_code, 
                                const char* key, const char* value) {
    if (!response || !key || !value) {
        return -1;
    }
    
    // 使用栈缓冲区避免动态分配
    char escaped_key[UVHTTP_JSON_ESCAPE_BUFFER_SIZE];
    char escaped_value[UVHTTP_JSON_VALUE_BUFFER_SIZE];
    char json_buffer[UVHTTP_JSON_BUFFER_SIZE];    
    // 转义键和值，检查错误
    if (json_escape_complete(key, escaped_key, sizeof(escaped_key)) != 0) {
        return -1;
    }
    if (json_escape_complete(value, escaped_value, sizeof(escaped_value)) != 0) {
        return -1;
    }
    
    // 安全的snprintf调用，检查返回值
    int result = snprintf(json_buffer, sizeof(json_buffer), "{\"%s\":\"%s\"}", escaped_key, escaped_value);
    if (result < 0 || result >= (int)sizeof(json_buffer)) {
        return -1; // 截断或错误
    }
    
    return uvhttp_response_json(response, status_code, json_buffer);
}

int uvhttp_response_json_error(uvhttp_response_t* response, int status_code, 
                               const char* error_message) {
    return uvhttp_response_json_simple(response, status_code, "error", 
                                       error_message ? error_message : "Unknown error");
}

int uvhttp_response_json_success(uvhttp_response_t* response, const char* message) {
    if (!response) {
        return -1;
    }
    
    char escaped_msg[UVHTTP_JSON_VALUE_BUFFER_SIZE];
    char json_buffer[UVHTTP_JSON_ERROR_BUFFER_SIZE];    
    const char* msg = message ? message : "Success";
    if (json_escape_complete(msg, escaped_msg, sizeof(escaped_msg)) != 0) {
        return -1;
    }
    
    int result = snprintf(json_buffer, sizeof(json_buffer), "{\"success\":true,\"message\":\"%s\"}", escaped_msg);
    if (result < 0 || result >= (int)sizeof(json_buffer)) {
        return -1;
    }
    
    return uvhttp_response_json(response, 200, json_buffer);
}