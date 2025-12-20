#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>

/* 自定义分配器实现 - 在所有情况下都定义 */
uvhttp_allocator_t* uvhttp_custom_allocator = NULL;

/* 编译时确定的分配器实现 */

#if UVHTTP_ALLOCATOR_TYPE == 1  /* UVHTTP_ALLOCATOR_MIMALLOC */

/* mimalloc实现 - 直接映射，零开销 */
#include "mimalloc.h"

/* 这些函数在编译时会被优化为直接调用mimalloc */
void* uvhttp_mimalloc_malloc(size_t size) {
    return mi_malloc(size);
}

void* uvhttp_mimalloc_realloc(void* ptr, size_t size) {
    return mi_realloc(ptr, size);
}

void uvhttp_mimalloc_free(void* ptr) {
    mi_free(ptr);
}

void* uvhttp_mimalloc_calloc(size_t nmemb, size_t size) {
    return mi_calloc(nmemb, size);
}

#elif UVHTTP_ALLOCATOR_TYPE == 2  /* UVHTTP_ALLOCATOR_CUSTOM */

void* uvhttp_custom_malloc(size_t size) {
    if (uvhttp_custom_allocator && uvhttp_custom_allocator->malloc) {
        return uvhttp_custom_allocator->malloc(size);
    }
    return malloc(size);
}

void* uvhttp_custom_realloc(void* ptr, size_t size) {
    if (uvhttp_custom_allocator && uvhttp_custom_allocator->realloc) {
        return uvhttp_custom_allocator->realloc(ptr, size);
    }
    return realloc(ptr, size);
}

void uvhttp_custom_free(void* ptr) {
    if (uvhttp_custom_allocator && uvhttp_custom_allocator->free) {
        uvhttp_custom_allocator->free(ptr);
    } else {
        free(ptr);
    }
}

void* uvhttp_custom_calloc(size_t nmemb, size_t size) {
    if (uvhttp_custom_allocator && uvhttp_custom_allocator->calloc) {
        return uvhttp_custom_allocator->calloc(nmemb, size);
    }
    return calloc(nmemb, size);
}

/* 运行时分配器管理 */
uvhttp_allocator_t* uvhttp_allocator_get(void) {
    static uvhttp_allocator_t default_allocator = {
        .malloc = malloc,
        .realloc = realloc,
        .free = free,
        .calloc = calloc,
        .data = NULL,
        .type = UVHTTP_ALLOCATOR_DEFAULT
    };
    
    if (uvhttp_custom_allocator) {
        return uvhttp_custom_allocator;
    }
    return &default_allocator;
}

void uvhttp_allocator_set(uvhttp_allocator_t* allocator) {
    uvhttp_custom_allocator = allocator;
}

#endif /* UVHTTP_ALLOCATOR_TYPE */

/* 获取当前分配器名称 */
const char* uvhttp_allocator_name(void) {
    return UVHTTP_ALLOCATOR_NAME;
}

/* 自定义分配器函数的通用定义（确保在所有分配器类型下都可用） */
#if UVHTTP_ALLOCATOR_TYPE != 2  /* UVHTTP_ALLOCATOR_CUSTOM */

void* uvhttp_custom_malloc(size_t size) {
    return malloc(size);
}

void* uvhttp_custom_realloc(void* ptr, size_t size) {
    return realloc(ptr, size);
}

void uvhttp_custom_free(void* ptr) {
    free(ptr);
}

void* uvhttp_custom_calloc(size_t nmemb, size_t size) {
    return calloc(nmemb, size);
}

#endif /* UVHTTP_ALLOCATOR_TYPE != 2 */