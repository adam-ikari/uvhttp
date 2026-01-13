#include <stdio.h>
#include <assert.h>
#include <stddef.h>
#include <uv.h>
#include "uvhttp_context.h"

void test_create_destroy(void) {
    printf("test_create_destroy: START\n");
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    assert(context != NULL);
    assert(context->loop == loop);
    assert(context->initialized == 0);
    
    uvhttp_context_destroy(context);
    printf("test_create_destroy: PASSED\n");
}

void test_logger_provider(void) {
    printf("test_logger_provider: START\n");
    uvhttp_logger_provider_t* provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    assert(provider != NULL);
    assert(provider->log != NULL);
    assert(provider->set_level != NULL);
    
    printf("test_logger_provider: Calling log...\n");
    provider->log(provider, UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__, "Test message");
    printf("test_logger_provider: Called log successfully\n");
    
    printf("test_logger_provider: Calling set_level...\n");
    provider->set_level(provider, UVHTTP_LOG_LEVEL_ERROR);
    printf("test_logger_provider: Called set_level successfully\n");
    
    /* 注意：这里不手动释放，让 context_destroy 来处理 */
    /* uvhttp_default_logger_provider_t* impl = 
        (uvhttp_default_logger_provider_t*)((char*)provider - offsetof(uvhttp_default_logger_provider_t, base));
    free(impl); */
    
    printf("test_logger_provider: PASSED\n");
}

void test_config_provider(void) {
    printf("test_config_provider: START\n");
    uvhttp_config_provider_t* provider = uvhttp_default_config_provider_create();
    assert(provider != NULL);
    assert(provider->get_string != NULL);
    assert(provider->get_int != NULL);
    assert(provider->set_string != NULL);
    assert(provider->set_int != NULL);
    
    printf("test_config_provider: Calling get_string...\n");
    const char* value = provider->get_string(provider, "test_key", "default");
    assert(value != NULL);
    printf("test_config_provider: get_string returned: %s\n", value);
    
    printf("test_config_provider: Calling set_string...\n");
    int result = provider->set_string(provider, "test_key", "test_value");
    assert(result == 0);
    printf("test_config_provider: set_string returned: %d\n", result);
    
    printf("test_config_provider: Calling get_int...\n");
    int int_value = provider->get_int(provider, "test_int", 42);
    assert(int_value == 42);
    printf("test_config_provider: get_int returned: %d\n", int_value);
    
    printf("test_config_provider: Calling set_int...\n");
    result = provider->set_int(provider, "test_int", 100);
    assert(result == 0);
    printf("test_config_provider: set_int returned: %d\n", result);
    
    /* 注意：这里不手动释放，让 context_destroy 来处理 */
    /* uvhttp_default_config_provider_t* impl = 
        (uvhttp_default_config_provider_t*)((char*)provider - offsetof(uvhttp_default_config_provider_t, base));
    free(impl); */
    
    printf("test_config_provider: PASSED\n");
}

void test_context_with_providers(void) {
    printf("test_context_with_providers: START\n");
    uv_loop_t* loop = uv_default_loop();
    uvhttp_context_t* context = uvhttp_context_create(loop);
    assert(context != NULL);
    
    /* 创建提供者 */
    uvhttp_logger_provider_t* logger = uvhttp_default_logger_provider_create(UVHTTP_LOG_LEVEL_INFO);
    uvhttp_config_provider_t* config = uvhttp_default_config_provider_create();
    
    /* 设置提供者 */
    int result = uvhttp_context_set_logger_provider(context, logger);
    assert(result == 0);
    
    result = uvhttp_context_set_config_provider(context, config);
    assert(result == 0);
    
    /* 测试日志 */
    if (context->logger_provider) {
        context->logger_provider->log(context->logger_provider, UVHTTP_LOG_LEVEL_INFO, 
                                      __FILE__, __LINE__, __func__, "Test via context");
    }
    
    /* 销毁上下文（会正确释放提供者） */
    uvhttp_context_destroy(context);
    
    printf("test_context_with_providers: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_context 简化测试 ===\n\n");
    
    test_create_destroy();
    test_logger_provider();
    test_config_provider();
    test_context_with_providers();
    
    printf("\n=== 所有测试通过 ===\n");
    return 0;
}