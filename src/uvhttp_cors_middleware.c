/*
 * UVHTTP CORS 中间件实现
 */

#include "uvhttp_cors_middleware.h"
#include "uvhttp_allocator.h"
#include "uvhttp_utils.h"
#include <string.h>
#include <stdio.h>
#include <strings.h>  /* for strcasecmp */

/* 全局默认配置 */
static uvhttp_cors_config_t g_default_cors_config = {
    .allow_origin = "*",
    .allow_methods = "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH",
    .allow_headers = "Content-Type, Authorization, X-Requested-With",
    .expose_headers = "",
    .allow_credentials = "false",
    .max_age = "86400",
    .allow_all_origins = 1,
    .allow_credentials_enabled = 0
};

/* 创建默认 CORS 配置 */
uvhttp_cors_config_t* uvhttp_cors_config_default(void) {
    uvhttp_cors_config_t* config = (uvhttp_cors_config_t*)uvhttp_alloc(sizeof(uvhttp_cors_config_t));
    if (!config) {
        return NULL;
    }

    memcpy(config, &g_default_cors_config, sizeof(uvhttp_cors_config_t));
    return config;
}

/* 创建自定义 CORS 配置 */
uvhttp_cors_config_t* uvhttp_cors_config_create(
    const char* allow_origin,
    const char* allow_methods,
    const char* allow_headers
) {
    uvhttp_cors_config_t* config = (uvhttp_cors_config_t*)uvhttp_alloc(sizeof(uvhttp_cors_config_t));
    if (!config) {
        return NULL;
    }

    /* 初始化所有指针为 NULL */
    memset(config, 0, sizeof(uvhttp_cors_config_t));
    config->owns_strings = 1;  /* 标记拥有字符串 */

    /* 辅助函数：复制字符串 */
    char* copy_string(const char* src) {
        if (!src) return NULL;
        size_t len = strlen(src) + 1;
        char* copy = (char*)uvhttp_alloc(len);
        if (copy) {
            memcpy(copy, src, len);
        }
        return copy;
    }

    /* 复制自定义配置（复制字符串以确保安全） */
    if (allow_origin) {
        config->allow_origin = copy_string(allow_origin);
        if (config->allow_origin && strcmp(allow_origin, "*") == 0) {
            config->allow_all_origins = 1;
        }
    }

    if (allow_methods) {
        config->allow_methods = copy_string(allow_methods);
    }

    if (allow_headers) {
        config->allow_headers = copy_string(allow_headers);
    }

    /* 设置默认值（如果复制失败） */
    if (!config->allow_origin) {
        config->allow_origin = copy_string("*");
        config->allow_all_origins = 1;
    }
    if (!config->allow_methods) {
        config->allow_methods = copy_string("GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH");
    }
    if (!config->allow_headers) {
        config->allow_headers = copy_string("Content-Type, Authorization, X-Requested-With");
    }

    /* 设置其他默认值 */
    config->expose_headers = copy_string("");
    config->allow_credentials = copy_string("false");
    config->max_age = copy_string("86400");
    config->allow_credentials_enabled = 0;

    return config;
}

/* 销毁 CORS 配置 */
void uvhttp_cors_config_destroy(uvhttp_cors_config_t* config) {
    if (config) {
        /* 只释放拥有的字符串 */
        if (config->owns_strings) {
            if (config->allow_origin) {
                uvhttp_free(config->allow_origin);
            }
            if (config->allow_methods) {
                uvhttp_free(config->allow_methods);
            }
            if (config->allow_headers) {
                uvhttp_free(config->allow_headers);
            }
            if (config->expose_headers) {
                uvhttp_free(config->expose_headers);
            }
            if (config->allow_credentials) {
                uvhttp_free(config->allow_credentials);
            }
            if (config->max_age) {
                uvhttp_free(config->max_age);
            }
        }
        uvhttp_free(config);
    }
}

/* 检查是否为预检请求 */
int uvhttp_cors_is_preflight_request(const uvhttp_request_t* request) {
    return request->method == UVHTTP_OPTIONS;
}

/* 设置 CORS 响应头 */
void uvhttp_cors_set_headers(
    uvhttp_response_t* response,
    const uvhttp_cors_config_t* config,
    const char* origin
) {
    if (!response || !config) {
        return;
    }

    /* 设置 Access-Control-Allow-Origin */
    if (config->allow_all_origins) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    } else if (origin && config->allow_origin) {
        /* 如果指定了特定来源，检查是否匹配 */
        if (strcmp(config->allow_origin, origin) == 0) {
            uvhttp_response_set_header(response, "Access-Control-Allow-Origin", origin);
            /* 添加 Vary: Origin 头以防止缓存问题 */
            /* 检查是否已有 Vary 头 */
            const char* existing_vary = NULL;
            for (size_t i = 0; i < response->header_count; i++) {
                if (strcasecmp(response->headers[i].name, "Vary") == 0) {
                    existing_vary = response->headers[i].value;
                    break;
                }
            }
            
            if (existing_vary) {
                /* 检查 Origin 是否已在 Vary 中 */
                if (strstr(existing_vary, "Origin") == NULL) {
                    /* 追加 Origin 到现有 Vary 头 */
                    char new_vary[512];
                    snprintf(new_vary, sizeof(new_vary), "%s, Origin", existing_vary);
                    uvhttp_response_set_header(response, "Vary", new_vary);
                }
            } else {
                /* 没有现有 Vary 头，直接设置 */
                uvhttp_response_set_header(response, "Vary", "Origin");
            }
        }
    }

    /* 设置 Access-Control-Allow-Methods */
    if (config->allow_methods) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Methods", config->allow_methods);
    }

    /* 设置 Access-Control-Allow-Headers */
    if (config->allow_headers) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Headers", config->allow_headers);
    }

    /* 设置 Access-Control-Expose-Headers */
    if (config->expose_headers && strlen(config->expose_headers) > 0) {
        uvhttp_response_set_header(response, "Access-Control-Expose-Headers", config->expose_headers);
    }

    /* 设置 Access-Control-Allow-Credentials */
    if (config->allow_credentials_enabled) {
        uvhttp_response_set_header(response, "Access-Control-Allow-Credentials", "true");
    }

    /* 设置 Access-Control-Max-Age */
    if (config->max_age) {
        uvhttp_response_set_header(response, "Access-Control-Max-Age", config->max_age);
    }
}

/* CORS 中间件处理函数（用于动态中间件系统） */
int uvhttp_cors_middleware(
    const uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    if (!request || !response) {
        return UVHTTP_MIDDLEWARE_CONTINUE;
    }

    /* 获取请求的 Origin 头 */
    const char* origin = uvhttp_request_get_header((uvhttp_request_t*)request, "Origin");

    /* 获取或使用默认配置 */
    uvhttp_cors_config_t* config = NULL;
    if (ctx && ctx->data) {
        config = (uvhttp_cors_config_t*)ctx->data;
    } else {
        config = &g_default_cors_config;
    }

    /* 设置 CORS 响应头 */
    uvhttp_cors_set_headers(response, config, origin);

    /* 处理预检请求 */
    if (uvhttp_cors_is_preflight_request(request)) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_send(response);
        return UVHTTP_MIDDLEWARE_STOP;
    }

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* CORS 中间件处理函数（用于宏中间件系统） */
int uvhttp_cors_middleware_simple(
    const uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
) {
    /* 直接使用默认配置 */
    return uvhttp_cors_middleware(request, response, ctx);
}