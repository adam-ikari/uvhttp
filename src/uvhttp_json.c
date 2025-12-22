#include "uvhttp_json.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../../deps/cjson/cJSON.h"

// ===== 公共API - 基于cJSON =====

uvhttp_json_t* uvhttp_json_parse(const char* json_string, int* error) {
    if (!json_string) {
        if (error) *error = -1;
        return NULL;
    }
    
    cJSON* cjson_obj = cJSON_Parse(json_string);
    if (!cjson_obj) {
        if (error) *error = -1;
        return NULL;
    }
    
    uvhttp_json_t* json = uvhttp_malloc(sizeof(uvhttp_json_t));
    if (!json) {
        cJSON_Delete(cjson_obj);
        if (error) *error = -2;
        return NULL;
    }
    
    // 根据cJSON类型设置uvhttp_json_t
    switch (cjson_obj->type) {
        case cJSON_NULL:
            json->type = UVHTTP_JSON_NULL;
            break;
        case cJSON_True:
        case cJSON_False:
            json->type = UVHTTP_JSON_BOOL;
            json->data.bool_val = cJSON_IsTrue(cjson_obj);
            break;
        case cJSON_Number:
            json->type = UVHTTP_JSON_NUMBER;
            json->data.number_val = cJSON_GetNumberValue(cjson_obj);
            break;
        case cJSON_String:
            json->type = UVHTTP_JSON_STRING;
            json->data.string_val = strdup(cJSON_GetStringValue(cjson_obj));
            break;
        case cJSON_Array:
            json->type = UVHTTP_JSON_ARRAY;
            json->data.array_val = cjson_obj;
            break;
        case cJSON_Object:
            json->type = UVHTTP_JSON_OBJECT;
            json->data.object_val = cjson_obj;
            break;
        default:
            json->type = UVHTTP_JSON_NULL;
            break;
    }
    
    if (error) *error = 0;
    return json;
}

uvhttp_json_t* uvhttp_json_parse_file(const char* filename, int* error) {
    if (!filename) {
        if (error) *error = -1;
        return NULL;
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        if (error) *error = -1;
        return NULL;
    }
    
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* buffer = uvhttp_malloc(size + 1);
    if (!buffer) {
        fclose(file);
        if (error) *error = -2;
        return NULL;
    }
    
    size_t bytes_read = fread(buffer, 1, (size_t)size, file);
    if (bytes_read < (size_t)size) {
        uvhttp_free(buffer);
        fclose(file);
        return NULL;
    }
    buffer[size] = '\0';
    fclose(file);
    
    uvhttp_json_t* result = uvhttp_json_parse(buffer, error);
    uvhttp_free(buffer);
    return result;
}

char* uvhttp_json_stringify(const uvhttp_json_t* json, int formatted) {
    if (!json) return NULL;
    
    cJSON* item = NULL;
    switch (json->type) {
        case UVHTTP_JSON_NULL:
            item = cJSON_CreateNull();
            break;
        case UVHTTP_JSON_BOOL:
            item = cJSON_CreateBool(json->data.bool_val);
            break;
        case UVHTTP_JSON_NUMBER:
            item = cJSON_CreateNumber(json->data.number_val);
            break;
        case UVHTTP_JSON_STRING:
            item = cJSON_CreateString(json->data.string_val);
            break;
        case UVHTTP_JSON_ARRAY:
            item = json->data.array_val;
            break;
        case UVHTTP_JSON_OBJECT:
            item = json->data.object_val;
            break;
    }
    
    if (!item) return NULL;
    
    char* result = formatted ? cJSON_Print(item) : cJSON_PrintUnformatted(item);
    
    // 如果是我们创建的item，需要删除
    if (json->type != UVHTTP_JSON_ARRAY && json->type != UVHTTP_JSON_OBJECT) {
        cJSON_Delete(item);
    }
    
    return result;
}

char* uvhttp_json_stringify_pretty(const uvhttp_json_t* json) {
    return uvhttp_json_stringify(json, 1);
}

uvhttp_json_t* uvhttp_json_get_object_item(const uvhttp_json_t* json, const char* key) {
    if (!json || json->type != UVHTTP_JSON_OBJECT || !key) return NULL;
    
    cJSON* item = cJSON_GetObjectItem(json->data.object_val, key);
    if (!item) return NULL;
    
    // 将cJSON转换为uvhttp_json_t
    int error;
    char* str = cJSON_PrintUnformatted(item);
    if (!str) return NULL;
    
    uvhttp_json_t* result = uvhttp_json_parse(str, &error);
    free(str);
    return result;
}

uvhttp_json_t* uvhttp_json_get_array_item(const uvhttp_json_t* json, size_t index) {
    if (!json || json->type != UVHTTP_JSON_ARRAY) return NULL;
    
    cJSON* item = cJSON_GetArrayItem(json->data.array_val, index);
    if (!item) return NULL;
    
    // 将cJSON转换为uvhttp_json_t
    int error;
    char* str = cJSON_PrintUnformatted(item);
    if (!str) return NULL;
    
    uvhttp_json_t* result = uvhttp_json_parse(str, &error);
    free(str);
    return result;
}

size_t uvhttp_json_get_array_size(const uvhttp_json_t* json) {
    if (!json || json->type != UVHTTP_JSON_ARRAY) return 0;
    return cJSON_GetArraySize(json->data.array_val);
}

size_t uvhttp_json_get_object_size(const uvhttp_json_t* json) {
    if (!json || json->type != UVHTTP_JSON_OBJECT) return 0;
    return cJSON_GetArraySize(json->data.object_val);
}

int uvhttp_json_get_bool(const uvhttp_json_t* json, int default_val) {
    if (!json || json->type != UVHTTP_JSON_BOOL) return default_val;
    return json->data.bool_val;
}

double uvhttp_json_get_number(const uvhttp_json_t* json, double default_val) {
    if (!json || json->type != UVHTTP_JSON_NUMBER) return default_val;
    return json->data.number_val;
}

const char* uvhttp_json_get_string(const uvhttp_json_t* json, const char* default_val) {
    if (!json || json->type != UVHTTP_JSON_STRING) return default_val;
    return json->data.string_val;
}

uvhttp_json_t* uvhttp_json_create_null(void) {
    uvhttp_json_t* json = uvhttp_malloc(sizeof(uvhttp_json_t));
    if (!json) return NULL;
    
    json->type = UVHTTP_JSON_NULL;
    return json;
}

uvhttp_json_t* uvhttp_json_create_bool(int value) {
    uvhttp_json_t* json = uvhttp_malloc(sizeof(uvhttp_json_t));
    if (!json) return NULL;
    
    json->type = UVHTTP_JSON_BOOL;
    json->data.bool_val = value;
    return json;
}

uvhttp_json_t* uvhttp_json_create_number(double value) {
    uvhttp_json_t* json = uvhttp_malloc(sizeof(uvhttp_json_t));
    if (!json) return NULL;
    
    json->type = UVHTTP_JSON_NUMBER;
    json->data.number_val = value;
    return json;
}

uvhttp_json_t* uvhttp_json_create_string(const char* string) {
    if (!string) return NULL;
    
    uvhttp_json_t* json = uvhttp_malloc(sizeof(uvhttp_json_t));
    if (!json) return NULL;
    
    json->type = UVHTTP_JSON_STRING;
    json->data.string_val = strdup(string);
    return json;
}

void uvhttp_json_free(uvhttp_json_t* json) {
    if (!json) return;
    
    if (json->type == UVHTTP_JSON_STRING && json->data.string_val) {
        free(json->data.string_val);
    } else if (json->type == UVHTTP_JSON_ARRAY && json->data.array_val) {
        cJSON_Delete(json->data.array_val);
    } else if (json->type == UVHTTP_JSON_OBJECT && json->data.object_val) {
        cJSON_Delete(json->data.object_val);
    }
    
    uvhttp_free(json);
}