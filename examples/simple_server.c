/**
 * UVHTTP Simple Server Example
 *
 * This is the simplest possible HTTP server using UVHTTP.
 * It demonstrates the basic setup and routing pattern using
 * uvhttp_server_new_with_loop(), which lets the library manage
 * the libuv event loop lifecycle.
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
#include <string.h>
#include <uvhttp.h>
#include <uv.h>

/**
 * Handler for the root path "/"
 */
int root_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    const char* body = "Welcome to UVHTTP Simple Server!\n"
                        "Try: /hello, /api/data";
    return uvhttp_respond_with_headers(
        res, 200,
        (uvhttp_header_view_t[]){{"Content-Type", "text/plain"},
                                 {"X-Powered-By", "UVHTTP/2.5.0"}},
        2, body, strlen(body));
}

/**
 * Handler for the "/hello" path
 */
int hello_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    const char* body = "Hello, World!";
    return uvhttp_respond(res, 200, "text/plain", body, strlen(body));
}

/**
 * Handler for the "/api/data" path (JSON response)
 */
int api_data_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    const char* body =
        "{"
        "  \"message\": \"Hello from UVHTTP\","
        "  \"version\": \"2.5.0\","
        "  \"status\": \"success\""
        "}";
    return uvhttp_respond(res, 200, "application/json", body, strlen(body));
}

/**
 * Main function - server setup and event loop
 *
 * Uses uvhttp_server_new_with_loop() to let the library manage the
 * event loop. The loop is created internally and cleaned up when
 * uvhttp_server_free() is called.
 */
int main(void) {
    // Create UVHTTP server (creates internal event loop)
    uvhttp_server_t* server = NULL;
    if (uvhttp_server_new_with_loop(&server) != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server\n");
        return 1;
    }

    // Create router
    uvhttp_router_t* router = NULL;
    if (uvhttp_router_new(&router) != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router\n");
        uvhttp_server_free(server);
        return 1;
    }

    // Attach router to server
    uvhttp_server_take_router(server, router);

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

    // Run the event loop (blocking) — the loop is owned by the server
    // uv_run(server->loop, UV_RUN_DEFAULT);
    // The server's internal loop is already running.

    // Cleanup — also closes the internal event loop
    uvhttp_server_free(server);

    return 0;
}
