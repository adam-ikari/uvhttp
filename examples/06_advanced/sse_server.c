/**
 * @file sse_server.c
 * @brief Server-Sent Events (SSE) example
 *
 * Demonstrates real-time event streaming using standard HTTP/1.1.
 * Uses uv_timer to send events asynchronously without blocking the
 * event loop.
 *
 * Test:
 *   curl -N http://127.0.0.1:8080/events
 *   # Or open in browser: http://127.0.0.1:8080/
 */

#include "uvhttp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ========== SSE context (per-connection state) ========== */

typedef struct {
    uvhttp_response_t* resp;   /* SSE response handle */
    uv_timer_t timer;          /* periodic event timer */
    int count;                 /* event counter */
    int max_events;            /* stop after this many */
} sse_ctx_t;

/* ========== SSE event sender ========== */

static int sse_send_event(const char* event, const char* data,
                           uvhttp_response_t* resp) {
    char buf[4096];
    int n = 0;

    if (event) {
        n += snprintf(buf + n, sizeof(buf) - n, "event: %s\n", event);
    }
    n += snprintf(buf + n, sizeof(buf) - n, "data: %s\n\n", data);

    return uvhttp_response_send_raw(buf, n, resp->client, resp);
}

/* ========== Timer callback (fires every 1s) ========== */

static void sse_timer_cb(uv_timer_t* timer) {
    sse_ctx_t* ctx = (sse_ctx_t*)timer->data;
    if (!ctx || !ctx->resp) return;

    if (ctx->count >= ctx->max_events) {
        /* Send done event and stop */
        sse_send_event("done", "{\"reason\": \"max_events\"}", ctx->resp);
        uv_timer_stop(timer);
        uvhttp_free(ctx);
        return;
    }

    char data[128];
    time_t now = time(NULL);
    snprintf(data, sizeof(data),
             "{\"count\": %d, \"timestamp\": %ld}", ctx->count, (long)now);

    if (sse_send_event("tick", data, ctx->resp) != 0) {
        /* Client disconnected — stop timer and free context */
        uv_timer_stop(timer);
        uvhttp_free(ctx);
        return;
    }
    ctx->count++;
}

/* ========== SSE event handler ========== */

static int events_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;

    /* Set SSE headers and send HTTP response head directly.
     * We cannot use uvhttp_response_send because it sets Content-Length.
     * SSE needs an unbounded response stream. */
    const char* head =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "\r\n";
    uvhttp_response_send_raw(head, strlen(head), resp->client, resp);

    /* Send initial comment (some proxies drop the first message) */
    uvhttp_response_send_raw(": SSE connection established\n\n", 30, resp->client, resp);

    /* Allocate per-connection context */
    sse_ctx_t* ctx = uvhttp_alloc(sizeof(sse_ctx_t));
    if (!ctx) return UVHTTP_ERROR_OUT_OF_MEMORY;

    ctx->resp = resp;
    ctx->count = 0;
    ctx->max_events = 10;

    /* Start a 1-second periodic timer on the default loop */
    uv_timer_init(uv_default_loop(), &ctx->timer);
    ctx->timer.data = ctx;
    uv_timer_start(&ctx->timer, sse_timer_cb, 1000, 1000);

    /* Send initial comment (some proxies drop the first message) */
    uvhttp_response_send_raw(": SSE connection established\n\n", 30, resp->client, resp);

    return 0;
}

/* ========== Index page ========== */

static int index_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    const char* html =
        "<!DOCTYPE html>"
        "<html><head><title>SSE Demo</title></head>"
        "<body>"
        "<h1>Server-Sent Events</h1>"
        "<div id=\"events\"></div>"
        "<script>"
        "var es = new EventSource('/events');"
        "es.addEventListener('tick', function(e) {"
        "  var div = document.getElementById('events');"
        "  div.innerHTML += '<p>' + e.data + '</p>';"
        "});"
        "es.addEventListener('done', function(e) {"
        "  es.close();"
        "  document.getElementById('events').innerHTML += '<p><strong>Done</strong></p>';"
        "});"
        "</script>"
        "</body></html>";

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/html");
    uvhttp_response_set_body(resp, html, strlen(html));
    return uvhttp_response_send(resp);
}

/* ========== Main ========== */

int main(int argc, char** argv) {
    int port = 8080;
    if (argc > 1) {
        port = atoi(argv[1]);
        if (port <= 0 || port > 65535) {
            fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
    }

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

    uvhttp_router_add_route(router, "/", index_handler);
    uvhttp_router_add_route(router, "/events", events_handler);
    uvhttp_server_set_router(server, router);

    err = uvhttp_server_listen(server, "127.0.0.1", port);
    if (err != UVHTTP_OK) {
        fprintf(stderr, "Failed to listen: %s\n", uvhttp_error_string(err));
        return 1;
    }

    printf("SSE server running on http://127.0.0.1:%d\n", port);
    printf("  curl -N http://127.0.0.1:%d/events\n", port);
    printf("  Browser: http://127.0.0.1:%d/\n", port);

    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_router_free(router);
    uvhttp_server_free(server);
    return 0;
}