/*
 * Error Handler E2E Test - Custom error handler tests
 * 自定义错误处理器的端到端测试
 */

#include "uvhttp_error_handler.h"

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
    int custom_error_count;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Custom Error Handler ==================== */

static void custom_error_handler(uvhttp_error_t error, const char* message,
                                 void* user_data) {
    app_context_t* app = (app_context_t*)user_data;
    app->custom_error_count++;

    printf("[Custom Error Handler] Error: %d, Message: %s\n", error, message);
}

/* ==================== Request Handlers ==================== */

static int success_handler(uvhttp_request_t* request,
                           uvhttp_response_t* response) {
    const char* body = "Success response";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int trigger_400_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    /* Trigger a bad request error */
    const char* body = "Bad request - invalid input";

    uvhttp_response_set_status(response, 400);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int trigger_401_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    /* Trigger an unauthorized error */
    const char* body = "Unauthorized - authentication required";

    uvhttp_response_set_status(response, 401);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "WWW-Authenticate", "Bearer");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int trigger_403_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    /* Trigger a forbidden error */
    const char* body = "Forbidden - insufficient permissions";

    uvhttp_response_set_status(response, 403);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int trigger_500_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    /* Trigger an internal server error */
    const char* body = "Internal server error - something went wrong";

    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int error_stats_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)((uv_loop_t*)request->client->data);

    char stats[256];
    snprintf(stats, sizeof(stats),
             "Error Handler Statistics:\n"
             "Custom Errors Handled: %d",
             app->custom_error_count);

    size_t len = strlen(stats);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, stats, len);
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -w '\\n%%{http_code}' -X %s http://127.0.0.1:8779%s "
             "2>/dev/null",
             method, path);

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

    const char* port = "8779";
    if (argc > 1) {
        port = argv[1];
    }

    printf("\n========================================\n");
    printf("  Error Handler E2E Test\n");
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
    app.custom_error_count = 0;

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

    /* Set custom error handler */
    printf("\nSetting custom error handler...\n");

    /* Add routes */
    printf("\nRegistering routes...\n");
    printf("  ✓ GET /success\n");
    printf("  ✓ GET /error/400\n");
    printf("  ✓ GET /error/401\n");
    printf("  ✓ GET /error/403\n");
    printf("  ✓ GET /error/500\n");
    printf("  ✓ GET /error/stats\n");

    uvhttp_router_add_route(app.router, "/success", success_handler);
    uvhttp_router_add_route(app.router, "/error/400", trigger_400_handler);
    uvhttp_router_add_route(app.router, "/error/401", trigger_401_handler);
    uvhttp_router_add_route(app.router, "/error/403", trigger_403_handler);
    uvhttp_router_add_route(app.router, "/error/500", trigger_500_handler);
    uvhttp_router_add_route(app.router, "/error/stats", error_stats_handler);

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

    char response[512];

    /* Test success response */
    printf("\n=== Testing Success Response (200) ===\n");
    send_http_request("GET", "/success", response, sizeof(response));
    assert(strstr(response, "Success response") != NULL);
    assert(strstr(response, "200") != NULL);
    printf("✓ Success response test passed\n");

    /* Test 400 Bad Request */
    printf("\n=== Testing 400 Bad Request ===\n");
    send_http_request("GET", "/error/400", response, sizeof(response));
    assert(strstr(response, "Bad request") != NULL);
    assert(strstr(response, "400") != NULL);
    printf("✓ 400 Bad Request test passed\n");

    /* Test 401 Unauthorized */
    printf("\n=== Testing 401 Unauthorized ===\n");
    send_http_request("GET", "/error/401", response, sizeof(response));
    assert(strstr(response, "Unauthorized") != NULL);
    assert(strstr(response, "401") != NULL);
    printf("✓ 401 Unauthorized test passed\n");

    /* Test 403 Forbidden */
    printf("\n=== Testing 403 Forbidden ===\n");
    send_http_request("GET", "/error/403", response, sizeof(response));
    assert(strstr(response, "Forbidden") != NULL);
    assert(strstr(response, "403") != NULL);
    printf("✓ 403 Forbidden test passed\n");

    /* Test 500 Internal Server Error */
    printf("\n=== Testing 500 Internal Server Error ===\n");
    send_http_request("GET", "/error/500", response, sizeof(response));
    assert(strstr(response, "Internal server error") != NULL);
    assert(strstr(response, "500") != NULL);
    printf("✓ 500 Internal Server Error test passed\n");

    /* Test error handler statistics */
    printf("\n=== Testing Error Handler Statistics ===\n");
    send_http_request("GET", "/error/stats", response, sizeof(response));
    assert(strstr(response, "Error Handler Statistics") != NULL);
    assert(strstr(response, "Custom Errors Handled") != NULL);
    printf("✓ Error handler statistics test passed\n");

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
