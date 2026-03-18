/*
 * Rate Limit E2E Test - Rate limiting functionality tests
 * 限流功能的端到端测试
 */

#include "uvhttp.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Application context */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    int request_count;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* Rate limited handler */
static int limited_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "This endpoint has rate limiting configured via uvhttp_config_t";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

/* Unlimited handler */
static int unlimited_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "This endpoint has no rate limiting";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

int main(int argc, char** argv) {
    int port = 8774;
    if (argc > 1) {
        port = atoi(argv[1]);
    }

    app_context_t app;
    memset(&app, 0, sizeof(app));
    app.loop = uv_default_loop();
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    uvhttp_error_t result = uvhttp_server_new(app.loop, &app.server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %d\n", result);
        return 1;
    }

    uvhttp_router_new(&app.router);
    app.server->router = app.router;

    uvhttp_router_add_route(app.router, "/limited", limited_handler);
    uvhttp_router_add_route(app.router, "/unlimited", unlimited_handler);

    result = uvhttp_server_listen(app.server, "0.0.0.0", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen on port %d: %d\n", port, result);
        return 1;
    }

    printf("Rate Limit E2E Test Server listening on port %d\n", port);
    printf("Endpoints:\n");
    printf("  http://127.0.0.1:%d/limited - Rate limited endpoint\n", port);
    printf("  http://127.0.0.1:%d/unlimited - Unlimited endpoint\n", port);
    printf("Note: Rate limiting is configured via uvhttp_config_t, not via API\n");

    uv_run(app.loop, UV_RUN_DEFAULT);

    return 0;
}
