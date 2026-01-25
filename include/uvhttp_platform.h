/* UVHTTP - 平台兼容性头文件 */

#ifndef UVHTTP_PLATFORM_H
#define UVHTTP_PLATFORM_H

/* 平台检测 */
#ifdef _WIN32
    #define UVHTTP_PLATFORM_WINDOWS
    #ifdef _WIN64
        #define UVHTTP_PLATFORM_WINDOWS_64
    #else
        #define UVHTTP_PLATFORM_WINDOWS_32
    #endif
#elif defined(__APPLE__)
    #include <TargetConditionals.h>
    #if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
        #define UVHTTP_PLATFORM_IOS
    #elif defined(TARGET_OS_MAC) && TARGET_OS_MAC
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

/* 平台特定的线程支持 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #include <windows.h>
    typedef HANDLE uvhttp_thread_t;
    typedef DWORD uvhttp_thread_id_t;
#else
    #include <pthread.h>
    typedef pthread_t uvhttp_thread_t;
    typedef pthread_t uvhttp_thread_id_t;
#endif

/* 平台特定的互斥锁支持 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    typedef CRITICAL_SECTION uvhttp_mutex_t;
#else
    typedef pthread_mutex_t uvhttp_mutex_t;
#endif

/* 平台特定的条件变量支持 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    typedef CONDITION_VARIABLE uvhttp_cond_t;
#else
    typedef pthread_cond_t uvhttp_cond_t;
#endif

/* 平台特定的原子操作支持 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #include <windows.h>
    #define uvhttp_atomic_fetch_add(ptr, val) InterlockedExchangeAdd((LONG volatile*)(ptr), (val))
    #define uvhttp_atomic_compare_exchange(ptr, expected, desired) InterlockedCompareExchange((LONG volatile*)(ptr), (desired), *(expected))
#else
    #include <stdatomic.h>
    #define uvhttp_atomic_fetch_add(ptr, val) atomic_fetch_add((atomic_int*)(ptr), (val))
    #define uvhttp_atomic_compare_exchange(ptr, expected, desired) atomic_compare_exchange_strong((atomic_int*)(ptr), (expected), (desired))
#endif

/* 平台特定的内存屏障 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #define uvhttp_memory_barrier() MemoryBarrier()
#else
    #define uvhttp_memory_barrier() __sync_synchronize()
#endif

/* 平台特定的时间函数 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #include <windows.h>
    #define uvhttp_sleep_ms(ms) Sleep(ms)
#else
    #include <unistd.h>
    #define uvhttp_sleep_ms(ms) usleep((ms) * 1000)
#endif

/* 平台特定的文件路径处理 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #define UVHTTP_PATH_SEPARATOR "\\"
    #define UVHTTP_PATH_SEPARATOR_CHAR '\\'
#else
    #define UVHTTP_PATH_SEPARATOR "/"
    #define UVHTTP_PATH_SEPARATOR_CHAR '/'
#endif

/* 平台特定的动态库加载 */
#ifdef UVHTTP_PLATFORM_WINDOWS
    #define UVHTTP_DLOPEN(lib) LoadLibraryA(lib)
    #define UVHTTP_DLSYM(handle, symbol) (void*)GetProcAddress(handle, symbol)
    #define UVHTTP_DLCLOSE(handle) FreeLibrary(handle)
#else
    #include <dlfcn.h>
    #define UVHTTP_DLOPEN(lib) dlopen(lib, RTLD_LAZY)
    #define UVHTTP_DLSYM(handle, symbol) dlsym(handle, symbol)
    #define UVHTTP_DLCLOSE(handle) dlclose(handle)
#endif

#endif /* UVHTTP_PLATFORM_H */