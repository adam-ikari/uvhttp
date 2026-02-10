/*
 * Automated TLS/HTTPS end-to-end test
 */

#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_context.h"
#include <signal.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* Application context */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_tls_context_t* tls_ctx;
    volatile sig_atomic_t running;
    uv_async_t async_handle;  /* Async handle for stopping the loop */
} app_context_t;

static app_context_t* g_app_context = NULL;

void signal_handler(int sig) {
    fprintf(stderr, "\n[DEBUG] Received signal %d\n", sig);
    fflush(stderr);
    if (g_app_context) {
        fprintf(stderr, "[DEBUG] Setting running to 0\n");
        fflush(stderr);
        g_app_context->running = 0;
        /* Stop the event loop immediately */
        uv_stop(g_app_context->server->loop);
    }
    fprintf(stderr, "[DEBUG] Signal handler completed\n");
    fflush(stderr);
}

/* Async callback to stop the loop */
void async_stop_callback(uv_async_t* handle) {
    (void)handle;  /* unused parameter */
    fprintf(stderr, "[DEBUG] Async stop callback called\n");
    fflush(stderr);
    /* Stop the server to close listening socket */
    if (g_app_context && g_app_context->server) {
        fprintf(stderr, "[DEBUG] Stopping server\n");
        fflush(stderr);
        uvhttp_server_stop(g_app_context->server);
    }
}

/* Simple handler */
static int simple_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "{\"status\":\"ok\",\"message\":\"TLS test endpoint\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

int main(int argc, char** argv) {
    /* Check if logging is enabled */
#ifdef UVHTTP_LOGGING_ENABLED
    printf("LOGGING IS ENABLED\n");
#else
    printf("LOGGING IS DISABLED\n");
#endif
    
    const char* host = "127.0.0.1";
    int port = 8443;
    const char* cert_dir = "../test/certs";
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            return 1;
        }
    }
    
    if (argc > 2) {
        cert_dir = argv[2];
    }
    
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    uvhttp_error_t result;
    
    uv_loop_t* loop = uv_default_loop();
    
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->running = 1;
    g_app_context = ctx;
    
    /* Create server first */
    result = uvhttp_server_new(loop, &ctx->server);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* Create and set context for TLS */
    struct uvhttp_context* server_ctx = uvhttp_alloc(sizeof(struct uvhttp_context));
    if (!server_ctx) {
        fprintf(stderr, "Failed to allocate context\n");
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    memset(server_ctx, 0, sizeof(struct uvhttp_context));
    result = uvhttp_server_set_context(ctx->server, server_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to set context: %s\n", uvhttp_error_string(result));
        uvhttp_free(server_ctx);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* Initialize TLS with server context */
    result = uvhttp_tls_init(ctx->server->context);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to initialize TLS: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* Create TLS context */
    result = uvhttp_tls_context_new(&ctx->tls_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create TLS context: %s\n", uvhttp_error_string(result));
        uvhttp_free(ctx);
        return 1;
    }
    
    /* Load certificates */
    char cert_path[512];
    char key_path[512];
    char ca_path[512];
    
    snprintf(cert_path, sizeof(cert_path), "%s/server.crt", cert_dir);
    snprintf(key_path, sizeof(key_path), "%s/server.key", cert_dir);
    snprintf(ca_path, sizeof(ca_path), "%s/ca.crt", cert_dir);
    
    result = uvhttp_tls_context_load_cert_chain(ctx->tls_ctx, cert_path);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to load certificate: %s\n", uvhttp_error_string(result));
        uvhttp_tls_context_free(ctx->tls_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    result = uvhttp_tls_context_load_private_key(ctx->tls_ctx, key_path);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to load private key: %s\n", uvhttp_error_string(result));
        uvhttp_tls_context_free(ctx->tls_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    result = uvhttp_tls_context_load_ca_file(ctx->tls_ctx, ca_path);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to load CA certificate: %s\n", uvhttp_error_string(result));
        uvhttp_tls_context_free(ctx->tls_ctx);
        uvhttp_server_free(ctx->server);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* Enable TLS on server */
    result = uvhttp_server_enable_tls(ctx->server, ctx->tls_ctx);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to enable TLS: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_tls_context_free(ctx->tls_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    /* Create router */
    result = uvhttp_router_new(&ctx->router);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(ctx->server);
        uvhttp_tls_context_free(ctx->tls_ctx);
        uvhttp_free(ctx);
        return 1;
    }
    
    uvhttp_router_add_route(ctx->router, "/", simple_handler);
    uvhttp_router_add_route(ctx->router, "/api", simple_handler);
    
    ctx->server->router = ctx->router;
    
    /* Listen on port */
    printf("Attempting to listen on %s:%d...\n", host, port);
    fflush(stdout);
    result = uvhttp_server_listen(ctx->server, host, port);
    printf("uvhttp_server_listen returned: %d\n", result);
    fflush(stdout);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s (error code: %d)\n",
                uvhttp_error_string(result), result);
        fprintf(stderr, "Exiting immediately without cleanup...\n");
        fflush(stderr);
        _exit(1);
    }
    printf("Server listening successfully on %s:%d\n", host, port);
    fflush(stdout);
    
    printf("========================================\n");
    printf("TLS/HTTPS E2E Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("Certificates: %s\n", cert_dir);
    printf("========================================\n");
    printf("Test with:\n");
    printf("  curl -k https://%s:%d/\n", host, port);
    printf("  curl --cacert %s/ca.crt https://%s:%d/\n", cert_dir, host, port);
    printf("========================================\n\n");
    
    /* Run event loop until signal received */
    printf("Running event loop. Press Ctrl+C to stop...\n");
    fflush(stdout);
    
    /* Run the loop until signal received */
    int loop_count = 0;
    while (ctx->running) {
        /* Use UV_RUN_NOWAIT to avoid blocking */
        int ret = uv_run(loop, UV_RUN_NOWAIT);
        loop_count++;
        if (ret == 0) {
            printf("[DEBUG] uv_run returned 0, exiting loop\n");
            fflush(stdout);
            break;
        }
        /* Short sleep to avoid busy waiting */
        if (ret > 0) {
            usleep(10000);  /* 10ms */
        }
        if (loop_count % 100 == 0) {
            printf("[DEBUG] Loop iteration %d, running=%d\n", loop_count, ctx->running);
            fflush(stdout);
        }
    }
    
    printf("Loop finished after %d iterations, running=%d\n", loop_count, ctx->running);
    fflush(stdout);
    
    printf("Stopping server...\n");
    fflush(stdout);
    if (ctx->server) {
        uvhttp_server_stop(ctx->server);
    }
    
    /* Force cleanup - run loop with timeout */
    printf("Cleaning up event loop...\n");
    fflush(stdout);
    for (int i = 0; i < 10 && uv_loop_alive(loop); i++) {
        uv_run(loop, UV_RUN_NOWAIT);
    }
    
    /* Cleanup */
    if (ctx->server) {
        uvhttp_server_free(ctx->server);
        /* Note: uvhttp_server_free already frees tls_ctx, so don't free it again */
        ctx->tls_ctx = NULL;
    }
    /* Only free tls_ctx if server was not freed */
    if (ctx->tls_ctx) {
        uvhttp_tls_context_free(ctx->tls_ctx);
    }
    uvhttp_free(ctx);
    
    printf("TLS E2E test completed successfully\n");
    return 0;
}
