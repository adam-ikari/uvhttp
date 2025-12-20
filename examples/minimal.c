#include "../include/uvhttp.h"
#include <stdio.h>

int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* Simple text response */
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "Hello, UVHTTP!", 15);
    return UVHTTP_OK;
}

int api_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* JSON response example */
    uvhttp_response_json_success(response, "API call successful");
    return UVHTTP_OK;
}

int main() {
    printf("Starting UVHTTP minimal example server...\n");
    
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = uvhttp_server_new(loop);
    
    /* Create router */
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/api", api_handler);
    
    server->router = router;
    
    /* Start server */
    uvhttp_error_t ret = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (ret != UVHTTP_OK) {
        fprintf(stderr, "Start failed: %s\n", uvhttp_error_string(ret));
        return 1;
    }
    
    printf("Server running at: http://localhost:8080\n");
    printf("Access:\n");
    printf("  http://localhost:8080/     - Home\n");
    printf("  http://localhost:8080/api  - API\n");
    
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* Cleanup */
    uvhttp_server_free(server);
    uv_loop_close(loop);
    free(loop);
    
    return 0;
}