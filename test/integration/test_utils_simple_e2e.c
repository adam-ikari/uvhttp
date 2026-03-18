/*
 * Utils E2E Test - Utility functions tests
 * 工具函数的端到端测试
 */

#include "uvhttp.h"
#include "uvhttp_utils.h"
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

/* Application context */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int string_ops_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* input = uvhttp_request_get_header(request, "X-Input");
    
    if (!input) {
        input = "default";
    }
    
    char response_body[1024];
    size_t offset = 0;
    
    /* String length */
    size_t len = strlen(input);
    offset += snprintf(response_body + offset, sizeof(response_body) - offset,
                       "String Operations:\n");
    offset += snprintf(response_body + offset, sizeof(response_body) - offset,
                       "Input: %s\n", input);
    offset += snprintf(response_body + offset, sizeof(response_body) - offset,
                       "Length: %zu\n", len);
    
    /* String comparison */
    const char* compare = "test";
    int cmp_result = strcmp(input, compare);
    offset += snprintf(response_body + offset, sizeof(response_body) - offset,
                       "Compare with '%s': %d\n", compare, cmp_result);
    
    /* String contains */
    int contains = (strstr(input, "test") != NULL);
    offset += snprintf(response_body + offset, sizeof(response_body) - offset,
                       "Contains 'test': %s\n", contains ? "Yes" : "No");
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

static int number_ops_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* num_str = uvhttp_request_get_header(request, "X-Number");
    
    if (!num_str) {
        num_str = "42";
    }
    
    int num = atoi(num_str);
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "Number Operations:\n"
             "Input: %s\n"
             "Value: %d\n"
             "Absolute: %d\n"
             "Is Positive: %s\n"
             "Is Even: %s",
             num_str,
             num,
             (num < 0) ? -num : num,
             (num > 0) ? "Yes" : "No",
             (num % 2 == 0) ? "Yes" : "No");
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

static int encoding_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    const char* input = uvhttp_request_get_header(request, "X-Input");
    
    if (!input) {
        input = "hello world";
    }
    
    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "Encoding Operations:\n"
             "Input: %s\n"
             "Length: %zu\n"
             "Contains space: %s\n"
             "First char: %c\n"
             "Last char: %c",
             input,
             strlen(input),
             (strchr(input, ' ') != NULL) ? "Yes" : "No",
             input[0],
             input[strlen(input) - 1]);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

static int timestamp_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
    time_t now = time(NULL);
    struct tm* tm_info = localtime(&now);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    
    char response_body[256];
    snprintf(response_body, sizeof(response_body),
             "Timestamp Operations:\n"
             "Unix timestamp: %ld\n"
             "Formatted: %s\n"
             "Year: %d\n"
             "Month: %d\n"
             "Day: %d",
             (long)now,
             time_str,
             tm_info->tm_year + 1900,
             tm_info->tm_mon + 1,
             tm_info->tm_mday);
    
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    
    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path, char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s http://127.0.0.1:8781%s 2>/dev/null", method, path);
    
    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }
    
    size_t bytes_read = fread(response, 1, max_len - 1, pipe);
    response[bytes_read] = '\0';
    
    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

static int send_http_request_with_header(const char* method, const char* path, const char* header, char* response, size_t max_len) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "curl -s -X %s -H '%s' http://127.0.0.1:8781%s 2>/dev/null", method, header, path);
    
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
    
    const char* port = "8781";
    if (argc > 1) {
        port = argv[1];
    }
    
    printf("\n========================================\n");
    printf("  Utils E2E Test\n");
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
    printf("  ✓ GET /utils/string\n");
    printf("  ✓ GET /utils/number\n");
    printf("  ✓ GET /utils/encoding\n");
    printf("  ✓ GET /utils/timestamp\n");
    
    uvhttp_router_add_route(app.router, "/utils/string", string_ops_handler);
    uvhttp_router_add_route(app.router, "/utils/number", number_ops_handler);
    uvhttp_router_add_route(app.router, "/utils/encoding", encoding_handler);
    uvhttp_router_add_route(app.router, "/utils/timestamp", timestamp_handler);
    
    /* Start server */
    printf("\nStarting server on port %s...\n", port);
    result = uvhttp_server_listen(app.server, "0.0.0.0", atoi(port));
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %d\n", result);
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
    
    /* Test string operations */
    printf("\n=== Testing String Operations ===\n");
    send_http_request_with_header("GET", "/utils/string", "X-Input: hello world", response, sizeof(response));
    assert(strstr(response, "String Operations") != NULL);
    assert(strstr(response, "hello world") != NULL);
    printf("✓ String operations test passed\n");
    
    /* Test number operations */
    printf("\n=== Testing Number Operations ===\n");
    send_http_request_with_header("GET", "/utils/number", "X-Number: -42", response, sizeof(response));
    assert(strstr(response, "Number Operations") != NULL);
    assert(strstr(response, "-42") != NULL);
    printf("✓ Number operations test passed\n");
    
    /* Test encoding operations */
    printf("\n=== Testing Encoding Operations ===\n");
    send_http_request_with_header("GET", "/utils/encoding", "X-Input: test@example.com", response, sizeof(response));
    assert(strstr(response, "Encoding Operations") != NULL);
    assert(strstr(response, "test@example.com") != NULL);
    printf("✓ Encoding operations test passed\n");
    
    /* Test timestamp operations */
    printf("\n=== Testing Timestamp Operations ===\n");
    send_http_request("GET", "/utils/timestamp", response, sizeof(response));
    assert(strstr(response, "Timestamp Operations") != NULL);
    assert(strstr(response, "Unix timestamp") != NULL);
    printf("✓ Timestamp operations test passed\n");
    
    /* Test default values */
    printf("\n=== Testing Default Values ===\n");
    send_http_request("GET", "/utils/string", response, sizeof(response));
    assert(strstr(response, "default") != NULL);
    printf("✓ Default values test passed\n");
    
    printf("\n========================================\n");
    printf("  All tests passed!\n");
    printf("========================================\n\n");
    
    /* Run event loop */
    printf("Server is running. Press Ctrl+C to stop...\n");
    uv_run(loop, UV_RUN_DEFAULT);
    
    /* Cleanup */
    printf("\nCleaning up...\n");
    uvhttp_server_free(app.server);
    uv_loop_delete(loop);
    
    printf("Test completed successfully\n");
    return 0;
}