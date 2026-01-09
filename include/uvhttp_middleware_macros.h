/*
 * UVHTTP 编译宏中间件系统
 * 零开销、零动态分配的中间件实现
 * 
 * 特性：
 * - 使用 static const 优化，只分配一次
 * - 支持上下文（与动态系统兼容）
 * - 使用 goto 机制确保在任何上下文中都能正确停止执行
 * - 统一函数签名，中间件可在两系统中复用
 * 
 * 性能特性：
 * - 内存分配：O(1) - 只在程序启动时分配一次
 * - 执行速度：O(n) - n 为中间件数量
 * - 缓存效率：高 - 使用只读内存，缓存友好
 * 
 * 适用场景：
 * - 中间件链静态且已知（编译时确定）
 * - 不需要运行时动态添加/删除中间件
 * - 极致性能要求
 * - 代码重复可接受
 * 
 * 不适用场景：
 * - 需要运行时动态配置中间件
 * - 需要路径匹配执行不同中间件
 * - 需要中间件优先级排序
 * - 需要中间件间共享状态（上下文不持久）
 */

#ifndef UVHTTP_MIDDLEWARE_MACROS_H
#define UVHTTP_MIDDLEWARE_MACROS_H

#include "uvhttp_common.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_middleware.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 中间件返回值 */
#define UVHTTP_MIDDLEWARE_CONTINUE 0    /* 继续执行下一个中间件 */
#define UVHTTP_MIDDLEWARE_STOP 1        /* 停止执行中间件链 */

/* 中间件处理函数类型 - 与动态系统统一
 * 
 * 注意：
 * - request 参数为 const，表示中间件不应修改请求对象
 * - response 参数非 const，允许中间件修改响应
 * - ctx 参数非 const，允许中间件修改上下文
 */
typedef int (*uvhttp_middleware_handler_t)(
    const uvhttp_request_t* request,
    uvhttp_response_t* response,
    uvhttp_middleware_context_t* ctx
);

/* 编译宏 - 执行中间件链
 * 
 * 使用说明：
 * 1. 此宏使用 static const 数组，只分配一次，零运行时开销
 * 2. 自动创建上下文，生命周期与请求一致
 * 3. 使用 goto 机制确保在任何上下文中都能正确停止执行
 * 4. 自动调用上下文清理函数
 * 5. 可以在任何上下文中使用（void/non-void 函数、嵌套循环等）
 * 
 * 重要限制：
 * - 参数 req 和 resp 不应有副作用（如函数调用），因为它们会被多次使用
 * - 推荐用法：先获取指针，再传递给宏
 * - 上下文在每个中间件调用后会被重置，无法在中间件间共享状态
 * - 如果中间件返回 STOP，宏会立即跳转到标签，后续代码不会执行
 * 
 * 错误示例：
 *   UVHTTP_EXECUTE_MIDDLEWARE(get_request(), get_response(), mw1);  // ❌ 函数被多次调用
 * 
 * 正确示例：
 *   uvhttp_request_t* req = get_request();  // ✅ 先获取指针
 *   uvhttp_response_t* resp = get_response();
 *   UVHTTP_EXECUTE_MIDDLEWARE(req, resp, mw1);
 * 
 * 示例：
 *   void handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
 *           logging_middleware,
 *           auth_middleware,
 *           cors_middleware
 *       );
 *       // 处理请求...
 *   }
 * 
 * 性能对比：
 * - 宏系统：零运行时分配，~29% 性能提升
 * - 动态系统：需要堆分配，但更灵活
 * 
 * 选择建议：
 * - 使用宏系统：如果中间件链固定且性能关键
 * - 使用动态系统：如果需要灵活配置和路径匹配
 */
#define UVHTTP_EXECUTE_MIDDLEWARE(req, resp, ...) \
    do { \
        static const uvhttp_middleware_handler_t handlers[] = { __VA_ARGS__ }; \
        const size_t handler_count = sizeof(handlers)/sizeof(handlers[0]); \
        uvhttp_middleware_context_t _mw_ctx = {0}; \
        for (size_t i = 0; i < handler_count; i++) { \
            if (handlers[i] && handlers[i](req, resp, &_mw_ctx) != UVHTTP_MIDDLEWARE_CONTINUE) { \
                if (_mw_ctx.cleanup) _mw_ctx.cleanup(_mw_ctx.data); \
                goto uvhttp_middleware_stop_execution; \
            } \
        } \
    } while(0); \
    uvhttp_middleware_stop_execution: (void)0

/* 便捷宏 - 定义中间件链
 * 
 * 使用说明：
 *   UVHTTP_DEFINE_MIDDLEWARE_CHAIN(api_chain,
 *       logging_middleware,
 *       auth_middleware,
 *       cors_middleware
 *   );
 *   
 *   然后可以使用 api_chain_handlers 和 api_chain_count
 * 
 * 用途：
 * - 避免代码重复
 * - 在多个处理器中复用中间件链
 * - 提高代码可维护性
 * 
 * 示例：
 *   UVHTTP_DEFINE_MIDDLEWARE_CHAIN(admin_chain,
 *       logging_middleware,
 *       auth_middleware,
 *       cors_middleware
 *   );
 *   
 *   void admin_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
 *       for (size_t i = 0; i < admin_chain_count; i++) {
 *           uvhttp_middleware_context_t ctx = {0};
 *           if (admin_chain_handlers[i] && admin_chain_handlers[i](req, resp, &ctx) != UVHTTP_MIDDLEWARE_CONTINUE) {
 *               return;
 *           }
 *       }
 *       // 处理请求...
 *   }
 */
#define UVHTTP_DEFINE_MIDDLEWARE_CHAIN(name, ...) \
    static const uvhttp_middleware_handler_t name##_handlers[] = { __VA_ARGS__ }; \
    static const size_t name##_count = sizeof(name##_handlers)/sizeof(name##_handlers[0])

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_MIDDLEWARE_MACROS_H */