/*
 * Logging E2E Test - Logging functionality tests
 * 日志功能的端到端测试
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

/* Logging demo handler */
static int logging_demo_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    /* Logging macros are only available in Debug mode */
    #ifndef NDEBUG
    UVHTTP_LOG_TRACE("This is a TRACE message");
    UVHTTP_LOG_DEBUG("This is a DEBUG message");
    UVHTTP_LOG_INFO("This is an INFO message");
    UVHTTP_LOG_WARN("This is a WARN message");
    UVHTTP_LOG_ERROR("This is an ERROR message");
    UVHTTP_LOG_FATAL("This is a FATAL message");
    #endif
    
    const char* body = "Logging macros are only available in Debug mode.\n"
                      "They use stderr for output.\n"
                      "See uvhttp_logging.h for details.";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

/* Log level info handler */
static int log_level_info_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    
    char body[512];
    snprintf(body, sizeof(body),
             "UVHTTP Logging System\n"
             "==================\n\n"
             "Log Levels:\n"
             "  UVHTTP_LOG_LEVEL_TRACE (0) - Most detailed\n"
             "  UVHTTP_LOG_LEVEL_DEBUG (1) - Debug information\n"
             "  UVHTTP_LOG_LEVEL_INFO  (2) - General information\n"
             "  UVHTTP_LOG_LEVEL_WARN  (3) - Warnings\n"
             "  UVHTTP_LOG_LEVEL_ERROR (4) - Errors\n"
             "  UVHTTP_LOG_LEVEL_FATAL (5) - Fatal errors\n\n"
             "Log Macros:\n"
             "  UVHTTP_LOG(level, ...)\n"
             "  UVHTTP_LOG_TRACE(...)\n"
             "  UVHTTP_LOG_DEBUG(...)\n"
             "  UVHTTP_LOG_INFO(...)\n"
             "  UVHTTP_LOG_WARN(...)\n"
             "  UVHTTP_LOG_ERROR(...)\n"
             "  UVHTTP_LOG_FATAL(...)\n\n"
             "Note: Logging is only available in Debug mode.\n"
             "        In Release mode, all log macros are no-ops.\n"
             "        Use UVHTTP_FEATURE_LOGGING to enable.");
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

int main(int argc, char** argv) {
    int port = 8783;
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

    uvhttp_router_add_route(app.router, "/demo", logging_demo_handler);
    uvhttp_router_add_route(app.router, "/info", log_level_info_handler);

    result = uvhttp_server_listen(app.server, "0.0.0.0", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen on port %d: %d\n", port, result);
        return 1;
    }

    printf("Logging E2E Test Server listening on port %d\n", port);
    printf("Endpoints:\n");
    printf("  http://127.0.0.1:%d/demo - Logging demo\n", port);
    printf("  http://127.0.0.1:%d/info - Log level information\n", port);
    printf("Note: Logging uses macros (UVHTTP_LOG_TRACE, etc.)\n");
    printf("      They are only available in Debug mode.\n");

    uv_run(app.loop, UV_RUN_DEFAULT);

    return 0;
}
