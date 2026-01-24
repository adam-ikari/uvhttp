/*
 * UVHTTP WebSocket 认证模块
 * 提供 WebSocket 连接认证功能
 */

#if UVHTTP_FEATURE_WEBSOCKET

#ifndef UVHTTP_WEBSOCKET_AUTH_H
#define UVHTTP_WEBSOCKET_AUTH_H

#include <stddef.h>
#include <stdint.h>
#include "uvhttp_websocket_impl.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 认证结果 */
typedef enum {
    UVHTTP_WS_AUTH_SUCCESS = 0,           /* 认证成功 */
    UVHTTP_WS_AUTH_FAILED = -1,           /* 认证失败 */
    UVHTTP_WS_AUTH_NO_TOKEN = -2,         /* 缺少 Token */
    UVHTTP_WS_AUTH_INVALID_TOKEN = -3,    /* Token 无效 */
    UVHTTP_WS_AUTH_EXPIRED_TOKEN = -4,    /* Token 已过期 */
    UVHTTP_WS_AUTH_IP_BLOCKED = -5,       /* IP 被阻止 */
    UVHTTP_WS_AUTH_IP_NOT_ALLOWED = -6,   /* IP 不在白名单中 */
    UVHTTP_WS_AUTH_INTERNAL_ERROR = -7    /* 内部错误 */
} uvhttp_ws_auth_result_t;

/* IP 地址条目 */
typedef struct ip_entry {
    char ip[64];              /* IP 地址或 CIDR */
    struct ip_entry* next;    /* 下一个条目 */
} ip_entry_t;

/* Token 验证回调函数类型 */
typedef int (*uvhttp_ws_token_validator_callback)(const char* token, void* user_data);

/* WebSocket 认证配置 */
typedef struct {
    /* Token 认证 */
    int enable_token_auth;                    /* 是否启用 Token 认证 */
    char token_param_name[64];                /* Token 参数名（查询参数或头部） */
    uvhttp_ws_token_validator_callback token_validator;  /* Token 验证回调 */
    void* token_validator_data;               /* Token 验证回调的用户数据 */

    /* IP 白名单 */
    int enable_ip_whitelist;                  /* 是否启用 IP 白名单 */
    ip_entry_t* ip_whitelist;                 /* IP 白名单链表 */

    /* IP 黑名单 */
    int enable_ip_blacklist;                  /* 是否启用 IP 黑名单 */
    ip_entry_t* ip_blacklist;                 /* IP 黑名单链表 */

    /* 认证失败处理 */
    int send_auth_failed_response;            /* 是否发送认证失败响应 */
    char auth_failed_message[256];            /* 认证失败消息 */
} uvhttp_ws_auth_config_t;

/* WebSocket 认证上下文 */
typedef struct {
    uvhttp_ws_auth_config_t config;           /* 认证配置 */
    char client_ip[64];                       /* 客户端 IP */
    char token[256];                          /* 客户端 Token */
} uvhttp_ws_auth_context_t;

/* 创建认证配置 */
uvhttp_ws_auth_config_t* uvhttp_ws_auth_config_create(void);

/* 销毁认证配置 */
void uvhttp_ws_auth_config_destroy(uvhttp_ws_auth_config_t* config);

/* 设置 Token 验证回调 */
void uvhttp_ws_auth_set_token_validator(
    uvhttp_ws_auth_config_t* config,
    uvhttp_ws_token_validator_callback validator,
    void* user_data
);

/* 添加 IP 到白名单 */
int uvhttp_ws_auth_add_ip_to_whitelist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);

/* 添加 IP 到黑名单 */
int uvhttp_ws_auth_add_ip_to_blacklist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);

/* 从白名单移除 IP */
int uvhttp_ws_auth_remove_ip_from_whitelist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);

/* 从黑名单移除 IP */
int uvhttp_ws_auth_remove_ip_from_blacklist(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);

/* 验证 IP 地址是否在白名单中 */
int uvhttp_ws_auth_is_ip_whitelisted(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);

/* 验证 IP 地址是否在黑名单中 */
int uvhttp_ws_auth_is_ip_blacklisted(
    uvhttp_ws_auth_config_t* config,
    const char* ip
);

/* 执行认证 */
uvhttp_ws_auth_result_t uvhttp_ws_authenticate(
    uvhttp_ws_auth_config_t* config,
    const char* client_ip,
    const char* token
);

/* 获取认证结果描述 */
const char* uvhttp_ws_auth_result_string(uvhttp_ws_auth_result_t result);

/* 默认配置 */
#define UVHTTP_WS_AUTH_DEFAULT_CONFIG { \
    .enable_token_auth = 0, \
    .token_param_name = "token", \
    .token_validator = NULL, \
    .token_validator_data = NULL, \
    .enable_ip_whitelist = 0, \
    .ip_whitelist = NULL, \
    .enable_ip_blacklist = 0, \
    .ip_blacklist = NULL, \
    .send_auth_failed_response = 1, \
    .auth_failed_message = "Authentication failed" \
}

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_WEBSOCKET_AUTH_H */

#endif /* UVHTTP_FEATURE_WEBSOCKET */
