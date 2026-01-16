#include <gtest/gtest.h>
#include <uvhttp_cors_middleware.h>
#include <uvhttp_request.h>
#include <uvhttp_response.h>
#include <uvhttp_server.h>
#include <uvhttp_allocator.h>
#include <string.h>

/* 测试创建默认 CORS 配置 */
TEST(UvhttpCorsMiddlewareTest, ConfigDefault) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH", "Content-Type, Authorization, X-Requested-With");
    ASSERT_NE(config, nullptr);
    EXPECT_STREQ(config->allow_origin, "*");
    EXPECT_STREQ(config->allow_methods, "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH");
    EXPECT_STREQ(config->allow_headers, "Content-Type, Authorization, X-Requested-With");
    EXPECT_EQ(config->allow_all_origins, 1);
    EXPECT_EQ(config->allow_credentials_enabled, 0);
    
    uvhttp_cors_config_destroy(config);
}

/* 测试创建自定义 CORS 配置 */
TEST(UvhttpCorsMiddlewareTest, ConfigCreate) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(
        "https://example.com",
        "GET, POST",
        "Content-Type"
    );
    ASSERT_NE(config, nullptr);
    EXPECT_STREQ(config->allow_origin, "https://example.com");
    EXPECT_STREQ(config->allow_methods, "GET, POST");
    EXPECT_STREQ(config->allow_headers, "Content-Type");
    EXPECT_EQ(config->allow_all_origins, 0);
    EXPECT_EQ(config->owns_strings, 1);
    
    uvhttp_cors_config_destroy(config);
}

/* 测试创建自定义 CORS 配置允许所有来源 */
TEST(UvhttpCorsMiddlewareTest, ConfigCreateAllowAll) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST", "Content-Type");
    ASSERT_NE(config, nullptr);
    EXPECT_STREQ(config->allow_origin, "*");
    EXPECT_EQ(config->allow_all_origins, 1);
    
    uvhttp_cors_config_destroy(config);
}

/* 测试创建自定义 CORS 配置 NULL 参数 */
TEST(UvhttpCorsMiddlewareTest, ConfigCreateNullParams) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(NULL, NULL, NULL);
    ASSERT_NE(config, nullptr);
    /* 应该使用默认值 */
    EXPECT_STREQ(config->allow_origin, "*");
    EXPECT_EQ(config->allow_all_origins, 1);
    
    uvhttp_cors_config_destroy(config);
}

/* 测试销毁 CORS 配置 NULL */
TEST(UvhttpCorsMiddlewareTest, ConfigDestroyNull) {
    uvhttp_cors_config_destroy(NULL);
    /* 不应该崩溃 */
}

/* 测试销毁 CORS 配置 */
TEST(UvhttpCorsMiddlewareTest, ConfigDestroy) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH", "Content-Type, Authorization, X-Requested-With");
    ASSERT_NE(config, nullptr);
    
    uvhttp_cors_config_destroy(config);
    /* 不应该崩溃 */
}

/* 测试检查预检请求 NULL 请求 */
TEST(UvhttpCorsMiddlewareTest, IsPreflightRequestNull) {
    int result = uvhttp_cors_is_preflight_request(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试检查预检请求 OPTIONS 方法 */
TEST(UvhttpCorsMiddlewareTest, IsPreflightRequestOptions) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_OPTIONS;
    
    int result = uvhttp_cors_is_preflight_request(&request);
    EXPECT_EQ(result, 1); /* OPTIONS 方法是预检请求 */
}

/* 测试设置 CORS 头 NULL 响应 */
TEST(UvhttpCorsMiddlewareTest, SetHeadersNullResponse) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH", "Content-Type, Authorization, X-Requested-With");
    ASSERT_NE(config, nullptr);
    
    uvhttp_cors_set_headers(NULL, config, "https://example.com");
    /* 不应该崩溃 */
    
    uvhttp_cors_config_destroy(config);
}

/* 测试设置 CORS 头 NULL 配置 */
TEST(UvhttpCorsMiddlewareTest, SetHeadersNullConfig) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_cors_set_headers(&response, NULL, "https://example.com");
    /* 不应该崩溃 */
}

/* 测试设置 CORS 头 NULL 来源 */
TEST(UvhttpCorsMiddlewareTest, SetHeadersNullOrigin) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH", "Content-Type, Authorization, X-Requested-With");
    ASSERT_NE(config, nullptr);
    
    uvhttp_cors_set_headers(&response, config, NULL);
    /* 不应该崩溃 */
    
    uvhttp_cors_config_destroy(config);
}

/* 测试设置 CORS 头 */
TEST(UvhttpCorsMiddlewareTest, SetHeaders) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH", "Content-Type, Authorization, X-Requested-With");
    ASSERT_NE(config, nullptr);
    
    uvhttp_cors_set_headers(&response, config, "https://example.com");
    /* 不应该崩溃 */
    
    uvhttp_cors_config_destroy(config);
}

/* 测试设置 CORS 头允许所有来源 */
TEST(UvhttpCorsMiddlewareTest, SetHeadersAllowAll) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST, PUT, DELETE, OPTIONS, HEAD, PATCH", "Content-Type, Authorization, X-Requested-With");
    ASSERT_NE(config, nullptr);
    config->allow_all_origins = 1;
    
    uvhttp_cors_set_headers(&response, config, "https://example.com");
    /* 不应该崩溃 */
    
    uvhttp_cors_config_destroy(config);
}

/* 测试 CORS 中间件 NULL 请求 */
TEST(UvhttpCorsMiddlewareTest, MiddlewareNullRequest) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    int result = uvhttp_cors_middleware(NULL, &response, NULL);
    EXPECT_EQ(result, 0); /* 中间件应该继续执行 */
}

/* 测试 CORS 中间件 NULL 响应 */
TEST(UvhttpCorsMiddlewareTest, MiddlewareNullResponse) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    int result = uvhttp_cors_middleware(&request, NULL, NULL);
    EXPECT_EQ(result, 0); /* 中间件应该继续执行 */
}

/* 测试 CORS 中间件 */
TEST(UvhttpCorsMiddlewareTest, Middleware) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_GET;
    
    /* 设置 Origin 头 */
    strncpy(request.headers[0].name, "Origin", sizeof(request.headers[0].name) - 1);
    strncpy(request.headers[0].value, "https://example.com", sizeof(request.headers[0].value) - 1);
    request.header_count = 1;
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化 client 字段以避免段错误 */
    uv_tcp_t client;
    response.client = &client;
    
    int result = uvhttp_cors_middleware(&request, &response, NULL);
    EXPECT_EQ(result, 0); /* 中间件应该继续执行 */
}

/* 测试 CORS 中间件预检请求 */
TEST(UvhttpCorsMiddlewareTest, MiddlewarePreflight) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_OPTIONS;
    
    /* 设置 Origin 头 */
    strncpy(request.headers[0].name, "Origin", sizeof(request.headers[0].name) - 1);
    strncpy(request.headers[0].value, "https://example.com", sizeof(request.headers[0].value) - 1);
    
    /* 设置 Access-Control-Request-Method 头 */
    strncpy(request.headers[1].name, "Access-Control-Request-Method", sizeof(request.headers[1].name) - 1);
    strncpy(request.headers[1].value, "POST", sizeof(request.headers[1].value) - 1);
    request.header_count = 2;
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化 client 字段以避免段错误 */
    uv_tcp_t client;
    response.client = &client;
    
    int result = uvhttp_cors_middleware(&request, &response, NULL);
    EXPECT_EQ(result, 1); /* 预检请求应该停止中间件链 */
}

/* 测试 CORS 简单中间件 NULL 请求 */
TEST(UvhttpCorsMiddlewareTest, MiddlewareSimpleNullRequest) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    int result = uvhttp_cors_middleware_simple(NULL, &response, NULL);
    EXPECT_EQ(result, 0); /* 中间件应该继续执行 */
}

/* 测试 CORS 简单中间件 NULL 响应 */
TEST(UvhttpCorsMiddlewareTest, MiddlewareSimpleNullResponse) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    int result = uvhttp_cors_middleware_simple(&request, NULL, NULL);
    EXPECT_EQ(result, 0); /* 中间件应该继续执行 */
}

/* 测试 CORS 简单中间件 */
TEST(UvhttpCorsMiddlewareTest, MiddlewareSimple) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_GET;
    
    /* 设置 Origin 头 */
    strncpy(request.headers[0].name, "Origin", sizeof(request.headers[0].name) - 1);
    strncpy(request.headers[0].value, "https://example.com", sizeof(request.headers[0].value) - 1);
    request.header_count = 1;
    
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    /* 初始化 client 字段以避免段错误 */
    uv_tcp_t client;
    response.client = &client;
    
    int result = uvhttp_cors_middleware_simple(&request, &response, NULL);
    EXPECT_EQ(result, 0); /* 中间件应该继续执行 */
}