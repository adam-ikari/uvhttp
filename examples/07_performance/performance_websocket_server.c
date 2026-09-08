/**
 * @file performance_websocket_server.c
 * @brief WebSocket performance testing server
 *
 * A WebSocket server for performance testing WebSocket capabilities.
 * Supports various test scenarios:
 * - Connection establishment performance
 * - Message throughput (echo server)
 * - Multiple concurrent connections
 * - Bidirectional communication
 * - Latency measurement
 *
 * Build with:
 *   cmake -DBUILD_WITH_WEBSOCKET=ON ..
 *   make
 *
 * Run:
 *   ./performance_websocket_server -p 18081
 *
 * Test with:
 *   # Simple connection test
 *   wscat -c "ws://127.0.0.1:18081/ws"
 *
 *   # Load test (requires websocket-bench or similar)
 *   websocket-bench -a 100 -c 10 ws://127.0.0.1:18081/ws
 */

#include <uv.h>
#include <uvhttp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <signal.h>
#include <time.h>

#define DEFAULT_PORT 18081

typedef struct {
    volatile sig_atomic_t running;
    uv_loop_t* loop;
    uvhttp_server_builder_t* server;

    /* Statistics */
    unsigned long long total_connections;
    unsigned long long current_connections;
    unsigned long long total_messages_sent;
    unsigned long long total_messages_received;
    unsigned long long total_bytes_sent;
    unsigned long long total_bytes_received;

    uv_timer_t stats_timer;
} app_context_t;

static app_context_t* g_ctx = NULL;

/* Statistics timer callback */
static void on_stats_timer(uv_timer_t* handle) {
    app_context_t* ctx = (app_context_t*)handle->data;

    if (!ctx) return;

    printf("\rConnections: %llu (current) | Sent: %llu msg, %llu bytes | Received: %llu msg, %llu bytes     ",
           ctx->current_connections,
           ctx->total_messages_sent,
           ctx->total_bytes_sent,
           ctx->total_messages_received,
           ctx->total_bytes_received);
    fflush(stdout);
}

/* WebSocket connection handler */
static int on_ws_connect(uvhttp_ws_connection_t* ws) {
    (void)ws;
    app_context_t* ctx = g_ctx;

    if (!ctx) return 0;

    ctx->total_connections++;
    ctx->current_connections++;

    if (ctx->total_connections == 1) {
        /* Start statistics timer on first connection */
        uv_timer_start(&ctx->stats_timer, on_stats_timer, 1000, 1000);
    }

    return 0;
}

/* WebSocket message handler (echo) */
static int on_ws_message(uvhttp_ws_connection_t* ws, const char* data,
                         size_t len, int opcode) {
    (void)opcode;
    app_context_t* ctx = g_ctx;

    if (!ctx) return 0;

    ctx->total_messages_received++;
    ctx->total_bytes_received += len;

    /* Echo the message back */
    int ret = uvhttp_server_ws_send(ws, data, len);
    if (ret == UVHTTP_OK) {
        ctx->total_messages_sent++;
        ctx->total_bytes_sent += len;
    }

    return 0;
}

/* WebSocket close handler */
static int on_ws_close(uvhttp_ws_connection_t* ws) {
    (void)ws;
    app_context_t* ctx = g_ctx;

    if (!ctx) return 0;

    if (ctx->current_connections > 0) {
        ctx->current_connections--;
    }

    if (ctx->current_connections == 0) {
        /* Stop statistics timer when no connections */
        uv_timer_stop(&ctx->stats_timer);
    }

    return 0;
}

/* HTTP handler for health check / info */
static int on_http_request(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);
    const char* method = uvhttp_request_get_method(request);

    /* Only handle GET requests */
    if (strcmp(method, "GET") != 0) {
        uvhttp_response_set_status(response, 405);
        uvhttp_response_set_body(response, "Method Not Allowed", 18);
        uvhttp_response_send(response);
        return UVHTTP_ERROR_INVALID_HTTP_METHOD;
    }

    /* Health check endpoint */
    if (strcmp(path, "/health") == 0) {
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "application/json");

        if (g_ctx) {
            char stats[256];
            snprintf(stats, sizeof(stats),
                    "{\"status\":\"ok\",\"connections\":%llu,\"total_messages\":%llu}",
                    g_ctx->current_connections,
                    g_ctx->total_messages_received);
            uvhttp_response_set_body(response, stats, strlen(stats));
        } else {
            const char* body = "{\"status\":\"ok\",\"connections\":0,\"total_messages\":0}";
            uvhttp_response_set_body(response, body, strlen(body));
        }

        uvhttp_response_send(response);
        return UVHTTP_OK;
    }

    /* Default response */
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");

    const char* body = "WebSocket Performance Test Server\n\nEndpoints:\n"
                      "  /ws - WebSocket endpoint (echo server)\n"
                      "  /health - Health check\n";
    uvhttp_response_set_body(response, body, strlen(body));

    uvhttp_response_send(response);
    return UVHTTP_OK;
}

/* Signal handler */
static void on_signal(uv_signal_t* handle, int signum) {
    (void)handle;
    (void)signum;

    fprintf(stderr, "\n\nReceived signal %d, shutting down gracefully...\n", signum);

    if (g_ctx && g_ctx->running) {
        g_ctx->running = 0;

        /* Stop the server */
        if (g_ctx->server && g_ctx->server->server) {
            uvhttp_server_stop(g_ctx->server->server);
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
    fprintf(stderr, "  -h, --host HOST        Bind address (default: 0.0.0.0)\n");
    fprintf(stderr, "  -v, --verbose          Enable verbose output\n");
    fprintf(stderr, "  --help                 Show this help message\n");
    fprintf(stderr, "\nExample:\n");
    fprintf(stderr, "  %s -p 18081\n", prog);
    fprintf(stderr, "\nWebSocket Endpoints:\n");
    fprintf(stderr, "  ws://host:port/ws      Echo server\n");
    fprintf(stderr, "  http://host:port/health Health check\n");
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;
    const char* host = "0.0.0.0";
    int verbose = 0;

    static struct option long_options[] = {
        {"port",    required_argument, 0, 'p'},
        {"host",    required_argument, 0, 'h'},
        {"verbose", no_argument,       0, 'v'},
        {"help",    no_argument,       0, 0},
        {0, 0, 0, 0}
    };

    int opt;
    int option_index = 0;

    while ((opt = getopt_long(argc, argv, "p:h:v", long_options, &option_index)) != -1) {
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
            case 'v':
                verbose = 1;
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

    /* Setup statistics timer */
    uv_timer_init(ctx.loop, &ctx.stats_timer);
    ctx.stats_timer.data = &ctx;

    /* Setup signal handlers */
    uv_signal_t sigint;
    uv_signal_init(ctx.loop, &sigint);
    uv_signal_start(&sigint, on_signal, SIGINT);

    uv_signal_t sigterm;
    uv_signal_init(ctx.loop, &sigterm);
    uv_signal_start(&sigterm, on_signal, SIGTERM);

    /* Print banner */
    printf("========================================\n");
    printf("  UVHTTP WebSocket Performance Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    printf("========================================\n");
    if (verbose) {
        printf("Verbose mode enabled\n");
    }

    /* Create server (simple API: creates loop-bound server, router and config) */
    uvhttp_server_builder_t* server = NULL;
    uvhttp_error_t err = uvhttp_server_create(ctx.loop, host, port, &server);
    if (err != UVHTTP_OK || !server) {
        fprintf(stderr, "Error: Failed to create server: %d\n", err);
        return 1;
    }
    ctx.server = server;

    /* Register WebSocket echo handler on /ws */
    uvhttp_ws_handler_t ws_handler;
    memset(&ws_handler, 0, sizeof(ws_handler));
    ws_handler.on_connect = on_ws_connect;
    ws_handler.on_message = on_ws_message;
    ws_handler.on_close = on_ws_close;

    err = uvhttp_server_register_ws_handler(server->server, "/ws", &ws_handler);
    if (err != UVHTTP_OK) {
        fprintf(stderr, "Error: Failed to register WebSocket handler: %d\n", err);
        uvhttp_server_simple_free(server);
        return 1;
    }

    /* HTTP routes (health check + info) */
    uvhttp_get(server, "/", on_http_request);
    uvhttp_get(server, "/health", on_http_request);

    printf("WebSocket server listening on ws://%s:%d/ws\n", host, port);
    printf("Health check: http://%s:%d/health\n", host, port);
    printf("Press Ctrl+C to stop\n\n");

    /* Run event loop */
    uvhttp_server_run(server);

    /* Cleanup */
    printf("\n\n========================================\n");
    printf("  Server Statistics\n");
    printf("========================================\n");
    printf("Total Connections: %llu\n", ctx.total_connections);
    printf("Messages Sent: %llu (%llu bytes)\n", ctx.total_messages_sent, ctx.total_bytes_sent);
    printf("Messages Received: %llu (%llu bytes)\n", ctx.total_messages_received, ctx.total_bytes_received);
    printf("========================================\n");

    uv_timer_stop(&ctx.stats_timer);
    uv_signal_stop(&sigint);
    uv_signal_stop(&sigterm);
    uv_close((uv_handle_t*)&ctx.stats_timer, NULL);
    uv_close((uv_handle_t*)&sigint, NULL);
    uv_close((uv_handle_t*)&sigterm, NULL);

    uvhttp_server_simple_free(server);
    uv_loop_close(ctx.loop);

    printf("Server stopped gracefully\n");

    return 0;
}
