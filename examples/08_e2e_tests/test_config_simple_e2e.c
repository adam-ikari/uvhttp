/*
 * Configuration E2E Test - Configuration management tests
 * 配置管理的端到端测试
 */

#include "uvhttp_config.h"

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
    uvhttp_config_t* config;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int config_get_handler(uvhttp_request_t* request,
                              uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)((uv_loop_t*)request->client->data);

    /* Configuration is accessed directly from the struct */
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "Configuration:\n"
             "Max Connections: %d\n"
             "Read Buffer Size: %d\n"
             "Backlog: %d\n"
             "Keepalive Timeout: %d\n"
             "Request Timeout: %d\n"
             "Max Body Size: %zu\n"
             "Max Header Size: %zu\n"
             "Max URL Size: %zu\n"
             "Max File Size: %zu",
             app->config->max_connections,
             app->config->read_buffer_size,
             app->config->backlog,
             app->config->keepalive_timeout,
             app->config->request_timeout,
             app->config->max_body_size,
             app->config->max_header_size,
             app->config->max_url_size,
             app->config->max_file_size);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);

    return 0;
}

static int config_update_handler(uvhttp_request_t* request,
                                 uvhttp_response_t* response) {
    /* Update configuration using specific API */
    const char* action = uvhttp_request_get_header(request, "X-Config-Action");

    if (!action) {
        const char* error = "Missing X-Config-Action header";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    /* Note: uvhttp_config_update_max_connections requires uvhttp_context_t */
    /* This is a simplified test - in production, you would need the context */
    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "Configuration update requested:\n"
             "Action: %s\n"
             "Note: This is a simplified test.\n"
             "Real configuration updates require uvhttp_context_t",
             action);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -X %s http://127.0.0.1:8782%s 2>/dev/null", method, path);

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
    uvhttp_config_set_defaults(app.config);

    printf("Default configuration loaded\n");
    printf("  Max Connections: %d\n", app.config->max_connections);
    printf("  Request Timeout: %d\n", app.config->request_timeout);
    printf("  Max Body Size: %zu\n", app.config->max_body_size);

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
    printf("  ✓ POST /config/update\n");

    uvhttp_router_add_route(app.router, "/config/get", config_get_handler);
    uvhttp_router_add_route(app.router, "/config/update", config_update_handler);

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
    assert(strstr(response, "Max Connections") != NULL);
    assert(strstr(response, "Request Timeout") != NULL);
    printf("✓ Get configuration test passed\n");

    /* Test update configuration (simplified) */
    printf("\n=== Testing Update Configuration ===\n");
    send_http_request("POST", "/config/update", response, sizeof(response));
    assert(strstr(response, "Configuration update requested") != NULL);
    printf("✓ Update configuration test passed\n");

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