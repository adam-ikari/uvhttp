/* UVHTTP Dependency Injection and Context Management */

#ifndef UVHTTP_CONTEXT_H
#define UVHTTP_CONTEXT_H

#include "uvhttp_config.h"
#include "uvhttp_error_handler.h"
#if UVHTTP_FEATURE_LOGGING
#    include "uvhttp_logging.h"
#endif
#include <time.h>
#include <uv.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declarations */
struct uvhttp_server;
struct uvhttp_router;

/* ============ Memory Allocator Explanation ============ */
/*
 * UVHTTP Memory allocationwhen, zero
 *
 * Userunwheninterface, :
 * 1. Performance priority: Function
 * 2. whenoptimization: andoptimization
 * 3. simple: reduce, raisemaintain
 *
 * Memory allocationclass UVHTTP_ALLOCATOR_TYPE Compile macro:
 * - 0: SystemDefault (malloc/free)
 * - 1: mimalloc
 * - 2: custom (external)
 *
 * Use:
 *   #include "uvhttp_allocator.h"
 *   void* ptr = UVHTTP_MALLOC(size);
 *   UVHTTP_FREE(ptr);
 *
 *  uvhttp_allocator.h description
 */

/* ============ Main Context Structure ============ */

typedef struct uvhttp_context {
    /* Core components */
    uv_loop_t* loop;
    struct uvhttp_server* server;
    struct uvhttp_router* router;

    /* Context state */
    int initialized;
    time_t created_at;

    /* Statistics */
    uint64_t total_requests;
    uint64_t total_connections;
    uint64_t active_connections;

    /* ===== Global Variable Replacement Fields ===== */

    /* TLS module state */
    int tls_initialized;
    void* tls_entropy; /* mbedtls_entropy_context* */
    void* tls_drbg;    /* mbedtls_ctr_drbg_context* */

    /* WebSocket module state */
    int ws_drbg_initialized;
    void* ws_entropy; /* mbedtls_entropy_context* */
    void* ws_drbg;    /* mbedtls_ctr_drbg_context* */

    /* Configuration management */
    void* current_config;  /* uvhttp_config_t* */
    void* config_callback; /* uvhttp_config_change_callback_t */

    /* User data (for storing application-specific context) */
    void* user_data;

} uvhttp_context_t;

/* ============ Context Management Functions ============ */

/* Create new context */
uvhttp_error_t uvhttp_context_create(uv_loop_t* loop,
                                     uvhttp_context_t** context);

/* Destroy context */
void uvhttp_context_destroy(uvhttp_context_t* context);

/* Initialize context (set default providers) */
uvhttp_error_t uvhttp_context_init(uvhttp_context_t* context);

/* ===== Global Variable Replacement Field Initialization Functions ===== */

/* Initialize TLS module state */
uvhttp_error_t uvhttp_context_init_tls(uvhttp_context_t* context);

/* Cleanup TLS module state */
void uvhttp_context_cleanup_tls(uvhttp_context_t* context);

/* Initialize WebSocket module state */
uvhttp_error_t uvhttp_context_init_websocket(uvhttp_context_t* context);

/*  WebSocket blockstate */
void uvhttp_context_cleanup_websocket(uvhttp_context_t* context);

/* Configuration management */
uvhttp_error_t uvhttp_context_init_config(uvhttp_context_t* context);

/* Configuration management */
void uvhttp_context_cleanup_config(uvhttp_context_t* context);

/* ============ Default ============ */
/* Note: Memory allocationUsewhen, create
 * SystemDefault: Use malloc/free
 * mimalloc: when mimalloc
 * custom: whenUser
 */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONTEXT_H */