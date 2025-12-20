/* JSON响应模块 */

#ifndef UVHTTP_JSON_H
#define UVHTTP_JSON_H

#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

/* JSON响应函数 */
int uvhttp_response_set_json(uvhttp_response_t* response, const char* json_string);
int uvhttp_response_json_error(uvhttp_response_t* response, int status_code, const char* error_message);
int uvhttp_response_json_success(uvhttp_response_t* response, const char* message);
int uvhttp_response_json_simple(uvhttp_response_t* response, int status_code, 
                                const char* key, const char* value);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_JSON_H */