/*
 * UVHTTP 日志中间件
 * 提供可插拔的日志功能，支持编译时关闭
 */

#if UVHTTP_FEATURE_LOGGING

#ifndef UVHTTP_LOG_MIDDLEWARE_H
#define UVHTTP_LOG_MIDDLEWARE_H

#include <stdarg.h>
#include "uvhttp_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别 */
typedef enum {
    UVHTTP_LOG_LEVEL_TRACE = 0,
    UVHTTP_LOG_LEVEL_DEBUG = 1,
    UVHTTP_LOG_LEVEL_INFO = 2,
    UVHTTP_LOG_LEVEL_WARN = 3,
    UVHTTP_LOG_LEVEL_ERROR = 4,
    UVHTTP_LOG_LEVEL_FATAL = 5
} uvhttp_log_level_t;

/* 日志格式 */
typedef enum {
    UVHTTP_LOG_FORMAT_TEXT = 0,    /* 文本格式 */
    UVHTTP_LOG_FORMAT_JSON = 1     /* JSON 格式 */
} uvhttp_log_format_t;

/* 日志输出目标 */
typedef enum {
    UVHTTP_LOG_OUTPUT_STDOUT = 0,  /* 标准输出 */
    UVHTTP_LOG_OUTPUT_STDERR = 1,  /* 标准错误 */
    UVHTTP_LOG_OUTPUT_FILE = 2,    /* 文件 */
    UVHTTP_LOG_OUTPUT_CALLBACK = 3 /* 自定义回调 */
} uvhttp_log_output_t;

/* 日志上下文 */
typedef struct {
    uvhttp_log_level_t level;      /* 日志级别 */
    uvhttp_log_format_t format;    /* 日志格式 */
    uvhttp_log_output_t output;    /* 输出目标 */
    char file_path[256];           /* 文件路径（如果输出到文件） */
    void (*callback)(const char* message, void* user_data);  /* 自定义回调 */
    void* user_data;               /* 回调用户数据 */
    int enable_colors;             /* 启用颜色 */
    int enable_timestamp;          /* 启用时间戳 */
    int enable_source_location;    /* 启用源代码位置 */
    int thread_safe;               /* 线程安全 */
} uvhttp_log_context_t;

/* 日志中间件结构 */
typedef struct uvhttp_log_middleware {
    uvhttp_log_context_t context;
    void* internal_data;
} uvhttp_log_middleware_t;

/* 创建日志中间件 */
uvhttp_log_middleware_t* uvhttp_log_middleware_create(const uvhttp_log_context_t* context);

/* 销毁日志中间件 */
void uvhttp_log_middleware_destroy(uvhttp_log_middleware_t* middleware);

/* 设置日志级别 */
void uvhttp_log_middleware_set_level(uvhttp_log_middleware_t* middleware, uvhttp_log_level_t level);

/* 获取日志级别 */
uvhttp_log_level_t uvhttp_log_middleware_get_level(uvhttp_log_middleware_t* middleware);

/* 记录日志 */
void uvhttp_log_middleware_log(uvhttp_log_middleware_t* middleware,
                                uvhttp_log_level_t level,
                                const char* file,
                                int line,
                                const char* func,
                                const char* fmt,
                                ...);

/* 便捷宏 - 已禁用 */
/* 日志功能已完全移除以提高性能 */
#define UVHTTP_LOG_TRACE(middleware, fmt, ...) ((void)0)
#define UVHTTP_LOG_DEBUG(middleware, fmt, ...) ((void)0)
#define UVHTTP_LOG_INFO(middleware, fmt, ...)  ((void)0)
#define UVHTTP_LOG_WARN(middleware, fmt, ...)  ((void)0)
#define UVHTTP_LOG_ERROR(middleware, fmt, ...) ((void)0)
#define UVHTTP_LOG_FATAL(middleware, fmt, ...) ((void)0)

/* 全局日志中间件（可选） */
extern uvhttp_log_middleware_t* g_uvhttp_log_middleware;

/* 设置全局日志中间件 */
void uvhttp_log_set_global_middleware(uvhttp_log_middleware_t* middleware);

/* 获取全局日志中间件 */
uvhttp_log_middleware_t* uvhttp_log_get_global_middleware(void);

/* 默认配置 */
#define UVHTTP_LOG_DEFAULT_CONTEXT { \
    .level = UVHTTP_LOG_LEVEL_INFO, \
    .format = UVHTTP_LOG_FORMAT_TEXT, \
    .output = UVHTTP_LOG_OUTPUT_STDOUT, \
    .enable_colors = 1, \
    .enable_timestamp = 1, \
    .enable_source_location = 1, \
    .thread_safe = 1 \
}

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_LOG_MIDDLEWARE_H */

#endif /* UVHTTP_FEATURE_LOGGING */