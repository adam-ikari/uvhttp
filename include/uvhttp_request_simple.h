#ifndef UVHTTP_REQUEST_SIMPLE_H
#define UVHTTP_REQUEST_SIMPLE_H

#include <stdlib.h>
#include <string.h>
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 类型定义
typedef struct uvhttp_request uvhttp_request_t;

// 简化的HTTP方法枚举
typedef enum {
    UVHTTP_GET = 0,
    UVHTTP_POST,
    UVHTTP_PUT,
    UVHTTP_DELETE,
    UVHTTP_HEAD,
    UVHTTP_OPTIONS,
    UVHTTP_PATCH,
    UVHTTP_METHOD_COUNT
} uvhttp_method_t;

#define MAX_URL_LEN 2048
#define MAX_BODY_LEN (1024 * 1024)  // 1MB

// 简化的请求结构
struct uvhttp_request {
    char* url;
    char* body;
    size_t body_length;
    size_t body_capacity;
    
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    
    uvhttp_method_t method;
    
    int parsing_complete;
    void* user_data;
    void* client;
};

// 简化的请求函数
int uvhttp_request_init(uvhttp_request_t* request, void* client);
void uvhttp_request_cleanup(uvhttp_request_t* request);
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);

#ifdef __cplusplus
}
#endif

#endif