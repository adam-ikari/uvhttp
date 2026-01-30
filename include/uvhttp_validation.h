/* UVHTTP 输入验证模块 */

#ifndef UVHTTP_VALIDATION_H
#define UVHTTP_VALIDATION_H

#include "uvhttp_constants.h"

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * 验证字符串长度是否在有效范围内
 * @param str 要验证的字符串
 * @param min_len 最小长度
 * @param max_len 最大长度
 * @return 1有效，0无效
 */
int uvhttp_validate_string_length(const char* str, size_t min_len,
                                  size_t max_len);

/* uvhttp_validate_http_method 已删除 - 使用 uvhttp_method_from_string 替代 */

/**
 * 验证URL路径是否安全
 * @param path URL路径
 * @return 1安全，0不安全
 */
int uvhttp_validate_url_path(const char* path);

/**
 * 验证HTTP头部名称是否有效
 * @param name 头部名称
 * @return 1有效，0无效
 */
int uvhttp_validate_header_name(const char* name);

/**
 * 验证HTTP头部值是否安全
 * @param value 头部值
 * @return 1安全，0不安全
 */
int uvhttp_validate_header_value_safe(const char* value);

/* uvhttp_validate_port 已删除 - 完全未使用 */
/* uvhttp_validate_content_length 已删除 - 完全未使用 */

/**
 * 验证查询字符串是否安全
 * @param query 查询字符串
 * @return 1安全，0不安全
 */
int uvhttp_validate_query_string(const char* query);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_VALIDATION_H */