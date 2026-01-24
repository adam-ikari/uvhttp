#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include <uv.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp_common.h"
#include "uvhttp_error.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_response uvhttp_response_t;

#define MAX_RESPONSE_BODY_LEN (1024 * 1024)  // 1MB

typedef struct {
    uv_write_t write_req;
    size_t length;
    uvhttp_response_t* response;
    char data[1];  /* 数据缓冲区，至少1字节 */
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
    int status_code;                  /* 4 字节 - 状态码 */
    int headers_sent;                  /* 4 字节 - 头部是否已发送 */
    int sent;                          /* 4 字节 - 响应是否已发送 */
    int finished;                      /* 4 字节 - 响应是否完成 */
    
    /* HTTP/1.1优化字段 */
    int keep_alive;                    /* 4 字节 - 是否保持连接 */
    int compress;                      /* 4 字节 - 是否启用压缩 */
    int cache_ttl;                     /* 4 字节 - 缓存TTL（秒） */
    
    /* 指针和计数器字段（8字节对齐） */
    uv_tcp_t* client;                 /* 8 字节 */
    char* body;                       /* 8 字节 */
    size_t header_count;               /* 8 字节 */
    size_t body_length;                /* 8 字节 */
    time_t cache_expires;              /* 8 字节 - 缓存过期时间 */
    
    /* 头部数组（放在最后） */
    uvhttp_header_t headers[MAX_HEADERS];  /* 48 * (256 + 4096) = 208,896 字节 */
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, client) % 8 == 0,
                      "client pointer not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, body) % 8 == 0,
                      "body pointer not 8-byte aligned");

/* 验证size_t对齐（8字节对齐） */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, header_count) % 8 == 0,
                      "header_count not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, body_length) % 8 == 0,
                      "body_length not 8-byte aligned");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, cache_expires) % 8 == 0,
                      "cache_expires not 8-byte aligned");

/* 验证大型缓冲区在结构体末尾 */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, headers) >= 64,
                      "headers array should be after first 64 bytes");

/* ============ 核心 API 函数 ============ */
uvhttp_error_t uvhttp_response_init(uvhttp_response_t* response, void* client);
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response, int status_code);
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response, const char* name, const char* value);
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response, const char* body, size_t length);

/* ============ 重构后的函数：分离纯函数和副作用 ============ */

/* 纯函数：构建HTTP响应数据，无副作用，易于测试
 * 调用者负责释放返回的 *out_data 内存
 */
uvhttp_error_t uvhttp_response_build_data(uvhttp_response_t* response, 
                                         char** out_data, 
                                         size_t* out_length);

/* 副作用函数：发送原始数据，包含网络I/O */
uvhttp_error_t uvhttp_response_send_raw(const char* data, 
                                       size_t length, 
                                       void* client, 
                                       uvhttp_response_t* response);

/* 响应发送函数 */
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);

/* ============ 测试专用函数 ============ */
#ifdef UVHTTP_TEST_MODE

/* 测试用纯函数：验证响应数据构建 */
uvhttp_error_t uvhttp_response_build_for_test(uvhttp_response_t* response, 
                                             char** out_data, 
                                             size_t* out_length);

/* 测试用函数：模拟发送但不实际网络I/O */
uvhttp_error_t uvhttp_response_send_mock(uvhttp_response_t* response);

#endif /* UVHTTP_TEST_MODE */

/* ============ 原有函数 ============ */
void uvhttp_response_cleanup(uvhttp_response_t* response);
void uvhttp_response_free(uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif