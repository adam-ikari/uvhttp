#include <gtest/gtest.h>
#include <stddef.h>
#include <uv.h>
#include "uvhttp_context.h"
#include "test_loop_helper.h"

/* 测试上下文创建和销毁 */
TEST(UvhttpContextSimpleTest, CreateDestroy) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    ASSERT_NE(context, nullptr);
    EXPECT_EQ(context->loop, loop.get());
    EXPECT_EQ(context->initialized, 0);
    
    uvhttp_context_destroy(context);
}

/* 测试日志提供者 */
TEST(UvhttpContextSimpleTest, LoggerProvider) {
    uvhttp_logger_provider_t* provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->log, nullptr);
    EXPECT_NE(provider->set_level, nullptr);
    
    provider->log(provider, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test message");
    provider->set_level(provider, UVHTTP_LOG_LEVEL_ERROR);
}

/* 测试配置提供者 */
TEST(UvhttpContextSimpleTest, ConfigProvider) {
    uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->get_string, nullptr);
    EXPECT_NE(provider->get_int, nullptr);
    EXPECT_NE(provider->set_string, nullptr);
    EXPECT_NE(provider->set_int, nullptr);
    
    const char* value = provider->get_string(provider, "test_key", "default");
    ASSERT_NE(value, nullptr);
    
    EXPECT_EQ(provider->set_string(provider, "test_key", "test_value"), 0);
    
    int int_value = provider->get_int(provider, "test_int", 42);
    EXPECT_EQ(int_value, 42);
    
    EXPECT_EQ(provider->set_int(provider, "test_int", 100), 0);
}

/* 测试上下文与提供者集成 */
TEST(UvhttpContextSimpleTest, ContextWithProviders) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    ASSERT_NE(context, nullptr);
    
    uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    uvhttp_config_provider_t* config = uvhttp_default_config_provider_create();
    
    EXPECT_EQ(uvhttp_context_set_logger_provider(context, logger), 0);
    EXPECT_EQ(uvhttp_context_set_config_provider(context, config), 0);
    
    if (context->logger_provider) {
        context->logger_provider->log(context->logger_provider, UVHTTP_LOG_LEVEL_INFO, 
                                      __FILE__, __LINE__, __func__, "Test via context");
    }
    
    uvhttp_context_destroy(context);
}