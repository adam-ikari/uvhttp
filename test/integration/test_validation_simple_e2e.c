/*
 * Validation E2E Test - Request validation tests
 * 请求验证的端到端测试
 */

#include "uvhttp_validation.h"

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
} app_context_t;

/* Signal handler */
static void signal_handler(int sig) {
    (void)sig;
    exit(0);
}

/* ==================== Request Handlers ==================== */

static int validate_json_handler(uvhttp_request_t* request,
                                 uvhttp_response_t* response) {
    const char* content_type =
        uvhttp_request_get_header(request, "Content-Type");

    if (!content_type || strstr(content_type, "application/json") == NULL) {
        const char* body = "Invalid Content-Type. Expected application/json";
        uvhttp_response_set_status(response, 415);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, body, strlen(body));
        uvhttp_response_send(response);
        return 0;
    }

    const char* body = uvhttp_request_get_body(request);
    size_t body_len = uvhttp_request_get_body_length(request);

    if (!body || body_len == 0) {
        const char* error = "Empty request body";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    /* Simple JSON validation (check for { and }) */
    if (body[0] != '{' || body[body_len - 1] != '}') {
        const char* error = "Invalid JSON format";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    const char* success = "Valid JSON received";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, success, strlen(success));
    uvhttp_response_send(response);

    return 0;
}

static int validate_path_handler(uvhttp_request_t* request,
                                 uvhttp_response_t* response) {
    const char* path = uvhttp_request_get_path(request);

    /* Validate path format */
    if (!path || strlen(path) == 0) {
        const char* error = "Invalid path";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    /* Check for path traversal attempts */
    if (strstr(path, "..") != NULL) {
        const char* error = "Path traversal detected";
        uvhttp_response_set_status(response, 403);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    char response_body[256];
    snprintf(response_body, sizeof(response_body), "Valid path: %s", path);

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);

    return 0;
}

static int validate_query_handler(uvhttp_request_t* request,
                                  uvhttp_response_t* response) {
    const char* query_string = uvhttp_request_get_query_string(request);

    if (!query_string || strlen(query_string) == 0) {
        const char* error = "Missing query parameters";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    /* Check for required parameters */
    if (strstr(query_string, "name=") == NULL) {
        const char* error = "Missing required parameter: name";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    const char* success = "Valid query parameters";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, success, strlen(success));
    uvhttp_response_send(response);

    return 0;
}

static int validate_headers_handler(uvhttp_request_t* request,
                                    uvhttp_response_t* response) {
    const char* user_agent = uvhttp_request_get_header(request, "User-Agent");
    const char* accept = uvhttp_request_get_header(request, "Accept");

    if (!user_agent) {
        const char* error = "Missing User-Agent header";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    if (!accept) {
        const char* error = "Missing Accept header";
        uvhttp_response_set_status(response, 400);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, error, strlen(error));
        uvhttp_response_send(response);
        return 0;
    }

    const char* success = "Valid headers";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, success, strlen(success));
    uvhttp_response_send(response);

    return 0;
}

/* ==================== Helper Functions ==================== */

static int send_http_request(const char* method, const char* path,
                             char* response, size_t max_len) {
    char cmd[512];
    snprintf(cmd, sizeof(cmd),
             "curl -s -w '\n%%{http_code}' -X %s http://127.0.0.1:8780%s "
             "2>/dev/null",
             method, path);

    FILE* pipe = popen(cmd, "r");
    if (!pipe) {
        return -1;
    }

    size_t bytes_read = fread(response, 1, max_len - 1, pipe);
    response[bytes_read] = '\0';

    int status = pclose(pipe);
    return (status == 0) ? (int)bytes_read : -1;
}

static int send_http_request_with_body(const char* method, const char* path,
                                       const char* body,
                                       const char* content_type, char* response,
                                       size_t max_len) {
    char cmd[1024];
    snprintf(cmd, sizeof(cmd),
             "curl -s -w '\n%%{http_code}' -X %s -H 'Content-Type: %s' -d '%s' "
             "http://127.0.0.1:8780%s 2>/dev/null",
             method, content_type, body, path);

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

    const char* port = "8780";
    if (argc > 1) {
        port = argv[1];
    }

    printf("\n========================================\n");
    printf("  Validation E2E Test\n");
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
    printf("  ✓ POST /validate/json\n");
    printf("  ✓ GET /validate/path\n");
    printf("  ✓ GET /validate/query\n");
    printf("  ✓ GET /validate/headers\n");

    uvhttp_router_add_route(app.router, "/validate/json",
                            validate_json_handler);
    uvhttp_router_add_route(app.router, "/validate/path",
                            validate_path_handler);
    uvhttp_router_add_route(app.router, "/validate/query",
                            validate_query_handler);
    uvhttp_router_add_route(app.router, "/validate/headers",
                            validate_headers_handler);

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

    char response[512];

    /* Test JSON validation - valid */
    printf("\n=== Testing JSON Validation (Valid) ===\n");
    send_http_request_with_body("POST", "/validate/json", "{\"name\":\"test\"}",
                                "application/json", response, sizeof(response));
    assert(strstr(response, "Valid JSON received") != NULL);
    assert(strstr(response, "200") != NULL);
    printf("✓ Valid JSON test passed\n");

    /* Test JSON validation - invalid format */
    printf("\n=== Testing JSON Validation (Invalid Format) ===\n");
    send_http_request_with_body("POST", "/validate/json", "not json",
                                "application/json", response, sizeof(response));
    assert(strstr(response, "Invalid JSON format") != NULL);
    assert(strstr(response, "400") != NULL);
    printf("✓ Invalid JSON format test passed\n");

    /* Test JSON validation - wrong content type */
    printf("\n=== Testing JSON Validation (Wrong Content-Type) ===\n");
    send_http_request_with_body("POST", "/validate/json", "{\"name\":\"test\"}",
                                "text/plain", response, sizeof(response));
    assert(strstr(response, "Invalid Content-Type") != NULL);
    assert(strstr(response, "415") != NULL);
    printf("✓ Wrong Content-Type test passed\n");

    /* Test path validation - valid */
    printf("\n=== Testing Path Validation (Valid) ===\n");
    send_http_request("GET", "/validate/path", response, sizeof(response));
    assert(strstr(response, "Valid path") != NULL);
    assert(strstr(response, "200") != NULL);
    printf("✓ Valid path test passed\n");

    /* Test path validation - path traversal */
    printf("\n=== Testing Path Validation (Path Traversal) ===\n");
    send_http_request("GET", "/validate/path/../../../etc/passwd", response,
                      sizeof(response));
    assert(strstr(response, "Path traversal detected") != NULL);
    assert(strstr(response, "403") != NULL);
    printf("✓ Path traversal test passed\n");

    /* Test query validation - valid */
    printf("\n=== Testing Query Validation (Valid) ===\n");
    send_http_request("GET", "/validate/query?name=test&age=25", response,
                      sizeof(response));
    assert(strstr(response, "Valid query parameters") != NULL);
    assert(strstr(response, "200") != NULL);
    printf("✓ Valid query test passed\n");

    /* Test query validation - missing parameter */
    printf("\n=== Testing Query Validation (Missing Parameter) ===\n");
    send_http_request("GET", "/validate/query?age=25", response,
                      sizeof(response));
    assert(strstr(response, "Missing required parameter") != NULL);
    assert(strstr(response, "400") != NULL);
    printf("✓ Missing parameter test passed\n");

    /* Test header validation - valid */
    printf("\n=== Testing Header Validation (Valid) ===\n");
    send_http_request_with_body("GET", "/validate/headers", "",
                                "application/json", response, sizeof(response));
    assert(strstr(response, "Valid headers") != NULL);
    assert(strstr(response, "200") != NULL);
    printf("✓ Valid headers test passed\n");

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
