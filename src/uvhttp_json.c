#include "uvhttp_json.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>

// 使用cjson库实现JSON功能

// ===== 公共API - 基于cJSON =====

json_value_t* json_parse(const char* json_string) {
    if (!json_string) return NULL;
    
    cJSON* cjson_obj = cJSON_Parse(json_string);
    return (json_value_t*)cjson_obj;
}

void json_value_free(json_value_t* value) {
    if (!value) return;
    cJSON_Delete((cJSON*)value);
}

void json_object_free(json_object_t* obj) {
    if (!obj) return;
    cJSON_Delete((cJSON*)obj);
}

void json_array_free(json_array_t* arr) {
    if (!arr) return;
    cJSON_Delete((cJSON*)arr);
}

// ===== 访问API =====

json_type_t json_get_type(const json_value_t* value) {
    if (!value) return JSON_NULL;
    
    cJSON* item = (cJSON*)value;
    switch (item->type) {
        case cJSON_False: return JSON_BOOL;
        case cJSON_True: return JSON_BOOL;
        case cJSON_NULL: return JSON_NULL;
        case cJSON_Number: return JSON_NUMBER;
        case cJSON_String: return JSON_STRING;
        case cJSON_Array: return JSON_ARRAY;
        case cJSON_Object: return JSON_OBJECT;
        default: return JSON_NULL;
    }
}

int json_get_bool(const json_value_t* value) {
    if (!value) return 0;
    cJSON* item = (cJSON*)value;
    return cJSON_IsBool(item) ? cJSON_IsTrue(item) : 0;
}

double json_get_number(const json_value_t* value) {
    if (!value) return 0.0;
    cJSON* item = (cJSON*)value;
    return cJSON_IsNumber(item) ? cJSON_GetNumberValue(item) : 0.0;
}

const char* json_get_string(const json_value_t* value) {
    if (!value) return NULL;
    cJSON* item = (cJSON*)value;
    return cJSON_IsString(item) ? cJSON_GetStringValue(item) : NULL;
}

json_array_t* json_get_array(const json_value_t* value) {
    if (!value) return NULL;
    cJSON* item = (cJSON*)value;
    return cJSON_IsArray(item) ? (json_array_t*)item : NULL;
}

json_object_t* json_get_object(const json_value_t* value) {
    if (!value) return NULL;
    cJSON* item = (cJSON*)value;
    return cJSON_IsObject(item) ? (json_object_t*)item : NULL;
}

json_value_t* json_object_get(const json_object_t* obj, const char* key) {
    if (!obj || !key) return NULL;
    cJSON* item = (cJSON*)obj;
    return (json_value_t*)cJSON_GetObjectItemCaseSensitive(item, key);
}

size_t json_object_size(const json_object_t* obj) {
    if (!obj) return 0;
    cJSON* item = (cJSON*)obj;
    return cJSON_GetArraySize(item);
}

json_value_t* json_array_get(const json_array_t* arr, size_t index) {
    if (!arr) return NULL;
    cJSON* item = (cJSON*)arr;
    return (json_value_t*)cJSON_GetArrayItem(item, (int)index);
}

size_t json_array_size(const json_array_t* arr) {
    if (!arr) return 0;
    cJSON* item = (cJSON*)arr;
    return cJSON_GetArraySize(item);
}

// ===== 创建和构建API - 基于cJSON =====

json_value_t* json_create_null(void) {
    return (json_value_t*)cJSON_CreateNull();
}

json_value_t* json_create_bool(int value) {
    return (json_value_t*)cJSON_CreateBool(value);
}

json_value_t* json_create_number(double value) {
    return (json_value_t*)cJSON_CreateNumber(value);
}

json_value_t* json_create_string(const char* value) {
    if (!value) return NULL;
    return (json_value_t*)cJSON_CreateString(value);
}

json_array_t* json_create_array(void) {
    return (json_array_t*)cJSON_CreateArray();
}

json_object_t* json_create_object(void) {
    return (json_object_t*)cJSON_CreateObject();
}

int json_array_add(json_array_t* arr, json_value_t* value) {
    if (!arr || !value) return -1;
    cJSON* array = (cJSON*)arr;
    cJSON* item = (cJSON*)value;
    
    if (!cJSON_AddItemToArray(array, item)) {
        return -1;
    }
    return 0;
}

int json_object_add(json_object_t* obj, const char* key, json_value_t* value) {
    if (!obj || !key || !value) return -1;
    cJSON* object = (cJSON*)obj;
    cJSON* item = (cJSON*)value;
    
    if (!cJSON_AddItemToObject(object, key, item)) {
        return -1;
    }
    return 0;
}

// ===== 序列化API =====

char* json_stringify(const json_value_t* value) {
    if (!value) return NULL;
    cJSON* item = (cJSON*)value;
    return cJSON_Print(item);
}

char* json_stringify_pretty(const json_value_t* value) {
    if (!value) return NULL;
    cJSON* item = (cJSON*)value;
    return cJSON_PrintUnformatted(item);
}