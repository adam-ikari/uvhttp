/**
 * @file uvhttp_middleware.h
 * @brief 简化的中间件系统
 * 
 * 通过编译选项控制中间件功能，减少运行时开销
 */

#ifndef UVHTTP_MIDDLEWARE_H
#define UVHTTP_MIDDLEWARE_H

#include "uvhttp_error.h"
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

// 前向声明
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 中间件函数类型 - 统一接口
typedef uvhttp_error_t (*uvhttp_middleware_func_t)(uvhttp_request_t* request, 
                                                  uvhttp_response_t* response);

// 简化的中间件配置结构
typedef struct {
    uvhttp_middleware_func_t request_handler;   // 请求处理函数
    uvhttp_middleware_func_t response_handler;  // 响应处理函数
    void* user_data;                            // 用户数据
} uvhttp_middleware_config_t;

// 全局中间件配置（编译时确定）
typedef struct {
#if UVHTTP_ENABLE_CORS
    uvhttp_middleware_config_t cors;
#endif
#if UVHTTP_ENABLE_RATE_LIMIT
    uvhttp_middleware_config_t rate_limit;
#endif
#if UVHTTP_ENABLE_AUTH
    uvhttp_middleware_config_t auth;
#endif
#if UVHTTP_ENABLE_COMPRESSION
    uvhttp_middleware_config_t compression;
#endif
#if UVHTTP_ENABLE_STATIC
    uvhttp_middleware_config_t static_files;
#endif
} uvhttp_middleware_global_config_t;
    // 简化的中间件API
uvhttp_error_t uvhttp_middleware_init_global_config(void);
uvhttp_error_t uvhttp_middleware_execute_request_handlers(uvhttp_request_t* request, 
                                                        uvhttp_response_t* response);
uvhttp_error_t uvhttp_middleware_execute_response_handlers(uvhttp_request_t* request, 
                                                         uvhttp_response_t* response);

// 中间件配置API（仅在相应功能启用时编译）
#if UVHTTP_ENABLE_CORS
uvhttp_error_t uvhttp_middleware_enable_cors(const char* allowed_origins, 
                                            const char* allowed_methods, 
                                            const char* allowed_headers);
#endif

#if UVHTTP_ENABLE_RATE_LIMIT
uvhttp_error_t uvhttp_middleware_set_rate_limit(int max_requests, int window_seconds);
#endif

#if UVHTTP_ENABLE_AUTH
uvhttp_error_t uvhttp_middleware_set_auth(const char* secret_key, const char* algorithm);
#endif

#if UVHTTP_ENABLE_COMPRESSION
uvhttp_error_t uvhttp_middleware_enable_compression(int level);
#endif

#if UVHTTP_ENABLE_STATIC
uvhttp_error_t uvhttp_middleware_set_static_root(const char* root_path);
#endif

// 中间件清理
void uvhttp_middleware_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_H */