/* UVHTTP 输入验证模块实现 */

#include "uvhttp_validation.h"

#include "uvhttp_utils.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* 支持的HTTP方法列表 */
static const char* valid_methods[] = {
    UVHTTP_METHOD_GET,   UVHTTP_METHOD_POST,
    UVHTTP_METHOD_PUT,   UVHTTP_METHOD_DELETE,
    UVHTTP_METHOD_HEAD,  UVHTTP_METHOD_OPTIONS,
    UVHTTP_METHOD_PATCH, NULL};

/* 危险的路径字符 */
static const char dangerous_path_chars[] = {'<', '>', ':',  '"', '|',
                                            '?', '*', '\n', '\r'};

/* 危险的查询字符 */
static const char dangerous_query_chars[] = {'<', '>', '"', '\'', '\n', '\r'};

int uvhttp_validate_string_length(const char* str, size_t min_len,
                                  size_t max_len) {
    if (!str)
        return FALSE;

    size_t len = strlen(str);
    return (len >= min_len && len <= max_len) ? TRUE : FALSE;
}

int uvhttp_validate_http_method(const char* method) {
    if (!method)
        return FALSE;

    for (int i = 0; valid_methods[i]; i++) {
        if (strcmp(method, valid_methods[i]) == 0) {
            return TRUE;
        }
    }
    return FALSE;
}

int uvhttp_validate_url_path(const char* path) {
    if (!path)
        return FALSE;

    // 检查长度
    if (!uvhttp_validate_string_length(path, 1, UVHTTP_MAX_PATH_SIZE)) {
        return FALSE;
    }

    // 检查危险字符
    for (size_t i = 0; i < sizeof(dangerous_path_chars); i++) {
        if (strchr(path, dangerous_path_chars[i])) {
            return FALSE;
        }
    }

    // 检查路径遍历攻击
    if (strstr(path, "..") || strstr(path, "//")) {
        return FALSE;
    }

    // 路径必须以/开头
    if (path[0] != '/') {
        return FALSE;
    }

    return TRUE;
}

int uvhttp_validate_header_name(const char* name) {
    if (!name)
        return FALSE;

    // 检查长度
    if (!uvhttp_validate_string_length(name, 1, UVHTTP_MAX_HEADER_NAME_SIZE)) {
        return FALSE;
    }

    // HTTP头部名称只能包含可打印ASCII字符，不能包含冒号
    for (size_t i = 0; name[i]; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!isprint(c) || c == ':') {
            return FALSE;
        }
    }

    return TRUE;
}

int uvhttp_validate_header_value_safe(const char* value) {
    if (!value)
        return FALSE;

    // 检查长度
    if (!uvhttp_validate_string_length(value, 0,
                                       UVHTTP_MAX_HEADER_VALUE_SIZE)) {
        return FALSE;
    }

    // 检查控制字符（除了制表符）
    for (size_t i = 0; value[i]; i++) {
        unsigned char c = (unsigned char)value[i];
        if (c < UVHTTP_SPACE_CHARACTER && c != UVHTTP_TAB_CHARACTER) {
            return FALSE;
        }
        if (c == UVHTTP_DELETE_CHARACTER) {
            return FALSE;
        }
    }

    return TRUE;
}

int uvhttp_validate_port(int port) {
    return (port >= UVHTTP_MIN_PORT_NUMBER && port <= UVHTTP_MAX_PORT_NUMBER);
}

int uvhttp_validate_content_length(size_t length) {
    return (length <= UVHTTP_MAX_BODY_SIZE);
}

int uvhttp_validate_query_string(const char* query) {
    if (!query)
        return TRUE;  /* 空查询字符串是有效的 */

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