/* UVHTTP - 简化的依赖注入支持 */

#ifndef UVHTTP_DEPS_H
#define UVHTTP_DEPS_H

#include "uvhttp_error.h"
#include <uv.h>
#include <stddef.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 前向声明 ========== */
typedef struct uvhttp_deps_t uvhttp_deps_t;

/* ========== Provider结构定义 ========== */
typedef struct uvhttp_loop_provider_t {
    uv_loop_t* (*get_default_loop)(void* user_data);
    uv_loop_t* (*create_loop)(void* user_data);
    int (*run_loop)(uv_loop_t* loop, uv_run_mode mode, void* user_data);
    void (*close_loop)(uv_loop_t* loop, void* user_data);
    void* user_data;
} uvhttp_loop_provider_t;

typedef struct uvhttp_memory_provider_t {
    void* (*malloc)(size_t size, void* user_data);
    void* (*calloc)(size_t nmemb, size_t size, void* user_data);
    void (*free)(void* ptr, void* user_data);
    void* (*realloc)(void* ptr, size_t size, void* user_data);
    size_t (*get_allocated_size)(void* ptr, void* user_data);
    void* user_data;
} uvhttp_memory_provider_t;

typedef struct uvhttp_network_provider_t {
    int (*create_socket)(int domain, int type, int protocol, void* user_data);
    int (*bind_socket)(int sockfd, const struct sockaddr* addr, socklen_t addrlen, void* user_data);
    int (*listen_socket)(int sockfd, int backlog, void* user_data);
    int (*accept_socket)(int sockfd, struct sockaddr* addr, socklen_t* addrlen, void* user_data);
    ssize_t (*send_data)(int sockfd, const void* buf, size_t len, int flags, void* user_data);
    ssize_t (*recv_data)(int sockfd, void* buf, size_t len, int flags, void* user_data);
    void (*close_socket)(int sockfd, void* user_data);
    void* user_data;
} uvhttp_network_provider_t;

typedef struct uvhttp_file_provider_t {
    FILE* (*fopen)(const char* filename, const char* mode, void* user_data);
    int (*fclose)(FILE* stream, void* user_data);
    size_t (*fread)(void* ptr, size_t size, size_t nmemb, FILE* stream, void* user_data);
    size_t (*fwrite)(const void* ptr, size_t size, size_t nmemb, FILE* stream, void* user_data);
    int (*fseek)(FILE* stream, long offset, int origin, void* user_data);
    long (*ftell)(FILE* stream, void* user_data);
    int (*access)(const char* pathname, int mode, void* user_data);
    void* user_data;
} uvhttp_file_provider_t;

/* ========== 简化的依赖结构 ========== */
struct uvhttp_deps_t {
    uv_loop_t* (*get_loop)(void);
    void* (*malloc)(size_t size);
    void (*free)(void* ptr);
    FILE* (*fopen)(const char* filename, const char* mode);
    int (*access)(const char* pathname, int mode);
    uvhttp_loop_provider_t* loop_provider;
    uvhttp_memory_provider_t* memory_provider;
    uvhttp_network_provider_t* network_provider;
    uvhttp_file_provider_t* file_provider;
    bool owns_providers;
    void (*cleanup)(uvhttp_deps_t* deps);
};

/* ========== 全局依赖实例 ========== */
extern uvhttp_deps_t* g_uvhttp_deps;

/* ========== 设置依赖 ========== */
void uvhttp_set_deps(uvhttp_deps_t* deps);

/* ========== 获取默认依赖 ========== */
uvhttp_deps_t* uvhttp_get_default_deps(void);

/* ========== 创建测试依赖 ========== */
uvhttp_deps_t* uvhttp_create_test_deps(void);
uvhttp_deps_t* uvhttp_deps_create_test(void);
uvhttp_deps_t* uvhttp_deps_create_default(void);

/* ========== 依赖管理 ========== */
uvhttp_deps_t* uvhttp_deps_new(void);
void uvhttp_deps_free(uvhttp_deps_t* deps);

/* ========== 释放依赖 ========== */
void uvhttp_free_deps(uvhttp_deps_t* deps);

/* ========== Provider Getter 函数 ========== */
uvhttp_loop_provider_t* uvhttp_deps_get_loop_provider(uvhttp_deps_t* deps);
uvhttp_memory_provider_t* uvhttp_deps_get_memory_provider(uvhttp_deps_t* deps);
uvhttp_network_provider_t* uvhttp_deps_get_network_provider(uvhttp_deps_t* deps);
uvhttp_file_provider_t* uvhttp_deps_get_file_provider(uvhttp_deps_t* deps);

/* ========== Provider Setter 函数 ========== */
uvhttp_error_t uvhttp_deps_set_loop_provider(uvhttp_deps_t* deps, uvhttp_loop_provider_t* provider);
uvhttp_error_t uvhttp_deps_set_memory_provider(uvhttp_deps_t* deps, uvhttp_memory_provider_t* provider);
uvhttp_error_t uvhttp_deps_set_network_provider(uvhttp_deps_t* deps, uvhttp_network_provider_t* provider);
uvhttp_error_t uvhttp_deps_set_file_provider(uvhttp_deps_t* deps, uvhttp_file_provider_t* provider);

/* ========== 便利宏 ========== */
#ifndef UVHTTP_DEPS_MACROS_DEFINED
#define UVHTTP_GET_LOOP() (g_uvhttp_deps ? g_uvhttp_deps->get_loop() : uv_default_loop())
#define UVHTTP_FOPEN(filename, mode) (g_uvhttp_deps ? g_uvhttp_deps->fopen(filename, mode) : fopen(filename, mode))
#define UVHTTP_ACCESS(pathname, mode) (g_uvhttp_deps ? g_uvhttp_deps->access(pathname, mode) : access(pathname, mode))
#define UVHTTP_DEPS_MACROS_DEFINED
#else
#define UVHTTP_GET_LOOP() (g_uvhttp_deps ? g_uvhttp_deps->get_loop() : uv_default_loop())
#define UVHTTP_FOPEN(filename, mode) (g_uvhttp_deps ? g_uvhttp_deps->fopen(filename, mode) : fopen(filename, mode))
#define UVHTTP_ACCESS(pathname, mode) (g_uvhttp_deps ? g_uvhttp_deps->access(pathname, mode) : access(pathname, mode))
#endif

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_DEPS_H */