/**
 * @file uvhttp_error_handler.h
 * @brief 错误恢复机制
 *
 * 提供自定义错误处理和恢复机制
 * 注意：错误统计功能由 uvhttp_error.h 提供，日志记录功能由 uvhttp_logging.h
 * 提供
 */

#ifndef UVHTTP_ERROR_HANDLER_H
#define UVHTTP_ERROR_HANDLER_H

#include "uvhttp_error.h"
#include "uvhttp_logging.h"

#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

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

/* 错误恢复配置 */
typedef struct {
    uvhttp_error_handler_t custom_handler;
    int enable_recovery;
    int max_retries;
    int base_delay_ms;
    int max_delay_ms;
    double backoff_multiplier;
} uvhttp_error_recovery_config_t;

/* 全局错误恢复配置 */
extern uvhttp_error_recovery_config_t g_error_recovery_config;

/* ========== 错误恢复函数 ========== */

/**
 * 初始化错误恢复系统
 */
void uvhttp_error_recovery_init(void);

/**
 * 清理错误恢复系统
 */
void uvhttp_error_cleanup(void);

/**
 * 设置错误恢复配置
 */
void uvhttp_error_set_recovery_config(
    const uvhttp_error_recovery_config_t* config);

/**
 * 尝试从错误中恢复
 *
 * @param context 错误上下文
 * @return 错误码，UVHTTP_OK 表示恢复成功
 */
uvhttp_error_t uvhttp_error_attempt_recovery(
    const uvhttp_error_context_t* context);

/* ========== 错误报告宏 ========== */

/**
 * 报告错误（使用日志系统记录）
 * 注意：实际日志记录由 UVHTTP_LOG_* 宏处理
 */
#define UVHTTP_ERROR_REPORT(error_code, message)                              \
    do {                                                                      \
        uvhttp_error_report_((error_code), (message), __func__, __FILE__,     \
                             __LINE__, NULL);                                 \
        UVHTTP_LOG_ERROR("%s: %s", uvhttp_error_string(error_code), message); \
    } while (0)

#define UVHTTP_ERROR_REPORT_WITH_DATA(error_code, message, data)              \
    do {                                                                      \
        uvhttp_error_report_((error_code), (message), __func__, __FILE__,     \
                             __LINE__, (data));                               \
        UVHTTP_LOG_ERROR("%s: %s", uvhttp_error_string(error_code), message); \
    } while (0)

/* ========== 内部函数 ========== */

/**
 * 内部错误报告函数（仅用于统计）
 */
void uvhttp_error_report_(uvhttp_error_t error_code, const char* message,
                          const char* function, const char* file, int line,
                          void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_HANDLER_H */