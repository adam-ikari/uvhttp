/*
 * UVHTTP 中间件系统 - 编译期配置版本
 *
 * 设计原则：
 * - 零开销抽象：使用编译期宏，无运行时动态分配
 * - 固定管线：中间件顺序在编译时确定
 * - 简洁清晰：避免链表、优先级等复杂概念
 * - 应用层主导：中间件逻辑由应用层实现
 *
 * 使用示例：
 *   int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *           logging_middleware,
 *           auth_middleware,
 *           cors_middleware
 *       );
 *       // 处理请求...
 *   }
 *
 * 重要说明：
 * - 如果中间件返回 UVHTTP_MIDDLEWARE_STOP，表示中间件已经处理了请求
 * - 处理器应该检查响应是否已经被发送，或者直接返回
 * - 推荐模式：在中间件返回 STOP 后，处理器应该立即返回
 */

#ifndef UVHTTP_MIDDLEWARE_H
#define UVHTTP_MIDDLEWARE_H

#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 中间件返回值 */
#define UVHTTP_MIDDLEWARE_CONTINUE 0
#define UVHTTP_MIDDLEWARE_STOP 1

/* 中间件上下文 */
typedef struct uvhttp_middleware_context {
    void* data;
    void (*cleanup)(void* data);
} uvhttp_middleware_context_t;

/* 中间件处理函数类型 */
typedef int (*uvhttp_middleware_handler_t)(uvhttp_request_t* request,
                                           uvhttp_response_t* response,
                                           uvhttp_middleware_context_t* ctx);

/* 执行中间件链 */
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

/* 定义中间件链（供复用） */
#define UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, ...)                  \
    static const uvhttp_middleware_handler_t name##_handlers[] = { \
        __VA_ARGS__};                                              \
    static const size_t name##_count =                             \
        sizeof(name##_handlers) / sizeof(name##_handlers[0])

/* 执行预定义的中间件链 */
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