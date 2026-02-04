/**
 * @file uvhttp_version.h
 * @brief UVHTTP version and build configuration query API
 *
 * This module provides APIs to query version information and compile-time
 * configuration of the UVHTTP library.
 */

#ifndef UVHTTP_VERSION_H
#define UVHTTP_VERSION_H

#include "uvhttp_common.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Build configuration information structure
 *
 * Contains all compile-time configuration options and their current status
 */
typedef struct {
    /* Version information */
    const char* version_string;       /**< Version string (e.g., "2.3.0") */
    int version_major;                /**< Major version number */
    int version_minor;                /**< Minor version number */
    int version_patch;                /**< Patch version number */
    int version_int;                  /**< Version as integer (e.g., 20300 for 2.3.0) */

    /* Build information */
    const char* build_type;           /**< Build type (Debug/Release) */
    const char* build_date;           /**< Build date (__DATE__) */
    const char* build_time;           /**< Build time (__TIME__) */
    const char* compiler;             /**< Compiler information */
    const char* platform;             /**< Platform information */

    /* Feature flags */
    int feature_websocket;            /**< WebSocket support enabled */
    int feature_static_files;         /**< Static file support enabled */
    int feature_tls;                  /**< TLS/SSL support enabled */
    int feature_middleware;           /**< Middleware support enabled */
    int feature_logging;              /**< Logging support enabled */
    int feature_router_cache;         /**< Router cache enabled */
    int feature_lru_cache;            /**< LRU cache enabled */
    int feature_cors;                 /**< CORS support enabled */
    int feature_rate_limit;           /**< Rate limit enabled */
    int feature_protocol_upgrade;     /**< Protocol upgrade enabled */

    /* Allocator configuration */
    const char* allocator_type;       /**< Allocator type (system/mimalloc) */
    int allocator_enabled;            /**< Custom allocator enabled */

    /* Memory configuration */
    int max_connections;              /**< Maximum connections */
    int max_headers;                  /**< Maximum headers per request */
    int max_body_size;                /**< Maximum body size */
    int buffer_size;                  /**< Default buffer size */

    /* Router configuration */
    int router_hash_size;             /**< Router hash table size */
    int router_hot_cache_size;        /**< Router hot cache size */
    int router_hybrid_threshold;      /**< Router hybrid threshold */

    /* Cache configuration */
    int lru_cache_size;               /**< LRU cache size (entries) */
    int lru_cache_max_memory;         /**< LRU cache max memory (bytes) */

    /* TLS configuration */
    int tls_enabled;                  /**< TLS enabled */
    const char* tls_version;          /**< TLS version */
} uvhttp_build_info_t;

/**
 * @brief Get UVHTTP version string
 *
 * @return const char* Version string (e.g., "2.3.0")
 *
 * @note This is the same as UVHTTP_VERSION_STRING macro
 * @note The returned string is statically allocated, do not free it
 */
const char* uvhttp_get_version_string(void);

/**
 * @brief Get UVHTTP version components
 *
 * @param major Pointer to store major version (can be NULL)
 * @param minor Pointer to store minor version (can be NULL)
 * @param patch Pointer to store patch version (can be NULL)
 *
 * @return void
 *
 * @note Any NULL pointer will be ignored
 */
void uvhttp_get_version(int* major, int* minor, int* patch);

/**
 * @brief Get UVHTTP version as integer
 *
 * @return int Version as integer (e.g., 20300 for 2.3.0)
 *
 * @note Calculated as: major * 10000 + minor * 100 + patch
 */
int uvhttp_get_version_int(void);

/**
 * @brief Get complete build information
 *
 * @param info Pointer to uvhttp_build_info_t structure to fill (can be NULL)
 *
 * @return uvhttp_error_t UVHTTP_OK on success, error code on failure
 *
 * @note If info is NULL, returns UVHTTP_ERROR_INVALID_PARAM
 * @note The returned structure contains pointers to statically allocated strings,
 *       do not free them
 * @note This function is thread-safe
 */
uvhttp_error_t uvhttp_get_build_info(uvhttp_build_info_t* info);

/**
 * @brief Check if a feature is enabled
 *
 * @param feature Feature name to check
 *
 * @return int 1 if enabled, 0 if disabled, -1 if feature name is invalid
 *
 * @note Supported feature names:
 *       - "websocket": WebSocket support
 *       - "static_files": Static file support
 *       - "tls": TLS/SSL support
 *       - "middleware": Middleware support
 *       - "logging": Logging support
 *       - "router_cache": Router cache
 *       - "lru_cache": LRU cache
 *       - "cors": CORS support
 *       - "rate_limit": Rate limit
 *       - "protocol_upgrade": Protocol upgrade
 *       - "allocator": Custom allocator
 */
int uvhttp_is_feature_enabled(const char* feature);

/**
 * @brief Get allocator type name
 *
 * @return const char* Allocator type name ("system" or "mimalloc")
 *
 * @note The returned string is statically allocated, do not free it
 */
const char* uvhttp_get_allocator_type(void);

/**
 * @brief Get build type
 *
 * @return const char* Build type ("Debug" or "Release")
 *
 * @note The returned string is statically allocated, do not free it
 */
const char* uvhttp_get_build_type(void);

/**
 * @brief Get compiler information
 *
 * @return const char* Compiler information string
 *
 * @note Format: "compiler_name version"
 * @note The returned string is statically allocated, do not free it
 */
const char* uvhttp_get_compiler_info(void);

/**
 * @brief Get platform information
 *
 * @return const char* Platform information string
 *
 * @note Format: "os_name architecture"
 * @note The returned string is statically allocated, do not free it
 */
const char* uvhttp_get_platform_info(void);

/**
 * @brief Print build information to stdout
 *
 * @return void
 *
 * @note Useful for debugging and diagnostics
 * @note Output format is human-readable
 */
void uvhttp_print_build_info(void);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_VERSION_H */