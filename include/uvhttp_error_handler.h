/**
 * @file uvhttp_error_handler.h
 * @brief Error handling interface
 *
 * This module provides error reporting utilities for logging and
 * handling errors throughout the UVHTTP library.
 *
 * @note Error reporting functions are inline-optimized for zero overhead
 */

#ifndef UVHTTP_ERROR_HANDLER_H
#define UVHTTP_ERROR_HANDLER_H

#include "uvhttp_error.h"
#include "uvhttp_logging.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Internal error reporting function
 *
 * @param error_code Error code to report
 * @param message Error message
 * @param function Function name where error occurred
 * @param file Source file name
 * @param line Line number in source file
 * @param user_data User-provided data (can be NULL)
 *
 * @note This function is inline-optimized for performance
 * @note Logs error message using UVHTTP_LOG_ERROR
 * @note Parameters are marked as unused to suppress warnings
 */
static inline void uvhttp_error_report_(uvhttp_error_t error_code,
                                        const char* message,
                                        const char* function, const char* file,
                                        int line, void* user_data) {
    (void)error_code;
    (void)message;
    (void)function;
    (void)file;
    (void)line;
    (void)user_data;
    UVHTTP_LOG_ERROR("%s: %s", uvhttp_error_string(error_code), message);
}

/**
 * @brief Error reporting macro
 *
 * @param error_code Error code to report
 * @param message Error message to log
 *
 * @note Automatically captures function name, file, and line number
 * @note Uses do-while(0) for safe macro expansion
 */
#define UVHTTP_ERROR_REPORT(error_code, message)                              \
    do {                                                                      \
        uvhttp_error_report_((error_code), (message), __func__, __FILE__,     \
                             __LINE__, NULL);                                 \
        UVHTTP_LOG_ERROR("%s: %s", uvhttp_error_string(error_code), message); \
    } while (0)

/**
 * @brief Error reporting macro with user data
 *
 * @param error_code Error code to report
 * @param message Error message to log
 * @param user_data User-provided data to pass to error handler
 *
 * @note Automatically captures function name, file, and line number
 * @note Uses do-while(0) for safe macro expansion
 */
#define UVHTTP_ERROR_REPORT_WITH_DATA(error_code, message, user_data)         \
    do {                                                                      \
        uvhttp_error_report_((error_code), (message), __func__, __FILE__,     \
                             __LINE__, (user_data));                          \
        UVHTTP_LOG_ERROR("%s: %s", uvhttp_error_string(error_code), message); \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_HANDLER_H */
