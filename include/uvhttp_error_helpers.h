/**
 * @file uvhttp_error_helpers.h
 * @brief Error handling helper functions
 *
 * This module provides helper functions for error handling, including
 * memory failure handling, write error handling, and secure error logging.
 */

#ifndef UVHTTP_ERROR_HELPERS_H
#define UVHTTP_ERROR_HELPERS_H

#include "uvhttp_constants.h"
#include "uvhttp_error.h"

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Handle memory allocation failure
 *
 * @param context Description of the context where failure occurred
 * @param cleanup_func Cleanup function pointer (can be NULL)
 * @param cleanup_data Argument to pass to cleanup function
 *
 * @note Logs error and optionally calls cleanup function
 * @note Useful for handling out-of-memory situations
 */
void uvhttp_handle_memory_failure(const char* context,
                                  void (*cleanup_func)(void*),
                                  void* cleanup_data);

/**
 * @brief Handle write operation error
 *
 * @param req Write request that failed
 * @param status Status code from uv_write callback
 * @param context Description of the context where error occurred
 *
 * @note Logs error with context information
 * @note Closes connection on fatal errors
 */
void uvhttp_handle_write_error(uv_write_t* req, int status,
                               const char* context);

/**
 * @brief Log error safely (avoid sensitive information leakage)
 *
 * @param error_code Error code to log
 * @param context Description of the context
 * @param user_msg User-provided message
 *
 * @note Filters sensitive information from error messages
 * @note Prevents leakage of paths, credentials, etc.
 */
void uvhttp_log_safe_error(int error_code, const char* context,
                           const char* user_msg);

/**
 * @brief Sanitize error message (filter sensitive information)
 *
 * @param message Original error message
 * @param safe_buffer Output buffer for sanitized message
 * @param buffer_size Size of output buffer
 * @return uvhttp_error_t UVHTTP_OK on success, error code otherwise
 *
 * @note Removes or masks sensitive information like paths, credentials
 * @note Ensures safe_buffer is null-terminated
 */
uvhttp_error_t uvhttp_sanitize_error_message(const char* message,
                                             char* safe_buffer,
                                             size_t buffer_size);

/* uvhttp_safe_free 已删除 - 完全未使用，直接使用 uvhttp_free */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_HELPERS_H */