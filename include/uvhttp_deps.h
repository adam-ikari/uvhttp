/* UVHTTP - 简化的依赖注入支持 */

#ifndef UVHTTP_DEPS_H
#define UVHTTP_DEPS_H

#include "uvhttp_error.h"
#include <uv.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 简化的依赖结构 ========== */
typedef struct {
    uv_loop_t* (*get_loop)(void);
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    FILE* (*fopen)(const char* filename, const char* mode);
    int (*access)(const char* pathname, int mode);
} uvhttp_deps_t;

/* ========== 全局依赖实例 ========== */
extern uvhttp_deps_t* g_uvhttp_deps;

/* ========== 设置依赖 ========== */
void uvhttp_set_deps(uvhttp_deps_t* deps);

/* ========== 获取默认依赖 ========== */
uvhttp_deps_t* uvhttp_get_default_deps(void);

/* ========== 创建测试依赖 ========== */
uvhttp_deps_t* uvhttp_create_test_deps(void);

/* ========== 释放依赖 ========== */
void uvhttp_free_deps(uvhttp_deps_t* deps);

/* ========== 便利宏 ========== */
#define UVHTTP_GET_LOOP() (g_uvhttp_deps ? g_uvhttp_deps->get_loop() : uv_default_loop())
#define UVHTTP_MALLOC(size) (g_uvhttp_deps ? g_uvhttp_deps->malloc(size) : malloc(size))
#define UVHTTP_FREE(ptr) do { if (g_uvhttp_deps) { g_uvhttp_deps->free(ptr); } else { free(ptr); } } while(0)
#define UVHTTP_FOPEN(filename, mode) (g_uvhttp_deps ? g_uvhttp_deps->fopen(filename, mode) : fopen(filename, mode))
#define UVHTTP_ACCESS(pathname, mode) (g_uvhttp_deps ? g_uvhttp_deps->access(pathname, mode) : access(pathname, mode))

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_DEPS_H */