/**
 * @file logging_middleware.c
 * @brief Request logging middleware example
 *
 * Logs HTTP method, path, status code, and request duration.
 * Uses the compile-time middleware system (uvhttp_middleware.h):
 * a logging middleware captures the request (method/path/start time)
 * and handlers run it with UVHTTP_EXECUTE_MIDDLEWARE before building
 * the response; the response line is logged after uvhttp_response_send.
 *
 * Build (after building the library):
 *   cc -I../include -I../deps/uthash/src -I../deps/llhttp/include \
 *      logging_middleware.c -o logging_middleware \
 *      -L../build/dist/lib -luvhttp -luv -lmbedtls -lmbedx509 -lmbedcrypto \
 *      -lxxhash -lllhttp -lpthread -lm -ldl
 *
 * Run:
 *   ./logging_middleware
 */

#include "uvhttp.h"
#include "uvhttp_middleware.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* ========== Logging middleware ========== */

typedef struct {
    const char* method;
    const char* path;
    time_t start;
} logging_ctx_t;

static void logging_ctx_free(void* data) {
    free(data);
}

static int logging_middleware(uvhttp_request_t* req,
                              uvhttp_response_t* resp,
                              uvhttp_middleware_context_t* ctx) {
    (void)resp;
    logging_ctx_t* log = (logging_ctx_t*)ctx->data;

    /* Allocate the shared log record on first use; the middleware context
     * releases it via ctx->cleanup at the end of the chain. */
    if (!log) {
        log = (logging_ctx_t*)calloc(1, sizeof(logging_ctx_t));
        if (!log) {
            return UVHTTP_MIDDLEWARE_STOP;
        }
        ctx->data = log;
        ctx->cleanup = logging_ctx_free;
    }

    log->method = uvhttp_request_get_method(req);
    log->path = uvhttp_request_get_path(req);
    log->start = time(NULL);

    fprintf(stderr, "[log] %s %s\n",
            log->method ? log->method : "?",
            log->path ? log->path : "/");

    return UVHTTP_MIDDLEWARE_CONTINUE;
}

/* ========== Response logging (called after the handler sends) ========== */

static void log_response(uvhttp_response_t* resp, time_t start) {
    double duration = difftime(time(NULL), start);
    fprintf(stderr, "[log] -> %d (%.0fs)\n", resp->status_code, duration);
}

/* ========== Example handler ========== */

static int hello_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, logging_middleware);

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "Hello, World!", 13);
    int rc = uvhttp_response_send(resp);
    log_response(resp, time(NULL));
    return rc;
}

static int delay_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, logging_middleware);

    /* Simulate a slow request */
    struct timespec ts = {1, 0};
    nanosleep(&ts, NULL);

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "Delayed response", 16);
    int rc = uvhttp_response_send(resp);
    log_response(resp, time(NULL));
    return rc;
}

static int notfound_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    UVHTTP_EXECUTE_MIDDLEWARE(req, resp, logging_middleware);

    uvhttp_response_set_status(resp, 404);
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    uvhttp_response_set_body(resp, "Not Found", 9);
    int rc = uvhttp_response_send(resp);
    log_response(resp, time(NULL));
    return rc;
}

/* ========== Main ========== */

int main(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_server_t* server = NULL;
    uvhttp_router_t* router = NULL;
    uvhttp_error_t err;

    err = uvhttp_server_new(loop, &server);
    if (err != UVHTTP_OK || !server) {
        fprintf(stderr, "Failed to create server: %s\n", uvhttp_error_string(err));
        return 1;
    }

    err = uvhttp_router_new(&router);
    if (err != UVHTTP_OK || !router) {
        fprintf(stderr, "Failed to create router: %s\n", uvhttp_error_string(err));
        uvhttp_server_free(server);
        return 1;
    }

    /* Wrap handlers with the logging middleware */
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

    uvhttp_server_free(server);
    return 0;
}
