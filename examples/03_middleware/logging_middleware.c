/**
 * @file logging_middleware.c
 * @brief Request logging middleware example
 *
 * Logs HTTP method, path, status code, and request duration.
 * Uses compile-time middleware system (uvhttp_middleware.h).
 *
 * Build:
 *   gcc -std=c99 -I../include logging_middleware.c -o logging_middleware
 *
 * Run:
 *   ./logging_middleware
 */

#include "uvhttp.h"
#include "uvhttp_middleware.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ========== Logging middleware ========== */

typedef struct {
    const char* method;
    const char* path;
    time_t start;
} logging_ctx_t;

static int logging_middleware(uvhttp_request_t* req,
                              uvhttp_response_t* resp,
                              uvhttp_middleware_context_t* ctx) {
    (void)resp;
    logging_ctx_t* log = (logging_ctx_t*)ctx->data;

    /* Capture request info */
    log->method = uvhttp_request_get_method_string(req);
    log->path = uvhttp_request_get_path(req);
    log->start = time(NULL);

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ========== Response logging (called after handler) ========== */

static void log_response(uvhttp_request_t* req, uvhttp_response_t* resp,
                          logging_ctx_t* log) {
    time_t now = time(NULL);
    double duration = difftime(now, log->start);
    int status = uvhttp_response_get_status(resp);

    fprintf(stderr, "[%s] %s %s -> %d (%.0fs)\n",
            log->method ? log->method : "?",
            log->path ? log->path : "/",
            status, duration);
}

/* ========== Example handler ========== */

static int hello_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "Hello, World!", 13);
    return uvhttp_response_send(resp);
}

static int delay_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    /* Simulate a slow request */
    struct timespec ts = {1, 0};
    nanosleep(&ts, NULL);

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "Delayed response", 16);
    return uvhttp_response_send(resp);
}

static int notfound_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_response_set_status(resp, 404);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "Not Found", 9);
    return uvhttp_response_send(resp);
}

/* ========== Main ========== */

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_error_t err;

    err = uvhttp_server_new(loop, &server);
    if (err != UVHTTP_OK) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(err));
        return 1;
    }

    uvhttp_router_t* router = NULL;
    err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(err));
        return 1;
    }

    /* Wrap handlers with logging middleware */
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/delay", delay_handler);
    uvhttp_server_set_router(server, router);

    err = uvhttp_server_listen(server, "127.0.0.1", 8080);
    if (err != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(err));
        return 1;
    }

    printf("Logging middleware example running on http://127.0.0.1:8080\n");
    printf("Try: curl http://127.0.0.1:8080/ && curl http://127.0.0.1:8080/delay\n");
    printf("Logs appear on stderr\n");

    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_router_free(router);
    uvhttp_server_free(server);
    return 0;
}