/**
 * @file sse_server.c
 * @brief Server-Sent Events (SSE) example
 *
 * Demonstrates real-time event streaming using standard HTTP/1.1.
 * SSE is simpler than WebSocket: unidirectional, text-based, works
 * over plain HTTP without upgrade.
 *
 * Build:
 *   gcc -std=c99 -I../include sse_server.c -o sse_server
 *   -L../build/dist/lib -luvhttp -luv -lpthread -lm -ldl
 *
 * Run:
 *   ./sse_server
 *
 * Test:
 *   curl -N http://127.0.0.1:8080/events
 *   # Or open in browser: http://127.0.0.1:8080/
 */

#include "uvhttp.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

/* ========== SSE helper ========== */

/* Send an SSE event via uvhttp_response_send_raw.
 * The response must already have been started with status 200 and
 * Content-Type: text/event-stream before calling this. */
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

/* ========== SSE event handler ========== */

static int events_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;

    /* Set SSE headers */
    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type", "text/event-stream");
    uvhttp_response_set_header(resp, "Cache-Control", "no-cache");
    uvhttp_response_set_header(resp, "Connection", "keep-alive");

    /* Send initial comment (some proxies drop the first message) */
    uvhttp_response_send_raw(": SSE connection established\n\n", 30, resp->client, resp);

    /* Send periodic events */
    int count = 0;
    while (count < 10) {
        char data[128];
        time_t now = time(NULL);
        snprintf(data, sizeof(data),
                 "{\"count\": %d, \"timestamp\": %ld}", count, (long)now);

        if (sse_send_event("tick", data, resp) != 0) {
            break; /* Client disconnected */
        }
        count++;

        struct timespec ts = {1, 0};
        nanosleep(&ts, NULL);
    }

    /* Send done event */
    sse_send_event("done", "{\"reason\": \"max_events\"}", resp);

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
    printf("  curl -N http://127.0.0.1:8080/events\n");
    printf("  Browser: http://127.0.0.1:8080/\n");

    uv_run(loop, UV_RUN_DEFAULT);

    uvhttp_router_free(router);
    uvhttp_server_free(server);
    return 0;
}