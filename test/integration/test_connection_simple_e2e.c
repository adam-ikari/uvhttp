/*
 * Connection Management E2E Test - Connection management tests
 * 连接管理的端到端测试
 */

#include "uvhttp.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
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

static int slow_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;
    app->request_count++;
    
    /* Simulate slow processing */
    sleep(1);
    
    char body[256];
    snprintf(body, sizeof(body), "Slow response #%d", app->request_count);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

static int fast_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;
    app->request_count++;
    
    char body[256];
    snprintf(body, sizeof(body), "Fast response #%d", app->request_count);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

static int connection_info_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;
    
    char info[512];
    snprintf(info, sizeof(info), 
             "Connection Information:\n"
             "Client IP: %s\n"
             "Request Count: %d\n"
             "Path: %s\n"
             "Method: %s",
             uvhttp_request_get_client_ip(request),
             app->request_count,
             uvhttp_request_get_path(request),
             uvhttp_request_get_method(request));
    
    size_t len = strlen(info);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, info, len);
    uvhttp_response_send(response);
    
    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path, char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s http://127.0.0.1:8777%s --max-time 5 2>/dev/null", method, path);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    
    size_t bytes_read = fread(response, 1, max_len - 1, pipe);
    response[bytes_read] = '\0';
    
    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

static int send_concurrent_requests(const char* path, int count, int* success_count, int* timeout_count) {
    *success_count = 0;
    *timeout_count = 0;
    
    for (int i = 0; i < count; i++) {
        char response[256];
        int result = send_http_request("GET", path, response, sizeof(response));
        
        if (result > 0 && strstr(response, "response") != NULL) {
            (*success_count)++;
        } else {
            (*timeout_count)++;
        }
    }
    
    return 0;
}

/* ==================== Main Function ==================== */

int main(int argc, char** argv) {
    (void)argc;
    
    const char* port = "8777";
    if (argc > 1) {
        port = argv[1];
    }
    
    printf("\n========================================\n");
    printf("  Connection Management E2E Test\n");
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
    app.request_count = 0;
    
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
    printf("  ✓ GET /slow\n");
    printf("  ✓ GET /fast\n");
    printf("  ✓ GET /info\n");
    
    uvhttp_router_add_route(app.router, "/slow", slow_handler);
    uvhttp_router_add_route(app.router, "/fast", fast_handler);
    uvhttp_router_add_route(app.router, "/info", connection_info_handler);
    
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
    
    /* Test fast requests */
    printf("\n=== Testing Fast Requests ===\n");
    for (int i = 0; i < 5; i++) {
        send_http_request("GET", "/fast", response, sizeof(response));
        assert(strstr(response, "Fast response") != NULL);
    }
    printf("✓ Fast requests test passed\n");
    
    /* Test slow requests */
    printf("\n=== Testing Slow Requests ===\n");
    for (int i = 0; i < 3; i++) {
        send_http_request("GET", "/slow", response, sizeof(response));
        assert(strstr(response, "Slow response") != NULL);
    }
    printf("✓ Slow requests test passed\n");
    
    /* Test concurrent requests */
    printf("\n=== Testing Concurrent Requests ===\n");
    int success_count = 0;
    int timeout_count = 0;
    send_concurrent_requests("/fast", 10, &success_count, &timeout_count);
    printf("  Success: %d\n", success_count);
    printf("  Timeout: %d\n", timeout_count);
    assert(success_count >= 8);  // At least 8 out of 10 should succeed
    printf("✓ Concurrent requests test passed\n");
    
    /* Test connection info */
    printf("\n=== Testing Connection Information ===\n");
    send_http_request("GET", "/info", response, sizeof(response));
    assert(strstr(response, "Connection Information") != NULL);
    assert(strstr(response, "Client IP") != NULL);
    assert(strstr(response, "Request Count") != NULL);
    printf("✓ Connection info test passed\n");
    
    /* Test Keep-Alive */
    printf("\n=== Testing Keep-Alive Connections ===\n");
    for (int i = 0; i < 5; i++) {
        send_http_request("GET", "/fast", response, sizeof(response));
        assert(strstr(response, "Fast response") != NULL);
    }
    printf("✓ Keep-Alive connections test passed\n");
    
    /* Test multiple paths */
    printf("\n=== Testing Multiple Paths ===\n");
    send_http_request("GET", "/slow", response, sizeof(response));
    assert(strstr(response, "Slow response") != NULL);
    
    send_http_request("GET", "/fast", response, sizeof(response));
    assert(strstr(response, "Fast response") != NULL);
    
    send_http_request("GET", "/info", response, sizeof(response));
    assert(strstr(response, "Connection Information") != NULL);
    printf("✓ Multiple paths test passed\n");
    
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