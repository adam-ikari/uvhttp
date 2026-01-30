/**
 * @file uvhttp_error_handler.c
 * @brief 错误处理实现
 *
 * 注意：已移除错误统计功能，使用日志系统记录错误
 */

#include "uvhttp_error_handler.h"

#include "uvhttp_logging.h"

/* ========== 内部错误报告函数 ========== */

void uvhttp_error_report_(uvhttp_error_t error_code, const char* message,
                          const char* function, const char* file, int line,
                          void* user_data) {
    (void)function;
    (void)file;
    (void)line;
    (void)user_data;

    /* 使用日志系统记录错误 */
    UVHTTP_LOG_ERROR("%s: %s", uvhttp_error_string(error_code), message);

    /* 标记参数为已使用，避免编译器警告 */
    (void)error_code;
    (void)message;
}