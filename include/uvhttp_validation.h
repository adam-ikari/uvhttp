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

/**
 * 验证HTTP方法是否有效
 * @param method HTTP方法字符串
 * @return 1有效，0无效
 */
int uvhttp_validate_http_method(const char* method);

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

/**
 * 验证端口号是否在有效范围内
 * @param port 端口号
 * @return 1有效，0无效
 */
int uvhttp_validate_port(int port);

/**
 * 验证IPv4地址格式
 * @param ip IP地址字符串
 * @return 1有效，0无效
 */
int uvhttp_validate_ipv4(const char* ip);

/**
 * 验证IPv6地址格式
 * @param ip IP地址字符串
 * @return 1有效，0无效
 */
int uvhttp_validate_ipv6(const char* ip);

/**
 * 验证内容长度是否在合理范围内
 * @param length 内容长度
 * @return 1有效，0无效
 */
int uvhttp_validate_content_length(size_t length);

/**
 * 验证WebSocket密钥格式
 * @param key WebSocket密钥
 * @param key_len 密钥长度
 * @return 1有效，0无效
 */
int uvhttp_validate_websocket_key(const char* key, size_t key_len);

/**
 * 验证文件路径是否安全（防止路径遍历攻击）
 * @param path 文件路径
 * @return 1安全，0不安全
 */
int uvhttp_validate_file_path(const char* path);

/**
 * 验证查询字符串是否安全
 * @param query 查询字符串
 * @return 1安全，0不安全
 */
int uvhttp_validate_query_string(const char* query);

/**
 * 通用字符串安全检查
 * @param str 要检查的字符串
 * @param allow_null_bytes 是否允许空字节
 * @param allow_control_chars 是否允许控制字符
 * @return 1安全，0不安全
 */
int uvhttp_validate_string_safety(const char* str, int allow_null_bytes,
                                  int allow_control_chars);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_VALIDATION_H */