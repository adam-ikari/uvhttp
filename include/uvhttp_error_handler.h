/**
 * @file uvhttp_error_handler.h
 * @brief 错误处理
 */

#ifndef UVHTTP_ERROR_HANDLER_H
#define UVHTTP_ERROR_HANDLER_H

#include "uvhttp_error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 错误报告宏 ========== */

/**
 * 报告错误（使用日志系统记录）
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
 * 内部错误报告函数（仅用于日志记录）
 */
void uvhttp_error_report_(uvhttp_error_t error_code, const char* message,
                          const char* function, const char* file, int line,
                          void* user_data);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_HANDLER_H */
