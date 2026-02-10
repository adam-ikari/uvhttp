/*
 * UVHTTP log
 * simpleLog output,  Debug modeEnable
 */

#ifndef UVHTTP_LOGGING_H
#define UVHTTP_LOGGING_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Log levels */
typedef enum {
    UVHTTP_LOG_LEVEL_TRACE = 0,
    UVHTTP_LOG_LEVEL_DEBUG = 1,
    UVHTTP_LOG_LEVEL_INFO = 2,
    UVHTTP_LOG_LEVEL_WARN = 3,
    UVHTTP_LOG_LEVEL_ERROR = 4,
    UVHTTP_LOG_LEVEL_FATAL = 5
} uvhttp_log_level_t;

/* Enable logging in Debug mode */
#if defined(NDEBUG) || !UVHTTP_FEATURE_LOGGING

/* In Release mode or when logging is disabled, log macros are no-ops */
#    define UVHTTP_LOG(level, ...) ((void)0)
#    define UVHTTP_LOG_TRACE(...) ((void)0)
#    define UVHTTP_LOG_DEBUG(...) ((void)0)
#    define UVHTTP_LOG_INFO(...) ((void)0)
#    define UVHTTP_LOG_WARN(...) ((void)0)
#    define UVHTTP_LOG_ERROR(...) ((void)0)
#    define UVHTTP_LOG_FATAL(...) ((void)0)

#else /* Debug modelogfunctionEnable */

/* In Debug mode, log output goes to stderr */
#    define UVHTTP_LOG(level, ...)                                    \
        do {                                                          \
            const char* level_str = "UNKNOWN";                        \
            switch (level) {                                          \
            case UVHTTP_LOG_LEVEL_TRACE:                              \
                level_str = "TRACE";                                  \
                break;                                                \
            case UVHTTP_LOG_LEVEL_DEBUG:                              \
                level_str = "DEBUG";                                  \
                break;                                                \
            case UVHTTP_LOG_LEVEL_INFO:                               \
                level_str = "INFO";                                   \
                break;                                                \
            case UVHTTP_LOG_LEVEL_WARN:                               \
                level_str = "WARN";                                   \
                break;                                                \
            case UVHTTP_LOG_LEVEL_ERROR:                              \
                level_str = "ERROR";                                  \
                break;                                                \
            case UVHTTP_LOG_LEVEL_FATAL:                              \
                level_str = "FATAL";                                  \
                break;                                                \
            }                                                         \
            fprintf(stderr, "[%s] %s:%d %s(): ", level_str, __FILE__, \
                    __LINE__, __func__);                              \
            fprintf(stderr, __VA_ARGS__);                             \
            fprintf(stderr, "\n");                                    \
            fflush(stderr);                                           \
        } while (0)

#    define UVHTTP_LOG_TRACE(...) \
        UVHTTP_LOG(UVHTTP_LOG_LEVEL_TRACE, __VA_ARGS__)
#    define UVHTTP_LOG_DEBUG(...) \
        UVHTTP_LOG(UVHTTP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#    define UVHTTP_LOG_INFO(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_INFO, __VA_ARGS__)
#    define UVHTTP_LOG_WARN(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_WARN, __VA_ARGS__)
#    define UVHTTP_LOG_ERROR(...) \
        UVHTTP_LOG(UVHTTP_LOG_LEVEL_ERROR, __VA_ARGS__)
#    define UVHTTP_LOG_FATAL(...) \
        UVHTTP_LOG(UVHTTP_LOG_LEVEL_FATAL, __VA_ARGS__)

#endif /* NDEBUG || !UVHTTP_FEATURE_LOGGING */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_LOGGING_H */