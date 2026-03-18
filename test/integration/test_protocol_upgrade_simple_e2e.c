/*
 * Protocol Upgrade E2E Test - Protocol upgrade functionality tests
 * 协议升级功能的端到端测试
 */

#include "uvhttp.h"
#include "uvhttp_protocol_upgrade.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Application context */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int http_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* body = "HTTP/1.1 response";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

static int upgrade_check_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* Check if protocol upgrade is requested */
    const char* upgrade_header = uvhttp_request_get_header(request, "Upgrade");
    const char* connection_header = uvhttp_request_get_header(request, "Connection");
    
    char response_body[512];
    
    if (upgrade_header && connection_header) {
        snprintf(response_body, sizeof(response_body), 
                 "Protocol Upgrade Detected:\n"
                 "Upgrade: %s\n"
                 "Connection: %s\n"
                 "Upgrade Supported: Yes",
                 upgrade_header,
                 connection_header);
    } else {
        snprintf(response_body, sizeof(response_body), 
                 "No Protocol Upgrade Requested:\n"
                 "Upgrade Header: %s\n"
                 "Connection Header: %s",
                 upgrade_header ? upgrade_header : "Not present",
                 connection_header ? connection_header : "Not present");
    }
    
    size_t len = strlen(response_body);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, len);
    uvhttp_response_send(response);
    
    return 0;
}

static int websocket_upgrade_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* Check for WebSocket upgrade request */
    const char* upgrade_header = uvhttp_request_get_header(request, "Upgrade");
    
    if (upgrade_header && strstr(upgrade_header, "websocket")) {
        /* Simulate WebSocket upgrade */
        const char* body = "WebSocket upgrade requested (101 Switching Protocols)";
        
        uvhttp_response_set_status(response, 101);
        uvhttp_response_set_header(response, "Upgrade", "websocket");
        uvhttp_response_set_header(response, "Connection", "Upgrade");
        uvhttp_response_set_header(response, "Sec-WebSocket-Accept", "simulated");
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, body, strlen(body));
        uvhttp_response_send(response);
    } else {
        const char* body = "WebSocket upgrade required";
        
        uvhttp_response_set_status(response, 426);
        uvhttp_response_set_header(response, "Upgrade", "websocket");
        uvhttp_response_set_header(response, "Connection", "Upgrade");
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, body, strlen(body));
        uvhttp_response_send(response);
    }
    
    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path, char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s http://127.0.0.1:8778%s 2>/dev/null", method, path);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    
    size_t bytes_read = fread(response, 1, max_len - 1, pipe);
    response[bytes_read] = '\0';
    
    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

static int send_http_request_with_headers(const char* method, const char* path, const char* headers, char* response, size_t max_len) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s %s http://127.0.0.1:8778%s 2>/dev/null", method, headers, path);
    
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
    
    const char* port = "8778";
    if (argc > 1) {
        port = argv[1];
    }
    
    printf("\n========================================\n");
    printf("  Protocol Upgrade E2E Test\n");
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
    printf("  ✓ GET /upgrade-check\n");
    printf("  ✓ GET /websocket\n");
    
    uvhttp_router_add_route(app.router, "/", http_handler);
    uvhttp_router_add_route(app.router, "/upgrade-check", upgrade_check_handler);
    uvhttp_router_add_route(app.router, "/websocket", websocket_upgrade_handler);
    
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
    
    char response[1024];
    
    /* Test regular HTTP request */
    printf("\n=== Testing Regular HTTP Request ===\n");
    send_http_request("GET", "/", response, sizeof(response));
    assert(strstr(response, "HTTP/1.1 response") != NULL);
    printf("✓ Regular HTTP request test passed\n");
    
    /* Test upgrade check without upgrade headers */
    printf("\n=== Testing Upgrade Check (No Headers) ===\n");
    send_http_request("GET", "/upgrade-check", response, sizeof(response));
    assert(strstr(response, "No Protocol Upgrade Requested") != NULL);
    printf("✓ Upgrade check without headers test passed\n");
    
    /* Test upgrade check with WebSocket headers */
    printf("\n=== Testing Upgrade Check (WebSocket Headers) ===\n");
    send_http_request_with_headers("GET", "/upgrade-check", 
                                   "-H 'Upgrade: websocket' -H 'Connection: Upgrade'",
                                   response, sizeof(response));
    assert(strstr(response, "Protocol Upgrade Detected") != NULL);
    assert(strstr(response, "websocket") != NULL);
    printf("✓ Upgrade check with WebSocket headers test passed\n");
    
    /* Test WebSocket upgrade request */
    printf("\n=== Testing WebSocket Upgrade ===\n");
    send_http_request_with_headers("GET", "/websocket", 
                                   "-H 'Upgrade: websocket' -H 'Connection: Upgrade' -H 'Sec-WebSocket-Key: test'",
                                   response, sizeof(response));
    assert(strstr(response, "WebSocket upgrade requested") != NULL);
    assert(strstr(response, "101 Switching Protocols") != NULL);
    printf("✓ WebSocket upgrade test passed\n");
    
    /* Test WebSocket upgrade without proper headers */
    printf("\n=== Testing WebSocket Upgrade (Missing Headers) ===\n");
    send_http_request("GET", "/websocket", response, sizeof(response));
    assert(strstr(response, "WebSocket upgrade required") != NULL);
    assert(strstr(response, "426") != NULL);
    printf("✓ WebSocket upgrade missing headers test passed\n");
    
    /* Test HTTP/2 upgrade (if supported) */
    printf("\n=== Testing HTTP/2 Upgrade ===\n");
    send_http_request_with_headers("GET", "/upgrade-check", 
                                   "-H 'Upgrade: h2c' -H 'Connection: Upgrade'",
                                   response, sizeof(response));
    assert(strstr(response, "Protocol Upgrade Detected") != NULL);
    assert(strstr(response, "h2c") != NULL);
    printf("✓ HTTP/2 upgrade test passed\n");
    
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