#include "../include/uvhttp.h"
#include <stdio.h>
#include <stdlib.h>

void hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    printf("Method: %s\n", uvhttp_request_get_method(request));
    printf("URL: %s\n", uvhttp_request_get_url(request));
    
    const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
    if (user_agent) {
        printf("User-Agent: %s\n", user_agent);
    }
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    
    const char* body = 
        "<html>"
        "<head><title>UVHTTP Server</title></head>"
        "<body>"
        "<h1>Hello from UVHTTP!</h1>"
        "<p>This is a simple HTTP server built with libuv and llhttp.</p>"
        "<p>Method: ";
        
    const char* method = uvhttp_request_get_method(request);
    if (uvhttp_response_set_body(response, body, strlen(body)) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return;
    }
    
    // 注意：多次调用set_body会覆盖之前的内容，这里需要重新设计
    // 为了演示，我们使用简单的响应
    const char* simple_body = "<html><body><h1>Hello from UVHTTP!</h1></body></html>";
    if (uvhttp_response_set_body(response, simple_body, strlen(simple_body)) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return;
    }
    
    uvhttp_response_send(response);
}

void api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    
    const char* json = "{\"message\": \"Hello from API\", \"status\": \"ok\"}";
    if (uvhttp_response_set_body(response, json, strlen(json)) != 0) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "Internal Server Error", 21);
    }
    uvhttp_response_send(response);
}

int main() {
    printf("Starting UVHTTP server...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    // 创建路由
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    
    // 设置路由
    server->router = router;
    
    // 启动服务器
    int ret = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (ret != 0) {
        fprintf(stderr, "Failed to start server: %s\n", uv_strerror(ret));
        return 1;
    }
    
    printf("Server listening on http://0.0.0.0:8080\n");
    printf("Try visiting:\n");
    printf("  http://localhost:8080/\n");
    printf("  http://localhost:8080/api\n");
    
    // 运行事件循环
    uv_run(loop, UV_RUN_DEFAULT);
    
    // 清理
    uvhttp_server_free(server);
    uv_loop_close(loop);
    free(loop);
    
    return 0;
}