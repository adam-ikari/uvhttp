/**
 * @file 01_hello_world.c
 * @brief UVHTTP 第一个示例 - Hello World
 * 
 * 这是 UVHTTP 的最简单示例，展示如何创建一个基本的 HTTP 服务器。
 * 
 * 学习目标：
 * 1. 理解 UVHTTP 的基本结构
 * 2. 学习如何创建服务器和路由
 * 3. 掌握请求处理器的基本写法
 * 4. 了解事件循环的运行方式
 * 
 * 运行方法：
 * gcc -o hello_world 01_hello_world.c -I../../include -L../../build -luvhttp -luv -lpthread
 * ./hello_world
 * 
 * 测试：
 * curl http://localhost:8080/
 */

#include "../../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>

/**
 * @brief 应用上下文结构
 * 
 * 封装所有应用相关的数据，避免使用全局变量
 */
typedef struct {
    uvhttp_server_t* server;
    uv_loop_t* loop;
    int is_running;
} app_context_t;

/**
 * @brief 信号处理函数
 * 
 * 处理 Ctrl+C 信号，优雅关闭服务器
 */
void signal_handler(int sig) {
    printf("\n收到信号 %d，正在关闭服务器...\n", sig);
    // 注意：这里我们只是设置标志，实际的清理在主循环中完成
    exit(0);
}

/**
 * @brief 请求处理器 - Hello World
 * 
 * 这是所有 HTTP 请求的入口点
 * 
 * @param req HTTP 请求对象
 * @param res HTTP 响应对象
 * @return 0 表示成功，非 0 表示错误
 */
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;  // 未使用的参数

    // 1. 设置 HTTP 状态码
    uvhttp_response_set_status(res, 200);

    // 2. 设置响应头
    uvhttp_response_set_header(res, "Content-Type", "text/plain; charset=utf-8");
    uvhttp_response_set_header(res, "Server", "UVHTTP/1.0");

    // 3. 设置响应体
    const char* body = "Hello, World! Welcome to UVHTTP.\n";
    uvhttp_response_set_body(res, body, strlen(body));

    // 4. 发送响应
    return uvhttp_response_send(res);
}

/**
 * @brief 主函数
 */
int main() {
    printf("========================================\n");
    printf("  UVHTTP Hello World 示例\n");
    printf("========================================\n\n");
    
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建应用上下文
    app_context_t ctx = {0};
    
    // 步骤 1: 创建事件循环
    printf("步骤 1: 创建事件循环...\n");
    ctx.loop = uv_default_loop();
    if (!ctx.loop) {
        fprintf(stderr, "错误: 无法创建事件循环\n");
        return 1;
    }
    printf("✓ 事件循环创建成功\n\n");
    
    // 步骤 2: 创建服务器
    printf("步骤 2: 创建 HTTP 服务器...\n");
    uvhttp_error_t server_result = uvhttp_server_new(ctx.loop, &ctx.server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "错误: 无法创建服务器: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    printf("✓ 服务器创建成功\n\n");
    
    // 步骤 3: 创建路由器
    printf("步骤 3: 创建路由器...\n");
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        return 1;
    }
    printf("✓ 路由器创建成功\n\n");
    
    // 步骤 4: 添加路由
    printf("步骤 4: 添加路由...\n");
    int route_result = uvhttp_router_add_route(router, "/", hello_handler);
    if (route_result != UVHTTP_OK) {
        fprintf(stderr, "错误: 无法添加路由\n");
        uvhttp_server_free(ctx.server);
        return 1;
    }
    printf("✓ 路由添加成功: /\n\n");
    
    // 步骤 5: 设置路由器到服务器
    printf("步骤 5: 设置路由器...\n");
    uvhttp_server_set_router(ctx.server, router);
    printf("✓ 路由器设置成功\n\n");
    
    // 步骤 6: 启动服务器监听
    printf("步骤 6: 启动服务器监听...\n");
    result = uvhttp_server_listen(ctx.server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "错误: 服务器启动失败 (错误码: %d)\n", result);
        uvhttp_server_free(ctx.server);
        return 1;
    }
    printf("✓ 服务器启动成功\n\n");
    
    // 打印服务器信息
    printf("========================================\n");
    printf("  服务器信息\n");
    printf("========================================\n");
    printf("  地址: http://0.0.0.0:8080\n");
    printf("  本地: http://localhost:8080\n");
    printf("  测试: curl http://localhost:8080/\n");
    printf("========================================\n\n");
    
    printf("服务器正在运行，按 Ctrl+C 停止...\n\n");
    
    // 步骤 7: 运行事件循环
    // UV_RUN_DEFAULT: 运行直到没有活动句柄
    uv_run(ctx.loop, UV_RUN_DEFAULT);
    
    // 清理资源（正常退出时）
    if (ctx.server) {
        printf("\n正在清理资源...\n");
        uvhttp_server_free(ctx.server);
        ctx.server = NULL;
    }
    
    printf("服务器已停止\n");
    return 0;
}
