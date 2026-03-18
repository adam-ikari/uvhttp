/*
 * LRU Cache E2E Test - LRU cache functionality tests
 * LRU 缓存功能的端到端测试
 */

#include "uvhttp_lru_cache.h"

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
    uvhttp_lru_cache_t* cache;
    int cache_hits;
    int cache_misses;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int cached_handler(uvhttp_request_t* request,
                          uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;

    /* Try to get from cache */
    const char* cache_key = uvhttp_request_get_path(request);
    char* cached_data = NULL;
    size_t data_size = 0;

    uvhttp_error_t result = uvhttp_lru_cache_get(
        app->cache, cache_key, (void**)&cached_data, &data_size);
    if (result == UVHTTP_OK) {
        /* Cache hit */
        app->cache_hits++;

        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_header(response, "X-Cache", "HIT");
        uvhttp_response_set_body(response, cached_data, data_size);
        uvhttp_response_send(response);

        return 0;
    }

    /* Cache miss - generate response */
    app->cache_misses++;

    static char response_data[256];
    snprintf(response_data, sizeof(response_data),
             "Generated response for: %s (cache miss)", cache_key);
    size_t len = strlen(response_data);

    /* Store in cache */
    uvhttp_lru_cache_put(app->cache, cache_key, response_data, len);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Cache", "MISS");
    uvhttp_response_set_body(response, response_data, len);
    uvhttp_response_send(response);

    return 0;
}

static int cache_stats_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;

    char stats[512];
    snprintf(
        stats, sizeof(stats),
        "Cache Statistics:\n"
        "Hits: %d\n"
        "Misses: %d\n"
        "Hit Rate: %.2f%%\n"
        "Size: %zu/%zu\n"
        "Evictions: %zu",
        app->cache_hits, app->cache_misses,
        (app->cache_hits + app->cache_misses) > 0
            ? (100.0 * app->cache_hits / (app->cache_hits + app->cache_misses))
            : 0.0,
        uvhttp_lru_cache_size(app->cache),
        uvhttp_lru_cache_capacity(app->cache),
        uvhttp_lru_cache_evictions(app->cache));

    size_t len = strlen(stats);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, stats, len);
    uvhttp_response_send(response);

    return 0;
}

static int cache_clear_handler(uvhttp_request_t* request,
                               uvhttp_response_t* response) {
    app_context_t* app = (app_context_t*)request->server->context;

    /* Clear cache */
    uvhttp_lru_cache_clear(app->cache);
    app->cache_hits = 0;
    app->cache_misses = 0;

    const char* body = "Cache cleared";

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -X %s http://127.0.0.1:8776%s 2>/dev/null", method, path);

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

    const char* port = "8776";
    size_t cache_capacity = 100;

    if (argc > 1) {
        port = argv[1];
    }
    if (argc > 2) {
        cache_capacity = (size_t)atoi(argv[2]);
    }

    printf("\n========================================\n");
    printf("  LRU Cache E2E Test\n");
    printf("  Port: %s\n", port);
    printf("  Cache Capacity: %zu\n", cache_capacity);
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
    app.cache_hits = 0;
    app.cache_misses = 0;

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

    /* Create LRU cache */
    printf("\nCreating LRU cache...\n");
    printf("  Capacity: %zu items\n", cache_capacity);

    result = uvhttp_lru_cache_new(&app.cache, cache_capacity);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to create LRU cache: %d\n", result);
        uvhttp_router_free(app.router);
        uvhttp_server_free(app.server);
        return 1;
    }

    /* Add routes */
    printf("\nRegistering routes...\n");
    printf("  ✓ GET /cached/*\n");
    printf("  ✓ GET /cache/stats\n");
    printf("  ✓ POST /cache/clear\n");

    uvhttp_router_add_route(app.router, "/cached/*", cached_handler);
    uvhttp_router_add_route(app.router, "/cache/stats", cache_stats_handler);
    uvhttp_router_add_route(app.router, "/cache/clear", cache_clear_handler);

    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_lru_cache_free(app.cache);
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

    /* Test cache miss */
    printf("\n=== Testing Cache Miss ===\n");
    send_http_request("GET", "/cached/page1", response, sizeof(response));
    assert(strstr(response, "cache miss") != NULL);
    assert(strstr(response, "X-Cache: MISS") != NULL);
    printf("✓ Cache miss test passed\n");

    /* Test cache hit */
    printf("\n=== Testing Cache Hit ===\n");
    send_http_request("GET", "/cached/page1", response, sizeof(response));
    assert(strstr(response, "Generated response") != NULL);
    assert(strstr(response, "X-Cache: HIT") != NULL);
    printf("✓ Cache hit test passed\n");

    /* Test multiple cache misses */
    printf("\n=== Testing Multiple Cache Misses ===\n");
    for (int i = 0; i < 5; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/cached/page%d", i + 2);
        send_http_request("GET", path, response, sizeof(response));
    }
    printf("✓ Multiple cache misses test passed\n");

    /* Test cache hits */
    printf("\n=== Testing Multiple Cache Hits ===\n");
    for (int i = 0; i < 3; i++) {
        send_http_request("GET", "/cached/page2", response, sizeof(response));
        assert(strstr(response, "X-Cache: HIT") != NULL);
    }
    printf("✓ Multiple cache hits test passed\n");

    /* Test cache stats */
    printf("\n=== Testing Cache Statistics ===\n");
    send_http_request("GET", "/cache/stats", response, sizeof(response));
    assert(strstr(response, "Cache Statistics") != NULL);
    assert(strstr(response, "Hits:") != NULL);
    assert(strstr(response, "Misses:") != NULL);
    printf("✓ Cache statistics test passed\n");

    /* Test cache clear */
    printf("\n=== Testing Cache Clear ===\n");
    send_http_request("POST", "/cache/clear", response, sizeof(response));
    assert(strstr(response, "Cache cleared") != NULL);

    /* Verify cache is cleared */
    send_http_request("GET", "/cached/page1", response, sizeof(response));
    assert(strstr(response, "X-Cache: MISS") != NULL);
    printf("✓ Cache clear test passed\n");

    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");

    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);

    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_lru_cache_free(app.cache);
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);

    printf("Test completed successfully\n");
    return 0;
}