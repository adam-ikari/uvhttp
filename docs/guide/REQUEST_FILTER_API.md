---
title: Request Filter API
description: UVHTTP's pre-dispatch request filter - the seam where application-level admission policy (rate limiting, allow/deny lists, quotas) runs, with a per-client fixed-window example.
---

# Request Filter API

## Overview

UVHTTP ships no rate limiter. Since v3.0.0 the built-in one is gone, replaced by
a single hook that runs before a request is dispatched:

```c
typedef uvhttp_error_t (*uvhttp_request_filter_t)(uvhttp_request_t* request,
                                                 uvhttp_response_t* response,
                                                 void* user_data);

uvhttp_error_t uvhttp_server_set_request_filter(uvhttp_server_t* server,
                                                uvhttp_request_filter_t filter,
                                                void* user_data);
```

The removal was deliberate. The old limiter counted every request on one
server-wide counter, so a single abusive client returned 429 for every visitor,
and its whitelist matched IPv4 peer addresses only. Any policy better than that
needs decisions an embedded library has no business making: which window
length, which key (client IP, tenant, API key), whether to trust a proxy
header, how much memory the table may take and what gets evicted. So the
library runs the seam and you own the semantics.

## Contract

| Item | Behavior |
| --- | --- |
| When | Once per parsed request, before protocol-upgrade detection and before router lookup. A denied request never reaches a detector or a handler. |
| Continue | Return `UVHTTP_OK`. |
| Stop | Return any other error code. `UVHTTP_ERROR_RATE_LIMIT_EXCEEDED` (-550) is kept in the error enum as the idiomatic value for exactly this case. |
| Reply | A filter that stops a request should send its own response (status, `Retry-After`, body) before returning. |
| Fallback | If it returns without sending anything, the library replies for it: the status the filter wrote is kept (403 `Forbidden` when it wrote none), and the body is always the `text/plain` `Forbidden` message — so a denial that needs its own payload (a 429 with a JSON body, say) has to send it inside the filter. The `sent` guard in `uvhttp_response_send()` makes the fallback a no-op when the filter already replied, so there is no double-send. |
| Connection | Stopping a request does not close it. Keep-alive clients can send the next request on the same connection. |
| Slot | One filter, not a chain. Compose your steps inside your own callback. |
| Thread | Runs on the event loop thread. Keep it O(1) and never block in it. |
| `user_data` | Borrowed, exactly like `server->handler`: the library never frees it. It must outlive the server, or be cleared by installing a `NULL` filter first. |

Passing `NULL` as the filter removes the one currently installed; a server from
`uvhttp_server_new()` starts with the seam closed.

Identify the client with `uvhttp_request_get_client_ip()`. It returns the IPv6
or IPv4 peer address, and honors `X-Forwarded-For` / `X-Real-IP` only when
`config->trust_proxy_headers` is set — proxy headers are client-controlled and
trivially spoofable, so leave that off unless you really do sit behind a proxy.
The returned pointer may address an internal buffer that the next call
overwrites; copy it if you keep it.

## Per-client fixed window

The example below is complete user code: no library support beyond the seam.
One slot per client IP, a 1-second window, 100 requests per window, and a
bounded table so a spray of source addresses cannot grow memory without limit.

```c
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"

#define MAX_CLIENTS 256
#define WINDOW_MS 1000
#define MAX_REQUESTS_PER_WINDOW 100

typedef struct {
    char ip[UVHTTP_IPV6_MAX_STRING_LENGTH];
    uint64_t window_start_ms; /* 0 means the slot is free */
    int count;
} client_slot_t;

typedef struct {
    client_slot_t slots[MAX_CLIENTS];
} limiter_t;

static client_slot_t* find_or_open_slot(limiter_t* lim, const char* ip) {
    client_slot_t* oldest = &lim->slots[0];
    for (size_t i = 0; i < MAX_CLIENTS; i++) {
        client_slot_t* s = &lim->slots[i];
        if (strcmp(s->ip, ip) == 0) {
            return s;
        }
        if (s->window_start_ms == 0) {
            return s; /* free slot */
        }
        if (s->window_start_ms < oldest->window_start_ms) {
            oldest = s;
        }
    }
    /* Table full: recycle the longest-idle client. Replace this with an LRU or
     * a per-prefix grouping if that is the policy you actually want. */
    return oldest;
}

static uvhttp_error_t limit_filter(uvhttp_request_t* request,
                                   uvhttp_response_t* response,
                                   void* user_data) {
    limiter_t* lim = (limiter_t*)user_data;
    const char* ip = uvhttp_request_get_client_ip(request);
    if (!ip) {
        return UVHTTP_OK; /* nothing to key on - do not punish the request */
    }

    uint64_t now_ms = uv_hrtime() / 1000000ULL;
    client_slot_t* s = find_or_open_slot(lim, ip);

    if (s->window_start_ms == 0 || now_ms - s->window_start_ms >= WINDOW_MS) {
        strncpy(s->ip, ip, sizeof(s->ip) - 1);
        s->ip[sizeof(s->ip) - 1] = '\0';
        s->window_start_ms = now_ms;
        s->count = 0;
    }

    if (++s->count > MAX_REQUESTS_PER_WINDOW) {
        uvhttp_response_set_status(response, 429);
        uvhttp_response_set_header(response, "Retry-After", "1");
        uvhttp_response_set_header(response, UVHTTP_HEADER_CONTENT_TYPE,
                                   UVHTTP_CONTENT_TYPE_TEXT);
        uvhttp_response_set_body(response, "Too Many Requests",
                                 strlen("Too Many Requests"));
        uvhttp_response_send(response);
        return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
    }
    return UVHTTP_OK;
}

int main(void) {
    limiter_t* lim = calloc(1, sizeof(*lim));
    uv_loop_t* loop = uv_default_loop();

    uvhttp_server_t* server = NULL;
    uvhttp_server_new(loop, &server);
    uvhttp_server_set_request_filter(server, limit_filter, lim);
    /* ... routes, listen, run ... */

    uvhttp_server_free(server);
    free(lim); /* the library never frees it - do it yourself */
    return 0;
}
```

Because the key is the real peer address, one noisy client no longer affects
anyone else, and IPv6 clients get their own slot instead of slipping past the
check.

## Migrating from the removed API

| Removed | Replacement |
| --- | --- |
| `uvhttp_server_enable_rate_limit(server, max, window)` | Install a filter that keeps its own counters (`max`, `window` become your constants). |
| `uvhttp_server_disable_rate_limit(server)` | `uvhttp_server_set_request_filter(server, NULL, NULL)`. |
| `uvhttp_server_check_rate_limit(server)` | Your filter runs before dispatch; there is nothing to call. |
| `uvhttp_server_add_rate_limit_whitelist()` / `clear_rate_limit_all()` | An allow-list is a `strcmp` at the top of your filter. |
| `uvhttp_server_get_rate_limit_status()` / `reset_rate_limit_client()` | Read or clear your own table. |
| `server->rate_limit_*` fields, `UVHTTP_FEATURE_RATE_LIMIT`, `BUILD_WITH_RATE_LIMIT` | Gone. The seam is a pair of pointers and costs no build option. |
| `uvhttp_is_feature_enabled("rate_limit")` | Still answers, now always "not available". |

`UVHTTP_ERROR_RATE_LIMIT_EXCEEDED` (-550) and its `uvhttp_error_string()` text
stay: the code is now an application-level convention rather than something the
library returns on its own.

## See also

- [Middleware API](/spec/middleware-api) - in-handler composition. Note that a
  middleware `STOP` only breaks out of the middleware loop, the handler body
  still runs; use this filter when you need to stop dispatch itself.
- [Request API](/spec/request-api) - `uvhttp_request_get_client_ip()` and the
  headers it reads.
- [CMake configuration](/guide/CMAKE_CONFIGURATION) - `trust_proxy_headers`.
