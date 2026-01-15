#include "../include/uvhttp.h"
#include <signal.h>
#include <stdlib.h>

static uvhttp_server_t* g_server = NULL;
static uvhttp_loop_t* g_loop = NULL;

void signal_handler(int sig) {
    (void)sig;
    if (g_server) {
        uvhttp_server_stop(g_server);
        uvhttp_server_free(g_server);
        g_server = NULL;
    }
    g_loop = NULL;
    exit(0);
}

int handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* body = "Hello World";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

int main() {
    fflush(stdout);
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    g_loop = uv_default_loop();
    
    fflush(stdout);
    g_server = uvhttp_server_new(g_loop);
    if (!g_server) {
        return 1;
    }
    fflush(stdout);
    
    uvhttp_server_set_handler(g_server, handler);
    
    fflush(stdout);
    int result = uvhttp_server_listen(g_server, "0.0.0.0", 8888);
    if (result != 0) {
        uvhttp_server_free(g_server);
        return 1;
    }
    fflush(stdout);
    
    uv_run(g_loop, UV_RUN_DEFAULT);
    
    uvhttp_server_free(g_server);
    
    return 0;
}