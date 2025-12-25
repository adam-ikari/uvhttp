/* UVHTTP 依赖注入和上下文管理实现 */

#include "uvhttp_context.h"
#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

/* 全局上下文实例（仅在测试模式下使用） */
#ifdef UVHTTP_TEST_MODE
uvhttp_context_t* g_uvhttp_context = NULL;
#endif

/* ============ 默认连接提供者实现 ============ */

typedef struct {
    uvhttp_connection_provider_t base;
    /* 这里可以包含连接池相关的状态 */
    size_t pool_size;
    size_t max_pool_size;
} uvhttp_default_connection_provider_t;

static struct uvhttp_connection* default_acquire_connection(uvhttp_connection_provider_t* provider) {
    (void)provider;
    /* 简化实现：直接创建新连接 */
    /* 在实际实现中，这里会从连接池获取连接 */
    return NULL;
}

static void default_release_connection(uvhttp_connection_provider_t* provider, 
                                      struct uvhttp_connection* conn) {
    (void)provider;
    if (conn) {
        /* 简化实现：直接释放连接 */
        /* 在实际实现中，这里会将连接放回连接池 */
        uvhttp_connection_free(conn);
    }
}

static size_t default_get_pool_size(uvhttp_connection_provider_t* provider) {
    uvhttp_default_connection_provider_t* impl = (uvhttp_default_connection_provider_t*)provider;
    return impl ? impl->pool_size : 0;
}

static void default_cleanup_expired(uvhttp_connection_provider_t* provider) {
    (void)provider;
    /* 清理过期连接 */
}

uvhttp_connection_provider_t* uvhttp_default_connection_provider_create(void) {
    uvhttp_default_connection_provider_t* provider = 
        (uvhttp_default_connection_provider_t*)malloc(sizeof(uvhttp_default_connection_provider_t));
    if (!provider) {
        return NULL;
    }
    
    memset(provider, 0, sizeof(uvhttp_default_connection_provider_t));
    
    provider->base.acquire_connection = default_acquire_connection;
    provider->base.release_connection = default_release_connection;
    provider->base.get_pool_size = default_get_pool_size;
    provider->base.cleanup_expired = default_cleanup_expired;
    
    provider->pool_size = 0;
    provider->max_pool_size = 100; /* 默认最大连接池大小 */
    
    return &provider->base;
}

/* ============ 测试连接提供者实现 ============ */

typedef struct {
    uvhttp_connection_provider_t base;
    struct uvhttp_connection* test_connection;
    int simulate_connection_failure;
} uvhttp_test_connection_provider_t;

static struct uvhttp_connection* test_acquire_connection(uvhttp_connection_provider_t* provider) {
    uvhttp_test_connection_provider_t* impl = (uvhttp_test_connection_provider_t*)provider;
    
    if (impl->simulate_connection_failure) {
        return NULL;
    }
    
    return impl->test_connection;
}

static void test_release_connection(uvhttp_connection_provider_t* provider, 
                                   struct uvhttp_connection* conn) {
    (void)provider;
    (void)conn;
    /* 测试环境不实际释放连接 */
}

static size_t test_get_pool_size(uvhttp_connection_provider_t* provider) {
    (void)provider;
    return 1; /* 测试环境总是返回1 */
}

static void test_cleanup_expired(uvhttp_connection_provider_t* provider) {
    (void)provider;
    /* 测试环境无需清理 */
}

uvhttp_connection_provider_t* uvhttp_test_connection_provider_create(void) {
    uvhttp_test_connection_provider_t* provider = 
        (uvhttp_test_connection_provider_t*)malloc(sizeof(uvhttp_test_connection_provider_t));
    if (!provider) {
        return NULL;
    }
    
    memset(provider, 0, sizeof(uvhttp_test_connection_provider_t));
    
    provider->base.acquire_connection = test_acquire_connection;
    provider->base.release_connection = test_release_connection;
    provider->base.get_pool_size = test_get_pool_size;
    provider->base.cleanup_expired = test_cleanup_expired;
    
    provider->test_connection = NULL;
    provider->simulate_connection_failure = 0;
    
    return &provider->base;
}

/* ============ 内存分配器说明 ============ */
/*
 * UVHTTP 内存分配器采用编译时宏设计，零开销抽象
 * 
 * 不实现运行时分配器提供者，原因：
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
 */

/* ============ 测试模式内存分配器说明 ============ */
/*
 * 测试模式下，内存分配器通过编译宏实现跟踪：
 * 
 * #ifdef UVHTTP_TEST_MODE
 *     #define UVHTTP_MALLOC(size) uvhttp_test_malloc(size, __FILE__, __LINE__)
 *     #define UVHTTP_FREE(ptr) uvhttp_test_free(ptr, __FILE__, __LINE__)
 * #endif
 * 
 * 这样既保持了性能（编译时宏展开），又提供了测试时的内存跟踪能力。
 * 详见 uvhttp_features.h 和 test/uvhttp_test_helpers.c 中的实现。
 */

/* ============ 默认日志提供者实现 ============ */

typedef struct {
    uvhttp_logger_provider_t base;
    uvhttp_log_level_t level;
    FILE* output;
} uvhttp_default_logger_provider_t;

static void default_log(uvhttp_logger_provider_t* provider,
                       uvhttp_log_level_t level,
                       const char* file,
                       int line,
                       const char* func,
                       const char* message) {
    uvhttp_default_logger_provider_t* impl = (uvhttp_default_logger_provider_t*)provider;
    
    if (level < impl->level) {
        return;
    }
    
    const char* level_str = "UNKNOWN";
    switch (level) {
        case UVHTTP_LOG_DEBUG: level_str = "DEBUG"; break;
        case UVHTTP_LOG_INFO:  level_str = "INFO"; break;
        case UVHTTP_LOG_WARN:  level_str = "WARN"; break;
        case UVHTTP_LOG_ERROR: level_str = "ERROR"; break;
        case UVHTTP_LOG_FATAL: level_str = "FATAL"; break;
    }
    
    fprintf(impl->output, "[%s] %s:%d %s(): %s\n", level_str, file, line, func, message);
    fflush(impl->output);
}

static void default_set_level(uvhttp_logger_provider_t* provider, uvhttp_log_level_t level) {
    uvhttp_default_logger_provider_t* impl = (uvhttp_default_logger_provider_t*)provider;
    impl->level = level;
}

uvhttp_logger_provider_t* uvhttp_default_logger_provider_create(uvhttp_log_level_t level) {
    uvhttp_default_logger_provider_t* provider = 
        (uvhttp_default_logger_provider_t*)malloc(sizeof(uvhttp_default_logger_provider_t));
    if (!provider) {
        return NULL;
    }
    
    memset(provider, 0, sizeof(uvhttp_default_logger_provider_t));
    
    provider->base.log = default_log;
    provider->base.set_level = default_set_level;
    
    provider->level = level;
    provider->output = stderr;
    
    return &provider->base;
}

/* ============ 测试日志提供者实现 ============ */

typedef struct {
    uvhttp_logger_provider_t base;
    uvhttp_log_level_t level;
    char* cached_logs;
    size_t cached_size;
    int silent;
} uvhttp_test_logger_provider_t;

static void test_log(uvhttp_logger_provider_t* provider,
                     uvhttp_log_level_t level,
                     const char* file,
                     int line,
                     const char* func,
                     const char* message) {
    uvhttp_test_logger_provider_t* impl = (uvhttp_test_logger_provider_t*)provider;
    
    if (impl->silent) {
        return;
    }
    
    if (level < impl->level) {
        return;
    }
    
    /* 缓存日志用于测试验证 */
    const char* level_str = "UNKNOWN";
    switch (level) {
        case UVHTTP_LOG_DEBUG: level_str = "DEBUG"; break;
        case UVHTTP_LOG_INFO:  level_str = "INFO"; break;
        case UVHTTP_LOG_WARN:  level_str = "WARN"; break;
        case UVHTTP_LOG_ERROR: level_str = "ERROR"; break;
        case UVHTTP_LOG_FATAL: level_str = "FATAL"; break;
    }
    
    char log_entry[512];
    snprintf(log_entry, sizeof(log_entry), "[%s] %s:%d %s(): %s\n", 
             level_str, file, line, func, message);
    
    /* 追加到缓存 */
    size_t entry_len = strlen(log_entry);
    char* new_logs = (char*)realloc(impl->cached_logs, impl->cached_size + entry_len + 1);
    if (new_logs) {
        strcpy(new_logs + impl->cached_size, log_entry);
        impl->cached_logs = new_logs;
        impl->cached_size += entry_len;
    }
}

static void test_set_level(uvhttp_logger_provider_t* provider, uvhttp_log_level_t level) {
    uvhttp_test_logger_provider_t* impl = (uvhttp_test_logger_provider_t*)provider;
    impl->level = level;
}

uvhttp_logger_provider_t* uvhttp_test_logger_provider_create(void) {
    uvhttp_test_logger_provider_t* provider = 
        (uvhttp_test_logger_provider_t*)malloc(sizeof(uvhttp_test_logger_provider_t));
    if (!provider) {
        return NULL;
    }
    
    memset(provider, 0, sizeof(uvhttp_test_logger_provider_t));
    
    provider->base.log = test_log;
    provider->base.set_level = test_set_level;
    
    provider->level = UVHTTP_LOG_DEBUG;
    provider->cached_logs = NULL;
    provider->cached_size = 0;
    provider->silent = 0;
    
    return &provider->base;
}

/* ============ 默认配置提供者实现 ============ */

typedef struct {
    uvhttp_config_provider_t base;
    char config_data[1024]; /* 简化的配置存储 */
} uvhttp_default_config_provider_t;

static const char* default_get_string(uvhttp_config_provider_t* provider, 
                                      const char* key, 
                                      const char* default_value) {
    (void)provider;
    (void)key;
    return default_value;
}

static int default_get_int(uvhttp_config_provider_t* provider, 
                           const char* key, 
                           int default_value) {
    (void)provider;
    (void)key;
    return default_value;
}

static int default_set_string(uvhttp_config_provider_t* provider, 
                              const char* key, 
                              const char* value) {
    (void)provider;
    (void)key;
    (void)value;
    return 0;
}

static int default_set_int(uvhttp_config_provider_t* provider, 
                           const char* key, 
                           int value) {
    (void)provider;
    (void)key;
    (void)value;
    return 0;
}

uvhttp_config_provider_t* uvhttp_default_config_provider_create(void) {
    uvhttp_default_config_provider_t* provider = 
        (uvhttp_default_config_provider_t*)malloc(sizeof(uvhttp_default_config_provider_t));
    if (!provider) {
        return NULL;
    }
    
    memset(provider, 0, sizeof(uvhttp_default_config_provider_t));
    
    provider->base.get_string = default_get_string;
    provider->base.get_int = default_get_int;
    provider->base.set_string = default_set_string;
    provider->base.set_int = default_set_int;
    
    return &provider->base;
}

/* ============ 上下文管理实现 ============ */

uvhttp_context_t* uvhttp_context_create(uv_loop_t* loop) {
    uvhttp_context_t* context = (uvhttp_context_t*)malloc(sizeof(uvhttp_context_t));
    if (!context) {
        return NULL;
    }
    
    memset(context, 0, sizeof(uvhttp_context_t));
    
    context->loop = loop;
    context->created_at = time(NULL);
    
    return context;
}

void uvhttp_context_destroy(uvhttp_context_t* context) {
    if (!context) {
        return;
    }
    
    /* 销毁各种提供者 */
    if (context->connection_provider) {
        free(context->connection_provider);
    }
    
    /* 注意：内存分配器使用编译时宏，无需运行时清理 */
    
    if (context->logger_provider) {
        uvhttp_test_logger_provider_t* logger = (uvhttp_test_logger_provider_t*)context->logger_provider;
        if (logger->cached_logs) {
            free(logger->cached_logs);
        }
        free(context->logger_provider);
    }
    
    if (context->config_provider) {
        free(context->config_provider);
    }
    
    if (context->network_interface) {
        uvhttp_network_interface_destroy(context->network_interface);
    }
    
    free(context);
}

int uvhttp_context_init(uvhttp_context_t* context) {
    if (!context) {
        return -1;
    }
    
    /* 创建默认提供者 */
    if (!context->connection_provider) {
        context->connection_provider = uvhttp_default_connection_provider_create();
        if (!context->connection_provider) {
            return -1;
        }
    }
    
    /* 注意：内存分配器使用编译时宏，无需运行时设置
     * 分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择
     */
    
    if (!context->logger_provider) {
        context->logger_provider = uvhttp_default_logger_provider_create(UVHTTP_LOG_INFO);
        if (!context->logger_provider) {
            return -1;
        }
    }
    
    if (!context->config_provider) {
        context->config_provider = uvhttp_default_config_provider_create();
        if (!context->config_provider) {
            return -1;
        }
    }
    
    if (!context->network_interface) {
        context->network_interface = uvhttp_network_interface_create(
            UVHTTP_NETWORK_LIBUV, context->loop);
        if (!context->network_interface) {
            return -1;
        }
    }
    
    context->initialized = 1;
    
    return 0;
}

int uvhttp_context_set_connection_provider(uvhttp_context_t* context, 
                                           uvhttp_connection_provider_t* provider) {
    if (!context) {
        return -1;
    }
    
    if (context->connection_provider) {
        free(context->connection_provider);
    }
    
    context->connection_provider = provider;
    return 0;
}

/* 注意：内存分配器使用编译时宏，无需运行时设置
 * 分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择：
 * 
 * 编译命令示例：
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=0  # 系统默认
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=1  # mimalloc
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=2  # 自定义
 */

int uvhttp_context_set_logger_provider(uvhttp_context_t* context, 
                                       uvhttp_logger_provider_t* provider) {
    if (!context) {
        return -1;
    }
    
    if (context->logger_provider) {
        uvhttp_test_logger_provider_t* logger = (uvhttp_test_logger_provider_t*)context->logger_provider;
        if (logger->cached_logs) {
            free(logger->cached_logs);
        }
        free(context->logger_provider);
    }
    
    context->logger_provider = provider;
    return 0;
}

int uvhttp_context_set_config_provider(uvhttp_context_t* context, 
                                       uvhttp_config_provider_t* provider) {
    if (!context) {
        return -1;
    }
    
    if (context->config_provider) {
        free(context->config_provider);
    }
    
    context->config_provider = provider;
    return 0;
}

int uvhttp_context_set_network_interface(uvhttp_context_t* context, 
                                         uvhttp_network_interface_t* interface) {
    if (!context) {
        return -1;
    }
    
    if (context->network_interface) {
        uvhttp_network_interface_destroy(context->network_interface);
    }
    
    context->network_interface = interface;
    return 0;
}

/* ============ 测试辅助函数 ============ */

#ifdef UVHTTP_TEST_MODE

int uvhttp_test_context_init(uv_loop_t* loop) {
    if (g_uvhttp_context) {
        uvhttp_context_destroy(g_uvhttp_context);
    }
    
    g_uvhttp_context = uvhttp_context_create(loop);
    if (!g_uvhttp_context) {
        return -1;
    }
    
    /* 设置测试专用提供者（不包含分配器） */
    g_uvhttp_context->connection_provider = uvhttp_test_connection_provider_create();
    g_uvhttp_context->logger_provider = uvhttp_test_logger_provider_create();
    g_uvhttp_context->config_provider = uvhttp_default_config_provider_create();
    g_uvhttp_context->network_interface = uvhttp_network_interface_create(
        UVHTTP_NETWORK_MOCK, loop);
    
    if (!g_uvhttp_context->connection_provider || 
        !g_uvhttp_context->logger_provider ||
        !g_uvhttp_context->config_provider ||
        !g_uvhttp_context->network_interface) {
        uvhttp_test_context_cleanup();
        return -1;
    }
    
    /* 注意：测试模式下内存分配器通过编译宏实现跟踪
     * 使用 UVHTTP_FEATURE_MEMORY_TRACKING 和相关测试宏
     */
    
    g_uvhttp_context->initialized = 1;
    
    return 0;
}

void uvhttp_test_context_cleanup(void) {
    if (g_uvhttp_context) {
        uvhttp_context_destroy(g_uvhttp_context);
        g_uvhttp_context = NULL;
    }
}

uvhttp_context_t* uvhttp_test_get_context(void) {
    return g_uvhttp_context;
}

void uvhttp_test_context_reset_stats(void) {
    if (!g_uvhttp_context) {
        return;
    }
    
    g_uvhttp_context->total_requests = 0;
    g_uvhttp_context->total_connections = 0;
    g_uvhttp_context->active_connections = 0;
    
    if (g_uvhttp_context->network_interface) {
        g_uvhttp_context->network_interface->reset_stats(g_uvhttp_context->network_interface);
    }
}

#endif /* UVHTTP_TEST_MODE */