/* UVHTTP 依赖注入和上下文管理实现 */

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

/* ============ 内存分配器说明 ============ */
/*
 * UVHTTP 内存分配器采用编译时宏设计，零开销抽象
 *
 * 不实现运行时分配器提供者，原因：
 * 1. 性能优先：避免函数指针调用开销
 * 2. 编译时优化：编译器可以内联和优化分配调用
 * 3. 简单直接：减少复杂性，提高可维护性
 *
 * 内存分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择：
 * - 0: 系统默认分配器 (malloc/free)
 * - 1: mimalloc 高性能分配器
 * - 2: 自定义分配器 (外部链接)
 *
 * 使用方式：
 *   #include "uvhttp_allocator.h"
 *   void* ptr = uvhttp_alloc(size);
 *   uvhttp_free(ptr);
 */

/* ============ 上下文管理实现 ============ */

uvhttp_error_t uvhttp_context_create(uv_loop_t* loop, uvhttp_context_t** context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_context_t* ctx = (uvhttp_context_t*)uvhttp_alloc(sizeof(uvhttp_context_t));
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

    /* 清理全局变量替代字段 */
    uvhttp_context_cleanup_tls(context);
    uvhttp_context_cleanup_websocket(context);
    uvhttp_context_cleanup_error_stats(context);
    uvhttp_context_cleanup_config(context);

    /* 注意：内存分配器使用编译时宏，无需运行时清理 */

    uvhttp_free(context);
}

uvhttp_error_t uvhttp_context_init(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 如果已经初始化，直接返回成功（幂等） */
    if (context->initialized) {
        return UVHTTP_OK;
    }

    /* 注意：内存分配器使用编译时宏，无需运行时设置
     * 分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择
     */

    context->initialized = 1;

    /* 初始化全局变量替代字段 */
    uvhttp_context_init_tls(context);
    uvhttp_context_init_websocket(context);
    uvhttp_context_init_error_stats(context);
    uvhttp_context_init_config(context);

    return UVHTTP_OK;
}

/* ===== 全局变量替代字段初始化函数 ===== */

/* 初始化 TLS 模块状态 */
uvhttp_error_t uvhttp_context_init_tls(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 如果已经初始化，直接返回成功（幂等） */
    if (context->tls_initialized) {
        return UVHTTP_OK;
    }

    /* 分配并初始化 entropy 上下文 */
    context->tls_entropy = uvhttp_alloc(sizeof(mbedtls_entropy_context));
    if (!context->tls_entropy) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_entropy_init((mbedtls_entropy_context*)context->tls_entropy);

    /* 分配并初始化 DRBG 上下文 */
    context->tls_drbg = uvhttp_alloc(sizeof(mbedtls_ctr_drbg_context));
    if (!context->tls_drbg) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->tls_entropy);
        uvhttp_free(context->tls_entropy);
        context->tls_entropy = NULL;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_ctr_drbg_init((mbedtls_ctr_drbg_context*)context->tls_drbg);

    /* 使用自定义熵源初始化 DRBG */
    int ret = mbedtls_ctr_drbg_seed(
        (mbedtls_ctr_drbg_context*)context->tls_drbg, mbedtls_entropy_func,
        (mbedtls_entropy_context*)context->tls_entropy, (const unsigned char*)"uvhttp_tls", 11);
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

/* 清理 TLS 模块状态 */
void uvhttp_context_cleanup_tls(uvhttp_context_t* context) {
    if (!context || !context->tls_initialized) {
        return;
    }

    /* 释放 mbedtls_entropy_context 和 mbedtls_ctr_drbg_context */
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

/* 初始化 WebSocket 模块状态 */
uvhttp_error_t uvhttp_context_init_websocket(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 如果已经初始化，直接返回成功（幂等） */
    if (context->ws_drbg_initialized) {
        return UVHTTP_OK;
    }

    /* 分配并初始化 entropy 上下文 */
    context->ws_entropy = uvhttp_alloc(sizeof(mbedtls_entropy_context));
    if (!context->ws_entropy) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_entropy_init((mbedtls_entropy_context*)context->ws_entropy);

    /* 分配并初始化 DRBG 上下文 */
    context->ws_drbg = uvhttp_alloc(sizeof(mbedtls_ctr_drbg_context));
    if (!context->ws_drbg) {
        mbedtls_entropy_free((mbedtls_entropy_context*)context->ws_entropy);
        uvhttp_free(context->ws_entropy);
        context->ws_entropy = NULL;
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    mbedtls_ctr_drbg_init((mbedtls_ctr_drbg_context*)context->ws_drbg);

    /* 初始化 DRBG */
    int ret =
        mbedtls_ctr_drbg_seed((mbedtls_ctr_drbg_context*)context->ws_drbg, mbedtls_entropy_func,
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

/* 清理 WebSocket 模块状态 */
void uvhttp_context_cleanup_websocket(uvhttp_context_t* context) {
    if (!context || !context->ws_drbg_initialized) {
        return;
    }

    /* 释放 mbedtls_entropy_context 和 mbedtls_ctr_drbg_context */
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

/* 初始化错误统计 */
uvhttp_error_t uvhttp_context_init_error_stats(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 如果已经初始化，直接返回成功（幂等） */
    if (context->error_stats) {
        return UVHTTP_OK;
    }

    /* 分配错误统计结构 */
    uvhttp_error_stats_t* error_stats = uvhttp_alloc(sizeof(uvhttp_error_stats_t));
    if (!error_stats) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    /* 初始化错误统计 */
    memset(error_stats, 0, sizeof(uvhttp_error_stats_t));
    context->error_stats = error_stats;

    return UVHTTP_OK;
}

/* 清理错误统计 */
void uvhttp_context_cleanup_error_stats(uvhttp_context_t* context) {
    if (!context) {
        return;
    }

    if (context->error_stats) {
        /* 释放错误统计结构 */
        uvhttp_free(context->error_stats);
        context->error_stats = NULL;
    }
}

/* 初始化配置管理 */
uvhttp_error_t uvhttp_context_init_config(uvhttp_context_t* context) {
    if (!context) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 如果已经初始化，直接返回成功（幂等） */
    if (context->current_config) {
        return UVHTTP_OK;
    }

    /* 初始化配置管理 */
    uvhttp_config_t* current_config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&current_config);
    if (result != UVHTTP_OK) {
        return result;
    }

    context->current_config = current_config;

    return UVHTTP_OK;
}

/* 清理配置管理 */
void uvhttp_context_cleanup_config(uvhttp_context_t* context) {
    if (!context) {
        return;
    }

    if (context->current_config) {
        /* 释放配置 */
        uvhttp_config_free(context->current_config);
        context->current_config = NULL;
    }
}

/* 注意：内存分配器使用编译时宏，无需运行时设置
 * 分配器类型通过 UVHTTP_ALLOCATOR_TYPE 编译宏选择：
 *
 * 编译命令示例：
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=0  # 系统默认
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=1  # mimalloc
 *   gcc -DUVHTTP_ALLOCATOR_TYPE=2  # 自定义
 */
