#ifndef UVHTTP_HEALTH_H
#define UVHTTP_HEALTH_H

#include "uvhttp_common.h"
#include "uvhttp_server.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 健康检查状态 */
typedef enum {
    UVHTTP_HEALTH_STATUS_PASSING,  /* 健康 */
    UVHTTP_HEALTH_STATUS_WARNING,  /* 警告 */
    UVHTTP_HEALTH_STATUS_CRITICAL, /* 严重 */
    UVHTTP_HEALTH_STATUS_UNKNOWN   /* 未知 */
} uvhttp_health_status_t;

/* 健康检查结果 */
typedef struct {
    uvhttp_health_status_t status;
    char message[256];
    uint64_t uptime_ms;
    size_t active_connections;
    size_t total_requests;
    size_t total_errors;
} uvhttp_health_result_t;

/* 健康检查配置 */
typedef struct {
    int enabled;
    char path[256];
    int include_metrics;
} uvhttp_health_config_t;

/* 健康检查函数 */
int uvhttp_health_check(uvhttp_server_t* server, uvhttp_health_result_t* result);
const char* uvhttp_health_status_string(uvhttp_health_status_t status);
int uvhttp_health_handler(uvhttp_request_t* request, uvhttp_response_t* response);
int uvhttp_server_enable_health_check(uvhttp_server_t* server, const char* path);
int uvhttp_server_disable_health_check(uvhttp_server_t* server);

#ifdef __cplusplus
}
#endif

#endif