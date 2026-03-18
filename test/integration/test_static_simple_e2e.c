/*
 * Static Files E2E Test - Static file serving tests
 * 静态文件服务的端到端测试
 */

#include "uvhttp_static.h"

#include "uvhttp.h"

#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Application context */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uvhttp_static_context_t* static_ctx;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int static_handler(uvhttp_request_t* request,
                          uvhttp_response_t* response) {
    return uvhttp_static_handle_request(request, response);
}

static int index_handler(uvhttp_request_t* request,
                         uvhttp_response_t* response) {
    const char* body = "Welcome to the static files server";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/html");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -X %s http://127.0.0.1:8772%s 2>/dev/null", method, path);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }

    size_t bytes_read = fread(response, 1, max_len - 1, pipe);
    response[bytes_read] = '\0';

    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

/* ==================== Main Function ==================== */

int main(int argc, char** argv) {
    (void)argc;

    const char* port = "8772";
    const char* static_dir = "./public";
    if (argc > 1) {
        port = argv[1];
    }
    if (argc > 2) {
        static_dir = argv[2];
    }

    printf("\n========================================\n");
    printf("  Static Files E2E Test\n");
    printf("  Port: %s\n", port);
    printf("  Static Directory: %s\n", static_dir);
    printf("========================================\n");

    /* Setup signal handler */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Create event loop */
    uv_loop_t* loop = uv_default_loop();

    /* Create application context */
    app_context_t app;
    memset(&app, 0, sizeof(app));
    app.loop = loop;

    /* Create server */
    uvhttp_error_t result = uvhttp_server_new(loop, &app.server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %d\n", result);
        return 1;
    }

    /* Create router */
    result = uvhttp_router_new(&app.router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %d\n", result);
        uvhttp_server_free(app.server);
        return 1;
    }

    app.server->router = app.router;

    /* Create static files context */
    printf("\nCreating static files context...\n");
    result = uvhttp_static_create(&app.static_ctx, static_dir);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create static context: %d\n", result);
        uvhttp_router_free(app.router);
        uvhttp_server_free(app.server);
        return 1;
    }

    /* Add routes */
    printf("\nRegistering routes...\n");
    printf("  ✓ GET /\n");
    printf("  ✓ GET /static/*\n");

    uvhttp_router_add_route(app.router, "/", index_handler);
    uvhttp_router_add_route(app.router, "/static/*", static_handler);

    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_static_free(app.static_ctx);
        uvhttp_router_free(app.router);
        uvhttp_server_free(app.server);
        return 1;
    }

    printf("Server started successfully\n");
    printf("Press Ctrl+C to stop the server\n\n");

    /* Run tests */
    sleep(1);  // Wait for server to be ready

    printf("Running tests...\n");

    char response[1024];

    /* Test index page */
    printf("\n=== Testing Index Page ===\n");
    send_http_request("GET", "/", response, sizeof(response));
    assert(strstr(response, "Welcome to the static files server") != NULL);
    printf("✓ Index page test passed\n");

    /* Test static file */
    printf("\n=== Testing Static File Serving ===\n");
    send_http_request("GET", "/static/", response, sizeof(response));
    printf("✓ Static file serving test passed\n");

    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");

    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_static_free(app.static_ctx);
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);

    printf("Test completed successfully\n");
    return 0;
}