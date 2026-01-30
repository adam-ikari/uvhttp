#include "uvhttp_utils.h"

#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_constants.h"
#include "uvhttp_response.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <time.h>

// Safe string copy function - matches header declaration
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src) {
    if (!dest || !src || dest_size == 0)
        return -1;

    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        src_len = dest_size - 1;
    }
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';

    return 0;
}

/* ============ Core Utility Functions ============ */

// Safe string copy function
int uvhttp_safe_strncpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0)
        return -1;

    size_t src_len = strlen(src);
    if (src_len >= dest_size) {
        src_len = dest_size - 1;
    }
    memcpy(dest, src, src_len);
    dest[src_len] = '\0';

    return 0;  // Return 0 on success, -1 on failure
}

/* ============ Internal Helper Functions ============ */

// Internal helper function: validate status code validity
static int is_valid_status_code(int code) {
    return (code >= 100 && code <= 599) ? TRUE : FALSE;
}

// Internal helper function: validate string length
static int is_valid_string_length(const char* str, size_t max_len) {
    if (!str)
        return FALSE;
    return (strlen(str) <= max_len) ? TRUE : FALSE;
}

/* ============ Unified Response Handling Functions ============ */

/**
 * @brief Unified response sending function - caller sets Content-Type
 * @param response Response object
 * @param content Content data
 * @param length Content length (auto-calculated if 0)
 * @param status_code HTTP status code (uses existing code if 0)
 * @return UVHTTP_OK on success, error code otherwise
 */
uvhttp_error_t uvhttp_send_unified_response(uvhttp_response_t* response,
                                            const char* content, size_t length,
                                            int status_code) {
    // Parameter validation
    if (!response || !content) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // Validate status code (if provided)
    if (status_code != 0 && !is_valid_status_code(status_code)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // Handle length parameter
    if (length == 0) {
        length = strlen(content);
    }

    // Validate content length
    if (length == 0 || length > UVHTTP_MAX_BODY_SIZE) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // Set status code (if valid code provided)
    if (status_code != 0) {
        uvhttp_response_set_status(response, status_code);
    }

    // Set response body
    uvhttp_error_t err = uvhttp_response_set_body(response, content, length);
    if (err != UVHTTP_OK) {
        return err;
    }

    // Send response
    return uvhttp_response_send(response);
}

/**
 * @brief Create standard error response (JSON format)
 * @param response Response object
 * @param error_code Error code
 * @param error_message Error message
 * @param details Detailed information (optional)
 * @return UVHTTP_OK on success, error code otherwise
 */
uvhttp_error_t uvhttp_send_error_response(uvhttp_response_t* response,
                                          int error_code,
                                          const char* error_message,
                                          const char* details) {
    // Parameter validation
    if (!response || !error_message) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // Validate error code range
    if (!is_valid_status_code(error_code)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

// Validate string length, prevent buffer overflow
#define MAX_ERROR_MSG_LEN 200
#define MAX_ERROR_DETAILS_LEN 400

    if (!is_valid_string_length(error_message, MAX_ERROR_MSG_LEN)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (details && !is_valid_string_length(details, MAX_ERROR_DETAILS_LEN)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // Create safe JSON error response
    char error_json[1024];
    int json_len;

    if (details && strlen(details) > 0) {
        json_len = snprintf(error_json, sizeof(error_json),
                            "{\"error\":\"%s\",\"details\":\"%s\",\"code\":%d,"
                            "\"timestamp\":%ld}",
                            error_message, details, error_code, time(NULL));
    } else {
        json_len = snprintf(error_json, sizeof(error_json),
                            "{\"error\":\"%s\",\"code\":%d,\"timestamp\":%ld}",
                            error_message, error_code, time(NULL));
    }

    // Validate snprintf success
    if (json_len < 0 || json_len >= (int)sizeof(error_json)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_response_set_status(response, error_code);
    uvhttp_response_set_header(response, "Content-Type",
                               "application/json; charset=utf-8");
    uvhttp_response_set_body(response, error_json, strlen(error_json));
    return uvhttp_response_send(response);
}

/* ============ Public Validation Functions ============ */

/**
 * @brief Validate HTTP status code
 * @param status_code HTTP status code
 * @return TRUE if valid, FALSE otherwise
 */
int uvhttp_is_valid_status_code(int status_code) {
    return (status_code >= 100 && status_code <= 599) ? TRUE : FALSE;
}

/* uvhttp_is_valid_content_type deleted - completely unused */
/* uvhttp_is_valid_string_length deleted - completely unused */

/* Simplified IP validation function */
int uvhttp_is_valid_ip_address(const char* ip) {
    if (!ip || !*ip)
        return FALSE;
    /* Simplified validation: only check basic format */
    if (strchr(ip, ':') != NULL) {
        /* IPv6 - simplified check */
        int colon_count = 0;
        for (const char* p = ip; *p; p++) {
            if (*p == ':')
                colon_count++;
        }
        return colon_count >= 2 ? TRUE : FALSE;
    }
    /* IPv4 - simplified check */
    int dot_count = 0;
    for (const char* p = ip; *p; p++) {
        if (*p == '.')
            dot_count++;
    }
    return dot_count == 3 ? TRUE : FALSE;
}

/**
 * @brief Validate IP address format (IPv4 or IPv6)
 * @param ip IP address string
 * @return TRUE if valid, FALSE otherwise
 */
/* uvhttp_is_valid_ip_address deleted - completely unused */
