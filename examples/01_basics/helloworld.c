/**
 * @file helloworld.c
 * @brief UVHTTP Hello World - demonstrates a server with configuration and
 *        application context (request counting + config info in the response).
 *
 * This example shows the core API: creating a loop, config, context, server,
 * and router with the output-parameter style constructors, and wiring an
 * application context through the server.
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_allocator.h"
#include "../include/uvhttp_config.h"
#include "../include/uvhttp_context.h"
#include "../include/uvhttp_constants.h"
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>

/* Application context - carries app-wide state instead of loose globals. */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_config_t* config;
    uvhttp_context_t* uvhttp_ctx;
    int request_count;
} app_context_t;

/* File-scope pointer so handlers can reach the app context. The framework
 * creates the response object, so handlers only need request + response. */
static app_context_t* g_ctx = NULL;

/* Allocate and zero-initialize an application context. */
static app_context_t* app_context_new(uv_loop_t* loop) {
    (void)loop;  /* loop is stored on the server, not the context */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        return NULL;
    }
    memset(ctx, 0, sizeof(app_context_t));
    return ctx;
}

/* Release an application context. The router is owned by the server, so it is
 * freed together with the server. */
static void app_context_free(app_context_t* ctx) {
    if (ctx) {
        if (ctx->server) {
            uvhttp_server_free(ctx->server);
            ctx->server = NULL;
        }
        if (ctx->config) {
            uvhttp_config_free(ctx->config);
            ctx->config = NULL;
        }
        if (ctx->uvhttp_ctx) {
            uvhttp_context_destroy(ctx->uvhttp_ctx);
            ctx->uvhttp_ctx = NULL;
        }
        uvhttp_free(ctx);
    }
}

/* Signal handler for graceful shutdown. */
static void signal_handler(int sig) {
    printf("\nReceived signal %d, shutting down gracefully...\n", sig);

    uv_loop_t* loop = uv_default_loop();
    uv_stop(loop);

    printf("Cleanup completed. Exiting.\n");
    exit(0);
}

/* Hello World handler - increments the request counter and reports config. */
static int hello_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!request || !response) {
        return -1;
    }

    app_context_t* ctx = g_ctx;
    if (!ctx || !ctx->config) {
        uvhttp_response_set_status(response, 500);
        const char* err = "Internal Server Error";
        uvhttp_response_set_body(response, err, strlen(err));
        return uvhttp_response_send(response);
    }

    ctx->request_count++;

    const uvhttp_config_t* config = ctx->config;
    size_t active_connections = ctx->server ? ctx->server->active_connections : 0;

    char response_body[512];
    int written = snprintf(response_body, sizeof(response_body),
        "Hello, World!\n\n"
        "=== Server Configuration ===\n"
        "Max connections: %d\n"
        "Max requests per connection: %d\n"
        "Active connections: %zu\n"
        "Max body size: %zuMB\n"
        "Read buffer size: %zuKB\n"
        "===========================\n",
        config->max_connections,
        config->max_requests_per_connection,
        active_connections,
        config->max_body_size / (1024 * 1024),
        (size_t)config->read_buffer_size / 1024
    );

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain; charset=utf-8");

    if (written < 0 || written >= (int)sizeof(response_body)) {
        const char* simple = "Hello, World!\n";
        uvhttp_response_set_body(response, simple, strlen(simple));
    } else {
        uvhttp_response_set_body(response, response_body, (size_t)written);
    }

    printf("Request handled: %s %s (Active connections: %zu/%d)\n",
           uvhttp_method_to_string(request->method),
           request->url,
           active_connections,
           config->max_connections);

    return uvhttp_response_send(response);
}

int main() {
    printf("Hello World Server starting...\n");

    /* Register signal handlers. */
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* Create and load configuration. */
    printf("Loading configuration...\n");
    uvhttp_config_t* config = NULL;
    uvhttp_error_t result = uvhttp_config_new(&config);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create configuration: %s\n",
                uvhttp_error_string(result));
        return 1;
    }

    /* Apply defaults, then override a few values. */
    uvhttp_config_set_defaults(config);
    config->max_connections = 500;
    config->max_requests_per_connection = 100;
    config->backlog = 128;
    config->max_body_size = 1048576;  /* 1MB */
    config->read_buffer_size = 8192;
    config->keepalive_timeout = 30;
    config->request_timeout = 60;

    if (uvhttp_config_validate(config) != UVHTTP_OK) {
        fprintf(stderr, "Configuration validation failed\n");
        uvhttp_config_free(config);
        return 1;
    }

    printf("Configuration loaded successfully:\n");
    printf("  Max connections: %d\n", config->max_connections);
    printf("  Max requests per connection: %d\n",
           config->max_requests_per_connection);
    printf("  Max body size: %zu bytes\n", config->max_body_size);
    printf("  Read buffer size: %d bytes\n", config->read_buffer_size);

    /* Get the default loop. */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to get default loop\n");
        uvhttp_config_free(config);
        return 1;
    }
    printf("Default loop obtained: %p\n", (void*)loop);

    /* Create the application context. */
    printf("Creating application context...\n");
    app_context_t* ctx = app_context_new(loop);
    if (!ctx) {
        fprintf(stderr, "Failed to create application context\n");
        uvhttp_config_free(config);
        return 1;
    }
    ctx->config = config;
    g_ctx = ctx;
    printf("Application context created successfully: %p\n", (void*)ctx);

    /* Create the server (output-parameter style). */
    printf("Creating server...\n");
    uvhttp_error_t server_result = uvhttp_server_new(loop, &ctx->server);
    if (server_result != UVHTTP_OK || !ctx->server) {
        fprintf(stderr, "Failed to create server: %s\n",
                uvhttp_error_string(server_result));
        g_ctx = NULL;
        app_context_free(ctx);
        return 1;
    }

    /* Attach the application context and configuration to the server. */
    ctx->server->user_data = ctx;
    ctx->server->config = config;
    printf("Server created successfully: %p\n", (void*)ctx->server);

    /* Create the UVHTTP context and wire it into the server. */
    uvhttp_error_t ctx_result = uvhttp_context_create(loop, &ctx->uvhttp_ctx);
    if (ctx_result != UVHTTP_OK || !ctx->uvhttp_ctx) {
        fprintf(stderr, "Failed to create uvhttp context: %s\n",
                uvhttp_error_string(ctx_result));
        g_ctx = NULL;
        app_context_free(ctx);
        return 1;
    }

    /* Publish the config globally via the context (avoids the
     * "Global configuration not initialized" warning) and attach the
     * context to the server. */
    uvhttp_config_set_current(ctx->uvhttp_ctx, config);
    uvhttp_server_set_context(ctx->server, ctx->uvhttp_ctx);
    printf("UVHTTP context set to server\n");

    /* Create the router (output-parameter style). */
    printf("Creating router...\n");
    uvhttp_error_t router_result = uvhttp_router_new(&ctx->router);
    if (router_result != UVHTTP_OK || !ctx->router) {
        fprintf(stderr, "Failed to create router: %s\n",
                uvhttp_error_string(router_result));
        g_ctx = NULL;
        app_context_free(ctx);
        return 1;
    }
    printf("Router created successfully: %p\n", (void*)ctx->router);

    /* Add the route. */
    printf("Adding route...\n");
    int route_result = uvhttp_router_add_route(ctx->router, "/", hello_handler);
    if (route_result != UVHTTP_OK) {
        fprintf(stderr, "Failed to add route, error: %d\n", route_result);
        g_ctx = NULL;
        app_context_free(ctx);
        return 1;
    }
    printf("Route added successfully\n");

    /* Wire the router into the server via the setter. */
    uvhttp_server_set_router(ctx->server, ctx->router);
    printf("Router set to server\n");

    /* Start listening. */
    printf("Starting server listen on port %d...\n", UVHTTP_DEFAULT_PORT);
    result = uvhttp_server_listen(ctx->server, UVHTTP_DEFAULT_HOST,
                                  UVHTTP_DEFAULT_PORT);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server, error code: %d\n", result);
        g_ctx = NULL;
        app_context_free(ctx);
        return 1;
    }
    printf("Server listening on http://%s:%d\n",
           UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    printf("Server is running! Press Ctrl+C to stop.\n");

    /* Run the event loop. */
    uv_run(loop, UV_RUN_DEFAULT);
    printf("Event loop finished\n");

    /* Final cleanup. */
    printf("Performing final cleanup...\n");
    g_ctx = NULL;
    app_context_free(ctx);

    return 0;
}
