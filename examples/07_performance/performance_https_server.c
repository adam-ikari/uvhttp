/**
 * @file performance_https_server.c
 * @brief HTTPS performance testing server
 * 
 * A minimal HTTPS server for performance testing TLS/SSL capabilities.
 * Supports various test scenarios:
 * - Simple HTTPS request handling
 * - Multiple concurrent connections
 * - Keep-alive connections
 * - Different response sizes
 * 
 * Build with:
 *   cmake -DBUILD_WITH_HTTPS=ON ..
 *   make
 * 
 * Run:
 *   ./performance_https_server -p 18443 -c /path/to/cert.pem -k /path/to/key.pem
 * 
 * Test with:
 *   wrk -t4 -c100 -d30s --timeout 10s https://127.0.0.1:18443/
 *   ab -n 10000 -c 100 -k https://127.0.0.1:18443/
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <getopt.h>

#define DEFAULT_PORT 18443
#define MAX_THREADS 4

typedef struct {
    volatile sig_atomic_t running;
    uv_loop_t* loop;
    uvhttp_server_t* server;
    unsigned long long total_requests;
} app_context_t;

static app_context_t* g_ctx = NULL;
static uvhttp_server_t* g_signal_server = NULL;

/* Simple response handler */
static int on_request(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* Set status and headers */
    uvhttp_response_set_status(response, 200, "OK");
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Server", "uvhttp/https-perf");
    uvhttp_response_set_header(response, "Connection", "keep-alive");
    
    /* Simple response body */
    const char* body = "HTTPS Performance Test OK";
    uvhttp_response_set_body(response, body, strlen(body));
    
    /* Send response */
    uvhttp_response_send(response);
    
    /* Update statistics */
    if (g_ctx) {
        g_ctx->total_requests++;
    }
    
    return UVHTTP_OK;
}

/* JSON response handler for API testing */
static int on_request_json(uvhttp_request_t* request, uvhttp_response_t* response) {
    /* Set status and headers */
    uvhttp_response_set_status(response, 200, "OK");
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Server", "uvhttp/https-perf");
    
    /* JSON response body */
    const char* json_body = "{\"status\":\"ok\",\"message\":\"HTTPS performance test\",\"requests\":0}";
    uvhttp_response_set_body(response, json_body, strlen(json_body));
    
    /* Send response */
    uvhttp_response_send(response);
    
    if (g_ctx) {
        g_ctx->total_requests++;
    }
    
    return UVHTTP_OK;
}

/* Signal handler */
static void on_signal(uv_signal_t* handle, int signum) {
    (void)handle;
    (void)signum;
    
    fprintf(stderr, "\n\n⚠️  Received signal %d, shutting down gracefully...\n", signum);
    
    if (g_ctx && g_ctx->running) {
        g_ctx->running = 0;
        
        /* Stop the server */
        if (g_ctx->server) {
            uvhttp_server_stop(g_ctx->server);
        }
        
        /* Stop the event loop */
        if (g_ctx->loop) {
            uv_stop(g_ctx->loop);
        }
    }
}

/* Print usage */
static void print_usage(const char* prog) {
    fprintf(stderr, "Usage: %s [OPTIONS]\n", prog);
    fprintf(stderr, "\nOptions:\n");
    fprintf(stderr, "  -p, --port PORT        Port number (default: %d)\n", DEFAULT_PORT);
    fprintf(stderr, "  -c, --cert FILE        Certificate file path (required)\n");
    fprintf(stderr, "  -k, --key FILE         Private key file path (required)\n");
    fprintf(stderr, "  -h, --host HOST        Bind address (default: 0.0.0.0)\n");
    fprintf(stderr, "  -t, --threads NUM      Number of threads (default: 1)\n");
    fprintf(stderr, "  -v, --verbose          Enable verbose output\n");
    fprintf(stderr, "  --help                 Show this help message\n");
    fprintf(stderr, "\nExample:\n");
    fprintf(stderr, "  %s -p 18443 -c cert.pem -k key.pem\n", prog);
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    const char* host = "0.0.0.0";
    const char* cert_file = NULL;
    const char* key_file = NULL;
    int threads = 1;
    (void)threads;  // Unused parameter
    
    static struct option long_options[] = {
        {"port",    required_argument, 0, 'p'},
        {"host",    required_argument, 0, 'h'},
        {"cert",    required_argument, 0, 'c'},
        {"key",     required_argument, 0, 'k'},
        {"threads", required_argument, 0, 't'},
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 0},
        {0, 0, 0, 0}
    };
    
    int opt;
    int option_index = 0;
    
    while ((opt = getopt_long(argc, argv, "p:h:c:k:t:v", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'p':
                port = atoi(optarg);
                if (port <= 0 || port > 65535) {
                    fprintf(stderr, "Error: Invalid port number: %s\n", optarg);
                    return 1;
                }
                break;
            case 'h':
                host = optarg;
                break;
            case 'c':
                cert_file = optarg;
                break;
            case 'k':
                key_file = optarg;
                break;
            case 't':
                threads = atoi(optarg);
                if (threads < 1 || threads > MAX_THREADS) {
                    fprintf(stderr, "Error: Invalid thread count: %s (must be 1-%d)\n", optarg, MAX_THREADS);
                    return 1;
                }
                break;
            case 0:
                if (strcmp(long_options[option_index].name, "help") == 0) {
                    print_usage(argv[0]);
                    return 0;
                }
                break;
            default:
                print_usage(argv[0]);
                return 1;
        }
    }
    
    /* Validate required arguments */
    if (!cert_file) {
        fprintf(stderr, "Error: Certificate file is required (use -c or --cert)\n");
        print_usage(argv[0]);
        return 1;
    }
    
    if (!key_file) {
        fprintf(stderr, "Error: Private key file is required (use -k or --key)\n");
        print_usage(argv[0]);
        return 1;
    }
    
    /* Check if certificate and key files exist */
    if (access(cert_file, R_OK) != 0) {
        fprintf(stderr, "Error: Cannot read certificate file: %s\n", cert_file);
        return 1;
    }
    
    if (access(key_file, R_OK) != 0) {
        fprintf(stderr, "Error: Cannot read private key file: %s\n", key_file);
        return 1;
    }
    
    /* Create application context */
    app_context_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.running = 1;
    g_ctx = &ctx;
    
    /* Create event loop */
    ctx.loop = uv_default_loop();
    if (!ctx.loop) {
        fprintf(stderr, "Error: Failed to create event loop\n");
        return 1;
    }
    
    /* Setup signal handlers */
    uv_signal_t sigint;
    uv_signal_init(ctx.loop, &sigint);
    uv_signal_start(&sigint, on_signal, SIGINT);
    
    uv_signal_t sigterm;
    uv_signal_init(ctx.loop, &sigterm);
    uv_signal_start(&sigterm, on_signal, SIGTERM);
    
    /* Print banner */
    printf("========================================\n");
    printf("  UVHTTP HTTPS Performance Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("Certificate: %s\n", cert_file);
    printf("Private Key: %s\n", key_file);
    printf("Threads: %d\n", threads);
    printf("========================================\n");
    
    /* Create server with TLS support */
    ctx.server = uvhttp_server_new(ctx.loop);
    if (!ctx.server) {
        fprintf(stderr, "Error: Failed to create server\n");
        return 1;
    }
    
    g_signal_server = ctx.server;
    
    /* Create router */
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "Error: Failed to create router\n");
        uvhttp_server_free(ctx.server);
        return 1;
    }
    
    /* Setup TLS context */
    uvhttp_tls_context_t* tls_ctx = uvhttp_tls_context_new();
    if (!tls_ctx) {
        fprintf(stderr, "Error: Failed to create TLS context\n");
        uvhttp_router_free(router);
        uvhttp_server_free(ctx.server);
        return 1;
    }
    
    /* Load certificate and key */
    int ret = uvhttp_tls_context_load_cert(tls_ctx, cert_file, key_file);
    if (ret != UVHTTP_OK) {
        fprintf(stderr, "Error: Failed to load certificate/key: %d\n", ret);
        uvhttp_tls_context_free(tls_ctx);
        uvhttp_router_free(router);
        uvhttp_server_free(ctx.server);
        return 1;
    }
    
    /* Configure server with TLS */
    uvhttp_server_set_tls(ctx.server, tls_ctx);
    
    /* Add routes */
    uvhttp_router_add_route(router, "/", on_request);
    uvhttp_router_add_route(router, "/api", on_request_json);
    
    /* Set router */
    ctx.server->router = router;
    
    /* Listen */
    ret = uvhttp_server_listen(ctx.server, host, port);
    if (ret != UVHTTP_OK) {
        fprintf(stderr, "Error: Failed to listen on %s:%d - %d\n", host, port, ret);
        uvhttp_tls_context_free(tls_ctx);
        uvhttp_router_free(router);
        uvhttp_server_free(ctx.server);
        return 1;
    }
    
    printf("✓ HTTPS server listening on https://%s:%d/\n", host, port);
    printf("✓ Press Ctrl+C to stop\n\n");
    
    /* Run event loop */
    uv_run(ctx.loop, UV_RUN_DEFAULT);
    
    /* Cleanup */
    printf("\n\n========================================\n");
    printf("  Server Statistics\n");
    printf("========================================\n");
    printf("Total Requests: %llu\n", ctx.total_requests);
    printf("========================================\n");
    
    uv_signal_stop(&sigint);
    uv_signal_stop(&sigterm);
    uv_close((uv_handle_t*)&sigint, NULL);
    uv_close((uv_handle_t*)&sigterm, NULL);
    
    uvhttp_server_free(ctx.server);
    uv_loop_close(ctx.loop);
    
    printf("✓ Server stopped gracefully\n");
    
    return 0;
}