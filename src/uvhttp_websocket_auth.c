/*
 * UVHTTP WebSocket 认证模块实现
 */

#if UVHTTP_FEATURE_WEBSOCKET

#include "uvhttp_websocket_auth.h"
#include "uvhttp_allocator.h"
#include "uvhttp_utils.h"
#include "uvhttp_constants.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* IP 地址匹配辅助函数 */
static int ip_match(const char* pattern, const char* ip) {
    /* 支持精确匹配和 CIDR 表示法 */
    const char* slash = strchr(pattern, '/');
    if (!slash) {
        /* 精确匹配 */
        return strcmp(pattern, ip) == 0;
    }

    /* CIDR 匹配 */
    int prefix_len = atoi(slash + 1);
    if (prefix_len < 0 || prefix_len > 32) {
        return 0;  /* 无效的前缀长度 */
    }

    /* 将 IP 地址转换为 32 位整数 */
    unsigned int pattern_ip = 0;
    unsigned int target_ip = 0;
    char pattern_copy[INET_ADDRSTRLEN];
    char target_copy[INET_ADDRSTRLEN];

    /* 复制并截断 CIDR 部分 */
    size_t pattern_len = slash - pattern;
    if (pattern_len >= sizeof(pattern_copy)) {
        return 0;
    }
    strncpy(pattern_copy, pattern, pattern_len);
    pattern_copy[pattern_len] = '\0';

    /* 复制目标 IP */
    strncpy(target_copy, ip, sizeof(target_copy) - 1);
    target_copy[sizeof(target_copy) - 1] = '\0';

    /* 转换为网络字节序 */
    struct in_addr pattern_addr, target_addr;
    if (inet_pton(AF_INET, pattern_copy, &pattern_addr) != 1) {
        return 0;
    }
    if (inet_pton(AF_INET, target_copy, &target_addr) != 1) {
        return 0;
    }

    pattern_ip = ntohl(pattern_addr.s_addr);
    target_ip = ntohl(target_addr.s_addr);

    /* 创建掩码 */
    unsigned int mask = prefix_len == 0 ? 0 : (0xFFFFFFFF << (32 - prefix_len));

    /* 比较网络部分 */
    return (pattern_ip & mask) == (target_ip & mask);
}

/* 创建认证配置 */
uvhttp_ws_auth_config_t* uvhttp_ws_auth_config_create(void) {
    uvhttp_ws_auth_config_t* config = uvhttp_alloc(sizeof(uvhttp_ws_auth_config_t));
    if (!config) {
        return NULL;
    }

    /* 初始化默认配置 */
    memset(config, 0, sizeof(uvhttp_ws_auth_config_t));
    config->enable_token_auth = 0;
    strncpy(config->token_param_name, "token", sizeof(config->token_param_name) - 1);
    config->token_validator = NULL;
    config->token_validator_data = NULL;
    config->enable_ip_whitelist = FALSE;
    config->ip_whitelist = NULL;
    config->enable_ip_blacklist = FALSE;
    config->ip_blacklist = NULL;
    config->send_auth_failed_response = TRUE;
    strncpy(config->auth_failed_message, "Authentication failed",
            sizeof(config->auth_failed_message) - 1);

    return config;
}

/* 销毁认证配置 */
void uvhttp_ws_auth_config_destroy(uvhttp_ws_auth_config_t* config) {
    if (!config) {
        return;
    }

    /* 释放 IP 白名单 */
    ip_entry_t* whitelist = config->ip_whitelist;
    while (whitelist) {
        ip_entry_t* next = whitelist->next;
        uvhttp_free(whitelist);
        whitelist = next;
    }

    /* 释放 IP 黑名单 */
    ip_entry_t* blacklist = config->ip_blacklist;
    while (blacklist) {
        ip_entry_t* next = blacklist->next;
        uvhttp_free(blacklist);
        blacklist = next;
    }

    uvhttp_free(config);
}

/* 设置 Token 验证回调 */
void uvhttp_ws_auth_set_token_validator(
    uvhttp_ws_auth_config_t* config,
    uvhttp_ws_token_validator_callback validator,
    void* user_data
) {
    if (!config) {
        return;
    }

    config->token_validator = validator;
    config->token_validator_data = user_data;
    config->enable_token_auth = (validator != NULL);
}

/* 添加 IP 到白名单 */
int uvhttp_ws_auth_add_ip_to_whitelist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
) {
    if (!config || !ip) {
        return -1;
    }

    /* 检查 IP 是否已存在 */
    ip_entry_t* entry = config->ip_whitelist;
    while (entry) {
        if (strcmp(entry->ip, ip) == 0) {
            return 0;  /* 已存在 */
        }
        entry = entry->next;
    }

    /* 创建新条目 */
    ip_entry_t* new_entry = uvhttp_alloc(sizeof(ip_entry_t));
    if (!new_entry) {
        return -1;
    }

    strncpy(new_entry->ip, ip, sizeof(new_entry->ip) - 1);
    new_entry->ip[sizeof(new_entry->ip) - 1] = '\0';
    new_entry->next = config->ip_whitelist;
    config->ip_whitelist = new_entry;
    config->enable_ip_whitelist = TRUE;

    return 0;
}

/* 添加 IP 到黑名单 */
int uvhttp_ws_auth_add_ip_to_blacklist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
) {
    if (!config || !ip) {
        return -1;
    }

    /* 检查 IP 是否已存在 */
    ip_entry_t* entry = config->ip_blacklist;
    while (entry) {
        if (strcmp(entry->ip, ip) == 0) {
            return 0;  /* 已存在 */
        }
        entry = entry->next;
    }

    /* 创建新条目 */
    ip_entry_t* new_entry = uvhttp_alloc(sizeof(ip_entry_t));
    if (!new_entry) {
        return -1;
    }

    strncpy(new_entry->ip, ip, sizeof(new_entry->ip) - 1);
    new_entry->ip[sizeof(new_entry->ip) - 1] = '\0';
    new_entry->next = config->ip_blacklist;
    config->ip_blacklist = new_entry;
    config->enable_ip_blacklist = TRUE;

    return 0;
}

/* 从白名单移除 IP */
int uvhttp_ws_auth_remove_ip_from_whitelist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
) {
    if (!config || !ip) {
        return -1;
    }

    ip_entry_t** entry_ptr = &config->ip_whitelist;
    while (*entry_ptr) {
        if (strcmp((*entry_ptr)->ip, ip) == 0) {
            ip_entry_t* to_remove = *entry_ptr;
            *entry_ptr = to_remove->next;
            uvhttp_free(to_remove);
            return 0;
        }
        entry_ptr = &(*entry_ptr)->next;
    }

    /* 检查是否为空 */
    if (!config->ip_whitelist) {
        config->enable_ip_whitelist = FALSE;
    }

    return -1;  /* 未找到 */
}

/* 从黑名单移除 IP */
int uvhttp_ws_auth_remove_ip_from_blacklist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
) {
    if (!config || !ip) {
        return -1;
    }

    ip_entry_t** entry_ptr = &config->ip_blacklist;
    while (*entry_ptr) {
        if (strcmp((*entry_ptr)->ip, ip) == 0) {
            ip_entry_t* to_remove = *entry_ptr;
            *entry_ptr = to_remove->next;
            uvhttp_free(to_remove);
            return 0;
        }
        entry_ptr = &(*entry_ptr)->next;
    }

    /* 检查是否为空 */
    if (!config->ip_blacklist) {
        config->enable_ip_blacklist = FALSE;
    }

    return -1;  /* 未找到 */
}

/* 验证 IP 地址是否在白名单中 */
int uvhttp_ws_auth_is_ip_whitelisted(
    uvhttp_ws_auth_config_t* config,
    const char* ip
) {
    if (!config || !ip) {
        return 0;
    }

    if (!config->enable_ip_whitelist) {
        return 1;  /* 未启用白名单，允许所有 IP */
    }

    ip_entry_t* entry = config->ip_whitelist;
    while (entry) {
        if (ip_match(entry->ip, ip)) {
            return 1;
        }
        entry = entry->next;
    }

    return 0;  /* 不在白名单中 */
}

/* 验证 IP 地址是否在黑名单中 */
int uvhttp_ws_auth_is_ip_blacklisted(
    uvhttp_ws_auth_config_t* config,
    const char* ip
) {
    if (!config || !ip) {
        return 0;
    }

    if (!config->enable_ip_blacklist) {
        return 0;  /* 未启用黑名单，不阻止任何 IP */
    }

    ip_entry_t* entry = config->ip_blacklist;
    while (entry) {
        if (ip_match(entry->ip, ip)) {
            return 1;  /* 在黑名单中 */
        }
        entry = entry->next;
    }

    return 0;  /* 不在黑名单中 */
}

/* 执行认证 */
uvhttp_ws_auth_result_t uvhttp_ws_authenticate(
    uvhttp_ws_auth_config_t* config,
    const char* client_ip,
    const char* token
) {
    if (!config) {
        return UVHTTP_WS_AUTH_INTERNAL_ERROR;
    }

    /* 1. 检查 IP 黑名单 */
    if (config->enable_ip_blacklist && client_ip) {
        if (uvhttp_ws_auth_is_ip_blacklisted(config, client_ip)) {
            return UVHTTP_WS_AUTH_IP_BLOCKED;
        }
    }

    /* 2. 检查 IP 白名单 */
    if (config->enable_ip_whitelist && client_ip) {
        if (!uvhttp_ws_auth_is_ip_whitelisted(config, client_ip)) {
            return UVHTTP_WS_AUTH_IP_NOT_ALLOWED;
        }
    }

    /* 3. 检查 Token 认证 */
    if (config->enable_token_auth) {
        if (!token || strlen(token) == 0) {
            return UVHTTP_WS_AUTH_NO_TOKEN;
        }

        if (config->token_validator) {
            int result = config->token_validator(token, config->token_validator_data);
            if (result != 0) {
                return UVHTTP_WS_AUTH_INVALID_TOKEN;
            }
        } else {
            /* 未设置验证器，默认拒绝 */
            return UVHTTP_WS_AUTH_INTERNAL_ERROR;
        }
    }

    return UVHTTP_WS_AUTH_SUCCESS;
}

/* 获取认证结果描述 */
const char* uvhttp_ws_auth_result_string(uvhttp_ws_auth_result_t result) {
    switch (result) {
        case UVHTTP_WS_AUTH_SUCCESS:
            return "Authentication successful";
        case UVHTTP_WS_AUTH_FAILED:
            return "Authentication failed";
        case UVHTTP_WS_AUTH_NO_TOKEN:
            return "No token provided";
        case UVHTTP_WS_AUTH_INVALID_TOKEN:
            return "Invalid token";
        case UVHTTP_WS_AUTH_EXPIRED_TOKEN:
            return "Token expired";
        case UVHTTP_WS_AUTH_IP_BLOCKED:
            return "IP address blocked";
        case UVHTTP_WS_AUTH_IP_NOT_ALLOWED:
            return "IP address not allowed";
        case UVHTTP_WS_AUTH_INTERNAL_ERROR:
            return "Internal authentication error";
        default:
            return "Unknown authentication result";
    }
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */