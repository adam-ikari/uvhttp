#ifndef UVHTTP_COMMON_H
#define UVHTTP_COMMON_H

#include <stddef.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 静态断言宏定义 ========== */
#ifdef __cplusplus
#define UVHTTP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#define UVHTTP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

// 增加缓冲区大小以防止溢出，符合HTTP规范
typedef struct {
    char name[256];    // 增加到256字节
    char value[4096];  // 增加到4096字节以支持长值
} uvhttp_header_t;

// 安全的字符串复制函数
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);

// 注意：验证函数已移动到 uvhttp_validation.h
// 请使用 #include "uvhttp_validation.h" 来访问验证函数

// header最大数量限制
// 优化：从 64 减少到 32，实际应用中很少超过 20 个头部
// 每个连接节省约 137KB 内存 (32 * 4352 = 139,264 字节)
#define MAX_HEADERS 32
#define MAX_HEADER_NAME_LEN 255
#define MAX_HEADER_VALUE_LEN 4095

// 前向声明
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 请求处理器类型
typedef int (*uvhttp_request_handler_t)(uvhttp_request_t* request, uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif