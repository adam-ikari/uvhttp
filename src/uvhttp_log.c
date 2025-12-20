#include "uvhttp_log.h"
#include <stdarg.h>
#include <time.h>

static const char* level_strings[] = {
    "ERROR", "WARN", "INFO", "DEBUG"
};

void uvhttp_log(uvhttp_log_level_t level, const char* format, ...) {
    if (level > UVHTTP_LOG_LEVEL) {
        return;
    }
    
    /* 获取当前时间 */
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    
    /* 格式化时间戳 */
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);
    
    /* 输出日志 */
    fprintf(stderr, "[%s] ", timestamp);
    
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}