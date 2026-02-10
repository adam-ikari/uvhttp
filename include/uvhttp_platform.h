/* UVHTTP - Platform Compatibility Header */

#ifndef UVHTTP_PLATFORM_H
#define UVHTTP_PLATFORM_H

/* Platform detection - Only Linux supported */
#define UVHTTP_PLATFORM_LINUX

/* Must include stdint.h before using UINTPTR_MAX */
#include <stdint.h>

/* Pointer size detection */
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

/* Header includes */
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

/* classdefine */
typedef socklen_t uvhttp_socklen_t;

/* Macro definition */
#define UVHTTP_EXPORT __attribute__((visibility("default")))
#define UVHTTP_IMPORT
#define UVHTTP_API __attribute__((visibility("default")))

/* Function */
#define uvhttp_close_socket close

/* Error handling */
#define UVHTTP_SOCKET_ERROR(e) (e < 0)
#define UVHTTP_SOCKET_LAST_ERROR() errno
#define UVHTTP_SOCKET_WOULD_BLOCK(e) (e == EAGAIN || e == EWOULDBLOCK)

/* whenFunction */
#include <unistd.h>
#define uvhttp_sleep_ms(ms) usleep((ms)*1000)

/* 32bitsSystem */
#ifdef UVHTTP_32BIT
/* 32bitsSystem, size_t4bytes, uint64_t8bytes */
/* needStructurepair,  */
#    define UVHTTP_POINTER_ALIGNMENT 4
#    define UVHTTP_SIZE_T_ALIGNMENT 4
#    define UVHTTP_UINT64_ALIGNMENT \
        4 /* 32bitsSystemuint64_tpairto4bytes(sizeto8bytes) */
#else
/* 64bitsSystem */
#    define UVHTTP_POINTER_ALIGNMENT 8
#    define UVHTTP_SIZE_T_ALIGNMENT 8
#    define UVHTTP_UINT64_ALIGNMENT 8
#endif

/* pairvalidate - rootPlatform */
#define UVHTTP_CHECK_ALIGNMENT(type, member, expected_alignment)           \
    UVHTTP_STATIC_ASSERT(offsetof(type, member) % expected_alignment == 0, \
                         #type "." #member " not " #expected_alignment     \
                               "-byte aligned")

/* ========== CacheMacro definition ========== */

/* Cachesize( CPU  64 bytes) */
#ifndef UVHTTP_CACHE_LINE_SIZE
#    define UVHTTP_CACHE_LINE_SIZE 64
#endif

/* Cachepair - Used forStructureFieldpair */
#define UVHTTP_CACHE_LINE_ALIGNED \
    __attribute__((aligned(UVHTTP_CACHE_LINE_SIZE)))

/* Cache - prevent(False Sharing) */
#if defined(__GNUC__) || defined(__clang__)
#    define UVHTTP_CACHE_LINE_PAD char _pad[UVHTTP_CACHE_LINE_SIZE]
#else
#    define UVHTTP_CACHE_LINE_PAD char _pad[UVHTTP_CACHE_LINE_SIZE]
#endif

/* Cachevalidate - StructuresizeCache */
#define UVHTTP_ASSERT_CACHE_LINE_ALIGNED(type)                       \
    UVHTTP_STATIC_ASSERT(sizeof(type) % UVHTTP_CACHE_LINE_SIZE == 0, \
                         #type " size is not cache line aligned")

/* Cacheoffsetvalidate - FieldCacheedge */
#define UVHTTP_ASSERT_CACHE_LINE_OFFSET(type, member)                          \
    UVHTTP_STATIC_ASSERT(offsetof(type, member) % UVHTTP_CACHE_LINE_SIZE == 0, \
                         #type "." #member " not cache line aligned")

#endif /* UVHTTP_PLATFORM_H */