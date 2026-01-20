#ifndef UVHTTP_REQUEST_H
#define UVHTTP_REQUEST_H

#include <uv.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "llhttp.h"
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_URL_LEN 2048
#define MAX_BODY_LEN (1024 * 1024)  // 1MB

// HTTP方法枚举
typedef enum {
    UVHTTP_ANY = 0,
    UVHTTP_GET,
    UVHTTP_POST,
    UVHTTP_PUT,
    UVHTTP_DELETE,
    UVHTTP_HEAD,
    UVHTTP_OPTIONS,
    UVHTTP_PATCH
} uvhttp_method_t;



typedef struct uvhttp_request uvhttp_request_t;

struct uvhttp_request {
    /* 热路径字段（频繁访问）- 优化内存局部性 */
    uvhttp_method_t method;          /* 4 字节 */
    int parsing_complete;             /* 4 字节 */
    size_t header_count;              /* 8 字节 */
    
    /* 指针字段（8字节对齐） */
    uv_tcp_t* client;                /* 8 字节 */
    llhttp_t* parser;                 /* 8 字节 */
    llhttp_settings_t* parser_settings; /* 8 字节 */
    char* path;                       /* 8 字节 */
    char* query;                      /* 8 字节 */
    char* body;                       /* 8 字节 */
    void* user_data;                  /* 8 字节 */
    
    /* 缓冲区字段（大块内存，放在后面） */
    char url[MAX_URL_LEN];            /* 2048 字节 */
    size_t body_length;               /* 8 字节 */
    size_t body_capacity;             /* 8 字节 */
    
    /* 头部数组（放在最后） */
    uvhttp_header_t headers[MAX_HEADERS];  /* 64 * (256 + 4096) = 278,528 字节 */
};

/* API functions */
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
void uvhttp_request_free(uvhttp_request_t* request);
void uvhttp_request_cleanup(uvhttp_request_t* request);
const char* uvhttp_request_get_path(uvhttp_request_t* request);
const char* uvhttp_request_get_query_string(uvhttp_request_t* request);
const char* uvhttp_request_get_query_param(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_client_ip(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);

#ifdef __cplusplus
}
#endif

#endif