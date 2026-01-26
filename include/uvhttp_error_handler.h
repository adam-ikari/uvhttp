/**
 * @file uvhttp_error_handler.h
 * @brief 统一错误处理框架
 * 
 * 提供统一的错误处理、日志记录和恢复机制
 */

#ifndef UVHTTP_ERROR_HANDLER_H
#define UVHTTP_ERROR_HANDLER_H

#include "uvhttp_error.h"
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 错误处理级别 */
typedef enum {
    UVHTTP_LOG_LEVEL_DEBUG = 0,
    UVHTTP_LOG_LEVEL_INFO,
    UVHTTP_LOG_LEVEL_WARN,
    UVHTTP_LOG_LEVEL_ERROR,
    UVHTTP_LOG_LEVEL_FATAL
} uvhttp_log_level_t;

/* 错误上下文 */
typedef struct {
    uvhttp_error_t error_code;
    const char* function;
    const char* file;
    int line;
    const char* message;
    time_t timestamp;
    void* user_data;
} uvhttp_error_context_t;

/* 错误处理器类型 */
typedef void (*uvhttp_error_handler_t)(const uvhttp_error_context_t* context);

/* 错误处理配置 */
typedef struct {
    uvhttp_log_level_t min_logLevel;
    uvhttp_error_handler_t customHandler;
    int enableRecovery;
    int maxRetries;
    int baseDelayMs;
    int maxDelayMs;
    double backoffMultiplier;
} uvhttp_error_config_t;

/* 全局错误处理配置 */
extern uvhttp_error_config_t g_error_config;

/* 核心错误处理函数 */
void uvhttp_error_init(void);
void uvhttp_error_cleanup(void);
void uvhttp_error_set_config(const uvhttp_error_config_t* config);

/* 错误报告宏 */
#define UVHTTP_ERROR_REPORT(error_code, message) \
    uvhttp_error_report_((error_code), (message), __func__, __FILE__, __LINE__, NULL)

#define UVHTTP_ERROR_REPORT_WITH_DATA(error_code, message, data) \
    uvhttp_error_report_((error_code), (message), __func__, __FILE__, __LINE__, (data))

/* 内部错误报告函数 */
void uvhttp_error_report_(uvhttp_error_t error_code, 
                         const char* message,
                         const char* function,
                         const char* file,
                         int line,
                         void* user_data);

/* 错误恢复函数 */
uvhttp_error_t uvhttp_error_attempt_recovery(const uvhttp_error_context_t* context);

/* 日志函数 - 开发环境日志接口 */
/* 应用层可以自定义日志实现来记录错误信息 */
void uvhttp_log(uvhttp_log_level_t level, const char* format, ...);

/* 日志宏 - 生产环境为空操作，开发环境可启用 */
#ifdef UVHTTP_DEV_MODE
#define UVHTTP_LOG_DEBUG(fmt, ...) uvhttp_log(UVHTTP_LOG_LEVEL_DEBUG, fmt, ##__VA_ARGS__)
#define UVHTTP_LOG_INFO(fmt, ...)  uvhttp_log(UVHTTP_LOG_LEVEL_INFO, fmt, ##__VA_ARGS__)
#define UVHTTP_LOG_WARN(fmt, ...)  uvhttp_log(UVHTTP_LOG_LEVEL_WARN, fmt, ##__VA_ARGS__)
#define UVHTTP_LOG_ERROR(fmt, ...) uvhttp_log(UVHTTP_LOG_LEVEL_ERROR, fmt, ##__VA_ARGS__)
#define UVHTTP_LOG_FATAL(fmt, ...) uvhttp_log(UVHTTP_LOG_LEVEL_FATAL, fmt, ##__VA_ARGS__)
#else
#define UVHTTP_LOG_DEBUG(fmt, ...) ((void)0)
#define UVHTTP_LOG_INFO(fmt, ...)  ((void)0)
#define UVHTTP_LOG_WARN(fmt, ...)  ((void)0)
#define UVHTTP_LOG_ERROR(fmt, ...) ((void)0)
#define UVHTTP_LOG_FATAL(fmt, ...) ((void)0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_HANDLER_H */