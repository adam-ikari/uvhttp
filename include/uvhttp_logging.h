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

/* ============ Embedder logging interface ============
 *
 * UVHTTP declares this contract but does NOT implement it. An embedder
 * implements uvhttp_log_set_sink (and its own emit function) to route logs
 * into its application logging stack. This keeps log state out of the
 * library, per PHILOSOPHY §7 "No global variables".
 *
 * To route the library's UVHTTP_LOG_* sites into the embedder, define
 * UVHTTP_LOG_EMIT to the embedder's function before including this header:
 *
 *   #define UVHTTP_LOG_EMIT app_log_emit
 *   #include <uvhttp.h>
 */
typedef void (*uvhttp_log_sink_t)(uvhttp_log_level_t level, const char* file,
                                  int line, const char* func, const char* msg,
                                  void* user_data);

void uvhttp_log_set_sink(uvhttp_log_level_t min_level, uvhttp_log_sink_t sink,
                         void* user_data);

/* Default log emitter, used only when the embedder has not overridden it.
 * With logging compiled out it is a true no-op (zero cost). */
#ifndef UVHTTP_LOG_EMIT
#    if defined(NDEBUG) || !UVHTTP_FEATURE_LOGGING
#        define UVHTTP_LOG_EMIT(level, file, line, func, ...) ((void)0)
#    else
#        define UVHTTP_LOG_EMIT(level, file, line, func, ...)                 \
            do {                                                              \
                const char* level_str = "UNKNOWN";                            \
                switch (level) {                                              \
                case UVHTTP_LOG_LEVEL_TRACE: level_str = "TRACE"; break;      \
                case UVHTTP_LOG_LEVEL_DEBUG: level_str = "DEBUG"; break;      \
                case UVHTTP_LOG_LEVEL_INFO: level_str = "INFO"; break;        \
                case UVHTTP_LOG_LEVEL_WARN: level_str = "WARN"; break;        \
                case UVHTTP_LOG_LEVEL_ERROR: level_str = "ERROR"; break;      \
                case UVHTTP_LOG_LEVEL_FATAL: level_str = "FATAL"; break;      \
                }                                                             \
                fprintf(stderr, "[%s] %s:%d %s(): ", level_str, file, line,   \
                        func);                                                \
                fprintf(stderr, __VA_ARGS__);                                 \
                fprintf(stderr, "\n");                                        \
                fflush(stderr);                                               \
            } while (0)
#    endif
#endif

#define UVHTTP_LOG(level, ...) \
    UVHTTP_LOG_EMIT(level, __FILE__, __LINE__, __func__, __VA_ARGS__)
#define UVHTTP_LOG_TRACE(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_TRACE, __VA_ARGS__)
#define UVHTTP_LOG_DEBUG(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define UVHTTP_LOG_INFO(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_INFO, __VA_ARGS__)
#define UVHTTP_LOG_WARN(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_WARN, __VA_ARGS__)
#define UVHTTP_LOG_ERROR(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_ERROR, __VA_ARGS__)
#define UVHTTP_LOG_FATAL(...) UVHTTP_LOG(UVHTTP_LOG_LEVEL_FATAL, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_LOGGING_H */