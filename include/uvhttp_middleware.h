#ifndef UVHTTP_MIDDLEWARE_H
#define UVHTTP_MIDDLEWARE_H

#include "uvhttp_request.h"
#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

// 中间件函数类型定义
typedef int (*uvhttp_middleware_func_t)(uvhttp_request_t* request, 
                                        uvhttp_response_t* response, 
                                        void* next_data);

// 中间件链节点
typedef struct uvhttp_middleware {
    uvhttp_middleware_func_t func;
    void* data;
    struct uvhttp_middleware* next;
} uvhttp_middleware_t;

// 中间件链
typedef struct {
    uvhttp_middleware_t* head;
    uvhttp_middleware_t* tail;
    int count;
} uvhttp_middleware_chain_t;

// 创建中间件链
uvhttp_middleware_chain_t* uvhttp_middleware_chain_new(void);

// 释放中间件链
void uvhttp_middleware_chain_free(uvhttp_middleware_chain_t* chain);

// 添加中间件到链尾
int uvhttp_middleware_chain_add(uvhttp_middleware_chain_t* chain, 
                               uvhttp_middleware_func_t func, 
                               void* data);

// 执行中间件链
int uvhttp_middleware_chain_execute(uvhttp_middleware_chain_t* chain,
                                   uvhttp_request_t* request,
                                   uvhttp_response_t* response);

// 内置中间件函数

// CORS中间件数据
typedef struct {
    const char* allow_origin;
    const char* allow_methods;
    const char* allow_headers;
    const char* max_age;
} uvhttp_cors_middleware_data_t;

// 创建CORS中间件数据
uvhttp_cors_middleware_data_t* uvhttp_cors_middleware_data_new(const char* allow_origin,
                                                             const char* allow_methods,
                                                             const char* allow_headers,
                                                             const char* max_age);

// 释放CORS中间件数据
void uvhttp_cors_middleware_data_free(uvhttp_cors_middleware_data_t* data);

// CORS中间件函数
int uvhttp_cors_middleware(uvhttp_request_t* request,
                          uvhttp_response_t* response,
                          void* next_data);

// 限流中间件数据
typedef struct {
    int max_requests_per_minute;
    int current_requests;
    time_t window_start;
} uvhttp_rate_limit_middleware_data_t;

// 创建限流中间件数据
uvhttp_rate_limit_middleware_data_t* uvhttp_rate_limit_middleware_data_new(int max_requests_per_minute);

// 释放限流中间件数据
void uvhttp_rate_limit_middleware_data_free(uvhttp_rate_limit_middleware_data_t* data);

// 限流中间件函数
int uvhttp_rate_limit_middleware(uvhttp_request_t* request,
                                uvhttp_response_t* response,
                                void* next_data);

// 静态文件中间件数据
typedef struct {
    char* root_directory;       // 静态文件根目录
    char* url_prefix;          // URL前缀，如 "/static"
    int auto_index;            // 是否自动生成目录索引
    int enable_cache;          // 是否启用缓存
    int max_cache_size;        // 最大缓存大小（字节）
    char* index_file;          // 默认索引文件名，如 "index.html"
} uvhttp_static_middleware_data_t;

// 创建静态文件中间件数据
uvhttp_static_middleware_data_t* uvhttp_static_middleware_data_new(const char* root_directory,
                                                                  const char* url_prefix,
                                                                  int auto_index,
                                                                  int enable_cache,
                                                                  int max_cache_size,
                                                                  const char* index_file);

// 释放静态文件中间件数据
void uvhttp_static_middleware_data_free(uvhttp_static_middleware_data_t* data);

// 静态文件中间件函数
int uvhttp_static_middleware(uvhttp_request_t* request,
                            uvhttp_response_t* response,
                            void* next_data);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_H */