/**
 * @file uvhttp_middleware.h
 * @brief 编译时零开销中间件系统
 *
 * 设计哲学：
 * - 编译时宏展开，无运行时注册/查找开销
 * - 短路语义：中间件返回 STOP 立即中断链
 * - 共享上下文：所有中间件共享同一个 context
 * - 运行时中间件由用户自行实现
 *
 * 核心宏：
 * - UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw1, mw2, ...)
 *     内联执行中间件链，每个中间件按序调用
 *     任一返回 STOP 则跳过后续中间件
 *
 * - UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, mw1, mw2, ...)
 *     定义可复用的中间件链（静态数组）
 *
 * - UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, chain)
 *     执行预定义的中间件链
 *
 * - UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)
 *     将路由处理器包装为中间件（返回 0 = CONTINUE，非 0 = STOP）
 *
 * 用法示例：
 * @code
 *   int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *           auth_middleware,
 *           logging_middleware);
 *       // ... 处理请求 ...
 *       return 0;
 *   }
 *
 *   // 定义可复用链
 *   UVHTTP_DEFINE_MIDDLEWARE_CHAIN(api_chain,
 *       auth_middleware,
 *       rate_limit_middleware,
 *       logging_middleware);
 *
 *   // 执行链
 *   UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, api_chain);
 * @endcode
 */

#ifndef UVHTTP_MIDDLEWARE_H
#define UVHTTP_MIDDLEWARE_H

#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Middleware return values */
#define UVHTTP_MIDDLEWARE_CONTINUE 0
#define UVHTTP_MIDDLEWARE_STOP 1

/* Middleware context */
typedef struct uvhttp_middleware_context {
    void* data;
    void (*cleanup)(void* data);
} uvhttp_middleware_context_t;

/* Middleware handler function type */
typedef int (*uvhttp_middleware_handler_t)(uvhttp_request_t* request,
                                           uvhttp_response_t* response,
                                           uvhttp_middleware_context_t* ctx);

/* Execute middleware chain */
#define _UVHTTP_MW_EXECUTE_IMPL_(counter, req, resp, ...)                      \
    do {                                                                        \
        static const uvhttp_middleware_handler_t _uvhttp_mw_handlers_##counter[] = { \
            __VA_ARGS__};                                                       \
        uvhttp_middleware_context_t _uvhttp_mw_ctx_##counter = {0};             \
        for (size_t _uvhttp_mw_i = 0;                                           \
             _uvhttp_mw_i <                                                     \
             sizeof(_uvhttp_mw_handlers_##counter) / sizeof(_uvhttp_mw_handlers_##counter[0]); \
             _uvhttp_mw_i++) {                                                  \
            if (_uvhttp_mw_handlers_##counter[_uvhttp_mw_i] &&                  \
                _uvhttp_mw_handlers_##counter[_uvhttp_mw_i](req, resp,          \
                                                  &_uvhttp_mw_ctx_##counter) != \
                    UVHTTP_MIDDLEWARE_CONTINUE) {                               \
                goto _uvhttp_mw_stop_##counter;                                 \
            }                                                                   \
        }                                                                       \
        if (_uvhttp_mw_ctx_##counter.cleanup) {                                 \
            _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);    \
        }                                                                       \
        goto _uvhttp_mw_done_##counter;                                         \
        _uvhttp_mw_stop_##counter:                                              \
        if (_uvhttp_mw_ctx_##counter.cleanup) {                                 \
            _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);    \
        }                                                                       \
        _uvhttp_mw_done_##counter:;                                             \
    } while (0)

#define _UVHTTP_MW_EXECUTE_IMPL(counter, req, resp, ...)                       \
    _UVHTTP_MW_EXECUTE_IMPL_(counter, req, resp, __VA_ARGS__)

#define UVHTTP_EXECUTE_MIDDLEWARE(req, resp, ...)                              \
    _UVHTTP_MW_EXECUTE_IMPL(__COUNTER__, req, resp, __VA_ARGS__)

/* Define middleware chain (for reuse) */
#define UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, ...)                  \
    static const uvhttp_middleware_handler_t name##_handlers[] = { \
        __VA_ARGS__};                                              \
    static const size_t name##_count =                             \
        sizeof(name##_handlers) / sizeof(name##_handlers[0])

/* Execute predefined middleware chain */
#define _UVHTTP_MW_CHAIN_IMPL_(counter, req, resp, name)                       \
    do {                                                                        \
        uvhttp_middleware_context_t _uvhttp_mw_ctx_##counter = {0};             \
        for (size_t _uvhttp_mw_i = 0; _uvhttp_mw_i < name##_count;              \
             _uvhttp_mw_i++) {                                                  \
            if (name##_handlers[_uvhttp_mw_i] &&                                \
                name##_handlers[_uvhttp_mw_i](req, resp, &_uvhttp_mw_ctx_##counter) != \
                    UVHTTP_MIDDLEWARE_CONTINUE) {                               \
                goto _uvhttp_mw_stop_##counter;                                 \
            }                                                                   \
        }                                                                       \
        if (_uvhttp_mw_ctx_##counter.cleanup) {                                 \
            _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);    \
        }                                                                       \
        goto _uvhttp_mw_done_##counter;                                         \
        _uvhttp_mw_stop_##counter:                                              \
        if (_uvhttp_mw_ctx_##counter.cleanup) {                                 \
            _uvhttp_mw_ctx_##counter.cleanup(_uvhttp_mw_ctx_##counter.data);    \
        }                                                                       \
        _uvhttp_mw_done_##counter:;                                             \
    } while (0)

#define _UVHTTP_MW_CHAIN_IMPL(counter, req, resp, name)                        \
    _UVHTTP_MW_CHAIN_IMPL_(counter, req, resp, name)

#define UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, name)                       \
    _UVHTTP_MW_CHAIN_IMPL(__COUNTER__, req, resp, name)

/**
 * UVHTTP_MIDDLEWARE_HANDLER(handler)
 * 引用由 UVHTTP_DEFINE_MIDDLEWARE_HANDLER 创建的包装器函数名。
 * 用于在 UVHTTP_EXECUTE_MIDDLEWARE 中将路由处理器作为中间件使用。
 *
 * UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)
 * 将 uvhttp_request_handler_t 签名适配为 uvhttp_middleware_handler_t。
 * 路由处理器返回 0 视为 CONTINUE，非 0 视为 STOP。
 *
 * 用法：
 *   UVHTTP_DEFINE_MIDDLEWARE_HANDLER(my_handler);
 *   UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *       auth_middleware,
 *       UVHTTP_MIDDLEWARE_HANDLER(my_handler));
 */
#define UVHTTP_MIDDLEWARE_HANDLER(handler) \
    _uvhttp_mw_handler_wrapper_##handler

#define UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)                               \
    static int _uvhttp_mw_handler_wrapper_##handler(                            \
        uvhttp_request_t* _req, uvhttp_response_t* _resp,                      \
        uvhttp_middleware_context_t* _ctx) {                                    \
        (void)_ctx;                                                             \
        int _ret = handler(_req, _resp);                                        \
        return _ret == 0 ? UVHTTP_MIDDLEWARE_CONTINUE : UVHTTP_MIDDLEWARE_STOP; \
    }

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_H */