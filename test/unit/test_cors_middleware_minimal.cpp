#include <gtest/gtest.h>
#include <uvhttp_cors_middleware.h>

/* 测试销毁 CORS 配置 NULL */
TEST(UvhttpCorsMiddlewareMinimalTest, ConfigDestroyNull) {
    uvhttp_cors_config_destroy(NULL);
    SUCCEED();
}

/* 测试检查预检请求 NULL 请求 */
TEST(UvhttpCorsMiddlewareMinimalTest, IsPreflightRequestNull) {
    int result = uvhttp_cors_is_preflight_request(NULL);
    EXPECT_EQ(result, 0);
}