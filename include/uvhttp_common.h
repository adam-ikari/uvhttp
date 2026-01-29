#ifndef UVHTTP_COMMON_H
#define UVHTTP_COMMON_H

#include <assert.h>
#include <stddef.h>

/* 包含常量定义 */
#include "uvhttp_constants.h"

/* ========== 静态断言宏定义 ========== */
#ifdef __cplusplus
#    define UVHTTP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#    define UVHTTP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* ========== HTTP 头部常量 ========== */
// Header 名称和值的缓冲区大小（包括空终止符）
// 注意：这些常量已移至 uvhttp_constants.h 中定义
#ifndef UVHTTP_HEADER_NAME_BUFFER_SIZE
#    define UVHTTP_HEADER_NAME_BUFFER_SIZE 256
#endif

#ifndef UVHTTP_HEADER_VALUE_BUFFER_SIZE
#    define UVHTTP_HEADER_VALUE_BUFFER_SIZE 4096
#endif

// Header 最大数量限制
#ifndef MAX_HEADERS
#    define MAX_HEADERS UVHTTP_MAX_HEADERS
#endif

#define MAX_HEADER_NAME_LEN (UVHTTP_HEADER_NAME_BUFFER_SIZE - 1)
#define MAX_HEADER_VALUE_LEN (UVHTTP_HEADER_VALUE_BUFFER_SIZE - 1)

#ifdef __cplusplus
extern "C" {
#endif

// 增加缓冲区大小以防止溢出，符合HTTP规范
typedef struct {
    char name[UVHTTP_HEADER_NAME_BUFFER_SIZE];
    char value[UVHTTP_HEADER_VALUE_BUFFER_SIZE];
} uvhttp_header_t;

// 安全的字符串复制函数
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);

// 注意：验证函数已移动到 uvhttp_validation.h
// 请使用 #include "uvhttp_validation.h" 来访问验证函数

// 前向声明
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

// 请求处理器类型
typedef int (*uvhttp_request_handler_t)(uvhttp_request_t* request, uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif