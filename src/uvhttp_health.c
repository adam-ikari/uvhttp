#include "uvhttp_health.h"
#include "uvhttp_server.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

/* 全局健康检查配置 */
static uvhttp_health_config_t g_health_config = {
    .enabled = 0,
    .path = "/health",
    .include_metrics = 1
};

/* 全局统计信息 */
static struct {
    uint64_t start_time;
    size_t total_requests;
    size_t total_errors;
} g_stats = {0};

/* 全局服务器指针（用于健康检查） */
static uvhttp_server_t* g_health_server = NULL;

const char* uvhttp_health_status_string(uvhttp_health_status_t status) {
    switch (status) {
        case UVHTTP_HEALTH_STATUS_PASSING:
            return "passing";
        case UVHTTP_HEALTH_STATUS_WARNING:
            return "warning";
        case UVHTTP_HEALTH_STATUS_CRITICAL:
            return "critical";
        case UVHTTP_HEALTH_STATUS_UNKNOWN:
        default:
            return "unknown";
    }
}

int uvhttp_health_check(uvhttp_server_t* server, uvhttp_health_result_t* result) {
    if (!server || !result) {
        return -1;
    }

    memset(result, 0, sizeof(uvhttp_health_result_t));

    /* 计算运行时间 */
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    result->uptime_ms = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;

    /* 获取连接数 */
    result->active_connections = server->active_connections;

    /* 获取统计信息 */
    result->total_requests = g_stats.total_requests;
    result->total_errors = g_stats.total_errors;

    /* 判断健康状态 */
    if (result->total_errors > 0) {
        double error_rate = (double)result->total_errors / result->total_requests;
        if (error_rate > 0.1) {
            result->status = UVHTTP_HEALTH_STATUS_CRITICAL;
            snprintf(result->message, sizeof(result->message),
                     "Critical: High error rate (%.2f%%)", error_rate * 100);
        } else if (error_rate > 0.01) {
            result->status = UVHTTP_HEALTH_STATUS_WARNING;
            snprintf(result->message, sizeof(result->message),
                     "Warning: Elevated error rate (%.2f%%)", error_rate * 100);
        } else {
            result->status = UVHTTP_HEALTH_STATUS_PASSING;
            snprintf(result->message, sizeof(result->message), "OK");
        }
    } else {
        result->status = UVHTTP_HEALTH_STATUS_PASSING;
        snprintf(result->message, sizeof(result->message), "OK");
    }

    return 0;
}

int uvhttp_health_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    if (!response || !g_health_server) {
        return -1;
    }

    uvhttp_health_result_t result;

    /* 执行健康检查 */
    if (uvhttp_health_check(g_health_server, &result) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "application/json");
        const char* error_json = "{\"status\":\"error\",\"message\":\"Health check failed\"}";
        uvhttp_response_set_body(response, error_json, strlen(error_json));
        uvhttp_response_send(response);
        return -1;
    }

    /* 设置响应状态码 */
    int status_code = 200;
    if (result.status == UVHTTP_HEALTH_STATUS_WARNING) {
        status_code = 200; /* 警告也返回 200 */
    } else if (result.status == UVHTTP_HEALTH_STATUS_CRITICAL) {
        status_code = 503; /* 严重返回 503 */
    }
    uvhttp_response_set_status(response, status_code);

    /* 设置响应头 */
    uvhttp_response_set_header(response, "Content-Type", "application/json");

    /* 构建响应体 */
    char json[1024];
    int len = snprintf(json, sizeof(json),
        "{\"status\":\"%s\",\"message\":\"%s\",\"uptime_ms\":%llu,\"active_connections\":%zu,\"total_requests\":%zu,\"total_errors\":%zu}",
        uvhttp_health_status_string(result.status),
        result.message,
        (unsigned long long)result.uptime_ms,
        result.active_connections,
        result.total_requests,
        result.total_errors);

    if (len > 0 && len < (int)sizeof(json)) {
        uvhttp_response_set_body(response, json, len);
    }

    uvhttp_response_send(response);
    return 0;
}

int uvhttp_server_enable_health_check(uvhttp_server_t* server, const char* path) {
    if (!server) {
        return -1;
    }

    /* 保存服务器指针 */
    g_health_server = server;

    /* 初始化统计信息 */
    if (g_stats.start_time == 0) {
        struct timespec ts;
        clock_gettime(CLOCK_MONOTONIC, &ts);
        g_stats.start_time = (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    }

    /* 配置健康检查 */
    g_health_config.enabled = 1;
    if (path && strlen(path) > 0) {
        strncpy(g_health_config.path, path, sizeof(g_health_config.path) - 1);
        g_health_config.path[sizeof(g_health_config.path) - 1] = '\0';
    }

    /* 注意：路由添加由用户手动完成，这里只配置健康检查 */
    /* 用户需要调用：uvhttp_router_add_route(router, "/health", uvhttp_health_handler); */

    return 0;
}

int uvhttp_server_disable_health_check(uvhttp_server_t* server) {
    (void)server;
    g_health_config.enabled = 0;
    g_health_server = NULL;
    return 0;
}

/* 更新统计信息的辅助函数 */
void uvhttp_health_record_request(int is_error) {
    g_stats.total_requests++;
    if (is_error) {
        g_stats.total_errors++;
    }
}