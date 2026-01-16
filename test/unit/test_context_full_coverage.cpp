/* UVHTTP context 模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>
#include "uvhttp_context.h"
#include "uvhttp_allocator.h"
#include "test_loop_helper.h"

TEST(UvhttpContextFullCoverageTest, ContextCreate) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    
    if (context != nullptr) {
        EXPECT_EQ(context->loop, loop.get());
        EXPECT_EQ(context->initialized, 0);
        EXPECT_EQ(context->total_requests, 0);
        EXPECT_EQ(context->total_connections, 0);
        EXPECT_EQ(context->active_connections, 0);
        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, ContextDestroy) {
    uvhttp_context_destroy(nullptr);
    
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    if (context != nullptr) {
        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, ContextInit) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());

    if (context != nullptr) {
        EXPECT_EQ(uvhttp_context_init(context), 0);
        EXPECT_EQ(context->initialized, 1);
        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, ContextSetConnectionProvider) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());

    if (context != nullptr) {
        EXPECT_EQ(uvhttp_context_set_connection_provider(context, nullptr), 0);

        uvhttp_connection_provider_t* provider = uvhttp_default_connection_provider_create();
        if (provider) {
            EXPECT_EQ(uvhttp_context_set_connection_provider(context, provider), 0);
            EXPECT_EQ(context->connection_provider, provider);
        }

        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, ContextSetLoggerProvider) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    
    if (context != nullptr) {
        EXPECT_EQ(uvhttp_context_set_logger_provider(context, nullptr), 0);
        
        uvhttp_logger_provider_t* provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
        if (provider) {
            EXPECT_EQ(uvhttp_context_set_logger_provider(context, provider), 0);
            EXPECT_EQ(context->logger_provider, provider);
        }
        
        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, ContextSetConfigProvider) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    
    if (context != nullptr) {
        EXPECT_EQ(uvhttp_context_set_config_provider(context, nullptr), 0);
        
        uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
        if (provider) {
            EXPECT_EQ(uvhttp_context_set_config_provider(context, provider), 0);
            EXPECT_EQ(context->config_provider, provider);
        }
        
        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, ContextSetNetworkInterface) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    
    if (context != nullptr) {
        EXPECT_EQ(uvhttp_context_set_network_interface(context, nullptr), 0);
        
        uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(loop.get());
        if (interface) {
            EXPECT_EQ(uvhttp_context_set_network_interface(context, interface), 0);
            EXPECT_EQ(context->network_interface, interface);
            uvhttp_network_interface_destroy(interface);
        }
        
        uvhttp_context_destroy(context);
    }
}

TEST(UvhttpContextFullCoverageTest, DefaultConnectionProviderCreate) {
    uvhttp_connection_provider_t* provider = uvhttp_default_connection_provider_create();
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->acquire_connection, nullptr);
    EXPECT_NE(provider->release_connection, nullptr);
    
    if (provider->get_pool_size) {
        size_t pool_size = provider->get_pool_size(provider);
        EXPECT_GE(pool_size, 0);
    }
    
    if (provider->cleanup_expired) {
        provider->cleanup_expired(provider);
    }
}

TEST(UvhttpContextFullCoverageTest, DefaultLoggerProviderCreate) {
    uvhttp_logger_provider_t* provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->log, nullptr);
    EXPECT_NE(provider->set_level, nullptr);

    provider->log(provider, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test message");
    provider->set_level(provider, UVHTTP_LOG_LEVEL_ERROR);
}

TEST(UvhttpContextFullCoverageTest, DefaultConfigProviderCreate) {
    uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->get_string, nullptr);
    EXPECT_NE(provider->get_int, nullptr);
    EXPECT_NE(provider->set_string, nullptr);
    EXPECT_NE(provider->set_int, nullptr);

    const char* value = provider->get_string(provider, "test_key", "default");
    EXPECT_NE(value, nullptr);

    EXPECT_EQ(provider->set_string(provider, "test_key", "test_value"), 0);

    int int_value = provider->get_int(provider, "test_int", 42);
    EXPECT_EQ(int_value, 42);

    EXPECT_EQ(provider->set_int(provider, "test_int", 100), 0);
}

TEST(UvhttpContextFullCoverageTest, TestConnectionProviderCreate) {
    uvhttp_connection_provider_t* provider = uvhttp_test_connection_provider_create();
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->acquire_connection, nullptr);
    EXPECT_NE(provider->release_connection, nullptr);
}

TEST(UvhttpContextFullCoverageTest, TestLoggerProviderCreate) {
    uvhttp_logger_provider_t* provider = uvhttp_test_logger_provider_create();
    ASSERT_NE(provider, nullptr);
    EXPECT_NE(provider->log, nullptr);
    
    provider->log(provider, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test message");
}

TEST(UvhttpContextFullCoverageTest, TestContextInit) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    EXPECT_EQ(uvhttp_test_context_init(loop.get()), 0);

    uvhttp_context_t* context = uvhttp_test_get_context();
    ASSERT_NE(context, nullptr);
    EXPECT_EQ(context->loop, loop.get());

    uvhttp_test_context_cleanup();
}

TEST(UvhttpContextFullCoverageTest, TestContextResetStats) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_test_context_init(loop.get());

    uvhttp_context_t* context = uvhttp_test_get_context();
    if (context) {
        context->total_requests = 100;
        context->total_connections = 50;
        context->active_connections = 10;

        uvhttp_test_context_reset_stats();
        EXPECT_EQ(context->total_requests, 0);
        EXPECT_EQ(context->total_connections, 0);
        EXPECT_EQ(context->active_connections, 0);
    }

    uvhttp_test_context_cleanup();
}

TEST(UvhttpContextFullCoverageTest, MultipleCreateDestroy) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());

    for (int i = 0; i < 10; i++) {
        uvhttp_context_t* context = uvhttp_context_create(loop.get());
        if (context) {
            uvhttp_context_destroy(context);
        }
    }
}

TEST(UvhttpContextFullCoverageTest, EdgeCases) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());
    uvhttp_context_t* context = uvhttp_context_create(loop.get());
    if (context) {
        uvhttp_context_init(context);
        uvhttp_context_init(context);
        uvhttp_context_destroy(context);
    }
}