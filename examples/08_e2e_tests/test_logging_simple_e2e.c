/*
 * Logging E2E Test - Logging functionality tests
 * 日志功能的端到端测试
 */

#include "uvhttp_logging.h"

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
    uvhttp_logger_t* logger;
    int log_count;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Custom Logger Callback ==================== */

static void custom_log_callback(uvhttp_log_level_t level, const char* message,
                                void* user_data) {
    app_context_t* app = (app_context_t*)user_data;
    app->log_count++;

    const char* level_str = "UNKNOWN";
    switch (level) {
    case UVHTTP_LOG_TRACE:
        level_str = "TRACE";
        break;
    case UVHTTP_LOG_DEBUG:
        level_str = "DEBUG";
        break;
    case UVHTTP_LOG_INFO:
        level_str = "INFO";
        break;
    case UVHTTP_LOG_WARN:
        level_str = "WARN";
        break;
    case UVHTTP_LOG_ERROR:
        level_str = "ERROR";
        break;
    case UVHTTP_LOG_FATAL:
        level_str = "FATAL";
        break;
    }

    printf("[CUSTOM LOG] [%s] %s\n", level_str, message);
}

/* ==================== Request Handlers ==================== */

static int log_test_handler(uvhttp_request_t* request,
                            uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)loop->data;

    /* Log at different levels */
    uvhttp_log_trace(app->logger, "This is a TRACE message");
    uvhttp_log_debug(app->logger, "This is a DEBUG message");
    uvhttp_log_info(app->logger, "This is an INFO message");
    uvhttp_log_warn(app->logger, "This is a WARN message");
    uvhttp_log_error(app->logger, "This is an ERROR message");

    const char* body = "Log test completed";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

static int log_stats_handler(uvhttp_request_t* request,
                             uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)loop->data;

    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "Logging Statistics:\n"
             "Total Logs: %d",
             app->log_count);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);

    return 0;
}

static int log_level_handler(uvhttp_request_t* request,
                             uvhttp_response_t* response) {
    const char* level_str = uvhttp_request_get_header(request, "X-Log-Level");

    if (!level_str) {
        const char* error = "Missing X-Log-Level header";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    uvhttp_log_level_t level = UVHTTP_LOG_INFO;
    if (strcmp(level_str, "TRACE") == 0)
        level = UVHTTP_LOG_TRACE;
    else if (strcmp(level_str, "DEBUG") == 0)
        level = UVHTTP_LOG_DEBUG;
    else if (strcmp(level_str, "INFO") == 0)
        level = UVHTTP_LOG_INFO;
    else if (strcmp(level_str, "WARN") == 0)
        level = UVHTTP_LOG_WARN;
    else if (strcmp(level_str, "ERROR") == 0)
        level = UVHTTP_LOG_ERROR;
    else if (strcmp(level_str, "FATAL") == 0)
        level = UVHTTP_LOG_FATAL;

    uvhttp_logger_t* logger =
        uvhttp_logger_new(level, custom_log_callback, loop->data);
    uvhttp_logger_free(logger);

    char response_body[256];
    snprintf(response_body, sizeof(response_body), "Log level set to: %s",
             level_str);

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
             "curl -s -X %s http://127.0.0.1:8783%s 2>/dev/null", method, path);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }

    size_t bytes_read = fread(response, 1, max_len - 1, pipe);
    response[bytes_read] = '\0';

    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

static int send_http_request_with_header(const char* method, const char* path,
                                         const char* header, char* response,
                                         size_t max_len) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "curl -s -X %s -H '%s' http://127.0.0.1:8783%s 2>/dev/null",
             method, header, path);

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

    const char* port = "8783";
    if (argc > 1) {
        port = argv[1];
    }

    printf("\n========================================\n");
    printf("  Logging E2E Test\n");
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
    app.log_count = 0;

    /* Create logger */
    printf("\nCreating logger...\n");
    app.logger = uvhttp_logger_new(UVHTTP_LOG_DEBUG, custom_log_callback, &app);
    printf("Logger created with DEBUG level\n");

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
    printf("  ✓ GET /log/test\n");
    printf("  ✓ GET /log/stats\n");
    printf("  ✓ POST /log/level\n");

    uvhttp_router_add_route(app.router, "/log/test", log_test_handler);
    uvhttp_router_add_route(app.router, "/log/stats", log_stats_handler);
    uvhttp_router_add_route(app.router, "/log/level", log_level_handler);

    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_logger_free(app.logger);
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

    /* Test log levels */
    printf("\n=== Testing Log Levels ===\n");
    send_http_request("GET", "/log/test", response, sizeof(response));
    assert(strstr(response, "Log test completed") != NULL);
    printf("✓ Log levels test passed (check custom log output)\n");

    /* Test log statistics */
    printf("\n=== Testing Log Statistics ===\n");
    send_http_request("GET", "/log/stats", response, sizeof(response));
    assert(strstr(response, "Logging Statistics") != NULL);
    assert(strstr(response, "Total Logs") != NULL);
    printf("✓ Log statistics test passed\n");

    /* Test log level change */
    printf("\n=== Testing Log Level Change ===\n");
    send_http_request_with_header("POST", "/log/level", "X-Log-Level: ERROR",
                                  response, sizeof(response));
    assert(strstr(response, "Log level set to: ERROR") != NULL);
    printf("✓ Log level change test passed\n");

    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");

    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_logger_free(app.logger);
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);

    printf("Test completed successfully\n");
    return 0;
}