#include <gtest/gtest.h>
#include <uvhttp_cors_middleware.h>
#include <string.h>

/* 测试创建自定义 CORS 配置 */
TEST(UvhttpCorsMiddlewareSimpleTest, ConfigCreate) {
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
TEST(UvhttpCorsMiddlewareSimpleTest, ConfigCreateAllowAll) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST", "Content-Type");
    ASSERT_NE(config, nullptr);
    EXPECT_STREQ(config->allow_origin, "*");
    EXPECT_EQ(config->allow_all_origins, 1);
    
    uvhttp_cors_config_destroy(config);
}

/* 测试创建自定义 CORS 配置 NULL 参数 */
TEST(UvhttpCorsMiddlewareSimpleTest, ConfigCreateNullParams) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(NULL, NULL, NULL);
    ASSERT_NE(config, nullptr);
    /* 应该使用默认值 */
    EXPECT_STREQ(config->allow_origin, "*");
    EXPECT_EQ(config->allow_all_origins, 1);
    
    uvhttp_cors_config_destroy(config);
}

/* 测试销毁 CORS 配置 NULL */
TEST(UvhttpCorsMiddlewareSimpleTest, ConfigDestroyNull) {
    uvhttp_cors_config_destroy(NULL);
    /* 不应该崩溃 */
}

/* 测试销毁 CORS 配置 */
TEST(UvhttpCorsMiddlewareSimpleTest, ConfigDestroy) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST", "Content-Type");
    ASSERT_NE(config, nullptr);
    
    uvhttp_cors_config_destroy(config);
    /* 不应该崩溃 */
}

/* 测试检查预检请求 NULL 请求 */
TEST(UvhttpCorsMiddlewareSimpleTest, IsPreflightRequestNull) {
    int result = uvhttp_cors_is_preflight_request(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试检查预检请求 OPTIONS 方法 */
TEST(UvhttpCorsMiddlewareSimpleTest, IsPreflightRequestOptions) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_OPTIONS;
    
    int result = uvhttp_cors_is_preflight_request(&request);
    EXPECT_EQ(result, 1);
}

/* 测试检查预检请求非 OPTIONS 方法 */
TEST(UvhttpCorsMiddlewareSimpleTest, IsPreflightRequestNotOptions) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_GET;
    
    int result = uvhttp_cors_is_preflight_request(&request);
    EXPECT_EQ(result, 0);
}