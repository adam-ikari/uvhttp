/*
 * UVHTTP MiddlewareSystem - Configuration version
 *
 * :
 * - zero: Use, runwhen
 * - : Middlewarewhen
 * - : 、
 * - : Middleware
 *
 * Useexample:
 *   int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *           logging_middleware,
 *           auth_middleware,
 *           cors_middleware
 *       );
 *       // handleRequest...
 *   }
 *
 * importantdescription:
 * - ifMiddlewareReturn UVHTTP_MIDDLEWARE_STOP,
 * representsMiddlewarealreadyhandleRequest
 * - handleResponsewhetheralreadyissend, orReturn
 * - mode: MiddlewareReturn STOP , handleReturn
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
#define UVHTTP_EXECUTE_MIDDLEWARE(req, resp, ...)                          \
    do {                                                                   \
        static const uvhttp_middleware_handler_t _uvhttp_mw_handlers[] = { \
            __VA_ARGS__};                                                  \
        uvhttp_middleware_context_t _uvhttp_mw_ctx = {0};                  \
        for (size_t _uvhttp_mw_i = 0;                                      \
             _uvhttp_mw_i <                                                \
             sizeof(_uvhttp_mw_handlers) / sizeof(_uvhttp_mw_handlers[0]); \
             _uvhttp_mw_i++) {                                             \
            if (_uvhttp_mw_handlers[_uvhttp_mw_i] &&                       \
                _uvhttp_mw_handlers[_uvhttp_mw_i](req, resp,               \
                                                  &_uvhttp_mw_ctx) !=      \
                    UVHTTP_MIDDLEWARE_CONTINUE) {                          \
                if (_uvhttp_mw_ctx.cleanup)                                \
                    _uvhttp_mw_ctx.cleanup(_uvhttp_mw_ctx.data);           \
                goto _uvhttp_mw_stop;                                      \
            }                                                              \
        }                                                                  \
    } while (0);                                                           \
_uvhttp_mw_stop:                                                           \
    (void)0

/* Define middleware chain (for reuse) */
#define UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, ...)                  \
    static const uvhttp_middleware_handler_t name##_handlers[] = { \
        __VA_ARGS__};                                              \
    static const size_t name##_count =                             \
        sizeof(name##_handlers) / sizeof(name##_handlers[0])

/* Execute predefined middleware chain */
#define UVHTTP_EXECUTE_MIDDLEWARE_CHAIN(req, resp, name)                     \
    do {                                                                     \
        uvhttp_middleware_context_t _uvhttp_mw_ctx = {0};                    \
        for (size_t _uvhttp_mw_i = 0; _uvhttp_mw_i < name##_count;           \
             _uvhttp_mw_i++) {                                               \
            if (name##_handlers[_uvhttp_mw_i] &&                             \
                name##_handlers[_uvhttp_mw_i](req, resp, &_uvhttp_mw_ctx) != \
                    UVHTTP_MIDDLEWARE_CONTINUE) {                            \
                if (_uvhttp_mw_ctx.cleanup)                                  \
                    _uvhttp_mw_ctx.cleanup(_uvhttp_mw_ctx.data);             \
                goto _uvhttp_mw_stop_##name;                                 \
            }                                                                \
        }                                                                    \
    } while (0);                                                             \
    _uvhttp_mw_stop_##name : (void)0

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_H */