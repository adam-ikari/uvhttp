/* UVHTTP 依赖注入和上下文管理 */

#ifndef UVHTTP_CONTEXT_H
#define UVHTTP_CONTEXT_H

#include "uvhttp_network.h"
#include "uvhttp_connection.h"
#include "uvhttp_config.h"
#include "uvhttp_error_handler.h"
#include <uv.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 前向声明 */
struct uvhttp_server;
struct uvhttp_router;

/* ============ 连接提供者接口 ============ */

typedef struct uvhttp_connection_provider {
    /* 获取连接 */
    struct uvhttp_connection* (*acquire_connection)(struct uvhttp_connection_provider* provider);
    
    /* 释放连接 */
    void (*release_connection)(struct uvhttp_connection_provider* provider, 
                              struct uvhttp_connection* conn);
    
    /* 连接池统计 */
    size_t (*get_pool_size)(struct uvhttp_connection_provider* provider);
    void (*cleanup_expired)(struct uvhttp_connection_provider* provider);
    
    /* 私有数据 */
    void* context;
    
} uvhttp_connection_provider_t;

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

/* ============ 日志提供者接口 ============ */

typedef struct uvhttp_logger_provider {
    /* 日志输出函数 */
    void (*log)(struct uvhttp_logger_provider* provider,
                uvhttp_log_level_t level,
                const char* file,
                int line,
                const char* func,
                const char* message);
    
    /* 设置日志级别 */
    void (*set_level)(struct uvhttp_logger_provider* provider, uvhttp_log_level_t level);
    
    /* 私有数据 */
    void* context;
    
} uvhttp_logger_provider_t;

/* ============ 配置提供者接口 ============ */

typedef struct uvhttp_config_provider {
    /* 获取配置值 */
    const char* (*get_string)(struct uvhttp_config_provider* provider, 
                              const char* key, 
                              const char* default_value);
    
    int (*get_int)(struct uvhttp_config_provider* provider, 
                   const char* key, 
                   int default_value);
    
    /* 设置配置值 */
    int (*set_string)(struct uvhttp_config_provider* provider, 
                      const char* key, 
                      const char* value);
    
    int (*set_int)(struct uvhttp_config_provider* provider, 
                   const char* key, 
                   int value);
    
    /* 私有数据 */
    void* context;
    
} uvhttp_config_provider_t;

/* ============ 主上下文结构 ============ */

typedef struct uvhttp_context {
    /* 核心组件 */
    uv_loop_t* loop;
    struct uvhttp_server* server;
    struct uvhttp_router* router;
    
    /* 依赖注入的提供者（不包含分配器） */
    uvhttp_connection_provider_t* connection_provider;
    uvhttp_logger_provider_t* logger_provider;
    uvhttp_config_provider_t* config_provider;
    uvhttp_network_interface_t* network_interface;
    
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
    void* tls_entropy;          /* mbedtls_entropy_context* */
    void* tls_drbg;             /* mbedtls_ctr_drbg_context* */
    
    /* WebSocket 模块状态 */
    int ws_drbg_initialized;
    void* ws_entropy;           /* mbedtls_entropy_context* */
    void* ws_drbg;              /* mbedtls_ctr_drbg_context* */
    
    /* 错误统计 */
    void* error_stats;          /* uvhttp_error_stats_t* */
    
    /* 配置管理 */
    void* current_config;       /* uvhttp_config_t* */
    void* config_callback;      /* uvhttp_config_change_callback_t */
    
    /* 用户数据（用于存储应用特定的上下文） */
    void* user_data;
    
} uvhttp_context_t;

/* ============ 上下文管理函数 ============ */

/* 创建新的上下文 */
uvhttp_context_t* uvhttp_context_create(uv_loop_t* loop);

/* 销毁上下文 */
void uvhttp_context_destroy(uvhttp_context_t* context);

/* 初始化上下文（设置默认提供者） */
int uvhttp_context_init(uvhttp_context_t* context);

/* ===== 全局变量替代字段初始化函数 ===== */

/* 初始化 TLS 模块状态 */
int uvhttp_context_init_tls(uvhttp_context_t* context);

/* 清理 TLS 模块状态 */
void uvhttp_context_cleanup_tls(uvhttp_context_t* context);

/* 初始化 WebSocket 模块状态 */
int uvhttp_context_init_websocket(uvhttp_context_t* context);

/* 清理 WebSocket 模块状态 */
void uvhttp_context_cleanup_websocket(uvhttp_context_t* context);

/* 初始化错误统计 */
int uvhttp_context_init_error_stats(uvhttp_context_t* context);

/* 清理错误统计 */
void uvhttp_context_cleanup_error_stats(uvhttp_context_t* context);

/* 初始化配置管理 */
int uvhttp_context_init_config(uvhttp_context_t* context);

/* 清理配置管理 */
void uvhttp_context_cleanup_config(uvhttp_context_t* context);

/* 设置各种提供者 */
int uvhttp_context_set_connection_provider(uvhttp_context_t* context, 
                                           uvhttp_connection_provider_t* provider);

/* 注意：内存分配器使用编译时宏，无需运行时设置
 * 详见 uvhttp_allocator.h
 */

int uvhttp_context_set_logger_provider(uvhttp_context_t* context, 
                                       uvhttp_logger_provider_t* provider);

int uvhttp_context_set_config_provider(uvhttp_context_t* context, 
                                       uvhttp_config_provider_t* provider);

int uvhttp_context_set_network_interface(uvhttp_context_t* context, 
                                         uvhttp_network_interface_t* interface);

/* ============ 默认提供者实现 ============ */

/* 默认连接提供者（基于全局连接池） */
uvhttp_connection_provider_t* uvhttp_default_connection_provider_create(void);

/* 注意：内存分配器使用编译时宏，无需创建提供者
 * 系统默认分配器：直接使用 malloc/free
 * mimalloc分配器：编译时链接 mimalloc 库
 * 自定义分配器：编译时链接用户实现
 */

/* 默认日志提供者（基于fprintf） */
uvhttp_logger_provider_t* uvhttp_default_logger_provider_create(uvhttp_log_level_t level);

/* 默认配置提供者（基于内存配置） */
uvhttp_config_provider_t* uvhttp_default_config_provider_create(void);

/* 测试用连接提供者（无连接池） */
uvhttp_connection_provider_t* uvhttp_test_connection_provider_create(void);

/* 注意：测试模式下内存跟踪通过编译宏实现
 * #define UVHTTP_TEST_MODE 1
 * 详见 uvhttp_features.h 中的测试宏定义
 */

/* 测试用日志提供者（静默或缓存日志） */
uvhttp_logger_provider_t* uvhttp_test_logger_provider_create(void);

/* ============ 编译时宏支持 ============ */

#ifdef UVHTTP_TEST_MODE
    /* 测试模式：使用上下文 */
    #define UVHTTP_USE_CONTEXT 1
    extern uvhttp_context_t* g_uvhttp_context;
    
    /* 便捷访问宏 */
    #define uvhttp_get_loop() (g_uvhttp_context ? g_uvhttp_context->loop : NULL)
    #define uvhttp_get_connection_provider() \
        (g_uvhttp_context ? g_uvhttp_context->connection_provider : NULL)
    #define uvhttp_get_logger_provider() \
        (g_uvhttp_context ? g_uvhttp_context->logger_provider : NULL)
    #define uvhttp_get_config_provider() \
        (g_uvhttp_context ? g_uvhttp_context->config_provider : NULL)
    #define uvhttp_get_network_interface() \
        (g_uvhttp_context ? g_uvhttp_context->network_interface : NULL)
    
    /* 注意：内存分配器使用编译时宏，零开销抽象
     * 直接使用 UVHTTP_MALLOC/UVHTTP_FREE 等宏
     * 详见 uvhttp_allocator.h
     */
        
    #define uvhttp_context_log(level, message) \
        do { \
            if (g_uvhttp_context && g_uvhttp_context->logger_provider) { \
                g_uvhttp_context->logger_provider->log(g_uvhttp_context->logger_provider, \
                                                       level, __FILE__, __LINE__, __func__, message); \
            } \
        } while(0)
        
    #define uvhttp_context_config_get_string(key, default_value) \
        (g_uvhttp_context && g_uvhttp_context->config_provider ? \
         g_uvhttp_context->config_provider->get_string(g_uvhttp_context->config_provider, key, default_value) : \
         default_value)
         
#else
    /* 生产模式：直接调用，零开销 */
    #define UVHTTP_USE_CONTEXT 0
    
    /* 内联函数优化 */
    static inline uv_loop_t* uvhttp_get_loop(void) {
        return uv_default_loop();
    }
    
    /* 注意：内存分配器宏在 uvhttp_allocator.h 中定义
     * 生产环境直接调用对应分配器，零开销
     */
    
    static inline void uvhttp_context_log(uvhttp_log_level_t level, const char* message) {
        (void)level;
        (void)message;
        /* 生产环境可选择性实现日志 */
    }
    
    static inline const char* uvhttp_context_config_get_string(const char* key, const char* default_value) {
        (void)key;
        return default_value;
    }
#endif

/* ============ 测试辅助函数 ============ */

#ifdef UVHTTP_TEST_MODE

/* 初始化测试上下文 */
int uvhttp_test_context_init(uv_loop_t* loop);

/* 清理测试上下文 */
void uvhttp_test_context_cleanup(void);

/* 获取测试上下文 */
uvhttp_context_t* uvhttp_test_get_context(void);

/* 重置测试上下文统计 */
void uvhttp_test_context_reset_stats(void);

#endif /* UVHTTP_TEST_MODE */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONTEXT_H */