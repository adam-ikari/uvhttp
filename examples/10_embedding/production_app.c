/*
 * A complete "real embedder" application built on UVHTTP.
 *
 * This example is intentionally owned by the application layer: policy,
 * logging, shutdown behaviour and static content live here, not in the
 * library. It exercises the public seams an embedder actually uses:
 *
 *   - take_config / get_config          (single config channel)
 *   - request filter                    (admission policy owned by the app)
 *   - error callback                    (async failure observer)
 *   - get_stats                         (metrics read-only)
 *   - set_draining + async/stop          (rolling-deploy shutdown)
 *   - respond / respond_with_headers    (atomic response API)
 *   - logging interface                 (declared by UVHTTP, implemented here)
 *   - static file handling via a catch-all handler
 *
 * Build (from examples/):
 *   make -f Makefile.examples production_app
 * Run (from examples/), port is required — the library is linked statically,
 * so no LD_LIBRARY_PATH is needed:
 *   ./bin/production_app 18080
 *   (build_static because static file serving needs that feature on)
 * Try:
 *   curl localhost:18080/
 *   curl localhost:18080/json
 *   curl localhost:18080/metrics
 *   curl localhost:18080/index.html
 *   curl -H 'X-Deny: 1' localhost:18080/          -> 403
 *   curl -X POST localhost:18080/admin/drain      -> draining on
 *   kill -TERM <pid>                              -> drain, then stop
 */

/* Route the library's log sites into the application's logger. This macro
 * must be defined before any UVHTTP header that includes uvhttp_logging.h. */
#define UVHTTP_LOG_EMIT app_log_emit

#include <uvhttp.h>

#include "uvhttp_config.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"
#include "uvhttp_logging.h"
#include "uvhttp_static.h"

#include <stdarg.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

/* ------------------------------------------------------------------ */
/* Application state. All state is owned by the embedder, never global
 * inside UVHTTP. The single static pointer below belongs to the embedder:
 * it lets the interface function uvhttp_log_set_sink find this instance. */
/* ------------------------------------------------------------------ */

typedef struct app_state {
    uv_loop_t* loop;
    uvhttp_server_t* server;
    uvhttp_static_context_t* static_ctx;
    uv_signal_t sigint;
    uv_signal_t sigterm;
    uv_timer_t drain_timer;
    int shutting_down;
    unsigned long denied;
    unsigned long errors;

    /* Embedder-owned logging configuration */
    uvhttp_log_level_t log_min;
    uvhttp_log_sink_t log_sink;
    void* log_user_data;
} app_state_t;

static app_state_t* g_app = NULL; /* embedder-side, one process-wide app */

/* Implementation of the UVHTTP-declared logging interface. */
void uvhttp_log_set_sink(uvhttp_log_level_t min_level, uvhttp_log_sink_t sink,
                         void* user_data) {
    if (!g_app) {
        return;
    }
    g_app->log_min = min_level;
    g_app->log_sink = sink;
    g_app->log_user_data = user_data;
}

/* Emitter referenced by UVHTTP_LOG_EMIT above. */
static void app_log_emit(uvhttp_log_level_t level, const char* file, int line,
                         const char* func, const char* fmt, ...) {
    app_state_t* app = g_app;
    if (!app || level < app->log_min) {
        return;
    }
    char msg[1024];
    va_list args;
    va_start(args, fmt);
    vsnprintf(msg, sizeof(msg), fmt, args);
    va_end(args);
    if (app->log_sink) {
        app->log_sink(level, file, line, func, msg, app->log_user_data);
    }
}

static void app_sink(uvhttp_log_level_t level, const char* file, int line,
                     const char* func, const char* msg, void* user_data) {
    (void)file;
    (void)line;
    (void)user_data;
    static const char* const names[] = {"TRACE", "DEBUG", "INFO",
                                        "WARN",  "ERROR", "FATAL"};
    const char* name =
        (level >= UVHTTP_LOG_LEVEL_TRACE && level <= UVHTTP_LOG_LEVEL_FATAL)
            ? names[level]
            : "UNKNOWN";
    fprintf(stderr, "[%s] %s(): %s\n", name, func ? func : "?", msg);
}

/* ------------------------------------------------------------------ */
/* Application policy seams                                            */
/* ------------------------------------------------------------------ */

static uvhttp_error_t app_request_filter(uvhttp_request_t* req,
                                         uvhttp_response_t* resp,
                                         void* user_data) {
    app_state_t* app = (app_state_t*)user_data;
    const char* deny = uvhttp_request_get_header(req, "X-Deny");
    if (deny && strcmp(deny, "1") == 0) {
        ++app->denied;
        return UVHTTP_ERROR_INVALID_PARAM; /* library emits a 403 */
    }
    (void)resp;
    return UVHTTP_OK;
}

static void app_error_observer(uvhttp_server_t* server, uvhttp_error_t error,
                               uvhttp_connection_t* conn, void* user_data) {
    app_state_t* app = (app_state_t*)user_data;
    (void)server;
    (void)conn;
    ++app->errors;
    app_log_emit(UVHTTP_LOG_LEVEL_WARN, __FILE__, __LINE__, __func__,
                 "async error: %s", uvhttp_error_string(error));
}

/* ------------------------------------------------------------------ */
/* Handlers                                                            */
/* ------------------------------------------------------------------ */

static int hello_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    return uvhttp_respond(resp, 200, "text/plain", "hello from uvhttp\n", strlen("hello from uvhttp\n"));
}

static int json_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    uvhttp_header_view_t headers[] = {
        {"Content-Type", "application/json"},
        {"X-App", "production-example"},
    };
    /* strlen("{\"ok\":true}") == 11; passing 12 appended a NUL, which made
     * JSON.parse fail in the dashboard. */
    return uvhttp_respond_with_headers(resp, 200, headers, 2,
                                       "{\"ok\":true}", 11);
}

static int metrics_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    app_state_t* app = g_app;
    uvhttp_server_stats_t stats;
    if (uvhttp_server_get_stats(app->server, &stats) != UVHTTP_OK) {
        return uvhttp_respond(resp, 500, "text/plain", "stats unavailable",
                              strlen("stats unavailable"));
    }
    char body[256];
    int n = snprintf(body, sizeof(body),
                     "{\"active\":%zu,\"connections\":%llu,"
                     "\"requests\":%llu,\"denied\":%lu,\"errors\":%lu,"
                     "\"draining\":%d}",
                     stats.active_connections,
                     (unsigned long long)stats.total_connections,
                     (unsigned long long)stats.total_requests, app->denied,
                     app->errors, app->shutting_down ? 1 : 0);
    if (n < 0 || (size_t)n >= sizeof(body)) {
        return uvhttp_respond(resp, 500, "text/plain", "metrics too large",
                              strlen("metrics too large"));
    }
    uvhttp_header_view_t headers[] = {
        {"Content-Type", "application/json"},
        {"Cache-Control", "no-store"},
    };
    return uvhttp_respond_with_headers(resp, 200, headers, 2, body,
                                       (size_t)n);
}

static int drain_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    app_state_t* app = g_app;
    if (uvhttp_server_set_draining(app->server, 1) != UVHTTP_OK) {
        return uvhttp_respond(resp, 500, "text/plain", "drain failed", 12);
    }
    app->shutting_down = 1;
    return uvhttp_respond(resp, 200, "text/plain", "draining\n", 9);
}

/* Catch-all: serve static files; the application owns the 404 decision. */
static int static_catch_all(uvhttp_request_t* req, uvhttp_response_t* resp) {
    app_state_t* app = g_app;
    uvhttp_result_t rc = uvhttp_static_handle_request(app->static_ctx, req,
                                                      resp);
    if (rc != UVHTTP_OK && !resp->sent) {
        return uvhttp_respond(resp, 404, "text/plain", "Not Found\n", 10);
    }
    return UVHTTP_OK;
}

/* ------------------------------------------------------------------ */
/* Lifecycle                                                           */
/* ------------------------------------------------------------------ */

static void on_drain_timeout(uv_timer_t* timer) {
    app_state_t* app = (app_state_t*)timer->data;
    uv_signal_stop(&app->sigint);
    uv_signal_stop(&app->sigterm);
#if UVHTTP_FEATURE_ASYNC_STOP
    (void)uvhttp_server_async_stop(app->server);
#else
    (void)uvhttp_server_stop(app->server);
#endif
    app_log_emit(UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__,
                 "drain window closed, stopping");
    /* The timer is embedded in the stack-allocated app state: no free. */
    uv_close((uv_handle_t*)timer, NULL);
    uv_stop(app->loop);
}

static void begin_drain(app_state_t* app) {
    if (app->shutting_down) {
        return;
    }
    app->shutting_down = 1;
    (void)uvhttp_server_set_draining(app->server, 1);
    app_log_emit(UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__,
                 "draining: new requests get 503");
    uv_timer_init(app->loop, &app->drain_timer);
    app->drain_timer.data = app;
    uv_timer_start(&app->drain_timer, on_drain_timeout, 1000, 0);
}

static void on_signal(uv_signal_t* handle, int signum) {
    begin_drain((app_state_t*)handle->data);
    app_log_emit(UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__,
                 "signal %d received", signum);
}

static int run_app(int port) {
    app_state_t app;
    memset(&app, 0, sizeof(app));
    app.log_min = UVHTTP_LOG_LEVEL_INFO;
    g_app = &app;

    uvhttp_log_set_sink(UVHTTP_LOG_LEVEL_INFO, app_sink, NULL);
    app_log_emit(UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__,
                 "starting on port %d", port);

    app.loop = uv_loop_new();
    if (!app.loop) {
        return 1;
    }
    app.loop->data = &app;

    /* Config is installed on the server's context in one step. */
    uvhttp_config_t* config = NULL;
    if (uvhttp_config_new(&config) != UVHTTP_OK) {
        return 1;
    }
    uvhttp_config_set_defaults(config);
    config->max_connections = 512;
    config->backlog = 256;
    config->connection_timeout = 30;

    if (uvhttp_server_new(app.loop, &app.server) != UVHTTP_OK) {
        uvhttp_config_free(config);
        return 1;
    }
    if (uvhttp_server_take_config(app.server, config) != UVHTTP_OK) {
        uvhttp_config_free(config);
        return 1;
    }

    uvhttp_router_t* router = NULL;
    if (uvhttp_router_new(&router) != UVHTTP_OK) {
        return 1;
    }
    uvhttp_router_add_route(router, "/", hello_handler);
    uvhttp_router_add_route(router, "/json", json_handler);
    uvhttp_router_add_route(router, "/metrics", metrics_handler);
    uvhttp_router_add_route(router, "/admin/drain", drain_handler);
    uvhttp_server_take_router(app.server, router);

    uvhttp_server_set_handler(app.server, static_catch_all);
    uvhttp_server_set_request_filter(app.server, app_request_filter, &app);
    uvhttp_server_set_error_callback(app.server, app_error_observer, &app);
    uvhttp_server_enable_health_check(app.server, "/health");

    uvhttp_static_config_t static_config;
    memset(&static_config, 0, sizeof(static_config));
    strncpy(static_config.root_directory, "10_embedding/www",
            sizeof(static_config.root_directory) - 1);
    static_config.max_cache_size = 4 * 1024 * 1024;
    static_config.max_cache_entries = 64;
    static_config.cache_ttl = 300;
    if (uvhttp_static_create(&static_config, &app.static_ctx) != UVHTTP_OK) {
        return 1;
    }

    if (uvhttp_server_listen(app.server, "0.0.0.0", port) != UVHTTP_OK) {
        return 1;
    }

    uv_signal_init(app.loop, &app.sigint);
    uv_signal_init(app.loop, &app.sigterm);
    app.sigint.data = &app;
    app.sigterm.data = &app;
    uv_signal_start(&app.sigint, on_signal, SIGINT);
    uv_signal_start(&app.sigterm, on_signal, SIGTERM);

    app_log_emit(UVHTTP_LOG_LEVEL_INFO, __FILE__, __LINE__, __func__,
                 "listening; GET /metrics to inspect the server");

    uv_run(app.loop, UV_RUN_DEFAULT);

    uvhttp_static_free(app.static_ctx);
    uvhttp_server_free(app.server);
    /* uv_signal_stop is not uv_close: close both signal handles before the
     * final loop drain, otherwise uv_loop_close returns UV_EBUSY and libuv's
     * loop-owned allocations show up as leaks. */
    uv_close((uv_handle_t*)&app.sigint, NULL);
    uv_close((uv_handle_t*)&app.sigterm, NULL);
    uv_run(app.loop, UV_RUN_NOWAIT);
    uv_loop_close(app.loop);
    uvhttp_free(app.loop);
    g_app = NULL;
    return 0;
}

int main(int argc, char** argv) {
    /* The port is deployment configuration: require it on the command line
     * rather than hard-coding a default. */
    if (argc != 2) {
        fprintf(stderr, "usage: %s <port 1..65535>\n", argv[0]);
        return 2;
    }
    int port = atoi(argv[1]);
    if (port <= 0 || port > 65535) {
        fprintf(stderr, "invalid port: %s (must be 1..65535)\n", argv[1]);
        return 2;
    }
    return run_app(port);
}
