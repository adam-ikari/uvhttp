/*
 * UVHTTP 性能测试专用服务器
 * 用于性能基准测试，无调试输出，最小化响应体
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include <signal.h>
#include <stdlib.h>

// 全局变量
static uvhttp_server_t* g_server = NULL;
static uvhttp_router_t* g_router = NULL;
static uv_loop_t* g_loop = NULL;
static uvhttp_context_t* g_context = NULL;

// 信号处理器
void signal_handler(int sig) {
    (void)sig;  // 未使用参数
    
    if (g_server) {
        uvhttp_server_stop(g_server);
        // 注意：config 由 g_server 管理，不需要单独释放
        uvhttp_server_free(g_server);
        g_server = NULL;
        g_router = NULL;
    }
    
    g_loop = NULL;
    exit(0);
}

// 最小化响应处理器（用于基准测试）
int minimal_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    // 最小化响应体
    const char* response_body = "OK";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, 2);
    uvhttp_response_send(response);
    
    return 0;
}

// 小响应处理器（1KB）
int small_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    static const char response_body[1024] = "OK";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, 1024);
    uvhttp_response_send(response);
    
    return 0;
}

// 中等响应处理器（10KB）
int medium_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    static const char response_body[10240] = "OK";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, 10240);
    uvhttp_response_send(response);
    
    return 0;
}

// 大响应处理器（100KB）
int large_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    static const char response_body[102400] = "OK";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, 102400);
    uvhttp_response_send(response);
    
    return 0;
}

// POST 请求处理器
int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "OK";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, 2);
    uvhttp_response_send(response);
    
    return 0;
}

// API 处理器
int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"status\":\"ok\"}";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 16);
    uvhttp_response_send(response);
    
    return 0;
}

// PUT 请求处理器
int put_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"status\":\"updated\"}";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 20);
    uvhttp_response_send(response);
    
    return 0;
}

// DELETE 请求处理器
int delete_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"status\":\"deleted\"}";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 21);
    uvhttp_response_send(response);
    
    return 0;
}

// PATCH 请求处理器
int patch_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"status\":\"patched\"}";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 21);
    uvhttp_response_send(response);
    
    return 0;
}

// HEAD 请求处理器
int head_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    // HEAD 请求只返回头，不返回体
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "2");
    uvhttp_response_send(response);
    
    return 0;
}

// OPTIONS 请求处理器
int options_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    // CORS 预检请求
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Access-Control-Allow-Origin", "*");
    uvhttp_response_set_header(response, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, PATCH, HEAD, OPTIONS");
    uvhttp_response_set_header(response, "Access-Control-Allow-Headers", "Content-Type");
    uvhttp_response_set_header(response, "Content-Length", "0");
    uvhttp_response_send(response);
    
    return 0;
}

// 404 错误处理器
int not_found_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"error\":\"Not Found\"}";
    
    uvhttp_response_set_status(response, 404);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 23);
    uvhttp_response_send(response);
    
    return 0;
}

// 400 错误处理器
int bad_request_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"error\":\"Bad Request\"}";
    
    uvhttp_response_set_status(response, 400);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 25);
    uvhttp_response_send(response);
    
    return 0;
}

// 500 错误处理器
int server_error_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"error\":\"Internal Server Error\"}";
    
    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, response_body, 33);
    uvhttp_response_send(response);
    
    return 0;
}

// 429 限流错误处理器
int rate_limit_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }
    
    const char* response_body = "{\"error\":\"Too Many Requests\"}";
    
    uvhttp_response_set_status(response, 429);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Retry-After", "60");
    uvhttp_response_set_body(response, response_body, 29);
    uvhttp_response_send(response);
    
    return 0;
}

int main() {
    // 注册信号处理器
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 创建配置
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n", uvhttp_error_string(result));
        return 1;
    }
    if (!config) {
        return 1;
    }
    
    // 设置默认值（优化高并发）
    config->max_connections = 10000;
    config->max_requests_per_connection = 1000;
    config->backlog = 8192;  // 增加连接队列
    config->max_body_size = 10485760;  // 10MB
    config->read_buffer_size = 32768;  // 32KB，增加读取缓冲区
    config->keepalive_timeout = 60;
    config->request_timeout = 120;
    
    // 验证配置
    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 获取默认循环
    g_loop = uv_default_loop();
    if (!g_loop) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 创建服务器
    uvhttp_error_t server_result = uvhttp_server_new(g_loop, &g_server);
    if (server_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(server_result));
        return 1;
    }
    if (!g_server) {
        uvhttp_config_free(config);
        return 1;
    }
    
    // 应用配置（注意：config 的所有权转移给 server，不要单独释放）
    g_server->config = config;

    // 创建上下文
    uvhttp_error_t result_g_context = uvhttp_context_create(g_loop, &g_context);
    if (result_g_context != UVHTTP_OK) {
        uvhttp_server_free(g_server);
        g_server = NULL;
        return 1;
    }

    uvhttp_config_set_current(g_context, config);
    config = NULL;  // 防止后续误用
    
    // 创建路由器
    uvhttp_error_t router_result = uvhttp_router_new(&g_router);
    if (router_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(router_result));
        uvhttp_server_free(g_server);
        g_server = NULL;
        return 1;
    }
    
    // 添加路由
    uvhttp_router_add_route(g_router, "/", minimal_handler);
    uvhttp_router_add_route(g_router, "/small", small_handler);
    uvhttp_router_add_route(g_router, "/medium", medium_handler);
    uvhttp_router_add_route(g_router, "/large", large_handler);
    uvhttp_router_add_route(g_router, "/api", api_handler);
    
    // 完整 HTTP 方法测试
    uvhttp_router_add_route(g_router, "/api/put", put_handler);
    uvhttp_router_add_route(g_router, "/api/delete", delete_handler);
    uvhttp_router_add_route(g_router, "/api/patch", patch_handler);
    uvhttp_router_add_route(g_router, "/api/head", head_handler);
    uvhttp_router_add_route(g_router, "/api/options", options_handler);
    
    // 错误处理测试
    uvhttp_router_add_route(g_router, "/error/notfound", not_found_handler);
    uvhttp_router_add_route(g_router, "/error/badrequest", bad_request_handler);
    uvhttp_router_add_route(g_router, "/error/server", server_error_handler);
    uvhttp_router_add_route(g_router, "/error/ratelimit", rate_limit_handler);
    
    // 设置路由器
    g_server->router = g_router;
    
    // 启动服务器
    uvhttp_error_t listen_result = uvhttp_server_listen(g_server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    if (listen_result != UVHTTP_OK) {
        uvhttp_server_free(g_server);
        g_server = NULL;
        return 1;
    }
    
    // 运行事件循环
    uv_run(g_loop, UV_RUN_DEFAULT);

    // 清理上下文
    if (g_context) {
        uvhttp_context_destroy(g_context);
    }

    return 0;
}