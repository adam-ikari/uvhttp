#include <gtest/gtest.h>
#include <uvhttp_cors_middleware.h>
#include <stdio.h>

/* 测试销毁 CORS 配置 NULL */
TEST(UvhttpCorsMiddlewareDebugTest, ConfigDestroyNull) {
    printf("Test ConfigDestroyNull started\n");
    fflush(stdout);
    uvhttp_cors_config_destroy(NULL);
    printf("Test ConfigDestroyNull passed\n");
    fflush(stdout);
    SUCCEED();
}

/* 测试检查预检请求 NULL 请求 */
TEST(UvhttpCorsMiddlewareDebugTest, IsPreflightRequestNull) {
    printf("Test IsPreflightRequestNull started\n");
    fflush(stdout);
    int result = uvhttp_cors_is_preflight_request(NULL);
    printf("Test IsPreflightRequestNull passed, result=%d\n", result);
    fflush(stdout);
    EXPECT_EQ(result, 0);
}

/* 测试创建自定义 CORS 配置 */
TEST(UvhttpCorsMiddlewareDebugTest, ConfigCreate) {
    printf("Test ConfigCreate started\n");
    fflush(stdout);
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(
        "https://example.com",
        "GET, POST",
        "Content-Type"
    );
    ASSERT_NE(config, nullptr);
    printf("Test ConfigCreate passed\n");
    fflush(stdout);
    uvhttp_cors_config_destroy(config);
}

/* 测试创建自定义 CORS 配置允许所有来源 */
TEST(UvhttpCorsMiddlewareDebugTest, ConfigCreateAllowAll) {
    printf("Test ConfigCreateAllowAll started\n");
    fflush(stdout);
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST", "Content-Type");
    ASSERT_NE(config, nullptr);
    printf("Test ConfigCreateAllowAll passed\n");
    fflush(stdout);
    uvhttp_cors_config_destroy(config);
}

/* 测试创建自定义 CORS 配置 NULL 参数 */
TEST(UvhttpCorsMiddlewareDebugTest, ConfigCreateNullParams) {
    printf("Test ConfigCreateNullParams started\n");
    fflush(stdout);
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(NULL, NULL, NULL);
    ASSERT_NE(config, nullptr);
    printf("Test ConfigCreateNullParams passed\n");
    fflush(stdout);
    uvhttp_cors_config_destroy(config);
}

/* 测试检查预检请求 OPTIONS 方法 */
TEST(UvhttpCorsMiddlewareDebugTest, IsPreflightRequestOptions) {
    printf("Test IsPreflightRequestOptions started\n");
    fflush(stdout);
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_OPTIONS;
    
    int result = uvhttp_cors_is_preflight_request(&request);
    printf("Test IsPreflightRequestOptions passed, result=%d\n", result);
    fflush(stdout);
    EXPECT_EQ(result, 1);
}

/* 测试检查预检请求非 OPTIONS 方法 */
TEST(UvhttpCorsMiddlewareDebugTest, IsPreflightRequestNotOptions) {
    printf("Test IsPreflightRequestNotOptions started\n");
    fflush(stdout);
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    request.method = UVHTTP_GET;
    
    int result = uvhttp_cors_is_preflight_request(&request);
    printf("Test IsPreflightRequestNotOptions passed, result=%d\n", result);
    fflush(stdout);
    EXPECT_EQ(result, 0);
}