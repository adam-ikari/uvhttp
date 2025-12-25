/**
 * @file uvhttp_json_v2.h
 * @brief UVHTTP JSON 处理库 V2 - 完善的序列化/反序列化方案
 * 
 * 特性：
 * - 链式构建器 API
 * - 类型安全的反序列化
 * - 路径表达式支持
 * - 高性能内存管理
 * - 完善的错误处理
 */

#ifndef UVHTTP_JSON_V2_H
#define UVHTTP_JSON_V2_H

#include "uvhttp_features.h"
#include <stdint.h>
#include <stdbool.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== 错误处理 ========== */

typedef enum {
    UVJSON_OK = 0,
    UVJSON_ERROR_INVALID_JSON = -1,
    UVJSON_ERROR_INVALID_PATH = -2,
    UVJSON_ERROR_TYPE_MISMATCH = -3,
    UVJSON_ERROR_OUT_OF_MEMORY = -4,
    UVJSON_ERROR_PARSE_ERROR = -5,
    UVJSON_ERROR_INVALID_VALUE = -6
} uvjson_error_t;

typedef struct {
    uvjson_error_t code;
    char message[256];
    char path[128];
    int line;
    int column;
    const char* function;
} uvjson_error_context_t;

/* ========== 类型系统 ========== */

typedef enum {
    UVJSON_NULL = 0,
    UVJSON_BOOL,
    UVJSON_INT,
    UVJSON_DOUBLE,
    UVJSON_STRING,
    UVJSON_ARRAY,
    UVJSON_OBJECT,
    UVJSON_BINARY,
    UVJSON_DATETIME,
    UVJSON_UUID
} uvjson_type_t;

typedef struct uvjson_value uvjson_value_t;
typedef struct uvjson_array uvjson_array_t;
typedef struct uvjson_object uvjson_object_t;

/* ========== 高级 API - 链式构建器 ========== */

typedef struct uvjson_builder uvjson_builder_t;

// 创建构建器
uvjson_builder_t* uvjson_builder_create(void);
uvjson_builder_t* uvjson_builder_create_object(void);
uvjson_builder_t* uvjson_builder_create_array(void);

// 对象属性设置
uvjson_builder_t* uvjson_builder_set_null(uvjson_builder_t* builder, const char* key);
uvjson_builder_t* uvjson_builder_set_bool(uvjson_builder_t* builder, const char* key, bool value);
uvjson_builder_t* uvjson_builder_set_int(uvjson_builder_t* builder, const char* key, int64_t value);
uvjson_builder_t* uvjson_builder_set_double(uvjson_builder_t* builder, const char* key, double value);
uvjson_builder_t* uvjson_builder_set_string(uvjson_builder_t* builder, const char* key, const char* value);
uvjson_builder_t* uvjson_builder_set_binary(uvjson_builder_t* builder, const char* key, const void* data, size_t len);
uvjson_builder_t* uvjson_builder_set_datetime(uvjson_builder_t* builder, const char* key, time_t value);
uvjson_builder_t* uvjson_builder_set_uuid(uvjson_builder_t* builder, const char* key, uint32_t high, uint32_t low);

// 数组元素添加
uvjson_builder_t* uvjson_builder_add_null(uvjson_builder_t* builder);
uvjson_builder_t* uvjson_builder_add_bool(uvjson_builder_t* builder, bool value);
uvjson_builder_t* uvjson_builder_add_int(uvjson_builder_t* builder, int64_t value);
uvjson_builder_t* uvjson_builder_add_double(uvjson_builder_t* builder, double value);
uvjson_builder_t* uvjson_builder_add_string(uvjson_builder_t* builder, const char* value);
uvjson_builder_t* uvjson_builder_add_binary(uvjson_builder_t* builder, const void* data, size_t len);
uvjson_builder_t* uvjson_builder_add_datetime(uvjson_builder_t* builder, time_t value);
uvjson_builder_t* uvjson_builder_add_uuid(uvjson_builder_t* builder, uint32_t high, uint32_t low);

// 嵌套结构
uvjson_builder_t* uvjson_builder_begin_object(uvjson_builder_t* builder, const char* key);
uvjson_builder_t* uvjson_builder_begin_array(uvjson_builder_t* builder, const char* key);
uvjson_builder_t* uvjson_builder_end(uvjson_builder_t* builder);

// 条件添加
uvjson_builder_t* uvjson_builder_set_if(uvjson_builder_t* builder, bool condition, 
                                       const char* key, const char* value);
uvjson_builder_t* uvjson_builder_add_if(uvjson_builder_t* builder, bool condition, const char* value);

// 序列化
char* uvjson_builder_stringify(uvjson_builder_t* builder);
char* uvjson_builder_stringify_pretty(uvjson_builder_t* builder);
uvjson_value_t* uvjson_builder_build(uvjson_builder_t* builder);

// 清理
void uvjson_builder_free(uvjson_builder_t* builder);

/* ========== 模板函数 ========== */

// HTTP 响应模板
char* uvjson_build_response(int status, const char* message, ...);
char* uvjson_build_error(const char* error, const char* details, ...);
char* uvjson_build_success(const char* message, ...);

// 数据结构模板
char* uvjson_build_pagination(int page, int limit, int total, uvjson_builder_t* data);
char* uvjson_build_list_result(int count, uvjson_builder_t* items);

/* ========== 反序列化 API ========== */

// 解析
uvjson_value_t* uvjson_parse(const char* json_string);
uvjson_value_t* uvjson_parse_with_options(const char* json_string, int options);

// 路径表达式
typedef struct uvjson_path uvjson_path_t;

uvjson_path_t* uvjson_path_create(const char* path_expression);
uvjson_path_t* uvjson_path_create_formatted(const char* format, ...);
void uvjson_path_free(uvjson_path_t* path);

// 类型安全的获取
bool uvjson_get_bool(uvjson_value_t* json, const char* path, bool default_val);
int64_t uvjson_get_int(uvjson_value_t* json, const char* path, int64_t default_val);
double uvjson_get_double(uvjson_value_t* json, const char* path, double default_val);
char* uvjson_get_string(uvjson_value_t* json, const char* path, const char* default_val);
time_t uvjson_get_datetime(uvjson_value_t* json, const char* path, time_t default_val);

// 路径表达式获取
uvjson_value_t* uvjson_get_value(uvjson_value_t* root, uvjson_path_t* path);
uvjson_value_t* uvjson_get_value_by_string(uvjson_value_t* root, const char* path);

// 批量提取
typedef struct {
    const char* path;
    uvjson_type_t expected_type;
    void* target;
    bool found;
    bool optional;
} uvjson_field_t;

int uvjson_extract_fields(uvjson_value_t* json, uvjson_field_t* fields, size_t count);

// 类型检查
uvjson_type_t uvjson_get_type(uvjson_value_t* value);
bool uvjson_is_type(uvjson_value_t* value, uvjson_type_t expected_type);
const char* uvjson_get_type_name(uvjson_type_t type);

/* ========== 低级 API ========== */

// 值操作
uvjson_value_t* uvjson_value_create(uvjson_type_t type);
void uvjson_value_free(uvjson_value_t* value);
uvjson_value_t* uvjson_value_ref(uvjson_value_t* value);
void uvjson_value_unref(uvjson_value_t* value);

// 对象操作
uvjson_object_t* uvjson_object_create(void);
size_t uvjson_object_size(uvjson_object_t* object);
uvjson_value_t* uvjson_object_get(uvjson_object_t* object, const char* key);
int uvjson_object_set(uvjson_object_t* object, const char* key, uvjson_value_t* value);
int uvjson_object_remove(uvjson_object_t* object, const char* key);
bool uvjson_object_has(uvjson_object_t* object, const char* key);

// 数组操作
uvjson_array_t* uvjson_array_create(void);
size_t uvjson_array_size(uvjson_array_t* array);
uvjson_value_t* uvjson_array_get(uvjson_array_t* array, size_t index);
int uvjson_array_append(uvjson_array_t* array, uvjson_value_t* value);
int uvjson_array_insert(uvjson_array_t* array, size_t index, uvjson_value_t* value);
int uvjson_array_remove(uvjson_array_t* array, size_t index);
void uvjson_array_clear(uvjson_array_t* array);

/* ========== 内存管理 ========== */

// 内存池
typedef struct uvjson_pool uvjson_pool_t;

uvjson_pool_t* uvjson_pool_create(size_t initial_size);
void uvjson_pool_destroy(uvjson_pool_t* pool);
void* uvjson_pool_alloc(uvjson_pool_t* pool, size_t size);
void uvjson_pool_reset(uvjson_pool_t* pool);

// 零拷贝字符串视图
typedef struct {
    const char* data;
    size_t len;
} uvjson_string_view_t;

uvjson_value_t* uvjson_parse_view(uvjson_string_view_t view);

/* ========== 流式解析 ========== */

typedef struct uvjson_parser uvjson_parser_t;

uvjson_parser_t* uvjson_parser_create(void);
uvjson_error_t uvjson_parser_feed(uvjson_parser_t* parser, const char* data, size_t len);
uvjson_value_t* uvjson_parser_finish(uvjson_parser_t* parser);
void uvjson_parser_destroy(uvjson_parser_t* parser);

/* ========== 错误处理 ========== */

uvjson_error_t uvjson_get_last_error(void);
const uvjson_error_context_t* uvjson_get_error_context(void);
void uvjson_clear_error(void);
void uvjson_set_error_handler(void (*handler)(const uvjson_error_context_t* ctx));

/* ========== 工具函数 ========== */

// 验证
bool uvjson_is_valid(const char* json_string);
bool uvjson_is_valid_view(uvjson_string_view_t view);

// 格式化
char* uvjson_minify(const char* json_string);
char* uvjson_prettify(const char* json_string);

// 比较
bool uvjson_equals(uvjson_value_t* a, uvjson_value_t* b);
int uvjson_compare(uvjson_value_t* a, uvjson_value_t* b);

// 合并
uvjson_value_t* uvjson_merge(uvjson_value_t* target, uvjson_value_t* source);
uvjson_value_t* uvjson_deep_copy(uvjson_value_t* value);

/* ========== HTTP 集成 ========== */

typedef struct {
    uvhttp_response_t* response;
    uvjson_builder_t* builder;
} uvhttp_json_response_t;

uvhttp_json_response_t* uvhttp_json_response_create(uvhttp_response_t* response);
uvhttp_result_t uvhttp_json_response_send(uvhttp_json_response_t* json_resp);
void uvhttp_json_response_free(uvhttp_json_response_t* json_resp);

/* ========== 宏定义 ========== */

// 便捷宏
#define UVJSON_BOOL(b) uvjson_builder_set_bool(builder, #b, (b))
#define UVJSON_INT(i) uvjson_builder_set_int(builder, #i, (i))
#define UVJSON_DOUBLE(d) uvjson_builder_set_double(builder, #d, (d))
#define UVJSON_STRING(s) uvjson_builder_set_string(builder, #s, (s))

// 条件宏
#define UVJSON_SET_IF(cond, key, value) \
    do { if (cond) uvjson_builder_set_string(builder, key, value); } while(0)

// 字段提取宏
#define UVJSON_FIELD(path, type, target) {path, type, &(target), false, false}
#define UVJSON_FIELD_OPT(path, type, target) {path, type, &(target), false, true}

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_JSON_V2_H */