#ifndef UVHTTP_RESPONSE_H
#define UVHTTP_RESPONSE_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"
#include "uvhttp_platform.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

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
    /* ========== 缓存行1（0-63字节）：热路径字段 - 最频繁访问 ========== */
    /* 在响应构建、发送过程中频繁访问 */
    int status_code;                   /* 4 字节 - HTTP 状态码 */
    int headers_sent;                  /* 4 字节 - 头部是否已发送 */
    int sent;                          /* 4 字节 - 响应是否已发送 */
    int finished;                      /* 4 字节 - 响应是否完成 */
    int keepalive;                     /* 4 字节 - 是否保持连接 */
    int compress;                      /* 4 字节 - 是否启用压缩 */
    int cache_ttl;                     /* 4 字节 - 缓存 TTL（秒） */
    int _padding1;                     /* 4 字节 - 填充到32字节 */
    size_t header_count;               /* 8 字节 - header 数量 */
    size_t body_length;                /* 8 字节 - body 长度 */
    uv_tcp_t* client;                  /* 8 字节 - TCP 客户端句柄 */
    char* body;                        /* 8 字节 - 响应体 */
    /* 缓存行1总计：64字节 */

    /* ========== 缓存行2（64-127字节）：指针和计数器字段 - 次频繁访问 ========== */
    /* 在响应构建、缓存管理中频繁访问 */
    time_t cache_expires;              /* 8 字节 - 缓存过期时间 */
    uvhttp_header_t* headers_extra;    /* 8 字节 - 额外 headers（动态扩容） */
    size_t headers_capacity;           /* 8 字节 - headers 总容量 */
    int _padding2[10];                 /* 40字节 - 填充到64字节 */
    /* 缓存行2总计：64字节 */

    /* ========== 缓存行3+（128+字节）：Headers 数组 ========== */
    /* 放在最后，避免影响热路径字段的缓存局部性 */
    /* Headers - 混合分配：内联 + 动态扩容（优化内存局部性） */
    uvhttp_header_t headers[UVHTTP_INLINE_HEADERS_CAPACITY]; /* 内联，减少动态分配 */
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, client, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, body, UVHTTP_POINTER_ALIGNMENT);

/* 验证size_t对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, header_count,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, body_length, UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_response_t, cache_expires,
                       UVHTTP_SIZE_T_ALIGNMENT);

/* 验证大型缓冲区在结构体末尾 */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_response_t, headers) >= 64,
                     "headers array should be after first 64 bytes");

/* ============ 核心 API 函数 ============ */
uvhttp_error_t uvhttp_response_init(uvhttp_response_t* response, void* client);
uvhttp_error_t uvhttp_response_set_status(uvhttp_response_t* response,
                                          int status_code);
uvhttp_error_t uvhttp_response_set_header(uvhttp_response_t* response,
                                          const char* name, const char* value);
uvhttp_error_t uvhttp_response_set_body(uvhttp_response_t* response,
                                        const char* body, size_t length);

/* ============ 重构后的函数：分离纯函数和副作用 ============ */

/* 纯函数：构建HTTP响应数据，无副作用，易于测试
 * 调用者负责释放返回的 *out_data 内存
 */
uvhttp_error_t uvhttp_response_build_data(uvhttp_response_t* response,
                                          char** out_data, size_t* out_length);

/* 副作用函数：发送原始数据，包含网络I/O */
uvhttp_error_t uvhttp_response_send_raw(const char* data, size_t length,
                                        void* client,
                                        uvhttp_response_t* response);

/* 响应发送函数 */
uvhttp_error_t uvhttp_response_send(uvhttp_response_t* response);

/* ============ 原有函数 ============ */
void uvhttp_response_cleanup(uvhttp_response_t* response);
void uvhttp_response_free(uvhttp_response_t* response);

/* ========== Headers 操作 API ========== */

/* 获取 header 数量 */
size_t uvhttp_response_get_header_count(uvhttp_response_t* response);

/* 获取指定索引的 header（内部使用） */
uvhttp_header_t* uvhttp_response_get_header_at(uvhttp_response_t* response,
                                               size_t index);

/* 遍历所有 headers */
typedef void (*uvhttp_header_callback_t)(const char* name, const char* value,
                                         void* user_data);
void uvhttp_response_foreach_header(uvhttp_response_t* response,
                                    uvhttp_header_callback_t callback,
                                    void* user_data);

#ifdef __cplusplus
}
#endif

#endif