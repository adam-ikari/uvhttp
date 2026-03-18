/*
 * Configuration E2E Test - Configuration management tests
 * 配置管理的端到端测试
 */

#include "uvhttp.h"
#include "uvhttp_config.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Application context */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    uvhttp_config_t* config;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int config_get_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "Configuration:\n"
             "Server Port: %d\n"
             "Worker Threads: %d\n"
             "Max Connections: %d\n"
             "Request Timeout: %d\n"
             "Keep-Alive: %s",
             uvhttp_config_get_int(app->config, "server_port", 8080),
             uvhttp_config_get_int(app->config, "worker_threads", 1),
             uvhttp_config_get_int(app->config, "max_connections", 1000),
             uvhttp_config_get_int(app->config, "request_timeout", 30),
             uvhttp_config_get_bool(app->config, "keep_alive", 1) ? "Enabled" : "Disabled");
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

static int config_set_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;
    
    const char* key = uvhttp_request_get_header(request, "X-Config-Key");
    const char* value = uvhttp_request_get_header(request, "X-Config-Value");
    
    if (!key || !value) {
        const char* error = "Missing X-Config-Key or X-Config-Value header";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }
    
    uvhttp_config_set(app->config, key, value);
    
    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "Configuration updated:\n"
             "Key: %s\n"
             "Value: %s",
             key, value);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

static int config_reload_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;
    
    /* Reload configuration */
    uvhttp_config_reload(app->config);
    
    const char* body = "Configuration reloaded successfully";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    
    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path, char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s http://127.0.0.1:8782%s 2>/dev/null", method, path);
    
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
    snprintf(cmd, sizeof(cmd), "curl -s -X %s %s http://127.0.0.1:8782%s 2>/dev/null", method, headers, path);
    
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
    
    const char* port = "8782";
    if (argc > 1) {
        port = argv[1];
    }
    
    printf("\n========================================\n");
    printf("  Configuration E2E Test\n");
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
    
    /* Create configuration */
    printf("\nCreating configuration...\n");
    uvhttp_config_new(&app.config);
    
    /* Set default configuration values */
    uvhttp_config_set(app.config, "server_port", port);
    uvhttp_config_set(app.config, "worker_threads", "1");
    uvhttp_config_set(app.config, "max_connections", "1000");
    uvhttp_config_set(app.config, "request_timeout", "30");
    uvhttp_config_set(app.config, "keep_alive", "1");
    
    printf("Default configuration loaded\n");
    
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
    printf("  ✓ GET /config/get\n");
    printf("  ✓ POST /config/set\n");
    printf("  ✓ POST /config/reload\n");
    
    uvhttp_router_add_route(app.router, "/config/get", config_get_handler);
    uvhttp_router_add_route(app.router, "/config/set", config_set_handler);
    uvhttp_router_add_route(app.router, "/config/reload", config_reload_handler);
    
    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_config_free(app.config);
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
    
    /* Test get configuration */
    printf("\n=== Testing Get Configuration ===\n");
    send_http_request("GET", "/config/get", response, sizeof(response));
    assert(strstr(response, "Configuration") != NULL);
    assert(strstr(response, "Server Port") != NULL);
    printf("✓ Get configuration test passed\n");
    
    /* Test set configuration */
    printf("\n=== Testing Set Configuration ===\n");
    send_http_request_with_headers("POST", "/config/set", 
                                   "-H 'X-Config-Key: test_key' -H 'X-Config-Value: test_value'",
                                   response, sizeof(response));
    assert(strstr(response, "Configuration updated") != NULL);
    assert(strstr(response, "test_key") != NULL);
    printf("✓ Set configuration test passed\n");
    
    /* Test reload configuration */
    printf("\n=== Testing Reload Configuration ===\n");
    send_http_request("POST", "/config/reload", response, sizeof(response));
    assert(strstr(response, "Configuration reloaded successfully") != NULL);
    printf("✓ Reload configuration test passed\n");
    
    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");
    
    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_config_free(app.config);
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);
    
    printf("Test completed successfully\n");
    return 0;
}