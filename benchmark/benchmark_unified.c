/**
 * @file benchmark_unified.c
 * @brief Unified comprehensive performance benchmark server
 * 
 * This is a comprehensive performance test server supporting all test scenarios:
 * - RPS testing (simple text response, JSON response, different size responses)
 * - Latency testing
 * - Connection management testing
 * - Memory usage testing
 * - File transfer testing
 * - Router performance testing
 * - Database query simulation testing (async I/O)
 * 
 * Use wrk or ab for stress testing:
 *   wrk -t4 -c100 -d30s http://127.0.0.1:18081/
 *   ab -n 100000 -c 100 -k http://127.0.0.1:18081/
 */

#include <uv.h>
#include <uvhttp.h>
#include <uvhttp_static.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <sys/stat.h>
#include <limits.h>

#define DEFAULT_PORT 18081
#define MAX_SAMPLES 100000

/* Buffer size constants */
#define SMALL_BUFFER_SIZE 1024
#define MEDIUM_BUFFER_SIZE 10240
#define LARGE_BUFFER_SIZE 102400

/* Comprehensive statistics */
typedef struct {
    /* RPS statistics */
    int total_requests;
    int successful_requests;
    int failed_requests;

    /* Latency statistics */
    uint64_t latency_samples[MAX_SAMPLES];
    int latency_count;
    uint64_t min_latency;
    uint64_t max_latency;
    double latency_sum;

    /* Memory statistics */
    size_t peak_memory;
    size_t current_memory;

    /* CPU statistics */
    double cpu_usage_percent;
    uint64_t cpu_time_total;

    /* Time statistics */
    uint64_t start_time;
    uint64_t end_time;
} comprehensive_stats_t;

/* Async database query context */
typedef struct {
    uvhttp_request_t* request;
    uvhttp_response_t* response;
    const char* response_body;
    size_t response_length;
    uint64_t start_time;
    uv_timer_t timer;
} db_query_context_t;

/* Application context */
typedef struct {
    volatile sig_atomic_t running;
    comprehensive_stats_t stats;
    uvhttp_static_context_t* static_ctx;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    uv_loop_t* loop;  /* Store loop pointer for uv_stop() */
} app_context_t;

/* Global signal server pointer (POSIX allowed for signal handlers) */
static uvhttp_server_t* g_signal_server = NULL;
static app_context_t* g_ctx = NULL;

/* Get current timestamp in microseconds */
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* Get current process memory usage in KB */
static size_t get_memory_usage_kb(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

/* Signal handler using libuv signals */
static void on_signal(uv_signal_t* handle, int signum) {
    (void)signum;
    (void)handle;
    printf("\nReceived stop signal, shutting down server...\n");
    
    if (g_ctx) {
        g_ctx->running = 0;
        
        /* Stop the event loop to allow graceful shutdown */
        if (g_ctx->loop && uv_loop_alive(g_ctx->loop)) {
            uv_stop(g_ctx->loop);
        }
    }
    
    if (g_signal_server) {
        uvhttp_server_stop(g_signal_server);
    }
}

/* Signal handle close callback */
static void on_signal_close(uv_handle_t* handle) {
    (void)handle;  /* No action needed, just for cleanup */
}

/* Async database query timer callback */
static void on_db_query_complete(uv_timer_t* handle) {
    db_query_context_t* ctx = (db_query_context_t*)handle->data;
    
    if (!ctx || !ctx->response) {
        if (ctx) {
            uvhttp_free(ctx);
        }
        return;
    }

    /* Record latency */
    if (g_ctx && g_ctx->stats.latency_count < MAX_SAMPLES) {
        uint64_t latency = get_timestamp_us() - ctx->start_time;
        g_ctx->stats.latency_samples[g_ctx->stats.latency_count] = latency;
        g_ctx->stats.latency_count++;
    }

    /* Send response */
    uvhttp_response_set_status(ctx->response, 200);
    uvhttp_response_set_header(ctx->response, "Content-Type", "application/json");
    
    char content_length_str[32];
    snprintf(content_length_str, sizeof(content_length_str), "%zu", ctx->response_length);
    uvhttp_response_set_header(ctx->response, "Content-Length", content_length_str);
    
    uvhttp_response_set_body(ctx->response, ctx->response_body, ctx->response_length);
    uvhttp_response_send(ctx->response);

    /* Cleanup */
    uv_timer_stop(&ctx->timer);
    uv_close((uv_handle_t*)&ctx->timer, NULL);
    uvhttp_free(ctx);
}

/* Simple GET request handler */
static int get_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "13");
    uvhttp_response_set_body(response, body, 13);
    uvhttp_response_send(response);

    return 0;
}

/* JSON response handler */
static int json_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    const char* body = "{\"status\":\"ok\",\"message\":\"Hello from UVHTTP\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "50");
    uvhttp_response_set_body(response, body, 50);
    uvhttp_response_send(response);

    return 0;
}

/* POST request handler */
static int post_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    const char* body = "{\"status\":\"received\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "23");
    uvhttp_response_set_body(response, body, 23);
    uvhttp_response_send(response);

    return 0;
}

/* Small response handler (1KB) */
static int small_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    static char small_body[SMALL_BUFFER_SIZE];
    static int small_initialized = 0;

    if (!small_initialized) {
        memset(small_body, 'A', sizeof(small_body) - 1);
        small_body[sizeof(small_body) - 1] = '\0';
        small_initialized = 1;
    }

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "1023");
    uvhttp_response_set_body(response, small_body, 1023);
    uvhttp_response_send(response);

    return 0;
}

/* Medium response handler (10KB) */
static int medium_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    static char medium_body[MEDIUM_BUFFER_SIZE];
    static int medium_initialized = 0;

    if (!medium_initialized) {
        memset(medium_body, 'B', sizeof(medium_body) - 1);
        medium_body[sizeof(medium_body) - 1] = '\0';
        medium_initialized = 1;
    }

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "10239");
    uvhttp_response_set_body(response, medium_body, 10239);
    uvhttp_response_send(response);

    return 0;
}

/* Large response handler (100KB) */
static int large_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    static char large_body[LARGE_BUFFER_SIZE];
    static int large_initialized = 0;

    if (!large_initialized) {
        memset(large_body, 'C', sizeof(large_body) - 1);
        large_body[sizeof(large_body) - 1] = '\0';
        large_initialized = 1;
    }

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "102399");
    uvhttp_response_set_body(response, large_body, 102399);
    uvhttp_response_send(response);

    return 0;
}

/* Latency test handler */
static int latency_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response || !g_ctx) {
        return -1;
    }

    uint64_t start_time = get_timestamp_us();

    const char* body = "OK";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "2");
    uvhttp_response_set_body(response, body, 2);
    uvhttp_response_send(response);

    /* Record latency */
    uint64_t latency = get_timestamp_us() - start_time;
    if (g_ctx->stats.latency_count < MAX_SAMPLES) {
        g_ctx->stats.latency_samples[g_ctx->stats.latency_count] = latency;
        g_ctx->stats.latency_count++;
    }

    return 0;
}

/* Memory test handler */
static int memory_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response || !g_ctx) {
        return -1;
    }

    /* Update memory statistics */
    size_t current_mem = get_memory_usage_kb();
    if (current_mem > g_ctx->stats.peak_memory) {
        g_ctx->stats.peak_memory = current_mem;
    }
    g_ctx->stats.current_memory = current_mem;

    const char* body = "OK";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "Content-Length", "2");
    uvhttp_response_set_body(response, body, 2);
    uvhttp_response_send(response);

    return 0;
}

/* Fast database query handler (1ms delay) - async I/O */
static int db_fast_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response || !g_ctx) {
        return -1;
    }

    /* Create async query context */
    db_query_context_t* ctx = (db_query_context_t*)uvhttp_alloc(sizeof(db_query_context_t));
    if (!ctx) {
        return -1;
    }

    ctx->request = request;
    ctx->response = response;
    ctx->response_body = "{\"query\":\"fast\",\"result\":[{\"id\":1,\"name\":\"Item1\"}]}";
    ctx->response_length = 60;
    ctx->start_time = get_timestamp_us();

    /* Get loop from request */
    uv_loop_t* loop = request->client->loop;
    if (!loop) {
        uvhttp_free(ctx);
        return -1;
    }

    /* Start timer to simulate async database query */
    uv_timer_init(loop, &ctx->timer);
    ctx->timer.data = ctx;
    uv_timer_start(&ctx->timer, on_db_query_complete, 1, 0);  /* 1ms delay */

    return 0;
}

/* Medium database query handler (10ms delay) - async I/O */
static int db_medium_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response || !g_ctx) {
        return -1;
    }

    /* Create async query context */
    db_query_context_t* ctx = (db_query_context_t*)uvhttp_alloc(sizeof(db_query_context_t));
    if (!ctx) {
        return -1;
    }

    ctx->request = request;
    ctx->response = response;
    ctx->response_body = "{\"query\":\"medium\",\"result\":[{\"id\":1,\"name\":\"Item1\"},{\"id\":2,\"name\":\"Item2\"}]}";
    ctx->response_length = 84;
    ctx->start_time = get_timestamp_us();

    /* Get loop from request */
    uv_loop_t* loop = request->client->loop;
    if (!loop) {
        uvhttp_free(ctx);
        return -1;
    }

    /* Start timer to simulate async database query */
    uv_timer_init(loop, &ctx->timer);
    ctx->timer.data = ctx;
    uv_timer_start(&ctx->timer, on_db_query_complete, 10, 0);  /* 10ms delay */

    return 0;
}

/* Slow database query handler (50ms delay) - async I/O */
static int db_slow_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    if (!response || !g_ctx || !request || !request->client) {
        return -1;
    }

    /* Create async query context */
    db_query_context_t* ctx = (db_query_context_t*)uvhttp_alloc(sizeof(db_query_context_t));
    if (!ctx) {
        return -1;
    }

    ctx->request = request;
    ctx->response = response;
    ctx->response_body = "{\"query\":\"slow\",\"result\":[{\"id\":1,\"name\":\"Item1\"},{\"id\":2,\"name\":\"Item2\"},{\"id\":3,\"name\":\"Item3\"},{\"id\":4,\"name\":\"Item4\"},{\"id\":5,\"name\":\"Item5\"}]}";
    ctx->response_length = 156;
    ctx->start_time = get_timestamp_us();

    /* Get loop from request->client */
    uv_loop_t* loop = request->client->loop;
    if (!loop) {
        uvhttp_free(ctx);
        return -1;
    }

    /* Start timer to simulate async database query */
    uv_timer_init(loop, &ctx->timer);
    ctx->timer.data = ctx;
    uv_timer_start(&ctx->timer, on_db_query_complete, 50, 0);  /* 50ms delay */

    return 0;
}

/* Statistics handler */
static int stats_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response || !g_ctx) {
        return -1;
    }

    /* Increment request counter */
    g_ctx->stats.total_requests++;
    g_ctx->stats.successful_requests++;

    char stats_body[512];
    char content_length_str[32];
    uint64_t elapsed = (uv_hrtime() - g_ctx->stats.start_time) / 1000000000;  /* Convert to seconds */
    double rps = elapsed > 0 ? (double)g_ctx->stats.total_requests / elapsed : 0.0;

    int len = snprintf(stats_body, sizeof(stats_body),
        "{\"total_requests\":%d,\"successful_requests\":%d,\"failed_requests\":%d,\"elapsed_seconds\":%" PRIu64 ",\"rps\":%.2f,\"peak_memory_kb\":%zu}",
        g_ctx->stats.total_requests, g_ctx->stats.successful_requests, g_ctx->stats.failed_requests,
        elapsed, rps, g_ctx->stats.peak_memory);
    snprintf(content_length_str, sizeof(content_length_str), "%d", len);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", content_length_str);
    uvhttp_response_set_body(response, stats_body, len);
    uvhttp_response_send(response);

    return 0;
}

/* Health check handler */
static int health_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;  /* Avoid unused parameter warning */

    if (!response) {
        return -1;
    }

    const char* body = "{\"status\":\"healthy\"}";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_header(response, "Content-Length", "22");
    uvhttp_response_set_body(response, body, 22);
    uvhttp_response_send(response);

    return 0;
}

/* Create test file */
static void create_test_file(const char* path, size_t size) {
    FILE* f = fopen(path, "wb");
    if (!f) {
        fprintf(stderr, "Failed to create test file: %s\n", path);
        return;
    }

    /* Write random data */
    unsigned char buffer[4096];
    for (size_t i = 0; i < sizeof(buffer); i++) {
        buffer[i] = (unsigned char)(rand() % 256);
    }

    size_t remaining = size;
    while (remaining > 0) {
        size_t chunk = (remaining > sizeof(buffer)) ? sizeof(buffer) : remaining;
        fwrite(buffer, 1, chunk, f);
        remaining -= chunk;
    }

    fclose(f);
}

/* Create directory using mkdir system call */
static int create_directory(const char* path) {
    return mkdir(path, 0755);
}

/* Print usage */
static void print_usage(const char* program) {
    printf("Usage: %s [port]\n", program);
    printf("\n");
    printf("Arguments:\n");
    printf("  port      Server port (default: %d)\n", DEFAULT_PORT);
    printf("\n");
    printf("Endpoints:\n");
    printf("  GET  /                 - Simple text response (13 bytes)\n");
    printf("  GET  /json             - JSON response (50 bytes)\n");
    printf("  POST /post             - POST request handler\n");
    printf("  GET  /small            - Small response (1KB)\n");
    printf("  GET  /medium           - Medium response (10KB)\n");
    printf("  GET  /large            - Large response (100KB)\n");
    printf("  GET  /latency          - Latency test endpoint\n");
    printf("  GET  /memory           - Memory test endpoint\n");
    printf("  GET  /db/fast          - Fast database query (1ms async delay)\n");
    printf("  GET  /db/medium        - Medium database query (10ms async delay)\n");
    printf("  GET  /db/slow          - Slow database query (50ms async delay)\n");
    printf("  GET  /stats            - Server statistics\n");
    printf("  GET  /health           - Health check\n");
    printf("  GET  /file/*           - Static file test\n");
    printf("\n");
    printf("Performance Testing:\n");
    printf("  wrk -t4 -c100 -d30s http://127.0.0.1:18081/\n");
    printf("  ab -n 100000 -c 100 -k http://127.0.0.1:18081/\n");
    printf("\n");
}

/* Print final statistics */
static void print_final_stats(void) {
    if (!g_ctx) {
        return;
    }

    uint64_t elapsed = (uv_hrtime() - g_ctx->stats.start_time) / 1000000000;
    printf("\n========================================\n");
    printf("  Server Statistics\n");
    printf("========================================\n");
    printf("Total requests: %d\n", g_ctx->stats.total_requests);
    printf("Successful requests: %d\n", g_ctx->stats.successful_requests);
    printf("Failed requests: %d\n", g_ctx->stats.failed_requests);
    printf("Elapsed time: %" PRIu64 " seconds\n", elapsed);
    if (elapsed > 0) {
        printf("Average RPS: %.2f\n", (double)g_ctx->stats.total_requests / elapsed);
    }
    printf("Peak memory: %zu KB (%.2f MB)\n",
           g_ctx->stats.peak_memory, g_ctx->stats.peak_memory / 1024.0);
    printf("========================================\n");
}

int main(int argc, char* argv[]) {
    int port = DEFAULT_PORT;

    /* Parse command line arguments */
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            print_usage(argv[0]);
            return 1;
        }
    }

    /* Print banner */
    printf("========================================\n");
    printf("  UVHTTP Unified Benchmark Server\n");
    printf("========================================\n");
    printf("Port: %d\n", port);
    printf("PID: %d\n", getpid());
    printf("========================================\n\n");

    /* Create event loop */
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Error: Failed to create event loop\n");
        return 1;
    }

    /* Create application context */
    app_context_t* ctx = (app_context_t*)uvhttp_alloc(sizeof(app_context_t));
    if (!ctx) {
        fprintf(stderr, "Error: Failed to allocate context\n");
        return 1;
    }
    memset(ctx, 0, sizeof(app_context_t));
    ctx->running = 1;
    ctx->stats.start_time = uv_hrtime();
    ctx->loop = loop;  /* Store loop pointer for uv_stop() */
    g_ctx = ctx;

    /* Create server */
    uvhttp_server_t* server = NULL;
    if (uvhttp_server_new(loop, &server) != UVHTTP_OK || !server) {
        fprintf(stderr, "Error: Failed to create server\n");
        uvhttp_free(ctx);
        g_ctx = NULL;
        return 1;
    }

    /* Set server context */
    g_signal_server = server;
    uvhttp_error_t result = uvhttp_server_set_context(server, NULL);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Warning: Failed to set server context: %s\n", uvhttp_error_string(result));
    }

    /* Create router */
    uvhttp_router_t* router = NULL;
    if (uvhttp_router_new(&router) != UVHTTP_OK || !router) {
        fprintf(stderr, "Error: Failed to create router\n");
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_ctx = NULL;
        g_signal_server = NULL;
        return 1;
    }

    /* Add all routes */
    uvhttp_router_add_route(router, "/", get_handler);
    uvhttp_router_add_route(router, "/json", json_handler);
    uvhttp_router_add_route(router, "/post", post_handler);
    uvhttp_router_add_route(router, "/small", small_handler);
    uvhttp_router_add_route(router, "/medium", medium_handler);
    uvhttp_router_add_route(router, "/large", large_handler);
    uvhttp_router_add_route(router, "/latency", latency_handler);
    uvhttp_router_add_route(router, "/memory", memory_handler);
    uvhttp_router_add_route(router, "/db/fast", db_fast_handler);
    uvhttp_router_add_route(router, "/db/medium", db_medium_handler);
    uvhttp_router_add_route(router, "/db/slow", db_slow_handler);
    uvhttp_router_add_route(router, "/stats", stats_handler);
    uvhttp_router_add_route(router, "/health", health_handler);

    /* Create test file directory */
    const char* test_dir = "./public/file_test";
    if (create_directory(test_dir) != 0 && errno != EEXIST) {
        fprintf(stderr, "Warning: Failed to create test directory: %s\n", test_dir);
    }

    /* Create test files */
    create_test_file("./public/file_test/small.txt", 1024);
    create_test_file("./public/file_test/medium.bin", 64 * 1024);
    create_test_file("./public/file_test/large.bin", 1024 * 1024);

    /* Create static file context */
    uvhttp_static_config_t static_config;
    memset(&static_config, 0, sizeof(static_config));
    static_config.max_cache_size = 20 * 1024 * 1024;  /* 20MB cache */
    static_config.cache_ttl = 3600;
    static_config.max_cache_entries = 100;
    static_config.max_file_size = 200 * 1024 * 1024;
    static_config.enable_sendfile = 1;

    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) != NULL) {
        strncpy(static_config.root_directory, cwd, sizeof(static_config.root_directory) - 1);
        static_config.root_directory[sizeof(static_config.root_directory) - 1] = '\0';
        strncat(static_config.root_directory, "/public", sizeof(static_config.root_directory) - strlen(static_config.root_directory) - 1);

        result = uvhttp_static_create(&static_config, &ctx->static_ctx);
        if (result == UVHTTP_OK) {
            result = uvhttp_router_add_static_route(router, "/file", ctx->static_ctx);
            if (result == UVHTTP_OK) {
                printf("Static file service enabled\n");
            } else {
                fprintf(stderr, "Warning: Failed to add static route: %s\n", uvhttp_error_string(result));
            }
        } else {
            fprintf(stderr, "Warning: Failed to create static file context: %s\n", uvhttp_error_string(result));
        }
    }

    /* Set router */
    server->router = router;

    /* Setup signal handlers using libuv */
    if (uv_signal_init(loop, &ctx->sigint) != 0) {
        fprintf(stderr, "Error: Failed to initialize SIGINT handler\n");
        if (ctx->static_ctx) {
            uvhttp_static_free(ctx->static_ctx);
        }
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_ctx = NULL;
        g_signal_server = NULL;
        return 1;
    }
    uv_signal_start(&ctx->sigint, on_signal, SIGINT);

    if (uv_signal_init(loop, &ctx->sigterm) != 0) {
        fprintf(stderr, "Error: Failed to initialize SIGTERM handler\n");
        uv_signal_stop(&ctx->sigint);
        if (ctx->static_ctx) {
            uvhttp_static_free(ctx->static_ctx);
        }
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_ctx = NULL;
        g_signal_server = NULL;
        return 1;
    }
    uv_signal_start(&ctx->sigterm, on_signal, SIGTERM);

    /* Start server */
    printf("Starting server on port %d...\n", port);
    result = uvhttp_server_listen(server, "127.0.0.1", port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Error: Failed to start server: %s\n", uvhttp_error_string(result));
        uv_signal_stop(&ctx->sigint);
        uv_signal_stop(&ctx->sigterm);
        if (ctx->static_ctx) {
            uvhttp_static_free(ctx->static_ctx);
        }
        uvhttp_server_free(server);
        uvhttp_free(ctx);
        g_ctx = NULL;
        g_signal_server = NULL;
        return 1;
    }

    printf("Server started successfully!\n");
    printf("Press Ctrl+C to stop the server\n\n");

    /* Run event loop with UV_RUN_DEFAULT for maximum performance */
    /* Signal handler will call uv_stop() to gracefully shutdown */
    uv_run(loop, UV_RUN_DEFAULT);

    /* Print final statistics */
    print_final_stats();

    /* Cleanup */
    /* Stop and close signal handles */
    uv_signal_stop(&ctx->sigint);
    uv_close((uv_handle_t*)&ctx->sigint, on_signal_close);
    
    uv_signal_stop(&ctx->sigterm);
    uv_close((uv_handle_t*)&ctx->sigterm, on_signal_close);
    
    /* Run loop once more to process close callbacks */
    uv_run(loop, UV_RUN_ONCE);
    
    if (ctx->static_ctx) {
        uvhttp_static_free(ctx->static_ctx);
        ctx->static_ctx = NULL;
    }
    uvhttp_server_free(server);
    uvhttp_free(ctx);
    g_ctx = NULL;
    g_signal_server = NULL;

    return 0;
}
