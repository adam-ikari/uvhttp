/*
 * UVHTTP 中间件测试
 * 验证中间件是否被调用
 * 
 * 注意：此示例使用全局变量以简化代码。
 * 在生产环境中，建议使用 libuv 数据指针模式或依赖注入来管理应用状态。
 */

#include "uvhttp.h"
#include "uvhttp_middleware.h"
#include <stdio.h>

/* ============ 测试中间件 ============ */

static int test_middleware(uvhttp_request_t* req, uvhttp_response_t* resp, uvhttp_middleware_context_t* ctx) {
    (void)ctx;
    (void)req;
    
    printf("[TEST] 中间件被调用！\n");
    
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "中间件测试", 11);
    
    uvhttp_response_send(resp);
    return UVHTTP_MIDDLEWARE_STOP;
}

/* ============ 请求处理器 ============ */

static int test_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    printf("[HANDLER] 处理器被调用！\n");
    
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp,
        test_middleware
    );
    
    printf("[HANDLER] 中间件执行完毕\n");
    
    return 0;
}

/* ============ 主函数 ============ */

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    
    uvhttp_error_t err;
    
    /* 创建服务器 */
    err = uvhttp_server_new(loop, &server);
    if (err != UVHTTP_OK || !server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    
    /* 创建路由 */
    err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK || !router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        return 1;
    }
    
    /* 添加路由 */
    uvhttp_router_add_route(router, "/test", test_handler);
    
    uvhttp_server_set_router(server, router);
    uvhttp_server_listen(server, "0.0.0.0", 8086);
    
    printf("服务器运行在 http://localhost:8086/test\n");
    printf("测试: curl http://localhost:8086/test\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* 清理资源 */
    uvhttp_server_free(server);
    
    return 0;
}