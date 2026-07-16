/**
 * @file simple_api_demo.c
 * @brief UVHTTP minimal server demo - the smallest possible HTTP server using
 *        the core API.
 *
 * Shows the bare-minimum path: loop -> server -> router -> route -> listen.
 */

#include "../include/uvhttp.h"
#include "../include/uvhttp_constants.h"
#include <stdio.h>
#include <string.h>

/* Simple HTML handler - echoes back the request path and method. */
static int simple_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    const char* html_prefix =
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>UVHTTP Simple Demo</title></head>"
        "<body>"
        "<h1>UVHTTP Simple Demo</h1>"
        "<p>This is the simplest HTTP server built with the core API.</p>"
        "<p>Request path: ";

    const char* path = uvhttp_request_get_path(req);
    if (!path) {
        path = "unknown";
    }

    const char* method = uvhttp_request_get_method(req);
    if (!method) {
        method = "UNKNOWN";
    }

    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/html; charset=utf-8");

    char response[1024];
    snprintf(response, sizeof(response),
        "%s%s</p>"
        "<p>Method: %s</p>"
        "</body>"
        "</html>", html_prefix, path, method);

    uvhttp_response_set_body(res, response, strlen(response));

    return uvhttp_response_send(res);
}

int main() {
    printf("Starting UVHTTP minimal demo server...\n");
    printf("Server will run at http://localhost:%d\n", UVHTTP_DEFAULT_PORT);
    printf("Press Ctrl+C to stop the server.\n");
    printf("\nThis demonstrates the minimal usage of the core API.\n");
    printf("Just a few lines of code start a full HTTP server!\n\n");

    /* Create the event loop. */
    uv_loop_t* loop = uv_default_loop();

    /* Create the server (output-parameter style). */
    uvhttp_server_t* server = NULL;
    uvhttp_error_t server_result = uvhttp_server_new(loop, &server);
    if (server_result != UVHTTP_OK || !server) {
        fprintf(stderr, "Failed to create server: %s\n",
                uvhttp_error_string(server_result));
        return 1;
    }

    /* Create the router (output-parameter style) and wire it to the server. */
    uvhttp_router_t* router = NULL;
    uvhttp_error_t result = uvhttp_router_new(&router);
    if (result != UVHTTP_OK || !router) {
        fprintf(stderr, "Failed to create router: %s\n",
                uvhttp_error_string(result));
        uvhttp_server_free(server);
        return 1;
    }
    uvhttp_server_set_router(server, router);

    /* Add a catch-all route. */
    uvhttp_router_add_route(router, "/*", simple_handler);

    /* Start listening. */
    uvhttp_error_t listen_result =
        uvhttp_server_listen(server, UVHTTP_DEFAULT_HOST, UVHTTP_DEFAULT_PORT);
    if (listen_result != UVHTTP_OK) {
        fprintf(stderr, "Server failed to start: %s\n",
                uvhttp_error_string(listen_result));
        uvhttp_server_free(server);
        return 1;
    }

    /* Run the event loop. */
    uv_run(loop, UV_RUN_DEFAULT);

    /* Cleanup. */
    uvhttp_server_free(server);

    return 0;
}
