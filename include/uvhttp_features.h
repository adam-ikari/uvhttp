/* UVHTTP 特性配置定义 */

#ifndef UVHTTP_FEATURES_H
#define UVHTTP_FEATURES_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 核心 HTTP 功能 - 始终启用 */
#define UVHTTP_FEATURE_HTTP 1

/* ============ 测试模式配置 ============ */
/* 测试模式开关 - 用户可通过编译时宏定义启用 */
#ifndef UVHTTP_TEST_MODE
#define UVHTTP_TEST_MODE 0  /* 默认关闭测试模式 */
#endif

/* 测试模式下的功能开关 */
#ifdef UVHTTP_TEST_MODE
    /* 测试模式自动启用的功能 */
    #define UVHTTP_FEATURE_TESTING 1           /* 测试支持 */
    #define UVHTTP_FEATURE_MEMORY_TRACKING 1   /* 内存跟踪 */
    #define UVHTTP_FEATURE_NETWORK_MOCK 1      /* 网络模拟 */
    #define UVHTTP_FEATURE_CONTEXT_INJECTION 1 /* 依赖注入 */
    #define UVHTTP_FEATURE_STATISTICS 1        /* 统计信息 */
    
    /* 测试模式下的调试功能 */
    #ifndef UVHTTP_TEST_VERBOSE_LOGGING
    #define UVHTTP_TEST_VERBOSE_LOGGING 0      /* 详细日志 */
    #endif
    
    #ifndef UVHTTP_TEST_MEMORY_LEAK_DETECTION
    #define UVHTTP_TEST_MEMORY_LEAK_DETECTION 1 /* 内存泄漏检测 */
    #endif
    
    #ifndef UVHTTP_TEST_PERFORMANCE_PROFILING
    #define UVHTTP_TEST_PERFORMANCE_PROFILING 0 /* 性能分析 */
    #endif
    
#else
    /* 生产环境关闭测试功能 */
    #define UVHTTP_FEATURE_TESTING 0
    #define UVHTTP_FEATURE_MEMORY_TRACKING 0
    #define UVHTTP_FEATURE_NETWORK_MOCK 0
    #define UVHTTP_FEATURE_CONTEXT_INJECTION 0
    #define UVHTTP_FEATURE_STATISTICS 0
#endif

/* ============ 可选功能定义 - 用户可通过编译宏控制 ============ */
#ifndef UVHTTP_FEATURE_WEBSOCKET
#define UVHTTP_FEATURE_WEBSOCKET 1  /* WebSocket 支持 */
#endif

#ifndef UVHTTP_FEATURE_TLS
#define UVHTTP_FEATURE_TLS 1        /* TLS/SSL 支持 */
#endif

#ifndef UVHTTP_FEATURE_JSON
#define UVHTTP_FEATURE_JSON 0       /* JSON 解析支持 - 已禁用 */
#endif

#ifndef UVHTTP_FEATURE_MIDDLEWARE
#define UVHTTP_FEATURE_MIDDLEWARE 0 /* 中间件支持 */
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
#define UVHTTP_FEATURE_RATE_LIMIT 0 /* 限流支持 */
#endif

#ifndef UVHTTP_FEATURE_STATIC_FILES
#define UVHTTP_FEATURE_STATIC_FILES 1 /* 静态文件支持 */
#endif

#ifndef UVHTTP_FEATURE_COMPRESSION
#define UVHTTP_FEATURE_COMPRESSION 0 /* 压缩支持 */
#endif

#ifndef UVHTTP_FEATURE_METRICS
#define UVHTTP_FEATURE_METRICS 0    /* 指标监控支持 */
#endif

#ifndef UVHTTP_FEATURE_CONFIG_FILE
#define UVHTTP_FEATURE_CONFIG_FILE 0 /* 配置文件支持 */
#endif

#ifndef UVHTTP_FEATURE_ALLOCATOR
#define UVHTTP_FEATURE_ALLOCATOR 0  /* 自定义分配器支持 */
#endif

/* ============ 零开销抽象控制 ============ */
/* 生产环境优化宏 */
#ifndef UVHTTP_INLINE_OPTIMIZED
#ifdef UVHTTP_TEST_MODE
    #define UVHTTP_INLINE_OPTIMIZED 0  /* 测试模式不内联，便于调试 */
#else
    #define UVHTTP_INLINE_OPTIMIZED 1  /* 生产模式启用内联优化 */
#endif
#endif

/* 网络接口控制 */
#ifndef UVHTTP_USE_NETWORK_INTERFACE
#ifdef UVHTTP_TEST_MODE
    #define UVHTTP_USE_NETWORK_INTERFACE 1  /* 测试模式使用网络接口 */
#else
    #define UVHTTP_USE_NETWORK_INTERFACE 0  /* 生产模式直接调用libuv */
#endif
#endif

/* 上下文注入控制 */
#ifndef UVHTTP_USE_CONTEXT
#ifdef UVHTTP_TEST_MODE
    #define UVHTTP_USE_CONTEXT 1  /* 测试模式使用上下文 */
#else
    #define UVHTTP_USE_CONTEXT 0  /* 生产模式不使用上下文 */
#endif
#endif

/* ============ 条件编译宏 ============ */

/* 基础功能宏 */
#if UVHTTP_FEATURE_WEBSOCKET
#define UVHTTP_WEBSOCKET_ENABLED
#endif

#if UVHTTP_FEATURE_TLS
#define UVHTTP_TLS_ENABLED
#endif

#if UVHTTP_FEATURE_JSON
#define UVHTTP_JSON_ENABLED
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

#if UVHTTP_FEATURE_COMPRESSION
#define UVHTTP_COMPRESSION_ENABLED
#endif

#if UVHTTP_FEATURE_METRICS
#define UVHTTP_METRICS_ENABLED
#endif

#if UVHTTP_FEATURE_CONFIG_FILE
#define UVHTTP_CONFIG_FILE_ENABLED
#endif

#if UVHTTP_FEATURE_ALLOCATOR
#define UVHTTP_ALLOCATOR_ENABLED
#endif

/* ============ 测试模式专用宏 ============ */
#if UVHTTP_FEATURE_TESTING
#define UVHTTP_TESTING_ENABLED
#endif

#if UVHTTP_FEATURE_MEMORY_TRACKING
#define UVHTTP_MEMORY_TRACKING_ENABLED
#endif

#if UVHTTP_FEATURE_NETWORK_MOCK
#define UVHTTP_NETWORK_MOCK_ENABLED
#endif

#if UVHTTP_FEATURE_CONTEXT_INJECTION
#define UVHTTP_CONTEXT_INJECTION_ENABLED
#endif

#if UVHTTP_FEATURE_STATISTICS
#define UVHTTP_STATISTICS_ENABLED
#endif

/* ============ 测试模式便利宏 ============ */
#ifdef UVHTTP_TEST_MODE

/* 内存跟踪宏 */
#ifdef UVHTTP_MEMORY_TRACKING_ENABLED
    #ifndef UVHTTP_MALLOC_DEFINED
        #define UVHTTP_MALLOC(size) uvhttp_test_malloc(size, __FILE__, __LINE__)
        #define UVHTTP_FREE(ptr) uvhttp_test_free(ptr, __FILE__, __LINE__)
        #define UVHTTP_REALLOC(ptr, size) uvhttp_test_realloc(ptr, size, __FILE__, __LINE__)
        #define UVHTTP_MALLOC_DEFINED
    #endif
    
    /* 内存跟踪函数声明 */
    void* uvhttp_test_malloc(size_t size, const char* file, int line);
    void uvhttp_test_free(void* ptr, const char* file, int line);
    void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line);
    
    /* 内存泄漏检测 */
    #define UVHTTP_MEMORY_CHECK_START() uvhttp_test_memory_check_start()
    #define UVHTTP_MEMORY_CHECK_END() uvhttp_test_memory_check_end()
    #define UVHTTP_MEMORY_LEAKS() uvhttp_test_memory_get_leak_count()
    
    void uvhttp_test_memory_check_start(void);
    void uvhttp_test_memory_check_end(void);
    int uvhttp_test_memory_get_leak_count(void);
    
#else
    /* 不启用内存跟踪时使用标准分配 */
    #ifndef UVHTTP_MALLOC_DEFINED
        #define UVHTTP_MALLOC(size) malloc(size)
        #define UVHTTP_FREE(ptr) free(ptr)
        #define UVHTTP_REALLOC(ptr, size) realloc(ptr, size)
        #define UVHTTP_MALLOC_DEFINED
    #endif
    
    #define UVHTTP_MEMORY_CHECK_START() do {} while(0)
    #define UVHTTP_MEMORY_CHECK_END() do {} while(0)
    #define UVHTTP_MEMORY_LEAKS() 0
#endif

/* 日志宏 */
#if UVHTTP_TEST_VERBOSE_LOGGING
    #define UVHTTP_TEST_LOG(fmt, ...) \
        fprintf(stderr, "[TEST] %s:%d: " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
    #define UVHTTP_TEST_LOG(fmt, ...) do {} while(0)
#endif

/* 断言宏 */
#define UVHTTP_TEST_ASSERT(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "[TEST ASSERTION FAILED] %s:%d: %s\n", \
                    __FILE__, __LINE__, #condition); \
            abort(); \
        } \
    } while(0)

/* 性能测试宏 */
#if UVHTTP_TEST_PERFORMANCE_PROFILING
    #define UVHTTP_PERF_START(name) \
        uint64_t perf_start_##name = uvhttp_test_get_timestamp()
    
    #define UVHTTP_PERF_END(name) \
        do { \
            uint64_t perf_end_##name = uvhttp_test_get_timestamp(); \
            fprintf(stderr, "[PERF] %s: %lu ms\n", #name, \
                    (unsigned long)(perf_end_##name - perf_start_##name)); \
        } while(0)
    
    uint64_t uvhttp_test_get_timestamp(void);
#else
    #define UVHTTP_PERF_START(name) do {} while(0)
    #define UVHTTP_PERF_END(name) do {} while(0)
#endif

#else
    /* ============ 生产环境宏 ============ */
    
    /* 生产环境使用优化后的内存分配 */
    #if UVHTTP_INLINE_OPTIMIZED
        /* 内联版本的内存分配，编译器优化 */
        static inline void* UVHTTP_MALLOC(size_t size) {
            return malloc(size);
        }
        static inline void UVHTTP_FREE(void* ptr) {
            free(ptr);
        }
        static inline void* UVHTTP_REALLOC(void* ptr, size_t size) {
            return realloc(ptr, size);
        }
    #else
        /* 函数调用版本 */
        #define UVHTTP_MALLOC(size) malloc(size)
        #define UVHTTP_FREE(ptr) free(ptr)
        #define UVHTTP_REALLOC(ptr, size) realloc(ptr, size)
    #endif
    
    /* 生产环境空宏 - 编译器会优化掉 */
    #define UVHTTP_MEMORY_CHECK_START() do {} while(0)
    #define UVHTTP_MEMORY_CHECK_END() do {} while(0)
    #define UVHTTP_MEMORY_LEAKS() 0
    #define UVHTTP_TEST_LOG(fmt, ...) do {} while(0)
    #define UVHTTP_TEST_ASSERT(condition) do {} while(0)
    #define UVHTTP_PERF_START(name) do {} while(0)
    #define UVHTTP_PERF_END(name) do {} while(0)
#endif

/* ============ 编译时断言宏 ============ */
#define UVHTTP_STATIC_ASSERT(condition, message) \
    _Static_assert(condition, message)

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

/* ============ 调试宏 ============ */
#ifndef NDEBUG
    #define UVHTTP_DEBUG_ONLY(code) code
#else
    #define UVHTTP_DEBUG_ONLY(code) do {} while(0)
#endif

/* ============ 错误处理宏 ============ */
#define UVHTTP_RETURN_IF_ERROR(expr) \
    do { \
        int _err = (expr); \
        if (UVHTTP_UNLIKELY(_err != 0)) { \
            return _err; \
        } \
    } while(0)

#define UVHTTP_GOTO_IF_ERROR(expr, label) \
    do { \
        int _err = (expr); \
        if (UVHTTP_UNLIKELY(_err != 0)) { \
            goto label; \
        } \
    } while(0)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_FEATURES_H */