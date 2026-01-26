/* UVHTTP - 平台兼容性头文件 */

#ifndef UVHTTP_PLATFORM_H
#define UVHTTP_PLATFORM_H

/* 平台检测 */
#ifdef _WIN32
    #define UVHTTP_PLATFORM_WINDOWS
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_MAC) && TARGET_OS_MAC
        #define UVHTTP_PLATFORM_MACOS
    #endif
#elif defined(__linux__)
    #define UVHTTP_PLATFORM_LINUX
#elif defined(__unix__)
    #define UVHTTP_PLATFORM_UNIX
#endif

/* 平台特定的头文件包含 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
#else
    #include <sys/socket.h>
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <netdb.h>
#endif

/* 平台特定的类型定义 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    typedef int uvhttp_socklen_t;
#else
    typedef socklen_t uvhttp_socklen_t;
#endif

/* 平台特定的宏 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #define UVHTTP_EXPORT __declspec(dllexport)
    #define UVHTTP_IMPORT __declspec(dllimport)
    #ifdef UVHTTP_BUILDING_DLL
        #define UVHTTP_API UVHTTP_EXPORT
    #else
        #define UVHTTP_API UVHTTP_IMPORT
    #endif
#else
    #define UVHTTP_EXPORT __attribute__((visibility("default")))
    #define UVHTTP_IMPORT
    #define UVHTTP_API __attribute__((visibility("default")))
#endif

/* 平台特定的函数 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #define uvhttp_close_socket closesocket
#else
    #define uvhttp_close_socket close
#endif

/* 平台特定的错误处理 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #define UVHTTP_SOCKET_ERROR(e) (e == SOCKET_ERROR)
    #define UVHTTP_SOCKET_LAST_ERROR() WSAGetLastError()
    #define UVHTTP_SOCKET_WOULD_BLOCK(e) (e == WSAEWOULDBLOCK)
#else
    #define UVHTTP_SOCKET_ERROR(e) (e < 0)
    #define UVHTTP_SOCKET_LAST_ERROR() errno
    #define UVHTTP_SOCKET_WOULD_BLOCK(e) (e == EAGAIN || e == EWOULDBLOCK)
#endif

/* 平台特定的时间函数 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #include <windows.h>
    #define uvhttp_sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define uvhttp_sleep_ms(ms) usleep((ms) * 1000)
#endif

#endif /* UVHTTP_PLATFORM_H */