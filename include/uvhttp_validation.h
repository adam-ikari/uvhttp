/**
 * @file uvhttp_validation.h
 * @brief Input validation module
 *
 * This module provides validation functions for HTTP input data including
 * URL paths, headers, and query strings. All functions are inline-optimized
 * for zero runtime overhead.
 *
 * @note These functions validate input against security best practices
 *       to prevent injection attacks and buffer overflows.
 */

#ifndef UVHTTP_VALIDATION_H
#define UVHTTP_VALIDATION_H

#include "uvhttp_constants.h"

#include <ctype.h>
#include <stddef.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 危险字符常量 */
static const char dangerous_path_chars[] = {'<', '>', ':',  '"', '|',
                                            '?', '*', '\n', '\r'};
static const char dangerous_query_chars[] = {'<', '>', '"', '\'', '\n', '\r'};
static const char dangerous_header_chars[] = {'\n', '\r'};

/* ========== 内联验证函数 ========== */

/**
 * @brief Validate string length is within specified range
 *
 * @param str String to validate (NULL returns FALSE)
 * @param min_len Minimum allowed length
 * @param max_len Maximum allowed length
 * @return int TRUE if length is valid, FALSE otherwise
 *
 * @note This function is inline-optimized for performance
 * @note NULL string returns FALSE
 */
static inline int uvhttp_validate_string_length(const char* str, size_t min_len,
                                                size_t max_len) {
    if (!str)
        return FALSE;
    size_t len = strlen(str);
    return (len >= min_len && len <= max_len) ? TRUE : FALSE;
}

/**
 * @brief Validate URL path is safe
 *
 * @param path URL path to validate
 * @return int TRUE if path is safe, FALSE otherwise
 *
 * @note Checks for dangerous characters: < > : " | ? * \n \r
 * @note Path length must be between 1 and UVHTTP_MAX_PATH_SIZE
 * @note NULL path returns FALSE
 */
static inline int uvhttp_validate_url_path(const char* path) {
    if (!path)
        return FALSE;
    if (!uvhttp_validate_string_length(path, 1, UVHTTP_MAX_PATH_SIZE))
        return FALSE;
    for (size_t i = 0; i < sizeof(dangerous_path_chars); i++) {
        if (strchr(path, dangerous_path_chars[i]))
            return FALSE;
    }
    return TRUE;
}

/**
 * @brief Validate HTTP header name is valid
 *
 * @param name HTTP header name to validate
 * @return int TRUE if name is valid, FALSE otherwise
 *
 * @note Valid characters: alphanumeric and hyphen (-)
 * @note Name length must be between 1 and UVHTTP_MAX_HEADER_NAME_SIZE
 * @note NULL name returns FALSE
 */
static inline int uvhttp_validate_header_name(const char* name) {
    if (!name)
        return FALSE;
    if (!uvhttp_validate_string_length(name, 1, UVHTTP_MAX_HEADER_NAME_SIZE))
        return FALSE;
    for (size_t i = 0; i < strlen(name); i++) {
        char c = name[i];
        if (!isalnum(c) && c != '-')
            return FALSE;
    }
    return TRUE;
}

/**
 * @brief Validate HTTP header value is safe
 *
 * @param value HTTP header value to validate
 * @return int TRUE if value is safe, FALSE otherwise
 *
 * @note Checks for dangerous characters: \n \r (header injection)
 * @note Value length must be between 0 and UVHTTP_MAX_HEADER_VALUE_SIZE
 * @note NULL value returns FALSE
 */
static inline int uvhttp_validate_header_value_safe(const char* value) {
    if (!value)
        return FALSE;
    if (!uvhttp_validate_string_length(value, 0, UVHTTP_MAX_HEADER_VALUE_SIZE))
        return FALSE;
    for (size_t i = 0; i < sizeof(dangerous_header_chars); i++) {
        if (strchr(value, dangerous_header_chars[i]))
            return FALSE;
    }
    return TRUE;
}

/**
 * @brief Validate query string is safe
 *
 * @param query Query string to validate
 * @return int TRUE if query is safe, FALSE otherwise
 *
 * @note Checks for dangerous characters: < > " ' \n \r
 * @note Query length must be between 0 and UVHTTP_MAX_URL_SIZE
 * @note NULL query returns TRUE (empty query is valid)
 */
static inline int uvhttp_validate_query_string(const char* query) {
    if (!query)
        return TRUE;
    if (!uvhttp_validate_string_length(query, 0, UVHTTP_MAX_URL_SIZE))
        return FALSE;
    for (size_t i = 0; i < sizeof(dangerous_query_chars); i++) {
        if (strchr(query, dangerous_query_chars[i]))
            return FALSE;
    }
    return TRUE;
}

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_VALIDATION_H */