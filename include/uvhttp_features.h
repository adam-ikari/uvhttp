/* UVHTTP 特性配置定义 */

#ifndef UVHTTP_FEATURES_H
#define UVHTTP_FEATURES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 核心 HTTP 功能 - 始终启用 */
#define UVHTTP_FEATURE_HTTP 1

/* 中间件支持 - 编译期配置 */
#ifndef UVHTTP_FEATURE_MIDDLEWARE
#define UVHTTP_FEATURE_MIDDLEWARE 1  /* 中间件支持 - 已启用 */
#endif

/* 可选功能模块 */
#ifndef UVHTTP_FEATURE_WEBSOCKET
#define UVHTTP_FEATURE_WEBSOCKET 1  /* WebSocket 功能模块 */
#endif

#ifndef UVHTTP_FEATURE_STATIC_FILES
#define UVHTTP_FEATURE_STATIC_FILES 1 /* 静态文件功能模块 */
#endif

#ifndef UVHTTP_FEATURE_LOGGING
#define UVHTTP_FEATURE_LOGGING 0  /* 日志中间件 - 已禁用以提高性能 */
#endif

#ifndef UVHTTP_FEATURE_TLS
#define UVHTTP_FEATURE_TLS 1        /* TLS/SSL 支持 */
#endif

#ifndef UVHTTP_FEATURE_ROUTER_CACHE
#define UVHTTP_FEATURE_ROUTER_CACHE 0 /* 路由缓存支持 */
#endif

#ifndef UVHTTP_FEATURE_LRU_CACHE
#define UVHTTP_FEATURE_LRU_CACHE 1 /* LRU缓存支持 */
#endif

#ifndef UVHTTP_FEATURE_CORS
#define UVHTTP_FEATURE_CORS 0       /* CORS 支持 */
#endif

#ifndef UVHTTP_FEATURE_RATE_LIMIT
#define UVHTTP_FEATURE_RATE_LIMIT 1 /* 限流支持 - 默认启用 */
#endif

#ifndef UVHTTP_FEATURE_ALLOCATOR
#define UVHTTP_FEATURE_ALLOCATOR 0  /* 自定义分配器支持 */
#endif

/* ============ 条件编译宏 ============ */

/* 基础功能宏 */
#if UVHTTP_FEATURE_WEBSOCKET
#define UVHTTP_WEBSOCKET_ENABLED
#endif

#if UVHTTP_FEATURE_LOGGING
#define UVHTTP_LOGGING_ENABLED
#endif

#if UVHTTP_FEATURE_TLS
#define UVHTTP_TLS_ENABLED
#endif

#if UVHTTP_FEATURE_MIDDLEWARE
#define UVHTTP_MIDDLEWARE_ENABLED
#endif

#if UVHTTP_FEATURE_ROUTER_CACHE
#define UVHTTP_ROUTER_CACHE_ENABLED
#endif

#if UVHTTP_FEATURE_CORS
#define UVHTTP_CORS_ENABLED
#endif

#if UVHTTP_FEATURE_RATE_LIMIT
#define UVHTTP_RATE_LIMIT_ENABLED
#endif

#if UVHTTP_FEATURE_STATIC_FILES
#define UVHTTP_STATIC_FILES_ENABLED
#endif

#if UVHTTP_FEATURE_LRU_CACHE
#define UVHTTP_LRU_CACHE_ENABLED
#endif

#if UVHTTP_FEATURE_ALLOCATOR
#define UVHTTP_ALLOCATOR_ENABLED
#endif

/* ============ 编译时断言宏 ============ */
/* 注意：UVHTTP_STATIC_ASSERT 已在 uvhttp_common.h 中定义 */

/* ============ 属性宏 ============ */
/* 函数属性 */
#define UVHTTP_INLINE __attribute__((always_inline)) static inline
#define UVHTTP_NOINLINE __attribute__((noinline))
#define UVHTTP_UNUSED __attribute__((unused))
#define UVHTTP_DEPRECATED __attribute__((deprecated))

/* 内存对齐 */
#define UVHTTP_ALIGNED(n) __attribute__((aligned(n)))

/* 分支预测优化 */
#define UVHTTP_LIKELY(x) __builtin_expect(!!(x), 1)
#define UVHTTP_UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_FEATURES_H */