/* UVHTTP Dependency Injection and Context Management Implementation */

#include "uvhttp_context.h"

#include "uvhttp_allocator.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ============ Memory Allocator Description ============ */
/*
 * UVHTTP memory allocator uses compile-time macro design, zero-overhead abstraction
 *
 * No runtime allocator provider implemented, reasons:
 * 1. Performance first: avoid function pointer call overhead
 * 2. Compile-time optimization: compiler can inline and optimize allocation calls
 * 3. Simple and direct: reduce complexity, improve maintainability
 *
 * Memory allocator type selected via UVHTTP_ALLOCATOR_TYPE compile macro:
 * - 0: System default allocator (malloc/free)
 * - 1: mimalloc high-performance allocator
 * - 2: Custom allocator (external link)
 *
 * Usage:
 *   #include "uvhttp_allocator.h"
 *   void* ptr = uvhttp_alloc(size);
 *   uvhttp_free(ptr);
 */

/* ============ Context Management Implementation ============ */

uvhttp_error_t uvhttp_context_create(uv_loop_t* loop,
                                     uvhttp_context_t** context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_context_t* ctx =
        (uvhttp_context_t*)uvhttp_alloc(sizeof(uvhttp_context_t));
    if (!ctx) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(ctx, 0, sizeof(uvhttp_context_t));

    ctx->loop = loop;
    ctx->created_at = time(NULL);

    *context = ctx;
    return UVHTTP_OK;
}

void uvhttp_context_destroy(uvhttp_context_t* context) {
    if (!context) {
        return;
    }

    /* Cleanup global variable replacement fields */
    uvhttp_context_cleanup_tls(context);
    uvhttp_context_cleanup_websocket(context);
    uvhttp_context_cleanup_config(context);

    /* Note: memory allocator uses compile-time macros, no runtime cleanup needed */

    uvhttp_free(context);
}

uvhttp_error_t uvhttp_context_init(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* If already initialized, return success directly (idempotent) */
    if (context->initialized) {
        return UVHTTP_OK;
    }

    /* Note: memory allocator uses compile-time macros, no runtime setup needed
     * Allocator type selected via UVHTTP_ALLOCATOR_TYPE compile macro
     */

    context->initialized = 1;

    /* Initialize global variable replacement fields */
    uvhttp_context_init_tls(context);
    uvhttp_context_init_websocket(context);
    uvhttp_context_init_config(context);

    return UVHTTP_OK;
}

/* ===== Global Variable Replacement Field Initialization Functions ===== */

/* Initialize TLS module state */
uvhttp_error_t uvhttp_context_init_tls(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* If already initialized, return success directly (idempotent) */
    if (context->tls_initialized) {
        return UVHTTP_OK;
    }

    /* Allocate and initialize entropy context */
    context->tls_entropy = uvhttp_alloc(sizeof(mbedtls_entropy_context));
    if (!context->tls_entropy) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_entropy_init((mbedtls_entropy_context*)context->tls_entropy);

    /* Allocate and initialize DRBG context */
    context->tls_drbg = uvhttp_alloc(sizeof(mbedtls_ctr_drbg_context));
    if (!context->tls_drbg) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->tls_entropy);
        uvhttp_free(context->tls_entropy);
        context->tls_entropy = NULL;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_ctr_drbg_init((mbedtls_ctr_drbg_context*)context->tls_drbg);

    /* Initialize DRBG with custom entropy source */
    int ret = mbedtls_ctr_drbg_seed(
        (mbedtls_ctr_drbg_context*)context->tls_drbg, mbedtls_entropy_func,
        (mbedtls_entropy_context*)context->tls_entropy,
        (const unsigned char*)"uvhttp_tls", 11);
    if (ret != 0) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->tls_entropy);
        mbedtls_ctr_drbg_free((mbedtls_ctr_drbg_context*)context->tls_drbg);
        uvhttp_free(context->tls_entropy);
        uvhttp_free(context->tls_drbg);
        context->tls_entropy = NULL;
        context->tls_drbg = NULL;
        return UVHTTP_ERROR_IO_ERROR;
    }

    context->tls_initialized = 1;

    return UVHTTP_OK;
}

/* Cleanup TLS module state */
void uvhttp_context_cleanup_tls(uvhttp_context_t* context) {
    if (!context || !context->tls_initialized) {
        return;
    }

    /* Free mbedtls_entropy_context and mbedtls_ctr_drbg_context */
    if (context->tls_entropy) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->tls_entropy);
        uvhttp_free(context->tls_entropy);
        context->tls_entropy = NULL;
    }

    if (context->tls_drbg) {
        mbedtls_ctr_drbg_free((mbedtls_ctr_drbg_context*)context->tls_drbg);
        uvhttp_free(context->tls_drbg);
        context->tls_drbg = NULL;
    }

    context->tls_initialized = 0;
}

/* Initialize WebSocket module state */
uvhttp_error_t uvhttp_context_init_websocket(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* If already initialized, return success directly (idempotent) */
    if (context->ws_drbg_initialized) {
        return UVHTTP_OK;
    }

    /* Allocate and initialize entropy context */
    context->ws_entropy = uvhttp_alloc(sizeof(mbedtls_entropy_context));
    if (!context->ws_entropy) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_entropy_init((mbedtls_entropy_context*)context->ws_entropy);

    /* Allocate and initialize DRBG context */
    context->ws_drbg = uvhttp_alloc(sizeof(mbedtls_ctr_drbg_context));
    if (!context->ws_drbg) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->ws_entropy);
        uvhttp_free(context->ws_entropy);
        context->ws_entropy = NULL;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_ctr_drbg_init((mbedtls_ctr_drbg_context*)context->ws_drbg);

    /* Initialize DRBG */
    int ret = mbedtls_ctr_drbg_seed(
        (mbedtls_ctr_drbg_context*)context->ws_drbg, mbedtls_entropy_func,
        (mbedtls_entropy_context*)context->ws_entropy, NULL, 0);
    if (ret != 0) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->ws_entropy);
        mbedtls_ctr_drbg_free((mbedtls_ctr_drbg_context*)context->ws_drbg);
        uvhttp_free(context->ws_entropy);
        uvhttp_free(context->ws_drbg);
        context->ws_entropy = NULL;
        context->ws_drbg = NULL;
        return UVHTTP_ERROR_IO_ERROR;
    }

    context->ws_drbg_initialized = 1;

    return UVHTTP_OK;
}

/* Cleanup WebSocket module state */
void uvhttp_context_cleanup_websocket(uvhttp_context_t* context) {
    if (!context || !context->ws_drbg_initialized) {
        return;
    }

    /* Free mbedtls_entropy_context and mbedtls_ctr_drbg_context */
    if (context->ws_entropy) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->ws_entropy);
        uvhttp_free(context->ws_entropy);
        context->ws_entropy = NULL;
    }

    if (context->ws_drbg) {
        mbedtls_ctr_drbg_free((mbedtls_ctr_drbg_context*)context->ws_drbg);
        uvhttp_free(context->ws_drbg);
        context->ws_drbg = NULL;
    }

    context->ws_drbg_initialized = 0;
}

/* Initialize configuration management */
uvhttp_error_t uvhttp_context_init_config(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* If already initialized, return success directly (idempotent) */
    if (context->current_config) {
        return UVHTTP_OK;
    }

    /* Initialize configuration management */
    uvhttp_config_t* current_config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&current_config);
    if (result != UVHTTP_OK) {
        return result;
    }

    context->current_config = current_config;

    return UVHTTP_OK;
}

/* Cleanup configuration management */
void uvhttp_context_cleanup_config(uvhttp_context_t* context) {
    if (!context) {
        return;
    }

    if (context->current_config) {
        /* Free configuration */
        uvhttp_config_free(context->current_config);
        context->current_config = NULL;
    }
}

/* Note: memory allocator uses compile-time macros, no runtime setup needed
 * Allocator type selected via UVHTTP_ALLOCATOR_TYPE compile macro:
 *
 * Compile command examples:
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=0  # System default
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=1  # mimalloc
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=2  # Custom
 */
