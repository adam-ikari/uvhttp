#include "../include/uvhttp.h"
#include <stdio.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    // Handler called - remove debug output for production
    if (!request || !response) {
        return -1;
    }
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    
    uvhttp_response_send(response);
    return 0;
}

int main() {
    printf("Hello World Server starting...\n");
    
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to get default loop\n");
        return 1;
    }
    
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "Failed to create router\n");
        return 1;
    }
    
    if (uvhttp_router_add_route(router, "/", hello_handler) != UVHTTP_OK) {
        fprintf(stderr, "Failed to add route\n");
        return 1;
    }
    
    server->router = router;
    printf("Server and router initialized\n");
    
    uvhttp_error_t result = uvhttp_server_listen(server, "127.0.0.1", 9999);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server, error code: %d\n", result);
        return 1;
    }
    
    printf("Server listening on 127.0.0.1:9999\n");
    printf("Starting event loop...\n");
    uv_run(loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");
    
    return 0;
}