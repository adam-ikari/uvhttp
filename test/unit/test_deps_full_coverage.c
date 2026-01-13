/* UVHTTP 依赖注入模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_deps.h"
#include "uvhttp_constants.h"

/* 测试依赖创建 - 空容器 */
void test_deps_new(void) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    
    if (deps != NULL) {
        /* uvhttp_deps_new 只创建空容器，所有字段都是 NULL */
        assert(deps->get_loop == NULL);
        assert(deps->malloc == NULL);
        assert(deps->free == NULL);
        assert(deps->fopen == NULL);
        assert(deps->access == NULL);
        assert(deps->loop_provider == NULL);
        assert(deps->memory_provider == NULL);
        assert(deps->network_provider == NULL);
        assert(deps->file_provider == NULL);
        assert(deps->owns_providers == 0);
        assert(deps->cleanup != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_new: PASSED\n");
}

/* 测试依赖创建 - 默认版本 */
void test_deps_create_default(void) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != NULL) {
        /* uvhttp_deps_create_default 只设置 Provider 字段 */
        assert(deps->loop_provider != NULL);
        assert(deps->memory_provider != NULL);
        assert(deps->network_provider != NULL);
        assert(deps->file_provider != NULL);
        assert(deps->owns_providers == 1);
        assert(deps->cleanup != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_create_default: PASSED\n");
}

/* 测试依赖创建 - 测试版本 */
void test_deps_create_test(void) {
    uvhttp_deps_t* deps = uvhttp_deps_create_test();
    
    if (deps != NULL) {
        /* uvhttp_deps_create_test 只设置 Provider 字段 */
        assert(deps->loop_provider != NULL);
        assert(deps->memory_provider != NULL);
        assert(deps->network_provider != NULL);
        assert(deps->file_provider != NULL);
        assert(deps->owns_providers == 1);
        assert(deps->cleanup != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_create_test: PASSED\n");
}

/* 测试依赖创建和销毁 */
void test_deps_create_destroy(void) {
    uvhttp_deps_t* deps;
    int i;
    
    for (i = 0; i < 10; i++) {
        deps = uvhttp_deps_new();
        if (deps != NULL) {
            uvhttp_deps_free(deps);
        }
    }
    
    printf("test_deps_create_destroy: PASSED\n");
}

/* 测试不同依赖创建函数 */
void test_deps_different_creates(void) {
    uvhttp_deps_t* deps1, * deps2, * deps3;
    
    deps1 = uvhttp_deps_new();
    deps2 = uvhttp_deps_create_default();
    deps3 = uvhttp_deps_create_test();
    
    if (deps1 != NULL) {
        uvhttp_deps_free(deps1);
    }
    if (deps2 != NULL) {
        uvhttp_deps_free(deps2);
    }
    if (deps3 != NULL) {
        uvhttp_deps_free(deps3);
    }
    
    printf("test_deps_different_creates: PASSED\n");
}

/* 测试依赖提供者一致性 */
void test_deps_provider_consistency(void) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != NULL) {
        assert(deps->loop_provider != NULL);
        assert(deps->memory_provider != NULL);
        assert(deps->network_provider != NULL);
        assert(deps->file_provider != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_provider_consistency: PASSED\n");
}

/* 测试依赖所有权 */
void test_deps_ownership(void) {
    uvhttp_deps_t* deps1, * deps2;
    
    deps1 = uvhttp_deps_new();
    if (deps1 != NULL) {
        assert(deps1->owns_providers == 0);
        uvhttp_deps_free(deps1);
    }
    
    deps2 = uvhttp_deps_create_default();
    if (deps2 != NULL) {
        assert(deps2->owns_providers == 1);
        uvhttp_deps_free(deps2);
    }
    
    printf("test_deps_ownership: PASSED\n");
}

/* 测试 NULL 参数处理 */
void test_deps_null_params(void) {
    /* 测试 NULL 参数处理 */
    uvhttp_deps_free(NULL);
    
    printf("test_deps_null_params: PASSED\n");
}

/* 测试 cleanup 函数 */
void test_deps_cleanup(void) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    
    if (deps != NULL) {
        assert(deps->cleanup != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_cleanup: PASSED\n");
}

/* 测试 Provider 字段 */
void test_deps_provider_fields(void) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != NULL) {
        assert(deps->loop_provider != NULL);
        assert(deps->memory_provider != NULL);
        assert(deps->network_provider != NULL);
        assert(deps->file_provider != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_provider_fields: PASSED\n");
}

/* 测试默认和测试依赖的区别 */
void test_deps_default_vs_test(void) {
    uvhttp_deps_t* default_deps = uvhttp_deps_create_default();
    uvhttp_deps_t* test_deps = uvhttp_deps_create_test();
    
    if (default_deps != NULL && test_deps != NULL) {
        /* 两者都应该有 Provider */
        assert(default_deps->loop_provider != NULL);
        assert(test_deps->loop_provider != NULL);
        assert(default_deps->memory_provider != NULL);
        assert(test_deps->memory_provider != NULL);
        assert(default_deps->network_provider != NULL);
        assert(test_deps->network_provider != NULL);
        assert(default_deps->file_provider != NULL);
        assert(test_deps->file_provider != NULL);
        
        /* 两者都应该有所有权 */
        assert(default_deps->owns_providers == 1);
        assert(test_deps->owns_providers == 1);
        
        uvhttp_deps_free(default_deps);
        uvhttp_deps_free(test_deps);
    }
    
    printf("test_deps_default_vs_test: PASSED\n");
}

/* 测试多次创建和销毁 */
void test_deps_multiple_cycles(void) {
    int i;
    
    for (i = 0; i < 5; i++) {
        uvhttp_deps_t* deps = uvhttp_deps_create_default();
        if (deps != NULL) {
            uvhttp_deps_free(deps);
        }
    }
    
    for (i = 0; i < 5; i++) {
        uvhttp_deps_t* deps = uvhttp_deps_create_test();
        if (deps != NULL) {
            uvhttp_deps_free(deps);
        }
    }
    
    printf("test_deps_multiple_cycles: PASSED\n");
}

/* 测试空容器的字段 */
void test_deps_empty_container(void) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    
    if (deps != NULL) {
        /* 空容器应该所有字段都是 NULL，除了 cleanup */
        assert(deps->get_loop == NULL);
        assert(deps->malloc == NULL);
        assert(deps->free == NULL);
        assert(deps->fopen == NULL);
        assert(deps->access == NULL);
        assert(deps->loop_provider == NULL);
        assert(deps->memory_provider == NULL);
        assert(deps->network_provider == NULL);
        assert(deps->file_provider == NULL);
        assert(deps->owns_providers == 0);
        assert(deps->cleanup != NULL);
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_empty_container: PASSED\n");
}

/* 测试默认容器的 Provider 字段 */
void test_deps_default_container(void) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != NULL) {
        /* 默认容器应该有 Provider */
        assert(deps->loop_provider != NULL);
        assert(deps->memory_provider != NULL);
        assert(deps->network_provider != NULL);
        assert(deps->file_provider != NULL);
        assert(deps->owns_providers == 1);
        
        /* 但函数指针字段仍然是 NULL */
        assert(deps->get_loop == NULL);
        assert(deps->malloc == NULL);
        assert(deps->free == NULL);
        assert(deps->fopen == NULL);
        assert(deps->access == NULL);
        
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_default_container: PASSED\n");
}

/* 测试测试容器的 Provider 字段 */
void test_deps_test_container(void) {
    uvhttp_deps_t* deps = uvhttp_deps_create_test();
    
    if (deps != NULL) {
        /* 测试容器应该有 Provider */
        assert(deps->loop_provider != NULL);
        assert(deps->memory_provider != NULL);
        assert(deps->network_provider != NULL);
        assert(deps->file_provider != NULL);
        assert(deps->owns_providers == 1);
        
        /* 但函数指针字段仍然是 NULL */
        assert(deps->get_loop == NULL);
        assert(deps->malloc == NULL);
        assert(deps->free == NULL);
        assert(deps->fopen == NULL);
        assert(deps->access == NULL);
        
        uvhttp_deps_free(deps);
    }
    
    printf("test_deps_test_container: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_deps.c 完整覆盖率测试 ===\n\n");

    test_deps_new();
    test_deps_create_default();
    test_deps_create_test();
    test_deps_create_destroy();
    test_deps_different_creates();
    test_deps_provider_consistency();
    test_deps_ownership();
    test_deps_null_params();
    test_deps_cleanup();
    test_deps_provider_fields();
    test_deps_default_vs_test();
    test_deps_multiple_cycles();
    test_deps_empty_container();
    test_deps_default_container();
    test_deps_test_container();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}