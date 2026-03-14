/**
 * @file compression_server.c
 * @brief Performance test server with HTTP response compression
 * 
 * This server demonstrates the compression feature:
 * - Enables compression for large responses (>1KB)
 * - Serves compressible content (text, JSON, HTML)
 * - Shows performance impact of compression
 * 
 * Build with compression enabled:
 *   cmake -B build_compression -DCMAKE_BUILD_TYPE=Release -DBUILD_WITH_COMPRESSION=ON ..
 *   cmake --build build_compression -j$(nproc)
 * 
 * Run:
 *   ./build_compression/dist/bin/compression_server <port>
 * 
 * Test with curl:
 *   curl -v http://localhost:8080/large-text
 *   curl -v -H "Accept-Encoding: gzip" http://localhost:8080/large-text
 *   curl -v -H "Accept-Encoding: deflate" http://localhost:8080/large-json
 */

#include "uvhttp.h"
#include "uvhttp_response.h"
#include "uvhttp_static.h"
#include "uvhttp_allocator.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Global server context
static uvhttp_server_t* g_server = NULL;

// Generate large text content (highly compressible)
static const char* generate_large_text(size_t size) {
    char* text = uvhttp_alloc(size + 1);
    if (!text) {
        return NULL;
    }

    // Repeat a pattern for good compression ratio
    const char* pattern = "This is a test string that will be compressed. "
                         "HTTP compression reduces network bandwidth usage "
                         "and improves page load times for text-based content. "
                         "JSON, HTML, CSS, and JavaScript are good candidates.\n";
    
    size_t pattern_len = strlen(pattern);
    size_t pos = 0;
    
    while (pos + pattern_len < size) {
        memcpy(text + pos, pattern, pattern_len);
        pos += pattern_len;
    }
    
    text[pos] = '\0';
    return text;
}

// Generate large JSON content
static const char* generate_large_json(size_t size) {
    char* json = uvhttp_alloc(size + 1);
    if (!json) {
        return NULL;
    }

    // Create a JSON array with many similar objects
    size_t pos = 0;
    pos += snprintf(json + pos, size - pos, "[\n");
    
    for (int i = 0; i < 100 && pos < size - 100; i++) {
        pos += snprintf(json + pos, size - pos,
                       "  {\"id\": %d, \"name\": \"Item %d\", \"description\": \"This is a test item\", "
                       "\"price\": 19.99, \"in_stock\": true, \"category\": \"test\"},\n",
                       i, i);
    }
    
    // Remove trailing comma
    if (pos > 2 && json[pos - 2] == ',') {
        pos -= 2;
        pos += snprintf(json + pos, size - pos, "\n");
    }
    
    pos += snprintf(json + pos, size - pos, "]\n");
    json[pos] = '\0';
    
    return json;
}

// Handler for large text response (with compression)
static void handle_large_text(uvhttp_request_t* request, void* user_data) {
    (void)user_data;
    
    uvhttp_response_t* response = uvhttp_request_get_response(request);
    assert(response != NULL);
    
    // Generate 10KB of text
    const char* text = generate_large_text(10 * 1024);
    if (!text) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return;
    }
    
    // Enable compression
    uvhttp_error_t result = uvhttp_response_set_compress(response, 1);
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to enable compression: %d\n", result);
    }
    
    // Set response
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, text, strlen(text));
    uvhttp_response_send(response);
    
    uvhttp_free((void*)text);
}

// Handler for large JSON response (with compression)
static void handle_large_json(uvhttp_request_t* request, void* user_data) {
    (void)user_data;
    
    uvhttp_response_t* response = uvhttp_request_get_response(request);
    assert(response != NULL);
    
    // Generate 20KB of JSON
    const char* json = generate_large_json(20 * 1024);
    if (!json) {
        uvhttp_response_set_status(response, 500);
        uvhttp_response_set_body(response, "Internal Server Error", 21);
        uvhttp_response_send(response);
        return;
    }
    
    // Enable compression with custom threshold
    uvhttp_error_t result = uvhttp_response_set_compress(response, 1);
    if (result != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to enable compression: %d\n", result);
    }
    
    uvhttp_response_set_compress_threshold(response, 512);  // Lower threshold
    
    // Set response
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "application/json");
    uvhttp_response_set_body(response, json, strlen(json));
    uvhttp_response_send(response);
    
    uvhttp_free((void*)json);
}

// Handler for small response (no compression)
static void handle_small(uvhttp_request_t* request, void* user_data) {
    (void)user_data;
    
    uvhttp_response_t* response = uvhttp_request_get_response(request);
    assert(response != NULL);
    
    const char* body = "Hello, World! This is a small response that won't be compressed.";
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
}

// Handler for status endpoint
static void handle_status(uvhttp_request_t* request, void* user_data) {
    (void)user_data;
    
    uvhttp_response_t* response = uvhttp_request_get_response(request);
    assert(response != NULL);
    
#if UVHTTP_FEATURE_COMPRESSION
    const char* status = "Compression: ENABLED\n";
#else
    const char* status = "Compression: DISABLED\n";
#endif
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, status, strlen(status));
    uvhttp_response_send(response);
}

int main(int argc, char** argv) {
    // Parse command line arguments
    const char* host = "0.0.0.0";
    int port = 8080;
    
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port number: %s\n", argv[1]);
            return 1;
        }
    }
    
    printf("========================================\n");
    printf("UVHTTP Compression Test Server\n");
    printf("========================================\n");
    printf("Host: %s\n", host);
    printf("Port: %d\n", port);
    
#if UVHTTP_FEATURE_COMPRESSION
    printf("Compression: ENABLED\n");
#else
    printf("Compression: DISABLED\n");
#endif
    
    printf("========================================\n");
    printf("\n");
    printf("Available endpoints:\n");
    printf("  GET /status          - Server status\n");
    printf("  GET /small           - Small response (no compression)\n");
    printf("  GET /large-text      - Large text response (compressed)\n");
    printf("  GET /large-json      - Large JSON response (compressed)\n");
    printf("\n");
    printf("Test with curl:\n");
    printf("  curl -v http://%s:%d/status\n", host, port);
    printf("  curl -v http://%s:%d/large-text\n", host, port);
    printf("  curl -v -H \"Accept-Encoding: gzip\" http://%s:%d/large-text\n", host, port);
    printf("\n");
    
    // Create event loop
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to create event loop\n");
        return 1;
    }
    
    // Create server
    g_server = uvhttp_server_new(loop);
    if (!g_server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }
    
    // Create router
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(g_server);
        return 1;
    }
    
    g_server->router = router;
    
    // Add routes
    uvhttp_router_add_route(router, "/status", handle_status);
    uvhttp_router_add_route(router, "/small", handle_small);
    uvhttp_router_add_route(router, "/large-text", handle_large_text);
    uvhttp_router_add_route(router, "/large-json", handle_large_json);
    
    // Start server
    uvhttp_error_t result = uvhttp_server_listen(g_server, host, port);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
        uvhttp_server_free(g_server);
        return 1;
    }
    
    printf("Server started successfully!\n");
    printf("Press Ctrl+C to stop\n");
    printf("\n");
    
    // Run event loop
    uv_run(loop, UV_RUN_DEFAULT);
    
    // Cleanup
    printf("\n");
    printf("Shutting down server...\n");
    uvhttp_server_free(g_server);
    
    printf("Server stopped.\n");
    return 0;
}