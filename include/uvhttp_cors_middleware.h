/*
 * UVHTTP CORS 中间件
 * 提供跨域资源共享（CORS）支持
 */

#ifndef UVHTTP_CORS_MIDDLEWARE_H
#define UVHTTP_CORS_MIDDLEWARE_H

#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_middleware.h"

#ifdef __cplusplus
extern "C" {
#endif

/* CORS 配置结构 */
typedef struct uvhttp_cors_config {
    char* allow_origin;                /* Access-Control-Allow-Origin (堆分配) */
    char* allow_methods;               /* Access-Control-Allow-Methods (堆分配) */
    char* allow_headers;               /* Access-Control-Allow-Headers (堆分配) */
    char* expose_headers;              /* Access-Control-Expose-Headers (堆分配) */
    char* allow_credentials;           /* Access-Control-Allow-Credentials (堆分配) */
    char* max_age;                     /* Access-Control-Max-Age (堆分配) */
    int allow_all_origins;             /* 是否允许所有来源 */
    int allow_credentials_enabled;     /* 是否允许携带凭证 */
    int owns_strings;                  /* 是否拥有字符串（用于销毁时判断） */
} uvhttp_cors_config_t;

/* 创建默认 CORS 配置 */
uvhttp_cors_config_t* uvhttp_cors_config_default(void);

/* 创建自定义 CORS 配置 */
uvhttp_cors_config_t* uvhttp_cors_config_create(
    const char* allow_origin,
    const char* allow_methods,
    const char* allow_headers
);

/* 销毁 CORS 配置 */
void uvhttp_cors_config_destroy(uvhttp_cors_config_t* config);

/* CORS 中间件处理函数（用于动态中间件系统） */
int uvhttp_cors_middleware(
    const uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
);

/* CORS 中间件处理函数（用于宏中间件系统） */
int uvhttp_cors_middleware_simple(
    const uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
);

/* 设置 CORS 响应头 */
void uvhttp_cors_set_headers(
    uvhttp_response_t* response,
    const uvhttp_cors_config_t* config,
    const char* origin
);

/* 检查是否为预检请求 */
int uvhttp_cors_is_preflight_request(const uvhttp_request_t* request);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CORS_MIDDLEWARE_H */