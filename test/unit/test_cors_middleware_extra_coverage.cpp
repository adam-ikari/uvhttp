/* UVHTTP CORS 中间件额外覆盖率测试
 * 
 * 目标：提升 uvhttp_cors_middleware.c 覆盖率从 77.3% 到 85%
 * 
 * 测试内容：
 * - uvhttp_cors_config_default: 创建默认 CORS 配置
 */

#include <gtest/gtest.h>
#include "uvhttp_cors_middleware.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_middleware.h"
#include <cstring>

/* 测试 uvhttp_cors_config_default */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefault) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 验证配置 */
    EXPECT_NE(config->allow_origin, nullptr);
    EXPECT_NE(config->allow_methods, nullptr);
    EXPECT_NE(config->allow_headers, nullptr);
    
    /* 验证 allow_all_origins */
    EXPECT_TRUE(config->allow_all_origins);
    
    /* 销毁配置 */
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default 配置值 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultValues) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 验证 allow_origin */
    EXPECT_STREQ(config->allow_origin, "*");
    
    /* 验证 allow_methods */
    EXPECT_STREQ(config->allow_methods, "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH");
    
    /* 验证 allow_headers */
    EXPECT_STREQ(config->allow_headers, "Content-Type, Authorization, X-Requested-With");
    
    /* 验证 allow_credentials_enabled */
    EXPECT_FALSE(config->allow_credentials_enabled);
    
    /* 销毁配置 */
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default 多次创建 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultMultiple) {
    /* 多次创建默认 CORS 配置 */
    uvhttp_cors_config_t* configs[3];
    
    for (int i = 0; i < 3; i++) {
        configs[i] = uvhttp_cors_config_default();
        ASSERT_NE(configs[i], nullptr);
    }
    
    /* 验证每个配置 */
    for (int i = 0; i < 3; i++) {
        EXPECT_STREQ(configs[i]->allow_origin, "*");
        EXPECT_TRUE(configs[i]->allow_all_origins);
    }
    
    /* 销毁所有配置 */
    for (int i = 0; i < 3; i++) {
        uvhttp_cors_config_destroy(configs[i]);
    }
}

/* 测试 uvhttp_cors_config_default 与自定义配置对比 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultVsCustom) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* default_config = uvhttp_cors_config_default();
    ASSERT_NE(default_config, nullptr);
    
    /* 创建自定义 CORS 配置 */
    uvhttp_cors_config_t* custom_config = uvhttp_cors_config_create(
        "https://example.com",
        "GET, POST",
        "Content-Type"
    );
    ASSERT_NE(custom_config, nullptr);
    
    /* 验证默认配置 */
    EXPECT_STREQ(default_config->allow_origin, "*");
    EXPECT_TRUE(default_config->allow_all_origins);
    
    /* 验证自定义配置 */
    EXPECT_STREQ(custom_config->allow_origin, "https://example.com");
    EXPECT_FALSE(custom_config->allow_all_origins);
    
    /* 销毁配置 */
    uvhttp_cors_config_destroy(default_config);
    uvhttp_cors_config_destroy(custom_config);
}

/* 测试 uvhttp_cors_config_default 用于预检请求 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultPreflight) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    /* 设置预检请求 */
    request.method = UVHTTP_OPTIONS;
    strcpy(request.headers[0].name, "Access-Control-Request-Method");
    strcpy(request.headers[0].value, "POST");
    strcpy(request.headers[1].name, "Origin");
    strcpy(request.headers[1].value, "https://example.com");
    request.header_count = 2;
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 检查是否为预检请求 */
    int is_preflight = uvhttp_cors_is_preflight_request(&request);
    EXPECT_TRUE(is_preflight);
    
    /* 设置 CORS 响应头 */
    uvhttp_cors_set_headers(&response, config, "https://example.com");
    
    /* 验证响应头已设置（通过验证 header_count） */
    EXPECT_GT(response.header_count, 0);
    
    /* 清理 */
    uvhttp_response_cleanup(&response);
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default 用于简单请求 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultSimpleRequest) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    uvhttp_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    /* 设置简单请求 */
    request.method = UVHTTP_GET;
    strcpy(request.headers[0].name, "Origin");
    strcpy(request.headers[0].value, "https://example.com");
    request.header_count = 1;
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 检查是否为预检请求 */
    int is_preflight = uvhttp_cors_is_preflight_request(&request);
    EXPECT_FALSE(is_preflight);
    
    /* 设置 CORS 响应头 */
    uvhttp_cors_set_headers(&response, config, "https://example.com");
    
    /* 验证响应头已设置（通过验证 header_count） */
    EXPECT_GT(response.header_count, 0);
    
    /* 清理 */
    uvhttp_response_cleanup(&response);
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default 用于中间件 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultMiddleware) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 创建请求和响应 */
    uvhttp_request_t request;
    uvhttp_response_t response;
    uvhttp_middleware_context_t ctx;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    memset(&ctx, 0, sizeof(ctx));
    
    /* 设置请求 */
    request.method = UVHTTP_GET;
    strcpy(request.headers[0].name, "Origin");
    strcpy(request.headers[0].value, "https://example.com");
    request.header_count = 1;
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 调用 CORS 中间件 */
    int result = uvhttp_cors_middleware(&request, &response, &ctx);
    EXPECT_EQ(result, 0);
    
    /* 验证响应头已设置（通过验证 header_count） */
    EXPECT_GT(response.header_count, 0);
    
    /* 清理 */
    uvhttp_response_cleanup(&response);
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default NULL 处理 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultNullHandling) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 创建响应 */
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化响应 */
    uvhttp_response_init(&response, NULL);
    
    /* 设置 CORS 响应头（origin 为 NULL） */
    uvhttp_cors_set_headers(&response, config, NULL);
    
    /* 验证响应头已设置（通过验证 header_count） */
    EXPECT_GT(response.header_count, 0);
    
    /* 清理 */
    uvhttp_response_cleanup(&response);
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default 所有来源 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultAllowAllOrigins) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 验证 allow_all_origins */
    EXPECT_TRUE(config->allow_all_origins);
    
    /* 验证 allow_origin 为 "*" */
    EXPECT_STREQ(config->allow_origin, "*");
    
    /* 清理 */
    uvhttp_cors_config_destroy(config);
}

/* 测试 uvhttp_cors_config_default 凭证 */
TEST(UvhttpCorsMiddlewareExtraTest, CorsConfigDefaultCredentials) {
    /* 创建默认 CORS 配置 */
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    ASSERT_NE(config, nullptr);
    
    /* 验证 allow_credentials_enabled */
    EXPECT_FALSE(config->allow_credentials_enabled);
    
    /* 验证 allow_credentials */
    EXPECT_STREQ(config->allow_credentials, "false");
    
    /* 清理 */
    uvhttp_cors_config_destroy(config);
}