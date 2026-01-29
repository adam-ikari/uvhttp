/* UVHTTP - 平台兼容性头文件 */

#ifndef UVHTTP_PLATFORM_H
#define UVHTTP_PLATFORM_H

/* 平台检测 - 只支持 Linux */
#define UVHTTP_PLATFORM_LINUX

/* 头文件包含 */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/* 类型定义 */
typedef socklen_t uvhttp_socklen_t;

/* 宏定义 */
#define UVHTTP_EXPORT __attribute__((visibility("default")))
#define UVHTTP_IMPORT
#define UVHTTP_API __attribute__((visibility("default")))

/* 函数宏 */
#define uvhttp_close_socket close

/* 错误处理 */
#define UVHTTP_SOCKET_ERROR(e) (e < 0)
#define UVHTTP_SOCKET_LAST_ERROR() errno
#define UVHTTP_SOCKET_WOULD_BLOCK(e) (e == EAGAIN || e == EWOULDBLOCK)

/* 时间函数 */
#include <unistd.h>
#define uvhttp_sleep_ms(ms) usleep((ms)*1000)

#endif /* UVHTTP_PLATFORM_H */