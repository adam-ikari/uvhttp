/* UVHTTP Feature Configuration Definitions */

#ifndef UVHTTP_FEATURES_H
#define UVHTTP_FEATURES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Core HTTP functionality - Always enabled */
#define UVHTTP_FEATURE_HTTP 1

/* Middleware support - Compile-time configuration */
#ifndef UVHTTP_FEATURE_MIDDLEWARE
#    define UVHTTP_FEATURE_MIDDLEWARE 1 /* MiddlewareSupport - Enable */
#endif

/* Optional feature modules */
#ifndef UVHTTP_FEATURE_WEBSOCKET
#    define UVHTTP_FEATURE_WEBSOCKET 1 /* WebSocket functionblock */
#endif

#ifndef UVHTTP_FEATURE_STATIC_FILES
#    define UVHTTP_FEATURE_STATIC_FILES 1 /* Static filefunctionblock */
#endif

#ifndef UVHTTP_FEATURE_LOGGING
#    define UVHTTP_FEATURE_LOGGING 0 /* logMiddleware - Disableraise */
#endif

#ifndef UVHTTP_FEATURE_TLS
#    define UVHTTP_FEATURE_TLS 1 /* TLS/SSL Support */
#endif

#ifndef UVHTTP_FEATURE_ROUTER_CACHE
#    define UVHTTP_FEATURE_ROUTER_CACHE 0 /* RouterCacheSupport */
#endif

#ifndef UVHTTP_FEATURE_LRU_CACHE
#    define UVHTTP_FEATURE_LRU_CACHE 1 /* LRUCacheSupport */
#endif

#ifndef UVHTTP_FEATURE_CORS
#    define UVHTTP_FEATURE_CORS 0 /* CORS Support */
#endif

#ifndef UVHTTP_FEATURE_RATE_LIMIT
#    define UVHTTP_FEATURE_RATE_LIMIT          \
        1 /* Rate limitSupport - DefaultEnable \
           */
#endif

#ifndef UVHTTP_FEATURE_ALLOCATOR
#    define UVHTTP_FEATURE_ALLOCATOR 0 /* customSupport */
#endif

/* ============ Conditional Compilation Macros ============ */

/* Basic feature macros */
#if UVHTTP_FEATURE_WEBSOCKET
#    define UVHTTP_WEBSOCKET_ENABLED
#endif

#if UVHTTP_FEATURE_LOGGING
#    define UVHTTP_LOGGING_ENABLED
#endif

#if UVHTTP_FEATURE_TLS
#    define UVHTTP_TLS_ENABLED
#endif

#if UVHTTP_FEATURE_MIDDLEWARE
#    define UVHTTP_MIDDLEWARE_ENABLED
#endif

#if UVHTTP_FEATURE_ROUTER_CACHE
#    define UVHTTP_ROUTER_CACHE_ENABLED
#endif

#if UVHTTP_FEATURE_CORS
#    define UVHTTP_CORS_ENABLED
#endif

#if UVHTTP_FEATURE_RATE_LIMIT
#    define UVHTTP_RATE_LIMIT_ENABLED
#endif

#if UVHTTP_FEATURE_STATIC_FILES
#    define UVHTTP_STATIC_FILES_ENABLED
#endif

#if UVHTTP_FEATURE_LRU_CACHE
#    define UVHTTP_LRU_CACHE_ENABLED
#endif

#if UVHTTP_FEATURE_ALLOCATOR
#    define UVHTTP_ALLOCATOR_ENABLED
#endif

/* ============ Compile-time Assertion Macros ============ */
/* Note: UVHTTP_STATIC_ASSERT is defined in uvhttp_common.h */

/* ============ Attribute Macros ============ */
/* Function attributes */
#define UVHTTP_INLINE __attribute__((always_inline)) static inline
#define UVHTTP_NOINLINE __attribute__((noinline))
#define UVHTTP_UNUSED __attribute__((unused))
#define UVHTTP_DEPRECATED __attribute__((deprecated))

/* Memory alignment */
#define UVHTTP_ALIGNED(n) __attribute__((aligned(n)))

/* Branch prediction optimization */
#define UVHTTP_LIKELY(x) __builtin_expect(!!(x), 1)
#define UVHTTP_UNLIKELY(x) __builtin_expect(!!(x), 0)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_FEATURES_H */