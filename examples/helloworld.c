#include "../include/uvhttp.h"
#include <stdio.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, World!", 13);
    uvhttp_response_send(response);
    return 0;
}

int main() {
    printf("Hello World Server starting...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    
    server->router = router;
    
    if (uvhttp_server_listen(server, "0.0.0.0", 8080) != 0) {
        fprintf(stderr, "Failed to start server\n");
        return 1;
    }
    
    printf("Server running on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}