#include "../include/uvhttp.h"
#include <stdio.h>
#include <unistd.h>

int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "OK", 2);
    uvhttp_response_send(response);
    return 0;
}

int main() {
    printf("Starting simple test...\n");
    fflush(stdout);
    
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        printf("Failed to get loop\n");
        return 1;
    }
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        printf("Failed to create server\n");
        return 1;
    }
    
    printf("Server created\n");
    fflush(stdout);
    
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", 7777);
    printf("Listen result: %d\n", result);
    fflush(stdout);
    
    if (result == UVHTTP_OK) {
        printf("Starting event loop for 3 seconds...\n");
        
        // 运行3秒后退出
        uv_timer_t timer;
        uv_timer_init(loop, &timer);
        uv_timer_start(&timer, (uv_timer_cb)uv_stop, 3000, 0);
        
        uv_run(loop, UV_RUN_DEFAULT);
        printf("Event loop stopped\n");
    } else {
        printf("Failed to listen: %d\n", result);
    }
    
    return 0;
}