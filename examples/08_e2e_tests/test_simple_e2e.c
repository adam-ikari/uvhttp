/*
 * Simple E2E Test - Based on existing working tests
 * 简化的端到端测试 - 基于现有可工作的测试
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

/* ==================== Request Handlers ==================== */

static int get_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "GET method response";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int post_handler(uvhttp_request_t* request,
                        uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    size_t body_len = uvhttp_request_get_body_length(request);
    const char* body = uvhttp_request_get_body(request);

    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "POST method received %zu bytes: %.*s", body_len, (int)body_len,
             body ? body : "(empty)");

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);

    printf("POST %s - 200 OK\n", path);
    return 0;
}

static int put_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "PUT method response";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int delete_handler(uvhttp_request_t* request,
                          uvhttp_response_t* response) {
    (void)request;
    const char* body = "DELETE method response";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int json_handler(uvhttp_request_t* request,
                        uvhttp_response_t* response) {
    (void)request;
    const char* json_body =
        "{\"status\":\"success\",\"message\":\"JSON response\"}";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, json_body, strlen(json_body));
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -X %s http://127.0.0.1:8770%s 2>/dev/null", method, path);

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

    const char* port = "8770";
    if (argc > 1) {
        port = argv[1];
    }

    printf("\n========================================\n");
    printf("  Simple E2E Test\n");
    printf("  Port: %s\n", port);
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

    /* Add routes */
    printf("\nRegistering routes...\n");
    printf("  ✓ GET /\n");
    printf("  ✓ POST /\n");
    printf("  ✓ PUT /\n");
    printf("  ✓ DELETE /\n");
    printf("  ✓ /json\n");

    uvhttp_router_add_route(app.router, "/", get_handler);
    uvhttp_router_add_route_method(app.router, "/", UVHTTP_POST, post_handler);
    uvhttp_router_add_route_method(app.router, "/", UVHTTP_PUT, put_handler);
    uvhttp_router_add_route_method(app.router, "/", UVHTTP_DELETE,
                                   delete_handler);
    uvhttp_router_add_route(app.router, "/json", json_handler);

    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_router_free(app.router);
        uvhttp_server_free(app.server);
        return 1;
    }

    printf("Server started successfully\n");
    printf("Press Ctrl+C to stop the server\n\n");

    /* Run tests */
    sleep(1);  // Wait for server to be ready

    printf("Running tests...\n");

    char response[256];

    /* Test GET request */
    printf("\n=== Testing GET Request ===\n");
    send_http_request("GET", "/", response, sizeof(response));
    assert(strstr(response, "GET method") != NULL);
    printf("✓ GET request test passed\n");

    /* Test POST request */
    printf("\n=== Testing POST Request ===\n");
    send_http_request("POST", "/", response, sizeof(response));
    assert(strstr(response, "POST method") != NULL);
    printf("✓ POST request test passed\n");

    /* Test PUT request */
    printf("\n=== Testing PUT Request ===\n");
    send_http_request("PUT", "/", response, sizeof(response));
    assert(strstr(response, "PUT method") != NULL);
    printf("✓ PUT request test passed\n");

    /* Test DELETE request */
    printf("\n=== Testing DELETE Request ===\n");
    send_http_request("DELETE", "/", response, sizeof(response));
    assert(strstr(response, "DELETE method") != NULL);
    printf("✓ DELETE request test passed\n");

    /* Test JSON response */
    printf("\n=== Testing JSON Response ===\n");
    send_http_request("GET", "/json", response, sizeof(response));
    assert(strstr(response, "application/json") != NULL);
    assert(strstr(response, "\"status\":\"success\"") != NULL);
    printf("✓ JSON response test passed\n");

    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");

    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);

    printf("Test completed successfully\n");
    return 0;
}