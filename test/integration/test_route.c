#include "include/uvhttp.h"
#include <signal.h>

static uvhttp_server_t* g_server = NULL;

void signal_handler(int sig) {
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
    }
    exit(0);
}

int test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_url(request);
    printf("Handler called with path: %s\n", path);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, "OK", 2);
    uvhttp_response_send(response);
    
    return 0;
}

int main() {
    signal(SIGINT, signal_handler);
    
    uv_loop_t* loop = uv_default_loop();
    g_server = uvhttp_server_new(loop);
    
    uvhttp_router_t* router = uvhttp_router_new();
    uvhttp_router_add_route(router, "/*", test_handler);
    g_server->router = router;
    
    uvhttp_server_listen(g_server, "0.0.0.0", 8080);
    
    printf("Server started on http://localhost:8080\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    return 0;
}
