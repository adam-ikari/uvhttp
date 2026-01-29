/* UVHTTP - 平台兼容性头文件 */

#ifndef UVHTTP_PLATFORM_H
#define UVHTTP_PLATFORM_H

/* 平台检测 - 只支持 Linux */
#define UVHTTP_PLATFORM_LINUX

/* 必须先包含 stdint.h 才能使用 UINTPTR_MAX */
#include <stdint.h>

/* 指针大小检测 */
#if UINTPTR_MAX == 0xFFFFFFFF
#    define UVHTTP_32BIT 1
#    define UVHTTP_POINTER_SIZE 4
#    define UVHTTP_SIZE_T_SIZE 4
#    define UVHTTP_ALIGNMENT 4
#elif UINTPTR_MAX == 0xFFFFFFFFFFFFFFFF
#    define UVHTTP_64BIT 1
#    define UVHTTP_POINTER_SIZE 8
#    define UVHTTP_SIZE_T_SIZE 8
#    define UVHTTP_ALIGNMENT 8
#else
#    error "Unsupported pointer size"
#endif

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

/* 32位系统兼容性宏 */
#ifdef UVHTTP_32BIT
/* 在32位系统上，size_t是4字节，但uint64_t仍然是8字节 */
/* 需要确保结构体对齐正确，避免性能下降 */
#    define UVHTTP_POINTER_ALIGNMENT 4
#    define UVHTTP_SIZE_T_ALIGNMENT 4
#    define UVHTTP_UINT64_ALIGNMENT 4 /* 32位系统上uint64_t自然对齐为4字节（但大小仍为8字节） */
#else
/* 64位系统 */
#    define UVHTTP_POINTER_ALIGNMENT 8
#    define UVHTTP_SIZE_T_ALIGNMENT 8
#    define UVHTTP_UINT64_ALIGNMENT 8
#endif

/* 对齐验证宏 - 根据平台自适应 */
#define UVHTTP_CHECK_ALIGNMENT(type, member, expected_alignment)           \
    UVHTTP_STATIC_ASSERT(offsetof(type, member) % expected_alignment == 0, \
                         #type "." #member " not " #expected_alignment     \
                               "-byte aligned")

#endif /* UVHTTP_PLATFORM_H */