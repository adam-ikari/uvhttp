/**
 * UVHTTP Simple Server Example
 *
 * This is the simplest possible HTTP server using UVHTTP.
 * It demonstrates the basic setup and routing pattern.
 *
 * Build:
 *   gcc -o simple_server simple_server.c -I../include -L../build/dist/lib -luvhttp -luv
 *
 * Run:
 *   export LD_LIBRARY_PATH=../build/dist/lib:$LD_LIBRARY_PATH
 *   ./simple_server
 *
 * Test:
 *   curl http://localhost:8080/
 *   curl http://localhost:8080/hello
 *   curl http://localhost:8080/api/data
 */

#include <stdio.h>
#include <uvhttp.h>
#include <uv.h>

/**
 * Handler for the root path "/"
 */
int root_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    // Set HTTP status code
    uvhttp_response_set_status(res, 200);

    // Set response headers
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_header(res, "X-Powered-By", "UVHTTP/2.5.0");

    // Set response body
    uvhttp_response_set_body(res, "Welcome to UVHTTP Simple Server!\n"
                              "Try: /hello, /api/data");

    // Send response
    return uvhttp_response_send(res);
}

/**
 * Handler for the "/hello" path
 */
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "Hello, World!");
    return uvhttp_response_send(res);
}

/**
 * Handler for the "/api/data" path (JSON response)
 */
int api_data_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res,
        "{"
        "  \"message\": \"Hello from UVHTTP\","
        "  \"version\": \"2.5.0\","
        "  \"status\": \"success\""
        "}"
    );
    return uvhttp_response_send(res);
}

/**
 * Main function - server setup and event loop
 */
int main(void) {
    // Create libuv event loop
    uv_loop_t* loop = uv_default_loop();
    if (!loop) {
        fprintf(stderr, "Failed to create event loop\n");
        return 1;
    }

    // Create UVHTTP server
    uvhttp_server_t* server = uvhttp_server_new(loop);
    if (!server) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    // Create router
    uvhttp_router_t* router = uvhttp_router_new();
    if (!router) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        return 1;
    }

    // Attach router to server
    server->router = router;

    // Add routes
    uvhttp_router_add_route(router, "/", root_handler);
    uvhttp_router_add_route(router, "/hello", hello_handler);
    uvhttp_router_add_route(router, "/api/data", api_data_handler);

    // Start listening on all interfaces, port 8080
    int result = uvhttp_server_listen(server, "0.0.0.0", 8080);
    if (result != UVHTTP_OK) {
        fprintf(stderr, "Failed to start server: %s\n", uvhttp_error_string(result));
        uvhttp_server_free(server);
        return 1;
    }

    printf("UVHTTP Simple Server\n");
    printf("====================\n");
    printf("Listening on http://0.0.0.0:8080\n");
    printf("\n");
    printf("Available endpoints:\n");
    printf("  GET  /           - Welcome message\n");
    printf("  GET  /hello      - Hello World\n");
    printf("  GET  /api/data   - JSON response\n");
    printf("\n");
    printf("Press Ctrl+C to stop\n");

    // Run the event loop (blocking)
    uv_run(loop, UV_RUN_DEFAULT);

    // Cleanup (this code is unreachable in normal operation)
    uvhttp_server_free(server);

    return 0;
}
