/*
 * UVHTTP 简化版编译宏中间件系统
 * 
 * 设计原则：
 * 1. 极简设计 - 只提供必要的功能
 * 2. 零开销 - 无动态分配，无运行时开销
 * 3. 静态配置 - 中间件链在编译时确定
 * 4. 类型安全 - 使用强类型检查
 * 5. 易于使用 - 简单的 API，清晰的语义
 * 
 * 适用场景：
 * - 中间件链在编译时已知
 * - 不需要运行时动态添加/删除中间件
 * - 追求极致性能
 * 
 * 不适用场景：
 * - 需要运行时动态配置中间件
 * - 需要中间件状态持久化（如限流）
 * - 需要条件执行中间件
 */

#ifndef UVHTTP_SIMPLE_MIDDLEWARE_H
#define UVHTTP_SIMPLE_MIDDLEWARE_H

#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 中间件返回值 */
#define UVHTTP_MIDDLEWARE_CONTINUE 0    /* 继续执行下一个中间件 */
#define UVHTTP_MIDDLEWARE_STOP 1        /* 停止执行中间件链 */

/* 中间件函数类型 - 简化版，无上下文参数 */
typedef int (*uvhttp_middleware_fn_t)(
    const uvhttp_request_t* request,
    uvhttp_response_t* response
);

/*
 * 执行中间件链
 * 
 * 使用示例：
 *   static int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_MIDDLEWARE(req, resp,
 *           logging_middleware,
 *           cors_middleware,
 *           auth_middleware
 *       );
 *       
 *       // 处理请求...
 *       return 0;
 *   }
 * 
 * 特性：
 * - 零动态分配：使用 static const 数组
 * - 零运行时开销：编译时确定
 * - 类型安全：使用函数指针类型
 * - 可预测：使用 goto 而非 continue
 * 
 * 注意：
 * - 中间件函数必须返回 UVHTTP_MIDDLEWARE_CONTINUE 或 UVHTTP_MIDDLEWARE_STOP
 * - 中间件函数不能有副作用（如修改全局状态）
 * - 中间件函数必须快速执行（避免阻塞）
 * - 不支持中间件状态（每个请求独立）
 */
#define UVHTTP_MIDDLEWARE(req, resp, ...) \
    do { \
        static const uvhttp_middleware_fn_t _mw_handlers[] = { __VA_ARGS__ }; \
        const size_t _mw_count = sizeof(_mw_handlers) / sizeof(_mw_handlers[0]); \
        for (size_t _mw_i = 0; _mw_i < _mw_count; _mw_i++) { \
            if (_mw_handlers[_mw_i] && _mw_handlers[_mw_i](req, resp) != UVHTTP_MIDDLEWARE_CONTINUE) { \
                goto _mw_end; \
            } \
        } \
    } while(0); \
    _mw_end: (void)0

/*
 * 定义中间件链（可选）
 * 
 * 使用示例：
 *   UVHTTP_MIDDLEWARE_CHAIN(api_chain,
 *       logging_middleware,
 *       cors_middleware,
 *       auth_middleware
 *   );
 *   
 *   static int my_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       for (size_t i = 0; i < api_chain_count; i++) {
 *           if (api_chain[i](req, resp) != UVHTTP_MIDDLEWARE_CONTINUE) {
 *               return 0;
 *           }
 *       }
 *       // 处理请求...
 *       return 0;
 *   }
 * 
 * 特性：
 * - 允许在多个处理器中重用中间件链
 * - 静态 const，零运行时开销
 * - 可读性好：命名清晰的中间件链
 * 
 * 注意：
 * - 链名必须唯一（避免命名冲突）
 * - 链定义必须在函数外部
 */
#define UVHTTP_MIDDLEWARE_CHAIN(name, ...) \
    static const uvhttp_middleware_fn_t name##_chain[] = { __VA_ARGS__ }; \
    static const size_t name##_count = sizeof(name##_chain) / sizeof(name##_chain[0])

/*
 * 条件中间件（可选）
 * 
 * 使用示例：
 *   #ifdef ENABLE_AUTH
 *   #define AUTH_MIDDLEWARE auth_middleware
 *   #else
 *   #define AUTH_MIDDLEWARE NULL
 *   #endif
 *   
 *   UVHTTP_MIDDLEWARE(req, resp,
 *       logging_middleware,
 *       AUTH_MIDDLEWARE,  // 可能为 NULL
 *       cors_middleware
 *   );
 * 
 * 特性：
 * - 支持编译时条件编译
 * - NULL 中间件会被自动跳过
 * - 灵活控制中间件启用/禁用
 */

/*
 * 常见中间件示例
 */

/* 日志中间件 */
static inline int uvhttp_middleware_logging(
    const uvhttp_request_t* request,
    uvhttp_response_t* response
) {
    (void)response;  /* 未使用 */
    printf("[LOG] %s %s\n", 
           uvhttp_method_to_string(request->method), 
           request->url);
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* CORS 中间件（简化版） */
static inline int uvhttp_middleware_cors(
    const uvhttp_request_t* request,
    uvhttp_response_t* response
) {
    (void)request;  /* 未使用 */
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    uvhttp_response_set_header(response, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* 预检请求处理中间件 */
static inline int uvhttp_middleware_preflight(
    const uvhttp_request_t* request,
    uvhttp_response_t* response
) {
    if (request->method == UVHTTP_OPTIONS) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_send(response);
        return UVHTTP_MIDDLEWARE_STOP;
    }
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_SIMPLE_MIDDLEWARE_H */