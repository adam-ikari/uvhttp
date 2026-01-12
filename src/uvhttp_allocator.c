/* UVHTTP 统一内存分配器实现 */

#include "uvhttp_allocator.h"

/* ========== 向后兼容的函数实现 ========== */

/* 提供小写函数接口，向后兼容 */
/* 注意：uvhttp_free、uvhttp_realloc、uvhttp_calloc 在 uvhttp_allocator.h 中定义为内联函数 */
/* 这里只实现 uvhttp_malloc，因为它直接使用 malloc */
void* uvhttp_malloc(size_t size) {
    return malloc(size);
}

/* ========== 测试模式内存跟踪函数 ========== */

/* 这些函数用于测试模式下的内存跟踪 */
/* 在生产模式下，UVHTTP_MALLOC 宏会直接映射到 malloc，不会调用这些函数 */

void* uvhttp_test_malloc(size_t size, const char* file, int line) {
    (void)file;  /* 避免未使用参数警告 */
    (void)line;
    return malloc(size);
}

void uvhttp_test_free(void* ptr, const char* file, int line) {
    (void)file;  /* 避免未使用参数警告 */
    (void)line;
    free(ptr);
}

void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line) {
    (void)file;  /* 避免未使用参数警告 */
    (void)line;
    return realloc(ptr, size);
}