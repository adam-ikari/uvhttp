#ifndef UVHTTP_COMMON_H
#define UVHTTP_COMMON_H

#include <assert.h>
#include <stddef.h>

/* Include constant definitions */
#include "uvhttp_constants.h"

/* ========== Static Assertion Macro Definitions ========== */
#ifdef __cplusplus
#    define UVHTTP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#    define UVHTTP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* ========== HTTP Header Constants ========== */
/* Buffer size for header names and values (including null terminator) */
/* Note: Optimized for better performance with mimalloc allocator */
#ifndef UVHTTP_HEADER_NAME_BUFFER_SIZE
#    define UVHTTP_HEADER_NAME_BUFFER_SIZE 256
#endif

#ifndef UVHTTP_HEADER_VALUE_BUFFER_SIZE
#    define UVHTTP_HEADER_VALUE_BUFFER_SIZE 2048
#endif

/* Maximum number of headers limit */
#ifndef MAX_HEADERS
#    define MAX_HEADERS UVHTTP_MAX_HEADERS
#endif

#define MAX_HEADER_NAME_LEN (UVHTTP_HEADER_NAME_BUFFER_SIZE - 1)
#define MAX_HEADER_VALUE_LEN (UVHTTP_HEADER_VALUE_BUFFER_SIZE - 1)

#ifdef __cplusplus
extern "C" {
#endif

/* Increase buffer size to prevent overflow, complying with HTTP specification
 */
typedef struct {
    char name[UVHTTP_HEADER_NAME_BUFFER_SIZE];
    char value[UVHTTP_HEADER_VALUE_BUFFER_SIZE];
} uvhttp_header_t;

/* Safe string copy function */
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src);

/* Note: Validation functions have been moved to uvhttp_validation.h */
/* Please use #include "uvhttp_validation.h" to access validation functions */

/* Forward declarations */
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;

/* Request handler type */
typedef int (*uvhttp_request_handler_t)(uvhttp_request_t* request,
                                        uvhttp_response_t* response);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_COMMON_H */