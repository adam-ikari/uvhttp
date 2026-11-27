/**
 * @file uvhttp_middleware.h
 * @brief 编译时零开销中间件系统
 *
 * 设计哲学：
 * - 链在编译期展开为静态数组，无运行时注册/查找开销
 * - 共享上下文：链内中间件共享同一个 context，链末统一清理
 * - 短路语义分两层，调用方必须按用途选对：
 *     - UVHTTP_EXECUTE_MIDDLEWARE / _CHAIN：只执行链，返回 CONTINUE/STOP。
 *       STOP 只停止链内后续中间件，**不会**跳出 handler 函数体。用于日志、
 *       计时这类即使 STOP 也无害的中间件。
 *     - UVHTTP_MIDDLEWARE_GUARD / _CHAIN_GUARD：在前者基础上，STOP 时发送
 *       当前响应并 `return UVHTTP_OK`，handler 体的副作用真正不发生。用于
 *       鉴权、限流、计费这类必须拦住的策略点。
 *   「STOP 不短路 handler 体」曾是本模块的已知缺陷；守卫宏是修复形态。
 *
 * 核心宏：
 * - UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw1, mw2, ...)
 *     内联执行中间件链，表达式值为 CONTINUE(0) / STOP(1)
 * - UVHTTP_MIDDLEWARE_GUARD(req, resp, mw1, mw2, ...)
 *     鉴权式执行：STOP 时发响应并立即返回，必须在返回 int 的 handler 体内用
 * - UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, mw1, mw2, ...)
 *     定义可复用的中间件链（静态数组）
 * - UVHTTP_EXECUTE_MIDDLEWARE_CHAIN / UVHTTP_MIDDLEWARE_CHAIN_GUARD
 *     预定义链对应的执行 / 守卫版本
 * - UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)
 *     将路由处理器包装为中间件（返回 0 = CONTINUE，非 0 = STOP）
 *
 * 用法示例：
 * @code
 *   int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_MIDDLEWARE_GUARD(req, resp, auth_middleware);
 *       // auth 返回 STOP 时，上面这行直接 return，下面不会执行
 *       UVHTTP_EXECUTE_MIDDLEWARE(req, resp, logging_middleware);
 *       uvhttp_response_prepare(resp, 200, NULL, 0, NULL, 0);
 *       ...
 *   }
 * @endcode
 */

#ifndef UVHTTP_MIDDLEWARE_H
#define UVHTTP_MIDDLEWARE_H

#include <stddef.h>

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

/*
 * 链执行器：链内共享一个 context，正常走完或因 STOP 中断都恰好清理一次。
 * 返回 UVHTTP_MIDDLEWARE_CONTINUE(0) 或 UVHTTP_MIDDLEWARE_STOP(1)。
 */
static inline int uvhttp_middleware_run(
    const uvhttp_middleware_handler_t* handlers, size_t count,
    uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_middleware_context_t ctx = {0, NULL};
    for (size_t i = 0; i < count; ++i) {
        if (handlers[i] != NULL &&
            handlers[i](request, response, &ctx) != UVHTTP_MIDDLEWARE_CONTINUE) {
            if (ctx.cleanup) {
                ctx.cleanup(ctx.data);
            }
            return UVHTTP_MIDDLEWARE_STOP;
        }
    }
    if (ctx.cleanup) {
        ctx.cleanup(ctx.data);
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/*
 * Inline chains keep their handler array in a call-site-unique static const
 * object instead of a compound literal. A compound literal is valid C99 but
 * a GNU extension in C++, which broke -pedantic C++ consumers of this public
 * header. The array lives in .rodata; no runtime allocation or copy.
 */
#define _UVHTTP_MW_RUN(counter, req, resp, ...)                              \
    do {                                                                     \
        static const uvhttp_middleware_handler_t _mw_handlers_##counter[] =  \
            {__VA_ARGS__};                                                   \
        (void)uvhttp_middleware_run(_mw_handlers_##counter,                  \
                                    sizeof(_mw_handlers_##counter) /         \
                                        sizeof(_mw_handlers_##counter[0]),   \
                                    (req), (resp));                          \
    } while (0)

#define UVHTTP_EXECUTE_MIDDLEWARE(req, resp, ...) \
    _UVHTTP_MW_RUN(__COUNTER__, req, resp, __VA_ARGS__)

/*
 * 守卫版本：STOP 时原子地发送中间件已设置好的状态/头部/响应体，并立即从
 * handler 返回。发送走公共 uvhttp_respond：读回响应里已经准备好的字段，
 * 因此中间件应当用 uvhttp_response_prepare 写入，而不是逐步 setter。
 * handler 返回类型必须是 int（库的标准 handler 签名），否则不要用本宏。
 */
#define _UVHTTP_MW_GUARD(counter, req, resp, ...)                            \
    do {                                                                     \
        static const uvhttp_middleware_handler_t _mw_guard_##counter[] =     \
            {__VA_ARGS__};                                                   \
        if (uvhttp_middleware_run(_mw_guard_##counter,                       \
                                  sizeof(_mw_guard_##counter) /              \
                                      sizeof(_mw_guard_##counter[0]),        \
                                  (req), (resp)) !=                          \
            UVHTTP_MIDDLEWARE_CONTINUE) {                                    \
            if (resp) {                                                       \
                uvhttp_respond((resp), (resp)->status_code, NULL,             \
                               (resp)->body, (resp)->body_length);           \
            }                                                                 \
            return UVHTTP_OK;                                                \
        }                                                                    \
    } while (0)

#define UVHTTP_MIDDLEWARE_GUARD(req, resp, ...) \
    _UVHTTP_MW_GUARD(__COUNTER__, req, resp, __VA_ARGS__)

/* Define middleware chain (for reuse) */
#define UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, ...)                  \
    static const uvhttp_middleware_handler_t name##_handlers[] = { \
        __VA_ARGS__};                                              \
    static const size_t name##_count =                             \
        sizeof(name##_handlers) / sizeof(name##_handlers[0])

#define UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, name)     uvhttp_middleware_run((name##_handlers), (name##_count), (req), (resp))

#define UVHTTP_MIDDLEWARE_CHAIN_GUARD(req, resp, name)                        \
    do {                                                                     \
        if (UVHTTP_EXECUTE_MIDDLEWARE_CHAIN((req), (resp), name) !=           \
            UVHTTP_MIDDLEWARE_CONTINUE) {                                    \
            if (resp) {                                                       \
                uvhttp_respond((resp), (resp)->status_code, NULL,             \
                               (resp)->body, (resp)->body_length);           \
            }                                                                 \
            return UVHTTP_OK;                                                \
        }                                                                    \
    } while (0)

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
#define UVHTTP_MIDDLEWARE_HANDLER(handler) _uvhttp_mw_handler_wrapper_##handler

#define UVHTTP_DEFINE_MIDDLEWARE_HANDLER(handler)         \
    static int _uvhttp_mw_handler_wrapper_##handler(      \
        uvhttp_request_t* _req, uvhttp_response_t* _resp, \
        uvhttp_middleware_context_t* _ctx) {              \
        (void)_ctx;                                       \
        int _ret = handler(_req, _resp);                  \
        return _ret == 0 ? UVHTTP_MIDDLEWARE_CONTINUE     \
                         : UVHTTP_MIDDLEWARE_STOP;        \
    }

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_H */