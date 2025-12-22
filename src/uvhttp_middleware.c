/*
 * uvhttp_middleware.c
 * 中间件系统实现
 */

#include "uvhttp_middleware.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_utils.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

// 全局中间件配置
static uvhttp_middleware_set_t g_middleware = {0};

/* 中间件链执行函数 */
uvhttp_error_t uvhttp_middleware_chain_execute(uvhttp_middleware_chain_t* chain,
                                             uvhttp_request_t* request,
                                             uvhttp_response_t* response) {
    if (!chain || !request || !response) {
        return UVHTTP_ERROR_INVALID_ARGUMENT;
    }
    
    for (size_t i = 0; i < chain->middleware_count; i++) {
        uvhttp_middleware_t* middleware = chain->middleware[i];
        if (!middleware) continue;
        
        uvhttp_error_t result = middleware->func(middleware, request, response);
        if (result != UVHTTP_ERROR_OK) {
            return result;
        }
        
        /* 检查是否需要终止中间件链 */
        if (response->status_code >= 400) {
            break;
        }
    }
    
    return UVHTTP_ERROR_OK;
}

// 中间件初始化
uvhttp_error_t uvhttp_middleware_init(uvhttp_middleware_set_t* middleware) {
    if (!middleware) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    
    memset(middleware, 0, sizeof(uvhttp_middleware_set_t));
    
    // 根据编译选项设置中间件处理器
#if UVHTTP_ENABLE_CORS
    middleware->cors_handler = uvhttp_cors_middleware;
#endif
#if UVHTTP_ENABLE_RATE_LIMIT
    middleware->rate_limit_handler = uvhttp_rate_limit_middleware;
#endif
#if UVHTTP_ENABLE_AUTH
    middleware->auth_handler = uvhttp_auth_middleware;
#endif
#if UVHTTP_ENABLE_COMPRESSION
    middleware->compression_handler = uvhttp_compression_middleware;
#endif
#if UVHTTP_ENABLE_STATIC
    middleware->static_handler = uvhttp_static_middleware;
#endif
    
    return UVHTTP_OK;
}

/* 添加中间件到链 */
uvhttp_error_t uvhttp_middleware_chain_add(uvhttp_middleware_chain_t* chain,
                                          uvhttp_middleware_t* middleware) {
    if (!chain || !middleware) {
        return UVHTTP_ERROR_INVALID_ARGUMENT;
    }
    
    if (chain->middleware_count >= chain->capacity) {
        /* 扩容 */
        size_t new_capacity = chain->capacity * 2;
        uvhttp_middleware_t** new_middleware = realloc(chain->middleware, 
                                                      sizeof(uvhttp_middleware_t*) * new_capacity);
        if (!new_middleware) {
            return UVHTTP_ERROR_OUT_OF_MEMORY;
        }
        
        chain->middleware = new_middleware;
        chain->capacity = new_capacity;
    }
    
    chain->middleware[chain->middleware_count++] = middleware;
    return UVHTTP_ERROR_OK;
}

void uvhttp_middleware_cleanup(uvhttp_middleware_set_t* middleware) {
    if (!middleware) {
        return;
    }
    
    // 清理所有中间件处理器
    memset(middleware, 0, sizeof(uvhttp_middleware_set_t));
}

/* 创建中间件 */
uvhttp_middleware_t* uvhttp_middleware_new(uvhttp_middleware_type_t type,
                                          uvhttp_middleware_func_t func,
                                          void* user_data) {
    uvhttp_middleware_t* middleware = UVHTTP_MALLOC(sizeof(uvhttp_middleware_t));
    if (!middleware) {
        return NULL;
    }
    
    middleware->type = type;
    middleware->func = func;
    middleware->user_data = user_data;
    middleware->next = NULL;
    
    return middleware;
}

/* 销毁中间件 */
void uvhttp_middleware_free(uvhttp_middleware_t* middleware) {
    if (!middleware) {
        return;
    }
    
    /* 如果有用户数据销毁函数，调用它 */
    if (middleware->user_data) {
        free(middleware->user_data);
    }
    
    UVHTTP_FREE(middleware);
}

/* CORS 中间件实现 */
static uvhttp_error_t cors_middleware_func(uvhttp_middleware_t* middleware,
                                          uvhttp_request_t* request,
                                          uvhttp_response_t* response) {
    uvhttp_cors_config_t* config = (uvhttp_cors_config_t*)middleware->user_data;
    if (!config) {
        return UVHTTP_ERROR_INVALID_ARGUMENT;
    }
    
    /* 设置 CORS 头 */
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", 
                              config->allowed_origins ? config->allowed_origins : "*");
    
    if (config->allowed_methods) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Methods", 
                                  config->allowed_methods);
    } else {
        uvhttp_response_set_header(response, "Access-Control-Allow-Methods", 
                                  "GET, POST, PUT, DELETE, OPTIONS");
    }
    
    if (config->allowed_headers) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Headers", 
                                  config->allowed_headers);
    } else {
        uvhttp_response_set_header(response, "Access-Control-Allow-Headers", 
                                  "Content-Type, Authorization");
    }
    
    if (config->exposed_headers) {
        uvhttp_response_set_header(response, "Access-Control-Expose-Headers", 
                                  config->exposed_headers);
    }
    
    if (config->allow_credentials) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Credentials", "true");
    }
    
    if (config->max_age > 0) {
        char max_age_str[16];
        snprintf(max_age_str, sizeof(max_age_str), "%d", config->max_age);
        uvhttp_response_set_header(response, "Access-Control-Max-Age", max_age_str);
    }
    
    /* 处理 OPTIONS 预检请求 */
    const char* method = uvhttp_request_get_method(request);
    if (strcmp(method, "OPTIONS") == 0) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_body(response, "", 0);
        return UVHTTP_ERROR_OK;
    }
    
    return UVHTTP_ERROR_OK;
}

/* 创建 CORS 中间件 */
uvhttp_middleware_t* uvhttp_cors_middleware_new(const uvhttp_cors_config_t* config) {
    if (!config) {
        /* 使用默认配置 */
        static uvhttp_cors_config_t default_config = {
            .allowed_origins = "*",
            .allowed_methods = "GET, POST, PUT, DELETE, OPTIONS",
            .allowed_headers = "Content-Type, Authorization",
            .exposed_headers = NULL,
            .allow_credentials = 0,
            .max_age = 86400
        };
        config = &default_config;
    }
    
    /* 复制配置 */
    uvhttp_cors_config_t* config_copy = UVHTTP_MALLOC(sizeof(uvhttp_cors_config_t));
    if (!config_copy) {
        return NULL;
    }
    
    memcpy(config_copy, config, sizeof(uvhttp_cors_config_t));
    
    /* 复制字符串字段 */
    if (config->allowed_origins) {
        config_copy->allowed_origins = strdup(config->allowed_origins);
    }
    if (config->allowed_methods) {
        config_copy->allowed_methods = strdup(config->allowed_methods);
    }
    if (config->allowed_headers) {
        config_copy->allowed_headers = strdup(config->allowed_headers);
    }
    if (config->exposed_headers) {
        config_copy->exposed_headers = strdup(config->exposed_headers);
    }
    
    return uvhttp_middleware_new(UVHTTP_MIDDLEWARE_REQUEST, cors_middleware_func, config_copy);
}

/* 速率限制中间件实现 */
static uvhttp_error_t rate_limit_middleware_func(uvhttp_middleware_t* middleware,
                                                uvhttp_request_t* request,
                                                uvhttp_response_t* response) {
    uvhttp_rate_limit_config_t* config = (uvhttp_rate_limit_config_t*)middleware->user_data;
    if (!config) {
        return UVHTTP_ERROR_INVALID_ARGUMENT;
    }
    
    /* 获取客户端 IP */
    const char* client_ip = uvhttp_request_get_remote_address(request);
    if (!client_ip) {
        client_ip = "unknown";
    }
    
    /* 简单的内存速率限制实现 */
    /* 在生产环境中应该使用更高效的实现，如 Redis */
    static struct {
        char ip[64];
        time_t reset_time;
        int request_count;
    } rate_limit_map[1000];
    static int map_size = 0;
    
    time_t current_time = time(NULL);
    
    /* 查找或创建客户端记录 */
    int found = 0;
    for (int i = 0; i < map_size; i++) {
        if (strcmp(rate_limit_map[i].ip, client_ip) == 0) {
            found = 1;
            
            /* 检查是否需要重置 */
            if (current_time >= rate_limit_map[i].reset_time) {
                rate_limit_map[i].request_count = 1;
                rate_limit_map[i].reset_time = current_time + config->window_seconds;
            } else {
                rate_limit_map[i].request_count++;
                
                /* 检查是否超过限制 */
                if (rate_limit_map[i].request_count > config->max_requests) {
                    uvhttp_response_set_status(response, 429);
                    uvhttp_response_set_header(response, "Retry-After", 
                                             config->window_seconds);
                    
                    if (config->error_message) {
                        uvhttp_response_set_body(response, config->error_message, 
                                               strlen(config->error_message));
                    } else {
                        const char* msg = "Rate limit exceeded";
                        uvhttp_response_set_body(response, msg, strlen(msg));
                    }
                    
                    return UVHTTP_ERROR_RATE_LIMITED;
                }
            }
            break;
        }
    }
    
    /* 如果没有找到记录，创建新记录 */
    if (!found && map_size < 1000) {
        strncpy(rate_limit_map[map_size].ip, client_ip, sizeof(rate_limit_map[map_size].ip) - 1);
        rate_limit_map[map_size].ip[sizeof(rate_limit_map[map_size].ip) - 1] = '\0';
        rate_limit_map[map_size].request_count = 1;
        rate_limit_map[map_size].reset_time = current_time + config->window_seconds;
        map_size++;
    }
    
    /* 添加速率限制头 */
    uvhttp_response_set_header(response, "X-RateLimit-Limit", "100");
    uvhttp_response_set_header(response, "X-RateLimit-Remaining", "99");
    uvhttp_response_set_header(response, "X-RateLimit-Reset", "3600");
    
    return UVHTTP_ERROR_OK;
}

/* 创建速率限制中间件 */
uvhttp_middleware_t* uvhttp_rate_limit_middleware_new(const uvhttp_rate_limit_config_t* config) {
    if (!config) {
        /* 使用默认配置 */
        static uvhttp_rate_limit_config_t default_config = {
            .max_requests = 100,
            .window_seconds = 3600,
            .error_message = "Rate limit exceeded. Please try again later."
        };
        config = &default_config;
    }
    
    /* 复制配置 */
    uvhttp_rate_limit_config_t* config_copy = UVHTTP_MALLOC(sizeof(uvhttp_rate_limit_config_t));
    if (!config_copy) {
        return NULL;
    }
    
    memcpy(config_copy, config, sizeof(uvhttp_rate_limit_config_t));
    
    /* 复制错误消息 */
    if (config->error_message) {
        config_copy->error_message = strdup(config->error_message);
    }
    
    return uvhttp_middleware_new(UVHTTP_MIDDLEWARE_REQUEST, rate_limit_middleware_func, config_copy);
}

/* 日志中间件实现 */
static uvhttp_error_t logging_middleware_func(uvhttp_middleware_t* middleware,
                                             uvhttp_request_t* request,
                                             uvhttp_response_t* response) {
    /* 获取请求信息 */
    const char* method = uvhttp_request_get_method(request);
    const char* path = uvhttp_request_get_path(request);
    const char* client_ip = uvhttp_request_get_remote_address(request);
    const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
    
    if (!client_ip) client_ip = "unknown";
    if (!user_agent) user_agent = "unknown";
    
    /* 记录请求日志 */
    time_t now = time(NULL);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", localtime(&now));
    
    printf("[%s] %s %s from %s (%s)\n", time_str, method, path, client_ip, user_agent);
    
    /* 在响应完成后记录响应日志 */
    /* 这里简化处理，实际应该在响应发送后记录 */
    
    return UVHTTP_ERROR_OK;
}

/* 创建日志中间件 */
uvhttp_middleware_t* uvhttp_logging_middleware_new(void) {
    return uvhttp_middleware_new(UVHTTP_MIDDLEWARE_REQUEST, logging_middleware_func, NULL);
}

/* 认证中间件实现 */
static uvhttp_error_t auth_middleware_func(uvhttp_middleware_t* middleware,
                                          uvhttp_request_t* request,
                                          uvhttp_response_t* response) {
    uvhttp_auth_config_t* config = (uvhttp_auth_config_t*)middleware->user_data;
    if (!config) {
        return UVHTTP_ERROR_INVALID_ARGUMENT;
    }
    
    /* 获取 Authorization 头 */
    const char* auth_header = uvhttp_request_get_header(request, "Authorization");
    if (!auth_header) {
        uvhttp_response_set_status(response, 401);
        uvhttp_response_set_header(response, "WWW-Authenticate", "Bearer realm=\"uvhttp\"");
        
        if (config->error_message) {
            uvhttp_response_set_body(response, config->error_message, 
                                   strlen(config->error_message));
        } else {
            const char* msg = "Unauthorized";
            uvhttp_response_set_body(response, msg, strlen(msg));
        }
        
        return UVHTTP_ERROR_UNAUTHORIZED;
    }
    
    /* 简单的 Bearer Token 验证 */
    if (strncmp(auth_header, "Bearer ", 7) != 0) {
        uvhttp_response_set_status(response, 401);
        const char* msg = "Invalid authorization format";
        uvhttp_response_set_body(response, msg, strlen(msg));
        return UVHTTP_ERROR_UNAUTHORIZED;
    }
    
    const char* token = auth_header + 7;
    
    /* 这里应该实现真正的 token 验证逻辑 */
    /* 简化实现：检查 token 是否匹配预定义的值 */
    if (!config->token_validator || !config->token_validator(token)) {
        uvhttp_response_set_status(response, 401);
        const char* msg = "Invalid token";
        uvhttp_response_set_body(response, msg, strlen(msg));
        return UVHTTP_ERROR_UNAUTHORIZED;
    }
    
    /* 设置用户信息到请求上下文 */
    /* 这里简化处理 */
    
    return UVHTTP_ERROR_OK;
}

/* 创建认证中间件 */
uvhttp_middleware_t* uvhttp_auth_middleware_new(const uvhttp_auth_config_t* config) {
    if (!config) {
        return NULL;
    }
    
    /* 复制配置 */
    uvhttp_auth_config_t* config_copy = UVHTTP_MALLOC(sizeof(uvhttp_auth_config_t));
    if (!config_copy) {
        return NULL;
    }
    
    memcpy(config_copy, config, sizeof(uvhttp_auth_config_t));
    
    /* 复制错误消息 */
    if (config->error_message) {
        config_copy->error_message = strdup(config->error_message);
    }
    
    return uvhttp_middleware_new(UVHTTP_MIDDLEWARE_REQUEST, auth_middleware_func, config_copy);
}