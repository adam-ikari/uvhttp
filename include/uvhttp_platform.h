/* UVHTTP - 平台兼容性头文件 */

#ifndef UVHTTP_PLATFORM_H
#define UVHTTP_PLATFORM_H

/* 平台检测 - 只支持 Linux */
#define UVHTTP_PLATFORM_LINUX

/* 必须先包含 stdint.h 才能使用 UINTPTR_MAX */
#include <stdint.h>

/* io_uring 支持检测 */
#ifdef __has_include
#    if __has_include(<liburing.h>)
#        define UVHTTP_HAS_IO_URING 1
#        include <liburing.h>
#    else
#        define UVHTTP_HAS_IO_URING 0
#    endif
#else
#    define UVHTTP_HAS_IO_URING 0
#endif

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
#    define UVHTTP_UINT64_ALIGNMENT \
        4 /* 32位系统上uint64_t自然对齐为4字节（但大小仍为8字节） */
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

/* ========== 缓存行填充宏定义 ========== */

/* 缓存行大小（现代 CPU 通常是 64 字节） */
#ifndef UVHTTP_CACHE_LINE_SIZE
#    define UVHTTP_CACHE_LINE_SIZE 64
#endif

/* 缓存行对齐宏 - 用于结构体字段对齐 */
#define UVHTTP_CACHE_LINE_ALIGNED \
    __attribute__((aligned(UVHTTP_CACHE_LINE_SIZE)))

/* 缓存行填充宏 - 防止伪共享（False Sharing） */
#if defined(__GNUC__) || defined(__clang__)
#    define UVHTTP_CACHE_LINE_PAD char _pad[UVHTTP_CACHE_LINE_SIZE]
#else
#    define UVHTTP_CACHE_LINE_PAD char _pad[UVHTTP_CACHE_LINE_SIZE]
#endif

/* 缓存行填充验证宏 - 确保结构体大小是缓存行的整数倍 */
#define UVHTTP_ASSERT_CACHE_LINE_ALIGNED(type)                       \
    UVHTTP_STATIC_ASSERT(sizeof(type) % UVHTTP_CACHE_LINE_SIZE == 0, \
                         #type " size is not cache line aligned")

/* 缓存行偏移验证宏 - 确保字段在缓存行边界 */
#define UVHTTP_ASSERT_CACHE_LINE_OFFSET(type, member)                          \
    UVHTTP_STATIC_ASSERT(offsetof(type, member) % UVHTTP_CACHE_LINE_SIZE == 0, \
                         #type "." #member " not cache line aligned")

/* ========== io_uring 配置 ========== */

#if UVHTTP_HAS_IO_URING
/* io_uring 队列大小 */
#    ifndef UVHTTP_IOURING_QUEUE_SIZE
#        define UVHTTP_IOURING_QUEUE_SIZE 256
#    endif

/* io_uring 缓冲区数量 */
#    ifndef UVHTTP_IOURING_BUFFER_COUNT
#        define UVHTTP_IOURING_BUFFER_COUNT 128
#    endif

/* io_uring 缓冲区大小 */
#    ifndef UVHTTP_IOURING_BUFFER_SIZE
#        define UVHTTP_IOURING_BUFFER_SIZE (64 * 1024) /* 64KB */
#    endif
#endif

#endif /* UVHTTP_PLATFORM_H */