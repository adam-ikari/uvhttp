/* UVHTTP 依赖注入和上下文管理 */

#ifndef UVHTTP_CONTEXT_H
#define UVHTTP_CONTEXT_H

#include "uvhttp_config.h"
#include "uvhttp_error_handler.h"
#if UVHTTP_FEATURE_LOGGING
#    include "uvhttp_logging.h"
#endif
#include <time.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct uvhttp_server;
struct uvhttp_router;

/* ============ 内存分配器说明 ============ */
/*
 * UVHTTP 内存分配器采用编译时宏设计，零开销抽象
 *
 * 不使用运行时分配器提供者接口，原因：
 * 1. 性能优先：避免函数指针调用开销
 * 2. 编译时优化：编译器可以内联和优化分配调用
 * 3. 简单直接：减少复杂性，提高可维护性
 *
 * 内存分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择：
 * - 0: 系统默认分配器 (malloc/free)
 * - 1: mimalloc 高性能分配器
 * - 2: 自定义分配器 (外部链接)
 *
 * 使用方式：
 *   #include "uvhttp_allocator.h"
 *   void* ptr = UVHTTP_MALLOC(size);
 *   UVHTTP_FREE(ptr);
 *
 * 详见 uvhttp_allocator.h 中的详细说明
 */

/* ============ 主上下文结构 ============ */

typedef struct uvhttp_context {
    /* 核心组件 */
    uv_loop_t* loop;
    struct uvhttp_server* server;
    struct uvhttp_router* router;

    /* 上下文状态 */
    int initialized;
    time_t created_at;

    /* 统计信息 */
    uint64_t total_requests;
    uint64_t total_connections;
    uint64_t active_connections;

    /* ===== 全局变量替代字段 ===== */

    /* TLS 模块状态 */
    int tls_initialized;
    void* tls_entropy; /* mbedtls_entropy_context* */
    void* tls_drbg;    /* mbedtls_ctr_drbg_context* */

    /* WebSocket 模块状态 */
    int ws_drbg_initialized;
    void* ws_entropy; /* mbedtls_entropy_context* */
    void* ws_drbg;    /* mbedtls_ctr_drbg_context* */

    /* 配置管理 */
    void* current_config;  /* uvhttp_config_t* */
    void* config_callback; /* uvhttp_config_change_callback_t */

    /* 用户数据（用于存储应用特定的上下文） */
    void* user_data;

} uvhttp_context_t;

/* ============ 上下文管理函数 ============ */

/* 创建新的上下文 */
uvhttp_error_t uvhttp_context_create(uv_loop_t* loop,
                                     uvhttp_context_t** context);

/* 销毁上下文 */
void uvhttp_context_destroy(uvhttp_context_t* context);

/* 初始化上下文（设置默认提供者） */
uvhttp_error_t uvhttp_context_init(uvhttp_context_t* context);

/* ===== 全局变量替代字段初始化函数 ===== */

/* 初始化 TLS 模块状态 */
uvhttp_error_t uvhttp_context_init_tls(uvhttp_context_t* context);

/* 清理 TLS 模块状态 */
void uvhttp_context_cleanup_tls(uvhttp_context_t* context);

/* 初始化 WebSocket 模块状态 */
uvhttp_error_t uvhttp_context_init_websocket(uvhttp_context_t* context);

/* 清理 WebSocket 模块状态 */
void uvhttp_context_cleanup_websocket(uvhttp_context_t* context);

/* 初始化配置管理 */
uvhttp_error_t uvhttp_context_init_config(uvhttp_context_t* context);

/* 清理配置管理 */
void uvhttp_context_cleanup_config(uvhttp_context_t* context);

/* ============ 默认提供者实现 ============ */
/* 注意：内存分配器使用编译时宏，无需创建提供者
 * 系统默认分配器：直接使用 malloc/free
 * mimalloc分配器：编译时链接 mimalloc 库
 * 自定义分配器：编译时链接用户实现
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONTEXT_H */