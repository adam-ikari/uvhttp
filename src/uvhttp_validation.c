/* UVHTTP 输入验证模块实现 */

#include "uvhttp_validation.h"

#include "uvhttp_utils.h"

#include <ctype.h>
#include <string.h>

/* 危险的路径字符 */
static const char dangerous_path_chars[] = {'<', '>', ':',  '"', '|',
                                            '?', '*', '\n', '\r'};

/* 危险的查询字符 */
static const char dangerous_query_chars[] = {'<', '>', '"', '\'', '\n', '\r'};

/* 危险的头部字符 */
static const char dangerous_header_chars[] = {'\n', '\r'};

int uvhttp_validate_string_length(const char* str, size_t min_len,
                                  size_t max_len) {
    if (!str)
        return FALSE;

    size_t len = strlen(str);
    return (len >= min_len && len <= max_len) ? TRUE : FALSE;
}

/* uvhttp_validate_http_method 已删除 - 使用 uvhttp_method_from_string 替代 */

int uvhttp_validate_url_path(const char* path) {
    if (!path)
        return FALSE;

    /* 检查长度 */
    if (!uvhttp_validate_string_length(path, 1, UVHTTP_MAX_PATH_SIZE)) {
        return FALSE;
    }

    /* 检查危险字符 */
    for (size_t i = 0; i < sizeof(dangerous_path_chars); i++) {
        if (strchr(path, dangerous_path_chars[i])) {
            return FALSE;
        }
    }

    return TRUE;
}

int uvhttp_validate_header_name(const char* name) {
    if (!name)
        return FALSE;

    /* 检查长度 */
    if (!uvhttp_validate_string_length(name, 1, UVHTTP_MAX_HEADER_NAME_SIZE)) {
        return FALSE;
    }

    /* 检查非法字符 */
    for (size_t i = 0; i < strlen(name); i++) {
        char c = name[i];
        /* 头部名称只能包含字母、数字和连字符 */
        if (!isalnum(c) && c != '-') {
            return FALSE;
        }
    }

    return TRUE;
}

int uvhttp_validate_header_value_safe(const char* value) {
    if (!value)
        return FALSE;

    /* 检查长度 */
    if (!uvhttp_validate_string_length(value, 0,
                                       UVHTTP_MAX_HEADER_VALUE_SIZE)) {
        return FALSE;
    }

    /* 检查危险字符 */
    for (size_t i = 0; i < sizeof(dangerous_header_chars); i++) {
        if (strchr(value, dangerous_header_chars[i])) {
            return FALSE;
        }
    }

    return TRUE;
}

/* uvhttp_validate_port 已删除 - 完全未使用 */
/* uvhttp_validate_content_length 已删除 - 完全未使用 */

int uvhttp_validate_query_string(const char* query) {
    if (!query)
        return TRUE; /* 空查询字符串是有效的 */

    /* 检查长度 */
    if (!uvhttp_validate_string_length(query, 0, UVHTTP_MAX_URL_SIZE)) {
        return FALSE;
    }

    /* 检查危险字符 */
    for (size_t i = 0; i < sizeof(dangerous_query_chars); i++) {
        if (strchr(query, dangerous_query_chars[i])) {
            return FALSE;
        }
    }

    return TRUE;
}