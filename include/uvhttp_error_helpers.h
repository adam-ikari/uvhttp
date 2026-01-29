/* UVHTTP 错误处理辅助函数 */

#ifndef UVHTTP_ERROR_HELPERS_H
#define UVHTTP_ERROR_HELPERS_H

#include "uvhttp_constants.h"
#include "uvhttp_error.h"

#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 通用的连接清理函数
 * @param handle 要清理的句柄
 * @param error_message 错误消息（可为NULL）
 */
void uvhttp_cleanup_connection(uv_handle_t* handle, const char* error_message);

/**
 * 通用的内存分配失败处理
 * @param context 上下文描述
 * @param cleanup_func 清理函数指针（可为NULL）
 * @param cleanup_data 清理函数参数
 */
void uvhttp_handle_memory_failure(const char* context, void (*cleanup_func)(void*),
                                  void* cleanup_data);

/**
 * 通用的写操作错误处理
 * @param req 写请求
 * @param status 写操作状态
 * @param context 上下文描述
 */
void uvhttp_handle_write_error(uv_write_t* req, int status, const char* context);

/**
 * 安全的错误日志记录（避免敏感信息泄露）
 * @param error_code 错误码
 * @param context 上下文描述
 * @param user_msg 用户提供的消息
 */
void uvhttp_log_safe_error(int error_code, const char* context, const char* user_msg);

/**
 * 验证错误消息安全性（过滤敏感信息）
 * @param message 原始错误消息
 * @param safe_buffer 安全输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return UVHTTP_OK成功，其他值表示失败
 */
uvhttp_error_t uvhttp_sanitize_error_message(const char* message, char* safe_buffer,
                                             size_t buffer_size);

/**
 * 通用的资源释放包装器
 * @param ptr 指向资源的指针
 * @param free_func 释放函数
 */
void uvhttp_safe_free(void** ptr, void (*free_func)(void*));

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_ERROR_HELPERS_H */