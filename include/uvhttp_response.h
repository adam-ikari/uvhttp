#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include <uv.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
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
    char data[];  /* 灵活数组成员，自动处理内存对齐 */
} uvhttp_write_data_t;

typedef struct {
    char* data;
    size_t length;
    uvhttp_response_t* response;
    uvhttp_connection_t* connection;
    size_t offset;
} uvhttp_tls_write_data_t;

struct uvhttp_response {
    uv_tcp_t* client;
    int status_code;
    
    uvhttp_header_t headers[MAX_HEADERS];
    size_t header_count;
    
    char* body;
    size_t body_length;
    
    int headers_sent;
    
    // HTTP/1.1优化字段
    int keep_alive;                     // 是否保持连接
    int sent;                            // 是否已发送响应
    int finished;                        // 响应是否完成
    int compress;           // 是否启用压缩
    int cache_ttl;          // 缓存TTL（秒）
    time_t cache_expires;   // 缓存过期时间
};

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