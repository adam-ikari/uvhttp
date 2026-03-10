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
#include <arpa/inet.h>

// Safe string copy function - uses snprintf for safety
int uvhttp_safe_strcpy(char* dest, size_t dest_size, const char* src) {
    if (!dest || !src || dest_size == 0)
        return -1;

    snprintf(dest, dest_size, "%s", src);
    return 0;
}

/* ============ Core Utility Functions ============ */

// Safe string copy function - uses snprintf for safety
int uvhttp_safe_strncpy(char* dest, const char* src, size_t dest_size) {
    if (!dest || !src || dest_size == 0)
        return -1;

    snprintf(dest, dest_size, "%s", src);
    return 0;
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

/* IP validation function - uses standard inet_pton for reliability */
int uvhttp_is_valid_ip_address(const char* ip) {
    if (!ip || !*ip)
        return FALSE;

    struct sockaddr_in sa4;
    struct sockaddr_in6 sa6;

    /* Try IPv4 */
    if (inet_pton(AF_INET, ip, &sa4.sin_addr) == 1) {
        return TRUE;
    }

    /* Try IPv6 */
    if (inet_pton(AF_INET6, ip, &sa6.sin6_addr) == 1) {
        return TRUE;
    }

    return FALSE;
}

/**
 * @brief Validate IP address format (IPv4 or IPv6)
 * @param ip IP address string
 * @return TRUE if valid, FALSE otherwise
 */
