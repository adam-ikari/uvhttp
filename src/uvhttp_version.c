/**
 * @file uvhttp_version.c
 * @brief UVHTTP version and build configuration query API implementation
 */

#include "uvhttp_version.h"
#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_utils.h"

#include <stdio.h>
#include <string.h>

/* Version macros from CMake */
#ifndef UVHTTP_VERSION_STRING
#define UVHTTP_VERSION_STRING "2.3.0"
#endif

#ifndef UVHTTP_VERSION_MAJOR
#define UVHTTP_VERSION_MAJOR 2
#endif

#ifndef UVHTTP_VERSION_MINOR
#define UVHTTP_VERSION_MINOR 3
#endif

#ifndef UVHTTP_VERSION_PATCH
#define UVHTTP_VERSION_PATCH 0
#endif

#ifndef UVHTTP_VERSION_INT
#define UVHTTP_VERSION_INT ((UVHTTP_VERSION_MAJOR * 10000) + (UVHTTP_VERSION_MINOR * 100) + UVHTTP_VERSION_PATCH)
#endif

/* Default values for configuration constants */
#ifndef UVHTTP_MAX_CONNECTIONS
#define UVHTTP_MAX_CONNECTIONS 1000
#endif

#ifndef UVHTTP_BUFFER_SIZE
#define UVHTTP_BUFFER_SIZE 8192
#endif

#ifndef UVHTTP_ROUTER_HASH_BASE_SIZE
#define UVHTTP_ROUTER_HASH_BASE_SIZE 256
#endif

#ifndef UVHTTP_ROUTER_HOT_MIN_SIZE
#define UVHTTP_ROUTER_HOT_MIN_SIZE 16
#endif

#ifndef UVHTTP_ROUTER_HYBRID_THRESHOLD
#define UVHTTP_ROUTER_HYBRID_THRESHOLD 100
#endif

#ifndef UVHTTP_LRU_CACHE_SIZE
#define UVHTTP_LRU_CACHE_SIZE 1000
#endif

#ifndef UVHTTP_LRU_CACHE_MAX_MEMORY
#define UVHTTP_LRU_CACHE_MAX_MEMORY 1048576
#endif

/* ========== Version Information ========== */

const char* uvhttp_get_version_string(void) {
    return UVHTTP_VERSION_STRING;
}

void uvhttp_get_version(int* major, int* minor, int* patch) {
    if (major) {
        *major = UVHTTP_VERSION_MAJOR;
    }
    if (minor) {
        *minor = UVHTTP_VERSION_MINOR;
    }
    if (patch) {
        *patch = UVHTTP_VERSION_PATCH;
    }
}

int uvhttp_get_version_int(void) {
    return UVHTTP_VERSION_INT;
}

/* ========== Build Information ========== */

uvhttp_error_t uvhttp_get_build_info(uvhttp_build_info_t* info) {
    if (!info) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(info, 0, sizeof(uvhttp_build_info_t));

    /* Version information */
    info->version_string = UVHTTP_VERSION_STRING;
    info->version_major = UVHTTP_VERSION_MAJOR;
    info->version_minor = UVHTTP_VERSION_MINOR;
    info->version_patch = UVHTTP_VERSION_PATCH;
    info->version_int = UVHTTP_VERSION_INT;

    /* Build information */
#ifdef NDEBUG
    info->build_type = "Release";
#else
    info->build_type = "Debug";
#endif
    info->build_date = __DATE__;
    info->build_time = __TIME__;

#if defined(__clang__)
    info->compiler = "Clang " __clang_version__;
#elif defined(__GNUC__)
    info->compiler = "GCC " __VERSION__;
#elif defined(_MSC_VER)
    info->compiler = "MSVC";
#else
    info->compiler = "Unknown";
#endif

#if defined(__linux__)
    info->platform = "Linux";
#elif defined(__APPLE__)
    info->platform = "macOS";
#elif defined(_WIN32)
    info->platform = "Windows";
#else
    info->platform = "Unknown";
#endif

/* Feature flags */
#ifdef UVHTTP_WEBSOCKET_ENABLED
    info->feature_websocket = 1;
#endif
#ifdef UVHTTP_STATIC_FILES_ENABLED
    info->feature_static_files = 1;
#endif
#ifdef UVHTTP_TLS_ENABLED
    info->feature_tls = 1;
#endif
#ifdef UVHTTP_MIDDLEWARE_ENABLED
    info->feature_middleware = 1;
#endif
#ifdef UVHTTP_LOGGING_ENABLED
    info->feature_logging = 1;
#endif
#ifdef UVHTTP_ROUTER_CACHE_ENABLED
    info->feature_router_cache = 1;
#endif
#ifdef UVHTTP_LRU_CACHE_ENABLED
    info->feature_lru_cache = 1;
#endif
#ifdef UVHTTP_RATE_LIMIT_ENABLED
    info->feature_rate_limit = 1;
#endif
    info->feature_protocol_upgrade = 1;

    /* Allocator configuration */
    info->allocator_type = uvhttp_allocator_name();

    /* Memory configuration */
    info->max_connections = UVHTTP_MAX_CONNECTIONS;
    info->max_headers = UVHTTP_MAX_HEADERS;
    info->max_body_size = UVHTTP_MAX_BODY_SIZE;
    info->buffer_size = UVHTTP_BUFFER_SIZE;

    /* Router configuration */
#ifdef UVHTTP_ROUTER_CACHE_ENABLED
    info->router_hash_size = UVHTTP_ROUTER_HASH_BASE_SIZE;
    info->router_hot_cache_size = UVHTTP_ROUTER_HOT_MIN_SIZE;
    info->router_hybrid_threshold = UVHTTP_ROUTER_HYBRID_THRESHOLD;
#endif

    /* Cache configuration */
#ifdef UVHTTP_LRU_CACHE_ENABLED
    info->lru_cache_size = UVHTTP_LRU_CACHE_SIZE;
    info->lru_cache_max_memory = UVHTTP_LRU_CACHE_MAX_MEMORY;
#endif

    /* TLS configuration */
#ifdef UVHTTP_TLS_ENABLED
    info->tls_enabled = 1;
    info->tls_version = "TLS 1.3";
#endif

    return UVHTTP_OK;
}

/* ========== Feature Checking ========== */

int uvhttp_is_feature_enabled(const char* feature) {
    if (!feature) {
        return -1;
    }

    if (strcmp(feature, "websocket") == 0) {
#ifdef UVHTTP_WEBSOCKET_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "static_files") == 0) {
#ifdef UVHTTP_STATIC_FILES_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "tls") == 0) {
#ifdef UVHTTP_TLS_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "middleware") == 0) {
#ifdef UVHTTP_MIDDLEWARE_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "logging") == 0) {
#ifdef UVHTTP_LOGGING_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "router_cache") == 0) {
#ifdef UVHTTP_ROUTER_CACHE_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "lru_cache") == 0) {
#ifdef UVHTTP_LRU_CACHE_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "rate_limit") == 0) {
#ifdef UVHTTP_RATE_LIMIT_ENABLED
        return 1;
#else
        return 0;
#endif
    } else if (strcmp(feature, "protocol_upgrade") == 0) {
        return 1;
    } else if (strcmp(feature, "allocator") == 0) {
        return 1;
    }

    return -1;
}

/* ========== Helper Functions ========== */

const char* uvhttp_get_allocator_type(void) {
    return uvhttp_allocator_name();
}

const char* uvhttp_get_build_type(void) {
#ifdef NDEBUG
    return "Release";
#else
    return "Debug";
#endif
}

const char* uvhttp_get_compiler_info(void) {
#if defined(__clang__)
    return "Clang " __clang_version__;
#elif defined(__GNUC__)
    return "GCC " __VERSION__;
#elif defined(_MSC_VER)
    return "MSVC";
#else
    return "Unknown";
#endif
}

const char* uvhttp_get_platform_info(void) {
#if defined(__linux__)
    return "Linux";
#elif defined(__APPLE__)
    return "macOS";
#elif defined(_WIN32)
    return "Windows";
#else
    return "Unknown";
#endif
}

/* ========== Print Functions ========== */

void uvhttp_print_build_info(void) {
    uvhttp_build_info_t info;
    if (uvhttp_get_build_info(&info) != UVHTTP_OK) {
        printf("Failed to get build information\n");
        return;
    }

    printf("========================================\n");
    printf("       UVHTTP Build Information          \n");
    printf("========================================\n");
    printf("\n");
    printf("Version:\n");
    printf("  String: %s\n", info.version_string);
    printf("  Major:  %d\n", info.version_major);
    printf("  Minor:  %d\n", info.version_minor);
    printf("  Patch:  %d\n", info.version_patch);
    printf("  Int:    %d\n", info.version_int);
    printf("\n");
    printf("Build:\n");
    printf("  Type:    %s\n", info.build_type);
    printf("  Date:    %s\n", info.build_date);
    printf("  Time:    %s\n", info.build_time);
    printf("  Compiler: %s\n", info.compiler);
    printf("  Platform: %s\n", info.platform);
    printf("\n");
    printf("Features:\n");
    printf("  WebSocket:       %s\n", info.feature_websocket ? "Enabled" : "Disabled");
    printf("  Static Files:    %s\n", info.feature_static_files ? "Enabled" : "Disabled");
    printf("  TLS/SSL:         %s\n", info.feature_tls ? "Enabled" : "Disabled");
    printf("  Middleware:      %s\n", info.feature_middleware ? "Enabled" : "Disabled");
    printf("  Logging:         %s\n", info.feature_logging ? "Enabled" : "Disabled");
    printf("  Router Cache:    %s\n", info.feature_router_cache ? "Enabled" : "Disabled");
    printf("  LRU Cache:       %s\n", info.feature_lru_cache ? "Enabled" : "Disabled");
    printf("  Rate Limit:      %s\n", info.feature_rate_limit ? "Enabled" : "Disabled");
    printf("  Protocol Upgrade: %s\n", info.feature_protocol_upgrade ? "Enabled" : "Disabled");
    printf("\n");
    printf("Allocator:\n");
    printf("  Type: %s\n", info.allocator_type);
    printf("\n");
    printf("Memory Configuration:\n");
    printf("  Max Connections: %d\n", info.max_connections);
    printf("  Max Headers:     %d\n", info.max_headers);
    printf("  Max Body Size:   %d\n", info.max_body_size);
    printf("  Buffer Size:     %d\n", info.buffer_size);
    printf("\n");
    printf("Router Configuration:\n");
    printf("  Hash Size:       %d\n", info.router_hash_size);
    printf("  Hot Cache Size:  %d\n", info.router_hot_cache_size);
    printf("  Hybrid Threshold: %d\n", info.router_hybrid_threshold);
    printf("\n");
    printf("Cache Configuration:\n");
    printf("  LRU Cache Size:  %d\n", info.lru_cache_size);
    printf("  Max Memory:      %d\n", info.lru_cache_max_memory);
    printf("\n");
    printf("TLS Configuration:\n");
    printf("  Enabled:         %s\n", info.tls_enabled ? "Yes" : "No");
    printf("  Version:         %s\n", info.tls_version ? info.tls_version : "N/A");
    printf("\n");
    printf("========================================\n");
}
