#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明（避免循环引用）
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_response uvhttp_response_t;

#define MAX_RESPONSE_BODY_LEN (1024 * 1024)  // 1MB

typedef struct {
    uv_write_t write_req;
    size_t length;
    uvhttp_response_t* response;
    char data[1]; /* 数据缓冲区，至少1字节 */
} uvhttp_write_data_t;

typedef struct {
    char* data;
    size_t length;
    uvhttp_response_t* response;
    uvhttp_connection_t* connection;
    size_t offset;
} uvhttp_tls_write_data_t;

struct uvhttp_response {
    /* 热路径字段（频繁访问）- 优化内存局部性 */
    int status_code;  /* 4 字节 - 状态码 */
    int headers_sent; /* 4 字节 - 头部是否已发送 */
    int body_sent;    /* 4 字节 - 主体是否已发送 */
    int is_chunked;   /* 4 字节 - 是否使用分块传输 */

    /* 缓存行2：指针字段（32字节）- 集中存储 */
    uvhttp_connection_t* connection; /* 8 字节 */
    char* body;                        /* 8 字节 */
    size_t body_length;                /* 8 字节 */
    size_t body_sent_length;           /* 8 字节 */

    /* 缓存行3：计数器和状态（32字节） */
    int header_count;  /* 4 字节 */
    int body_chunk;   /* 4 字节 */
    int is_sending;   /* 4 字节 */
    int _reserved[1];

    /* 缓存行4-5：内联headers（256字节）- 优化内存分配 */
    char headers[UVHTTP_INLINE_HEADERS_CAPACITY]
        [UVHTTP_MAX_HEADER_NAME_SIZE + UVHTTP_MAX_HEADER_VALUE_SIZE]; /* 128字节 */
    char header_values[UVHTTP_INLINE_HEADERS_CAPACITY]
        [UVHTTP_MAX_HEADER_VALUE_SIZE]; /* 128字节 */
    size_t header_lengths[UVHTTP_INLINE_HEADERS_CAPACITY]; /* 32字节 */

    /* 缓存行6：动态headers（指针）- 仅在内联headers不足时使用 */
    char** dynamic_header_names;    /* 8 字节 */
    char** dynamic_header_values;   /* 8 字节 */
    size_t* dynamic_header_lengths; /* 8 字节 */
    int dynamic_header_count;        /* 4 字节 */
    int _reserved2[4];

    /* 缓存行7-8：TLS写数据（指针）- 仅在TLS时使用 */
    uvhttp_tls_write_data_t* tls_write_data; /* 8 字节 */
    int _reserved3[7];
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, connection) % 8 == 0,
                     "connection pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, body) % 8 == 0,
                     "body pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, body_length) % 8 == 0,
                     "body_length not 8-byte aligned");

/* 验证size_t对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, body_sent_length) % 8 == 0,
                     "body_sent_length not 8-byte aligned");

/* 验证内联headers在结构体前部 */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, headers) >= 32,
                     "headers should be after first 32 bytes");

// 响应管理函数
uvhttp_error_t uvhttp_response_new(uvhttp_response_t** response);
void uvhttp_response_free(uvhttp_response_t* response);
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response,
                                          int status_code);
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                            const char* name,
                                            const char* value);
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                        const char* body,
                                        size_t body_length);
uvhttp_error_t uvhttp_response_set_body_string(uvhttp_response_t* response,
                                                 const char* body);
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);
const char* uvhttp_response_get_header(uvhttp_response_t* response,
                                       const char* name);
int uvhttp_response_get_status(uvhttp_response_t* response);
const char* uvhttp_response_get_body(uvhttp_response_t* response,
                                      size_t* body_length);
void uvhttp_response_reset(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif