/* UVHTTP context 模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <uv.h>
#include "uvhttp_context.h"
#include "uvhttp_allocator.h"

/* 测试上下文创建 */
void test_context_create(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    if (context != NULL) {
        assert(context->loop == loop);
        assert(context->initialized == 0);
        assert(context->total_requests == 0);
        assert(context->total_connections == 0);
        assert(context->active_connections == 0);
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_create: PASSED\n");
}

/* 测试上下文销毁 */
void test_context_destroy(void) {
    /* 测试 NULL 参数 */
    uvhttp_context_destroy(NULL);
    
    /* 测试正常销毁 */
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    if (context != NULL) {
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_destroy: PASSED\n");
}

/* 测试上下文初始化 */
void test_context_init(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    if (context != NULL) {
        int result = uvhttp_context_init(context);
        assert(result == 0);
        assert(context->initialized == 1);
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_init: PASSED\n");
}

/* 测试上下文设置连接提供者 */
void test_context_set_connection_provider(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    if (context != NULL) {
        /* 测试 NULL 参数 */
        int result = uvhttp_context_set_connection_provider(context, NULL);
        assert(result == 0);
        
        /* 测试正常设置 */
        uvhttp_connection_provider_t* provider = uvhttp_default_connection_provider_create();
        if (provider) {
            result = uvhttp_context_set_connection_provider(context, provider);
            assert(result == 0);
            assert(context->connection_provider == provider);
        }
        
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_set_connection_provider: PASSED\n");
}

/* 测试上下文设置日志提供者 */
void test_context_set_logger_provider(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    if (context != NULL) {
        /* 测试 NULL 参数 */
        int result = uvhttp_context_set_logger_provider(context, NULL);
        assert(result == 0);
        
        /* 测试正常设置 */
        uvhttp_logger_provider_t* provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
        if (provider) {
            result = uvhttp_context_set_logger_provider(context, provider);
            assert(result == 0);
            assert(context->logger_provider == provider);
        }
        
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_set_logger_provider: PASSED\n");
}

/* 测试上下文设置配置提供者 */
void test_context_set_config_provider(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    if (context != NULL) {
        /* 测试 NULL 参数 */
        int result = uvhttp_context_set_config_provider(context, NULL);
        assert(result == 0);
        
        /* 测试正常设置 */
        uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
        if (provider) {
            result = uvhttp_context_set_config_provider(context, provider);
            assert(result == 0);
            assert(context->config_provider == provider);
        }
        
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_set_config_provider: PASSED\n");
}

/* 测试上下文设置网络接口 */
void test_context_set_network_interface(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    
    if (context != NULL) {
        /* 测试 NULL 参数 */
        int result = uvhttp_context_set_network_interface(context, NULL);
        assert(result == 0);
        
        /* 测试正常设置 */
        uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(loop);
        if (interface) {
            result = uvhttp_context_set_network_interface(context, interface);
            assert(result == 0);
            assert(context->network_interface == interface);
            uvhttp_network_interface_destroy(interface);
        }
        
        uvhttp_context_destroy(context);
    }
    
    printf("test_context_set_network_interface: PASSED\n");
}

/* 测试默认连接提供者创建 */
void test_default_connection_provider_create(void) {
    uvhttp_connection_provider_t* provider = uvhttp_default_connection_provider_create();
    assert(provider != NULL);
    assert(provider->acquire_connection != NULL);
    assert(provider->release_connection != NULL);
    
    /* 测试连接池统计 */
    if (provider->get_pool_size) {
        size_t pool_size = provider->get_pool_size(provider);
        assert(pool_size >= 0);
    }
    
    /* 测试清理过期连接 */
    if (provider->cleanup_expired) {
        provider->cleanup_expired(provider);
    }
    
    printf("test_default_connection_provider_create: PASSED\n");
}

/* 测试默认日志提供者创建 */
void test_default_logger_provider_create(void) {
    uvhttp_logger_provider_t* provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    assert(provider != NULL);
    assert(provider->log != NULL);
    assert(provider->set_level != NULL);

    /* 测试日志输出 */
    provider->log(provider, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test message");

    /* 测试设置级别 */
    provider->set_level(provider, UVHTTP_LOG_LEVEL_ERROR);

    printf("test_default_logger_provider_create: PASSED\n");
    fflush(stdout);
}

/* 测试默认配置提供者创建 */
void test_default_config_provider_create(void) {
    printf("test_default_config_provider_create: START\n");
    uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
    assert(provider != NULL);
    assert(provider->get_string != NULL);
    assert(provider->get_int != NULL);
    assert(provider->set_string != NULL);
    assert(provider->set_int != NULL);
    
    /* 测试获取配置值 */
    const char* value = provider->get_string(provider, "test_key", "default");
    assert(value != NULL);
    
    /* 测试设置配置值 */
    int result = provider->set_string(provider, "test_key", "test_value");
    assert(result == 0);
    
    /* 测试获取整数配置 */
    int int_value = provider->get_int(provider, "test_int", 42);
    assert(int_value == 42);
    
    /* 测试设置整数配置 */
    result = provider->set_int(provider, "test_int", 100);
    assert(result == 0);
    
    printf("test_default_config_provider_create: PASSED\n");
}

/* 测试测试连接提供者创建 */
void test_test_connection_provider_create(void) {
    printf("test_test_connection_provider_create: START\n");
    fflush(stdout);
    uvhttp_connection_provider_t* provider = uvhttp_test_connection_provider_create();
    assert(provider != NULL);
    assert(provider->acquire_connection != NULL);
    assert(provider->release_connection != NULL);

    printf("test_test_connection_provider_create: PASSED\n");
    fflush(stdout);
}

/* 测试测试日志提供者创建 */
void test_test_logger_provider_create(void) {
    uvhttp_logger_provider_t* provider = uvhttp_test_logger_provider_create();
    assert(provider != NULL);
    assert(provider->log != NULL);
    
    /* 测试日志输出（应该静默） */
    provider->log(provider, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test message");
    
    printf("test_test_logger_provider_create: PASSED\n");
}

/* 测试测试上下文初始化 */
void test_test_context_init(void) {
    printf("test_test_context_init: START\n");
    fflush(stdout);
    uv_loop_t* loop = uv_default_loop();
    printf("test_test_context_init: Got loop\n");
    fflush(stdout);
    int result = uvhttp_test_context_init(loop);
    printf("test_test_context_init: uvhttp_test_context_init returned %d\n", result);
    fflush(stdout);
    assert(result == 0);

    /* 测试获取测试上下文 */
    uvhttp_context_t* context = uvhttp_test_get_context();
    printf("test_test_context_init: Got context\n");
    fflush(stdout);
    assert(context != NULL);
    assert(context->loop == loop);

    printf("test_test_context_init: About to call uvhttp_test_context_cleanup\n");
    fflush(stdout);
    uvhttp_test_context_cleanup();
    printf("test_test_context_init: Called uvhttp_test_context_cleanup\n");
    fflush(stdout);

    printf("test_test_context_init: PASSED\n");
    fflush(stdout);
}

/* 测试测试上下文统计重置 */
void test_test_context_reset_stats(void) {
    printf("test_test_context_reset_stats: START\n");
    fflush(stdout);
    uv_loop_t* loop = uv_default_loop();
    printf("test_test_context_reset_stats: Got loop\n");
    fflush(stdout);
    uvhttp_test_context_init(loop);
    printf("test_test_context_reset_stats: Called uvhttp_test_context_init\n");
    fflush(stdout);

    uvhttp_context_t* context = uvhttp_test_get_context();
    printf("test_test_context_reset_stats: Got context\n");
    fflush(stdout);
    if (context) {
        context->total_requests = 100;
        context->total_connections = 50;
        context->active_connections = 10;

        printf("test_test_context_reset_stats: About to call uvhttp_test_context_reset_stats\n");
        fflush(stdout);
        uvhttp_test_context_reset_stats();
        printf("test_test_context_reset_stats: Called uvhttp_test_context_reset_stats\n");
        fflush(stdout);
        assert(context->total_requests == 0);
        assert(context->total_connections == 0);
        assert(context->active_connections == 0);
    }

    printf("test_test_context_reset_stats: About to call uvhttp_test_context_cleanup\n");
    fflush(stdout);
    uvhttp_test_context_cleanup();
    printf("test_test_context_reset_stats: Called uvhttp_test_context_cleanup\n");
    fflush(stdout);

    printf("test_test_context_reset_stats: PASSED\n");
    fflush(stdout);
}

/* 测试多次创建和销毁 */
void test_multiple_create_destroy(void) {
    printf("test_multiple_create_destroy: START\n");
    fflush(stdout);
    uv_loop_t* loop = uv_default_loop();

    for (int i = 0; i < 10; i++) {
        printf("test_multiple_create_destroy: Iteration %d\n", i);
        fflush(stdout);
        uvhttp_context_t* context = uvhttp_context_create(loop);
        if (context) {
            uvhttp_context_destroy(context);
        }
    }

    printf("test_multiple_create_destroy: PASSED\n");
    fflush(stdout);
}

/* 测试边界条件 */
void test_edge_cases(void) {
    printf("test_edge_cases: START\n");
    fflush(stdout);

    /* 测试多次初始化 */
    printf("test_edge_cases: Testing multiple init\n");
    fflush(stdout);
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    if (context) {
        printf("test_edge_cases: First init\n");
        fflush(stdout);
        uvhttp_context_init(context);
        printf("test_edge_cases: Second init\n");
        fflush(stdout);
        uvhttp_context_init(context);  /* 应该幂等 */
        printf("test_edge_cases: Destroying context\n");
        fflush(stdout);
        uvhttp_context_destroy(context);
        printf("test_edge_cases: Context destroyed\n");
        fflush(stdout);
    }

    printf("test_edge_cases: PASSED\n");
    fflush(stdout);
}

int main(void) {
    printf("=== uvhttp_context.c 完整覆盖率测试 ===\n\n");
    fflush(stdout);

    test_context_create();
    test_context_destroy();
    test_context_init();
    test_context_set_connection_provider();
    test_context_set_logger_provider();
    test_context_set_config_provider();
    test_context_set_network_interface();
    test_default_connection_provider_create();
    test_default_logger_provider_create();
    printf("About to call test_default_config_provider_create\n");
    fflush(stdout);
    test_default_config_provider_create();
    printf("After test_default_config_provider_create\n");
    fflush(stdout);
    test_test_connection_provider_create();
    printf("About to call test_test_logger_provider_create\n");
    fflush(stdout);
    test_test_logger_provider_create();
    printf("After test_test_logger_provider_create\n");
    fflush(stdout);
    test_test_context_init();
    test_test_context_reset_stats();
    test_multiple_create_destroy();
    test_edge_cases();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}
