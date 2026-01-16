/* UVHTTP 测试模式内存分配函数声明 */

/* 这些函数在 uvhttp_allocator.c 中实现 */
/* 这里只提供声明，避免重复定义 */

#include <stddef.h>

extern void* uvhttp_test_malloc(size_t size, const char* file, int line);
extern void uvhttp_test_free(void* ptr, const char* file, int line);
extern void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line);