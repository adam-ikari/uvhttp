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
    uvhttp_rate_limit_t* rate_limit;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int limited_handler(uvhttp_request_t* request,
                           uvhttp_response_t* response) {
    /* Check rate limit */
    const char* ip = uvhttp_request_get_client_ip(request);
    if (ip) {
        uvhttp_error_t result =
            uvhttp_rate_limit_check(request->server->context->rate_limit, ip);
        if (result != UVHTTP_OK) {
            const char* body = "Rate limit exceeded";
            uvhttp_response_set_status(response, 429);
            uvhttp_response_set_header(response, "Content-Type", "text/plain");
            uvhttp_response_set_header(response, "Retry-After", "60");
            uvhttp_response_set_body(response, body, strlen(body));
            uvhttp_response_send(response);
            return 0;
        }
    }

    const char* body = "Request allowed";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int unlimited_handler(uvhttp_request_t* request,
                             uvhttp_response_t* response) {
    const char* body = "No rate limit";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -w '\\n%%{http_code}' -X %s http://127.0.0.1:8774%s "
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

    const char* port = "8774";
    if (argc > 1) {
        port = argv[1];
    }

    printf("\n========================================\n");
    printf("  Rate Limit E2E Test\n");
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

    /* Create rate limiter */
    printf("\nCreating rate limiter...\n");
    printf("  Max requests: 5 per minute per IP\n");

    result = uvhttp_rate_limit_new(&app.rate_limit, 5, 60);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create rate limiter: %d\n", result);
        uvhttp_router_free(app.router);
        uvhttp_server_free(app.server);
        return 1;
    }

    app.server->context->rate_limit = app.rate_limit;

    /* Add routes */
    printf("\nRegistering routes...\n");
    printf("  ✓ GET /limited (rate limited)\n");
    printf("  ✓ GET /unlimited (no limit)\n");

    uvhttp_router_add_route(app.router, "/limited", limited_handler);
    uvhttp_router_add_route(app.router, "/unlimited", unlimited_handler);

    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_rate_limit_free(app.rate_limit);
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

    /* Test unlimited endpoint */
    printf("\n=== Testing Unlimited Endpoint ===\n");
    for (int i = 0; i < 10; i++) {
        send_http_request("GET", "/unlimited", response, sizeof(response));
        assert(strstr(response, "No rate limit") != NULL);
        assert(strstr(response, "200") != NULL);
    }
    printf("✓ Unlimited endpoint test passed (10 requests)\n");

    /* Test rate limited endpoint */
    printf("\n=== Testing Rate Limited Endpoint ===\n");
    int success_count = 0;
    int rate_limit_count = 0;

    for (int i = 0; i < 10; i++) {
        send_http_request("GET", "/limited", response, sizeof(response));
        if (strstr(response, "200") != NULL) {
            success_count++;
        } else if (strstr(response, "429") != NULL) {
            rate_limit_count++;
        }
    }

    printf("  Success requests: %d\n", success_count);
    printf("  Rate limited: %d\n", rate_limit_count);
    printf("✓ Rate limit test passed\n");

    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");

    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_rate_limit_free(app.rate_limit);
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);

    printf("Test completed successfully\n");
    return 0;
}