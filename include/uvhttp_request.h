#ifndef UVHTTP_REQUEST_H
#define UVHTTP_REQUEST_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"
#include "uvhttp_platform.h"

#include "llhttp.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

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
    /* ========== 缓存行1（0-63字节）：热路径字段 - 最频繁访问 ========== */
    /* 在 HTTP 解析、路由匹配中频繁访问 */
    uvhttp_method_t method;             /* 4 字节 - HTTP 方法 */
    int parsing_complete;               /* 4 字节 - 解析是否完成 */
    int _padding1[2];                   /* 8 字节 - 填充到16字节 */
    size_t header_count;                /* 8 字节 - header 数量 */
    size_t body_length;                 /* 8 字节 - body 长度 */
    size_t body_capacity;               /* 8 字节 - body 容量 */
    uv_tcp_t* client;                   /* 8 字节 - TCP 客户端句柄 */
    llhttp_t* parser;                   /* 8 字节 - HTTP 解析器 */
    /* 缓存行1总计：56字节（剩余8字节填充） */

    /* ========== 缓存行2（64-127字节）：指针字段 - 次频繁访问 ========== */
    /* 在请求处理、响应构建中频繁访问 */
    llhttp_settings_t* parser_settings; /* 8 字节 - 解析器设置 */
    char* path;                         /* 8 字节 - 请求路径 */
    char* query;                        /* 8 字节 - 查询字符串 */
    char* body;                         /* 8 字节 - 请求体 */
    void* user_data;                    /* 8 字节 - 用户数据 */
    uvhttp_header_t* headers_extra;     /* 8 字节 - 额外 headers（动态扩容） */
    size_t headers_capacity;            /* 8 字节 - headers 总容量 */
    int _padding2[2];                   /* 8 字节 - 填充到64字节 */
    /* 缓存行2总计：64字节 */

    /* ========== 缓存行3+（128+字节）：大块缓冲区 ========== */
    /* 放在最后，避免影响热路径字段的缓存局部性 */
    char url[MAX_URL_LEN];              /* 2048 字节 - URL 缓冲区 */

    /* Headers - 混合分配：内联 + 动态扩容（优化内存局部性） */
    uvhttp_header_t headers[UVHTTP_INLINE_HEADERS_CAPACITY]; /* 内联，减少动态分配 */
};

/* ========== 内存布局验证静态断言 ========== */

/* 验证指针对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, client, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, parser, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, parser_settings,
                       UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, path, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, query, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, body, UVHTTP_POINTER_ALIGNMENT);

/* 验证size_t对齐（平台自适应） */
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, header_count, UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_request_t, body_length, UVHTTP_SIZE_T_ALIGNMENT);

/* 验证大型缓冲区在结构体末尾 */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_request_t, url) >= 64,
                     "url buffer should be after first 64 bytes");
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_request_t, headers) >= 64,
                     "headers array should be after first 64 bytes");

/* API functions */
const char* uvhttp_request_get_method(uvhttp_request_t* request);
const char* uvhttp_request_get_url(uvhttp_request_t* request);
void uvhttp_request_free(uvhttp_request_t* request);
void uvhttp_request_cleanup(uvhttp_request_t* request);
const char* uvhttp_request_get_path(uvhttp_request_t* request);
const char* uvhttp_request_get_query_string(uvhttp_request_t* request);
const char* uvhttp_request_get_query_param(uvhttp_request_t* request,
                                           const char* name);
const char* uvhttp_request_get_client_ip(uvhttp_request_t* request);
const char* uvhttp_request_get_header(uvhttp_request_t* request,
                                      const char* name);
const char* uvhttp_request_get_body(uvhttp_request_t* request);
size_t uvhttp_request_get_body_length(uvhttp_request_t* request);

/* ========== Headers 操作 API ========== */

/* 获取 header 数量 */
size_t uvhttp_request_get_header_count(uvhttp_request_t* request);

/* 获取指定索引的 header（内部使用） */
uvhttp_header_t* uvhttp_request_get_header_at(uvhttp_request_t* request,
                                              size_t index);

/* 添加 header（内部使用，自动扩容） */
uvhttp_error_t uvhttp_request_add_header(uvhttp_request_t* request,
                                         const char* name, const char* value);

/* 遍历所有 headers */
typedef void (*uvhttp_header_callback_t)(const char* name, const char* value,
                                         void* user_data);
void uvhttp_request_foreach_header(uvhttp_request_t* request,
                                   uvhttp_header_callback_t callback,
                                   void* user_data);

#ifdef __cplusplus
}
#endif

#endif