/* UVHTTP 输入验证模块实现 */

#include "uvhttp_validation.h"
#include "uvhttp_utils.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

/* 支持的HTTP方法列表 */
static const char* valid_methods[] = {
    UVHTTP_METHOD_GET, UVHTTP_METHOD_POST, UVHTTP_METHOD_PUT,
    UVHTTP_METHOD_DELETE, UVHTTP_METHOD_HEAD, UVHTTP_METHOD_OPTIONS,
    UVHTTP_METHOD_PATCH, NULL
};

/* 危险的路径字符 */
static const char dangerous_path_chars[] = {
    '<', '>', ':', '"', '|', '?', '*', '\n', '\r'
};

/* 危险的查询字符 */
static const char dangerous_query_chars[] = {
    '<', '>', '"', '\'', '\n', '\r'
};

int uvhttp_validate_string_length(const char* str, size_t min_len, size_t max_len) {
    if (!str) return UVHTTP_FALSE;
    
    size_t len = strlen(str);
    return (len >= min_len && len <= max_len) ? UVHTTP_TRUE : UVHTTP_FALSE;
}

int uvhttp_validate_http_method(const char* method) {
    if (!method) return UVHTTP_FALSE;
    
    for (int i = 0; valid_methods[i]; i++) {
        if (strcmp(method, valid_methods[i]) == 0) {
            return UVHTTP_TRUE;
        }
    }
    return UVHTTP_FALSE;
}

int uvhttp_validate_url_path(const char* path) {
    if (!path) return UVHTTP_FALSE;
    
    // 检查长度
    if (!uvhttp_validate_string_length(path, 1, UVHTTP_MAX_PATH_SIZE)) {
        return UVHTTP_FALSE;
    }
    
    // 检查危险字符
    for (size_t i = 0; i < sizeof(dangerous_path_chars); i++) {
        if (strchr(path, dangerous_path_chars[i])) {
            return UVHTTP_FALSE;
        }
    }
    
    // 检查路径遍历攻击
    if (strstr(path, "..") || strstr(path, "//")) {
        return UVHTTP_FALSE;
    }
    
    // 路径必须以/开头
    if (path[0] != '/') {
        return UVHTTP_FALSE;
    }
    
    return UVHTTP_TRUE;
}

int uvhttp_validate_header_name(const char* name) {
    if (!name) return UVHTTP_FALSE;
    
    // 检查长度
    if (!uvhttp_validate_string_length(name, 1, UVHTTP_MAX_HEADER_NAME_SIZE)) {
        return UVHTTP_FALSE;
    }
    
    // HTTP头部名称只能包含可打印ASCII字符，不能包含冒号
    for (size_t i = 0; name[i]; i++) {
        unsigned char c = (unsigned char)name[i];
        if (!isprint(c) || c == ':') {
            return UVHTTP_FALSE;
        }
    }
    
    return UVHTTP_TRUE;
}

int uvhttp_validate_header_value_safe(const char* value) {
    if (!value) return UVHTTP_FALSE;
    
    // 检查长度
    if (!uvhttp_validate_string_length(value, 0, UVHTTP_MAX_HEADER_VALUE_SIZE)) {
        return UVHTTP_FALSE;
    }
    
    // 检查控制字符（除了制表符）
    for (size_t i = 0; value[i]; i++) {
        unsigned char c = (unsigned char)value[i];
        if (c < UVHTTP_SPACE_CHARACTER && c != UVHTTP_TAB_CHARACTER) {
            return UVHTTP_FALSE;
        }
        if (c == UVHTTP_DELETE_CHARACTER) {
            return UVHTTP_FALSE;
        }
    }
    
    return UVHTTP_TRUE;
}

int uvhttp_validate_port(int port) {
    return (port >= UVHTTP_MIN_PORT_NUMBER && port <= UVHTTP_MAX_PORT_NUMBER);
}

int uvhttp_validate_ipv4(const char* ip) {
    if (!ip) return UVHTTP_FALSE;
    
    int octets = 0;
    int current = 0;
    
    for (size_t i = 0; ip[i]; i++) {
        if (ip[i] == '.') {
            octets++;
            if (current > 255 || current < 0) return UVHTTP_FALSE;
            current = 0;
        } else if (isdigit(ip[i])) {
            current = current * 10 + (ip[i] - '0');
        } else {
            return UVHTTP_FALSE;
        }
    }
    
    // 检查最后一个八位组
    if (current > 255 || current < 0) return UVHTTP_FALSE;
    
    return (octets == 3);
}

int uvhttp_validate_ipv6(const char* ip) {
    if (!ip) return UVHTTP_FALSE;
    
    // 简化的IPv6验证 - 检查基本格式
    int colons = 0;
    int digits = 0;
    
    for (size_t i = 0; ip[i]; i++) {
        if (ip[i] == ':') {
            colons++;
            digits = 0;
        } else if (isxdigit(ip[i])) {
            digits++;
            if (digits > 4) return UVHTTP_FALSE;
        } else {
            return UVHTTP_FALSE;
        }
    }
    
    return (colons >= 2 && colons <= 7);
}

int uvhttp_validate_content_length(size_t length) {
    return (length <= UVHTTP_MAX_BODY_SIZE);
}

int uvhttp_validate_websocket_key(const char* key, size_t key_len) {
    if (!key) return UVHTTP_FALSE;
    
    // 检查长度范围
    if (key_len < UVHTTP_WEBSOCKET_MIN_KEY_LENGTH || 
        key_len > UVHTTP_WEBSOCKET_MAX_KEY_LENGTH) {
        return UVHTTP_FALSE;
    }
    
    // 检查是否为有效的base64字符
    for (size_t i = 0; i < key_len; i++) {
        char c = key[i];
        if (!(isalnum(c) || c == '+' || c == '/' || c == '=')) {
            return UVHTTP_FALSE;
        }
    }
    
    return UVHTTP_TRUE;
}

int uvhttp_validate_file_path(const char* path) {
    if (!path) return UVHTTP_FALSE;
    
    // 检查长度
    if (!uvhttp_validate_string_length(path, 1, UVHTTP_MAX_FILE_PATH_SIZE)) {
        return UVHTTP_FALSE;
    }
    
    // 检查路径遍历攻击
    if (strstr(path, "..") || strstr(path, "//")) {
        return UVHTTP_FALSE;
    }
    
    // 检查绝对路径（在某些情况下可能不安全）
    if (path[0] == '/') {
        return UVHTTP_FALSE;
    }
    
    return UVHTTP_TRUE;
}

int uvhttp_validate_query_string(const char* query) {
    if (!query) return UVHTTP_TRUE; // 空查询字符串是有效的
    
    // 检查长度
    if (!uvhttp_validate_string_length(query, 0, UVHTTP_MAX_URL_SIZE)) {
        return UVHTTP_FALSE;
    }
    
    // 检查危险字符
    for (size_t i = 0; i < sizeof(dangerous_query_chars); i++) {
        if (strchr(query, dangerous_query_chars[i])) {
            return UVHTTP_FALSE;
        }
    }
    
    return UVHTTP_TRUE;
}

int uvhttp_validate_string_safety(const char* str, int allow_null_bytes, int allow_control_chars) {
    if (!str) return UVHTTP_FALSE;
    
    for (size_t i = 0; str[i]; i++) {
        unsigned char c = (unsigned char)str[i];
        
        // 检查空字节
        if (!allow_null_bytes && c == '\0') {
            return UVHTTP_FALSE;
        }
        
        // 检查控制字符
        if (!allow_control_chars && c < UVHTTP_SPACE_CHARACTER && c != UVHTTP_TAB_CHARACTER) {
            return UVHTTP_FALSE;
        }
        
        // 检查删除字符
        if (c == UVHTTP_DELETE_CHARACTER) {
            return UVHTTP_FALSE;
        }
    }
    
    return UVHTTP_TRUE;
}