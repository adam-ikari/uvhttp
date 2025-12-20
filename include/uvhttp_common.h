#ifndef UVHTTP_COMMON_H
#define UVHTTP_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

// 增加缓冲区大小以防止溢出，符合HTTP规范
typedef struct {
    char name[256];    // 增加到256字节
    char value[4096];  // 增加到4096字节以支持长值
} uvhttp_header_t;

// 安全的字符串复制函数
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);

// 验证header值是否有效
int uvhttp_validate_header_value(const char* name, const char* value);

// header最大数量限制
#define MAX_HEADERS 64
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