#ifndef UVHTTP_LOG_H
#define UVHTTP_LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 日志级别 */
typedef enum {
    UVHTTP_LOG_ERROR = 0,
    UVHTTP_LOG_WARN  = 1,
    UVHTTP_LOG_INFO  = 2,
    UVHTTP_LOG_DEBUG = 3
} uvhttp_log_level_t;

/* 日志宏定义 */
#ifndef UVHTTP_LOG_LEVEL
#define UVHTTP_LOG_LEVEL UVHTTP_LOG_INFO
#endif

/* 日志输出函数 */
void uvhttp_log(uvhttp_log_level_t level, const char* format, ...);

/* 日志宏 */
#if UVHTTP_LOG_LEVEL >= UVHTTP_LOG_ERROR
#define UVHTTP_LOG_ERROR(fmt, ...) uvhttp_log(UVHTTP_LOG_ERROR, "[ERROR] " fmt "\n", ##__VA_ARGS__)
#else
#define UVHTTP_LOG_ERROR(fmt, ...)
#endif

#if UVHTTP_LOG_LEVEL >= UVHTTP_LOG_WARN
#define UVHTTP_LOG_WARN(fmt, ...) uvhttp_log(UVHTTP_LOG_WARN, "[WARN] " fmt "\n", ##__VA_ARGS__)
#else
#define UVHTTP_LOG_WARN(fmt, ...)
#endif

#if UVHTTP_LOG_LEVEL >= UVHTTP_LOG_INFO
#define UVHTTP_LOG_INFO(fmt, ...) uvhttp_log(UVHTTP_LOG_INFO, "[INFO] " fmt "\n", ##__VA_ARGS__)
#else
#define UVHTTP_LOG_INFO(fmt, ...)
#endif

#if UVHTTP_LOG_LEVEL >= UVHTTP_LOG_DEBUG
#define UVHTTP_LOG_DEBUG(fmt, ...) uvhttp_log(UVHTTP_LOG_DEBUG, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define UVHTTP_LOG_DEBUG(fmt, ...)
#endif

/* 便捷日志宏 */
#define UVHTTP_LOG_INIT() UVHTTP_LOG_INFO("UVHTTP服务器初始化")
#define UVHTTP_LOG_START(port) UVHTTP_LOG_INFO("服务器启动，监听端口 %d", port)
#define UVHTTP_LOG_STOP() UVHTTP_LOG_INFO("服务器停止")
#define UVHTTP_LOG_CONN_ACCEPT() UVHTTP_LOG_DEBUG("接受新连接")
#define UVHTTP_LOG_CONN_CLOSE() UVHTTP_LOG_DEBUG("连接关闭")
#define UVHTTP_LOG_ALLOC_FAIL() UVHTTP_LOG_ERROR("内存分配失败")
#define UVHTTP_LOG_TLS_FAIL(msg) UVHTTP_LOG_ERROR("TLS错误: %s", msg)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_LOG_H */