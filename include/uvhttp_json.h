/**
 * @file uvhttp_json.h
 * @brief JSON 处理头文件
 * 
 * 基于 cjson 库提供 JSON 解析和生成功能，支持：
 * - JSON 对象解析和序列化
 * - 动态内存管理
 * - 错误处理
 */

#ifndef UVHTTP_JSON_H
#define UVHTTP_JSON_H

#include "uvhttp_features.h"

#ifdef UVHTTP_JSON_ENABLED

#include "../../deps/cjson/cJSON.h"
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* JSON 值类型 */
typedef enum {
    UVHTTP_JSON_NULL,
    UVHTTP_JSON_BOOL,
    UVHTTP_JSON_NUMBER,
    UVHTTP_JSON_STRING,
    UVHTTP_JSON_ARRAY,
    UVHTTP_JSON_OBJECT
} uvhttp_json_type_t;

/* JSON 值结构 */
typedef struct {
    uvhttp_json_type_t type;
    union {
        int bool_val;
        double number_val;
        char* string_val;
        cJSON* array_val;
        cJSON* object_val;
    } data;
} uvhttp_json_t;

/* JSON 解析函数 */
uvhttp_json_t* uvhttp_json_parse(const char* json_string, int* error);
uvhttp_json_t* uvhttp_json_parse_file(const char* filename, int* error);

/* JSON 生成函数 */
char* uvhttp_json_stringify(const uvhttp_json_t* json, int formatted);
char* uvhttp_json_stringify_pretty(const uvhttp_json_t* json);

/* JSON 对象操作 */
uvhttp_json_t* uvhttp_json_get_object_item(const uvhttp_json_t* json, const char* key);
uvhttp_json_t* uvhttp_json_get_array_item(const uvhttp_json_t* json, size_t index);
size_t uvhttp_json_get_array_size(const uvhttp_json_t* json);
size_t uvhttp_json_get_object_size(const uvhttp_json_t* json);

/* JSON 原始值获取 */
int uvhttp_json_get_bool(const uvhttp_json_t* json, int default_val);
double uvhttp_json_get_number(const uvhttp_json_t* json, double default_val);
const char* uvhttp_json_get_string(const uvhttp_json_t* json, const char* default_val);

/* JSON 创建函数 */
uvhttp_json_t* uvhttp_json_create_null(void);
uvhttp_json_t* uvhttp_json_create_bool(int value);
uvhttp_json_t* uvhttp_json_create_number(double value);
uvhttp_json_t* uvhttp_json_create_string(const char* string);
uvhttp_json_t* uvhttp_json_create_array(void);
uvhttp_json_t* uvhttp_json_create_object(void);

/* JSON 数组操作 */
int uvhttp_json_array_add_item(uvhttp_json_t* array, uvhttp_json_t* item);
int uvhttp_json_array_add_bool(uvhttp_json_t* array, int value);
int uvhttp_json_array_add_number(uvhttp_json_t* array, double value);
int uvhttp_json_array_add_string(uvhttp_json_t* array, const char* string);

/* JSON 对象操作 */
int uvhttp_json_object_set_item(uvhttp_json_t* object, const char* key, uvhttp_json_t* value);
int uvhttp_json_object_set_bool(uvhttp_json_t* object, const char* key, int value);
int uvhttp_object_set_number(uvhttp_json_t* object, const char* key, double value);
int uvhttp_object_set_string(uvhttp_json_t* object, const char* key, const char* value);

/* JSON 内存管理 */
void uvhttp_json_free(uvhttp_json_t* json);

/* JSON 工具函数 */
int uvhttp_json_is_valid(const char* json_string);
int uvhttp_json_get_type(const uvhttp_json_t* json);
const char* uvhttp_json_get_type_string(uvhttp_json_type_t type);

/* HTTP 响应构建辅助函数 */
char* uvhttp_build_json_response(int status, const char* message, ...);
char* uvhttp_build_json_error(const char* error, ...);
char* uvhttp_build_json_success(const char* data, ...);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_JSON_ENABLED */

#endif /* UVHTTP_JSON_H */