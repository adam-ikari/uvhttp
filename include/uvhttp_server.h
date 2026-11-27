#ifndef UVHTTP_SERVER_H
#define UVHTTP_SERVER_H

#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"
#include "uvhttp_platform.h"

#include <uv.h>

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_router uvhttp_router_t;
typedef struct uvhttp_connection uvhttp_connection_t;

/* uvhttp_method_t and uvhttp_request_handler_t come from uvhttp_common.h so
 * this header can declare uvhttp_route_t without including uvhttp_request.h.
 * Neither header includes the other: request/response are forward-declared
 * above, and only .c files need their full definitions. */

#if UVHTTP_FEATURE_TLS
typedef struct uvhttp_tls_context uvhttp_tls_context_t;
#endif

#if UVHTTP_FEATURE_WEBSOCKET
typedef struct uvhttp_ws_connection uvhttp_ws_connection_t;

/* WebSocket connection node */
typedef struct ws_connection_node {
    uvhttp_ws_connection_t* ws_conn;
    char path[4096];
    uint64_t last_activity;  /* lastwhen(seconds) */
    uint64_t last_ping_sent; /* lastsend Ping when(seconds) */
    int ping_pending;        /* usependinghandle Ping */
    struct ws_connection_node* next;
} ws_connection_node_t;

/* WebSocket connection manager */
typedef struct {
    ws_connection_node_t* connections; /* Connection */
    int connection_count;              /* Connectioncount */
    uv_timer_t timeout_timer;          /* Timeoutwhen */
    uv_timer_t heartbeat_timer;        /* when */
    int timeout_seconds;               /* Timeoutwhen(seconds) */
    int heartbeat_interval;            /* interval(seconds) */
    uint64_t ping_timeout_ms;          /* Ping Timeoutwhen(seconds) */
    int enabled;                       /* useEnableConnectionmanage */
    struct uvhttp_server* server;      /* ownedServer */
} ws_connection_manager_t;
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct uvhttp_server uvhttp_server_t;

/* One route table entry, for uvhttp_server_listen_routes. UVHTTP_ANY means
 * "match every method". */
typedef struct {
    const char* path;
    uvhttp_method_t method;
    uvhttp_request_handler_t handler;
} uvhttp_route_t;

/* ========== Timeout Statistics Callback ========== */
/**
 * @brief Timeoutstatisticscallback functionsclass
 *
 * , Used forstatisticsandrecordConnectionTimeoutEvent
 */
typedef void (*uvhttp_timeout_callback_t)(uvhttp_server_t* server,
                                          uvhttp_connection_t* conn,
                                          uint64_t timeout_ms, void* user_data);

/**
 * @brief Register the connection-timeout observer
 *
 * The callback fires on the connection's timeout path with the elapsed
 * milliseconds. Passing NULL clears it. Must be called on the loop thread.
 *
 * @return UVHTTP_OK, or UVHTTP_ERROR_INVALID_PARAM if @p server is NULL
 */
uvhttp_error_t uvhttp_server_set_timeout_callback(
    uvhttp_server_t* server, uvhttp_timeout_callback_t callback,
    void* user_data);

/* ========== Pre-dispatch Request Filter ========== */
/**
 * @brief Filter callback invoked before a request is dispatched
 *
 * The library does not implement request policy (rate limiting, admission
 * control, IP allow/deny lists). It provides this one seam instead: the
 * callback sees every parsed request before routing, and the policy is the
 * caller's.
 *
 * @return UVHTTP_OK to let the request continue to routing; any other code
 *         stops dispatch for this request
 */
typedef uvhttp_error_t (*uvhttp_request_filter_t)(uvhttp_request_t* request,
                                                  uvhttp_response_t* response,
                                                  void* user_data);

/* ========== Error observer seam ========== */
/**
 * Callback for asynchronous failures that have no return path to the caller:
 * llhttp parse errors and failed uv_write completions. `error` is the library
 * error code, `conn` is the affected connection (NULL when unavailable).
 * Called on the loop thread; keep work here short and non-blocking.
 */
typedef void (*uvhttp_error_callback_t)(uvhttp_server_t* server,
                                        uvhttp_error_t error,
                                        uvhttp_connection_t* conn,
                                        void* user_data);

struct uvhttp_server {
    /* ========== Cache line 1 (0-63 bytes): hot path fields - most frequently
     * accessed ========== */
    /* Frequently accessed in on_connection, connection management */
    int is_listening;          /* 4 bytes - useparsinglisten */
    int owns_loop;             /* 4 bytes - useloop */
    int freed;                 /* 4 bytes - flag to prevent double free */
    int draining;              /* 4 bytes - reject new requests with 503 */
    size_t active_connections; /* 8 bytes - Connection */
    size_t max_connections;    /* 8 bytes - Connection */
    size_t max_message_size;   /* 8 bytes - messagesize */
    uvhttp_request_handler_t handler; /* 8 bytes - Requesthandle */
    uvhttp_timeout_callback_t
        timeout_callback;             /* 8 bytes - Timeoutstatisticscallback */
    void* timeout_callback_user_data; /* 8 bytes - callbackUser */
    /* Cache line 1 total: 64 bytes */

    /* ========== Cache line 2 (64-127 bytes): core pointer fields - second most
     * frequently accessed ========== */
    /* Frequently accessed in server initialization, request routing */
    uv_loop_t* loop;                /* 8 bytes - Eventloop */
    uvhttp_router_t* router;        /* 8 bytes - Router */
    void* _padding_config;          /* 8 bytes - former config channel; kept
                                     * as padding to preserve cache offsets */
    struct uvhttp_context* context; /* 8 bytes -  */
    void* user_data;                /* 8 bytes -  */
#if UVHTTP_FEATURE_TLS
    uvhttp_tls_context_t* tls_ctx; /* 8 bytes - TLS context */
    int tls_enabled;               /* 4 bytes - TLS useEnable */
    int _padding2[3];              /* 12bytes - paddingto16bytes */
#else
    int _padding2[4]; /* 16bytes - paddingto64bytes */
#endif
    /* Cache line 2 total: approximately 64 bytes (depends on whether TLS is
     * enabled) */

    /* ========== Cache line 3 (128-191 bytes): libuv handles ========== */
    /* libuv internal structure, fixed size */
    uv_tcp_t tcp_handle; /* approximately40-48bytes */
    int _padding3[3];    /* 12bytes - paddingto64bytes */
    /* Cache line 3 total: approximately 64 bytes */

    /* ========== Cache line 4 (192-255 bytes): WebSocket related ========== */
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_routes; /* 8 bytes - WebSocket Router(deprecate) */
    ws_connection_manager_t*
        ws_connection_manager; /* 8 bytes - WebSocket connectionmanage */
    int _padding4[12];         /* 48bytes - paddingto64bytes */
#else
    int _padding4[16]; /* 64bytes - paddingto64bytes */
#endif
    /* Cache line 4 total: 64 bytes */

    /* ========== Cache line 5 (256-319 bytes): request filter seam ==========
     */
    /* Unconditional: the seam costs one pointer pair, so it needs no feature
     * gate. v3.0.0 replaced the built-in rate limiter (8 fields + its feature
     * flag) with these two - policy belongs to the caller. */
    uvhttp_request_filter_t request_filter; /* 8 bytes - pre-dispatch filter */
    void* request_filter_user_data;         /* 8 bytes - filter context */
    uvhttp_error_callback_t error_callback; /* 8 bytes - async error observer */
    void* error_callback_user_data;         /* 8 bytes - observer context */
    int _padding5[8];                       /* 32 bytes - padding to 64 bytes */
    /* Cache line 5 total: 64 bytes */

    /* ========== Cache line 6 (320-383 bytes): protocol upgrade ========== */
    void* protocol_registry; /* 8 bytes - Protocol upgrade registry */
#if UVHTTP_FEATURE_COMPRESSION
    void* gzip_cache; /* 8 bytes - Gzip compression cache (uvhttp_gzip_cache_t*)
                       */
    int _padding6[12]; /* 48bytes - paddingto64bytes */
#else
    int _padding6[14]; /* 56bytes - paddingto64bytes */
#endif
    /* Cache line 6 total: 64 bytes */

    /* Embedded at the end so all existing cache-line offsets stay unchanged.
     * Embedding (rather than heap-allocating) means embedders that close every
     * handle with uv_walk before server_free cannot leak it. */
#if UVHTTP_FEATURE_ASYNC_STOP
    uv_async_t async_stop_handle;
#endif
    /* Intrusive list of accepted connections, used by stop to close them. */
    struct uvhttp_connection* connections;
    size_t backpressure_pauses; /* times reads were paused */
};

/* ========== Memory Layout Verification Static Assertions ========== */

/* Verify pointer alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, loop, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, router, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, error_callback, UVHTTP_POINTER_ALIGNMENT);

/* Verify size_t alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, active_connections,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, max_connections,
                       UVHTTP_SIZE_T_ALIGNMENT);

/* API functions */
/**
 * @brief create new HTTP Server
 * @param loop Event loop (must be provided by application layer, cannot be
 * NULL)
 * @param server output parameter, used for receive server pointer
 * @return UVHTTP_OK success, other value represents failure
 * @note Success when, *server is set to valid server object, must use
 * uvhttp_server_free release
 * @note Failure when, *server is set to NULL
 */
uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server);

/**
 * @brief create Server withinfrastructureevent loop
 * @param server output parameter, used for receive server pointer
 * @return UVHTTP_OK success, other value represents failure
 * @note Creating the libuv event loop internally, the library will handle
 * the lifecycle of the event loop (including cleanup)
 * @note Related to uvhttp_server_new, this function is suitable for application
 * scenarios that do not need to manage the event loop themselves
 */
uvhttp_error_t uvhttp_server_new_with_loop(uvhttp_server_t** server);
/**
 * @brief Start listening for incoming connections
 *
 * @param server Server created with uvhttp_server_new /
 * uvhttp_server_new_with_loop
 * @param host Bind address (e.g. "0.0.0.0" or "127.0.0.1")
 * @param port TCP port to bind
 * @return UVHTTP_OK on success, error code otherwise
 *
 * @note The server must be driven by the libuv event loop (uv_run) after
 *       this call; the server keeps no background threads.
 */
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host,
                                    int port);
/**
 * @brief Stop listening and close accepted connections
 *
 * @param server Server to stop
 * @return UVHTTP_OK on success, error code otherwise
 *
 * @note The server object itself is NOT released; call uvhttp_server_free
 *       afterwards to release it.
 */
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);

/**
 * @brief Request a stop from any thread
 *
 * `uvhttp_server_stop` must run on the loop thread (it touches libuv
 * handles). This posts an `uv_async_t` to that loop instead; the callback
 * then calls `uvhttp_server_stop` there. The async handle is unref'd, so it
 * does not keep an otherwise idle loop alive.
 *
 * @note The caller must not race this against `uvhttp_server_free` — join all
 *       producer threads before freeing the server.
 */
#if UVHTTP_FEATURE_ASYNC_STOP
uvhttp_error_t uvhttp_server_async_stop(uvhttp_server_t* server);
#else
/* Compiled out: no uv_async_t instance cost. Callers can still stop on the
 * loop thread with uvhttp_server_stop. */
#    define uvhttp_server_async_stop(server) \
        UVHTTP_ERROR_NOT_SUPPORTED
#endif

/**
 * @brief Enter/leave draining mode
 *
 * While draining, every newly parsed request gets 503 + `Connection: close`
 * before the request filter and routing run. Existing in-flight requests are
 * unaffected. Intended for rolling deploys: mark draining, let the load
 * balancer's health probe fail, then stop once the active count reaches zero
 * (see uvhttp_server_get_stats).
 *
 * @note Must be called on the loop thread. The flag is a plain int read by
 *       the request parser; cross-thread callers must post through their own
 *       synchronisation. Use uvhttp_server_async_stop (when compiled in) for
 *       the stop side.
 */
uvhttp_error_t uvhttp_server_set_draining(uvhttp_server_t* server, int enabled);

/**
 * @brief Close every accepted connection the server still tracks
 *
 * Stop closes the listening handle only; this closes live connections too,
 * so uv_run(UV_RUN_DEFAULT) can actually return after a stop. Closing is
 * asynchronous: callers must pump the loop for the close callbacks to run.
 */
uvhttp_error_t uvhttp_server_close_connections(uvhttp_server_t* server);

/**
 * @brief Create a server, register routes, and start listening - atomically
 *
 * @param loop Event loop (owned by the caller; the server never closes it)
 * @param routes Route table (may be NULL only when route_count is 0)
 * @param route_count Number of entries in routes
 * @param host Bind address (e.g. "0.0.0.0" or "127.0.0.1")
 * @param port TCP port to bind
 * @param server Output parameter; set to NULL on every failure path
 * @return UVHTTP_OK on success, error code otherwise
 *
 * @note This is the ATOMIC form of what used to be five separate calls:
 *       uvhttp_server_new -> uvhttp_router_new -> uvhttp_router_add_route xN
 *       -> uvhttp_server_take_router -> uvhttp_server_listen.
 *       Either the returned server is fully listening with every route
 *       registered, or nothing was created. There is no intermediate state
 *       and no cleanup for the caller to write.
 *
 * @note On success the server owns the routes (it builds and holds the
 *       router internally) and releases them in uvhttp_server_free.
 *
 * @note Added in v3.0.0. The five-step sequence remains available for callers
 *       that need to keep the router handle - prefer this for everything else.
 *
 * @note This entry builds a bare server: no `uvhttp_context_t` and no
 *       `uvhttp_config_t` are attached. Every config-backed setting on the
 *       result therefore falls back to its compile-time constant (backlog,
 *       tcp_keepalive_timeout). If you need those tunable, use the five-step
 *       sequence with a context instead - wiring a config into this path is
 *       tracked separately.
 *
 * @example
 *   uvhttp_server_t* server = NULL;
 *   uvhttp_route_t routes[] = {
 *       {"/",        UVHTTP_ANY, hello},
 *       {"/health", UVHTTP_GET, health},
 *   };
 *   if (uvhttp_server_listen_routes(loop, routes, 2, "0.0.0.0", 8080,
 *                                   &server) != UVHTTP_OK) {
 *       // nothing was allocated; nothing to clean up
 *   }
 */
uvhttp_error_t uvhttp_server_listen_routes(uv_loop_t* loop,
                                           const uvhttp_route_t* routes,
                                           size_t route_count, const char* host,
                                           int port, uvhttp_server_t** server);
#if UVHTTP_FEATURE_TLS
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server,
                                        uvhttp_tls_context_t* tls_ctx);
uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server);
#endif
/** @return 1 when TLS has been enabled on @p server, 0 otherwise (NULL-safe). */
int uvhttp_server_is_tls_enabled(uvhttp_server_t* server);
/**
 * @brief Install a configuration on the server (transfers ownership)
 *
 * One step replaces the former "pick server->config or context->current_config,
 * attach a context, then set the field" sequence. The config is installed on
 * the server's context (created if absent) and released by the server on free.
 * Every execution path reads configuration through that one channel. Passing
 * NULL clears it and returns to built-in defaults.
 */
uvhttp_error_t uvhttp_server_take_config(uvhttp_server_t* server,
                                         uvhttp_config_t* config);

/** @return the single active configuration, or NULL when none is installed. */
const uvhttp_config_t* uvhttp_server_get_config(
    const uvhttp_server_t* server);

/**
 * @brief Install the observer for asynchronous protocol/write errors
 *
 * NULL clears it. Unlike the request filter this is purely observational: the
 * server still closes the connection / drops the failed write as before. Its
 * purpose is to let an embedder count and log failures that used to vanish
 * because the write callbacks cast `status` to void.
 */
uvhttp_error_t uvhttp_server_set_error_callback(
    uvhttp_server_t* server, uvhttp_error_callback_t callback, void* user_data);

/**
 * @brief Release a server and everything it owns
 *
 * @param server Server to release (may be NULL, then this is a no-op)
 * @return UVHTTP_OK on success, error code otherwise
 *
 * @note Ownership contract: once set with uvhttp_server_take_router /
 *       uvhttp_server_take_context, the router and context are owned by the
 *       server and are released by this function. The caller MUST NOT free
 *       them separately, otherwise the double free occurs. The same applies
 *       to any config reached through the context.
 * @note If the server was created with uvhttp_server_new_with_loop, the
 *       internal event loop is also closed here.
 */
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);

/** @brief Read-only snapshot of the counters the server already maintains. */
typedef struct uvhttp_server_stats {
    size_t active_connections; /* authoritative value used by the limit */
    uint64_t total_connections;
    uint64_t total_requests;
    size_t backpressure_pauses; /* reads paused due to write queue */
} uvhttp_server_stats_t;

/**
 * @brief Copy the server's live counters into @p out
 *
 * No allocation, no mutation. All three values come from fields that were
 * already being written on the accept/request/close paths; this is the read
 * side that lets an embedder expose them through its own metrics sink.
 */
uvhttp_error_t uvhttp_server_get_stats(uvhttp_server_t* server,
                                       uvhttp_server_stats_t* out);
/**
 * @brief Set the request handler used when no router is attached
 *
 * @param server Server to configure
 * @param handler Request handler callback
 * @return UVHTTP_OK on success, error code otherwise
 */
uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server,
                                         uvhttp_request_handler_t handler);
/**
 * @brief Hand a router over to the server (ownership transfer)
 *
 * @param server Server to configure
 * @param router Router created with uvhttp_router_new
 * @return UVHTTP_OK on success, error code otherwise
 *
 * @note OWNERSHIP TRANSFERS HERE. After this call the server owns the router
 *       and releases it in uvhttp_server_free. Do NOT call uvhttp_router_free
 *       on it yourself - doing so is a double free.
 *
 * @note Renamed from uvhttp_server_set_router in v3.0.0. The old name implied
 *       a borrow while the behavior was a transfer; that mismatch is what made
 *       examples/embedding/main.c double-free on its listen-failure path.
 *
 * @note Returns UVHTTP_ERROR_INVALID_PARAM if the server already owns a
 *       different router, rather than silently leaking the previous one.
 *       Passing the same router again is idempotent (teardown paths rely on
 *       it).
 *
 * @note There is no detach operation: a taken router cannot be handed back,
 *       replaced, or unset by the caller - the server owns it until
 *       uvhttp_server_free. A NULL router is only a no-op when the server
 *       holds none already; once a router has been taken, NULL is rejected
 *       like any other pointer.
 *
 * @note The guard above is per-server, so it does NOT stop the same router
 *       from being handed to two different servers. Do that and both
 *       uvhttp_server_free calls release it: the second one is a double free
 *       (ASan reports it at free time, far from the assignment). One router
 *       belongs to exactly one server.
 */
uvhttp_error_t uvhttp_server_take_router(uvhttp_server_t* server,
                                         uvhttp_router_t* router);
/**
 * @brief Hand a context over to the server (ownership transfer)
 *
 * @param server Server to configure
 * @param context Context created with uvhttp_context_create
 * @return UVHTTP_OK on success, error code otherwise
 *
 * @note OWNERSHIP TRANSFERS HERE. After this call the server owns the context
 *       and releases it in uvhttp_server_free. Do NOT free it yourself.
 *
 * @note Renamed from uvhttp_server_set_context in v3.0.0, for the same reason
 *       as uvhttp_server_take_router. No detach operation exists: a NULL
 *       context is only a no-op when the server holds none already, and is
 *       rejected once one has been taken.
 *
 * @note Like take_router, the guard is per-server: handing the same context
 *       to two servers makes both destroy it - a double free at free time.
 *       One context belongs to exactly one server.
 */
uvhttp_error_t uvhttp_server_take_context(uvhttp_server_t* server,
                                          struct uvhttp_context* context);

/* ========== Pre-dispatch Request Filter ========== */

/**
 * @brief Install a filter that runs before request dispatch
 *
 * The library ships no rate limiter on purpose: window size, the key to count
 * by (client IP, tenant, API key), storage and eviction are application
 * policy. This seam is where that policy runs. The filter is called once per
 * parsed request, before protocol-upgrade detection and before router lookup,
 * so it gates upgrades as well.
 *
 * Identify the client with uvhttp_request_get_client_ip(). It covers IPv6 and
 * honors X-Forwarded-For/X-Real-IP only when config->trust_proxy_headers is
 * set; the built-in limiter it replaces matched IPv4 peer addresses and
 * nothing else.
 *
 * @param server Server to configure
 * @param filter Callback, or NULL to remove the one currently installed
 * @param user_data Context passed back to @p filter. Borrowed: the library
 *        never frees it, exactly like uvhttp_server_set_handler. It must
 *        outlive the server, or be cleared by installing a NULL filter first.
 * @return UVHTTP_OK on success, UVHTTP_ERROR_INVALID_PARAM if @p server is
 * NULL
 *
 * @note Returning UVHTTP_OK lets the request continue. Any other code stops
 *       dispatch for that request.
 *
 * @note A filter that stops a request is expected to reply itself (status,
 *       Retry-After, body) before returning. If it returns without sending,
 *       the library replies so the client is never left waiting: the status
 *       the filter wrote is kept (403 when it wrote none) and the body is
 *       always the plain-text "Forbidden" message. The sent-guard in
 *       uvhttp_response_send makes that fallback a no-op when the filter
 *       already replied.
 *
 * @note One slot, not a chain - compose inside your own callback. It runs on
 *       the event loop thread: keep it O(1) and never block in it.
 */
uvhttp_error_t uvhttp_server_set_request_filter(uvhttp_server_t* server,
                                                uvhttp_request_filter_t filter,
                                                void* user_data);

/* ========== Misc ========== */

/**
 * @brief Register a health-check endpoint
 *
 * Adds a handler at @p path that replies HTTP 200 with the JSON body
 * {"status":"ok"}. Intended for load balancer and orchestration probes. If the
 * server has no router yet, one is created and owned by the server.
 *
 * @param server Server
 * @param path Route path (e.g. "/health")
 * @return UVHTTP_OK on success, UVHTTP_ERROR_INVALID_PARAM if @p server or
 *         @p path is NULL
 */
uvhttp_error_t uvhttp_server_enable_health_check(uvhttp_server_t* server,
                                                 const char* path);

/* WebSocket API */
#if UVHTTP_FEATURE_WEBSOCKET
#    include "uvhttp_websocket.h"

typedef struct {
    int (*on_connect)(uvhttp_ws_connection_t* ws_conn);
    int (*on_message)(uvhttp_ws_connection_t* ws_conn, const char* data,
                      size_t len, int opcode);
    int (*on_close)(uvhttp_ws_connection_t* ws_conn);
    int (*on_error)(uvhttp_ws_connection_t* ws_conn, int error_code,
                    const char* error_msg);
    void* user_data;
    /* Timeoutstatisticscallback */
    uvhttp_timeout_callback_t
        timeout_callback;             /* 8 bytes - Timeoutstatisticscallback */
    void* timeout_callback_user_data; /* 8 bytes - callbackUser */
} uvhttp_ws_handler_t;

/* WebSocket connection wrapper - links the HTTP connection and the
 * user-registered WS handler; stored as ws_connection->user_data. Kept
 * public so embedder-side helpers (uvhttp_server_ws_send / ws_close) can
 * reach the owning uvhttp_connection_t for the TLS context. */
typedef struct {
    uvhttp_connection_t* conn;
    uvhttp_ws_handler_t* user_handler;
} uvhttp_ws_wrapper_t;

uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server,
                                                 const char* path,
                                                 uvhttp_ws_handler_t* handler);
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn,
                                     const char* data, size_t len);
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code,
                                      const char* reason);

/* internalFunction */
uvhttp_ws_handler_t* uvhttp_server_find_ws_handler(uvhttp_server_t* server,
                                                   const char* path);

/* Connectionmanage API */
uvhttp_error_t uvhttp_server_ws_enable_connection_management(
    uvhttp_server_t* server, int timeout_seconds, int heartbeat_interval);

uvhttp_error_t uvhttp_server_ws_disable_connection_management(
    uvhttp_server_t* server);

int uvhttp_server_ws_get_connection_count(uvhttp_server_t* server);

int uvhttp_server_ws_get_connection_count_by_path(uvhttp_server_t* server,
                                                  const char* path);

uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t* server,
                                          const char* path, const char* data,
                                          size_t len);

uvhttp_error_t uvhttp_server_ws_close_all(uvhttp_server_t* server,
                                          const char* path);

/* internalFunction( uvhttp_connection )
 *
 * NOTE: these functions borrow the ws_conn pointer — the library never
 * copies or frees it. The caller owns the connection and must keep it
 * alive until it is removed from the manager (remove_connection/close_all)
 * or until uvhttp_server_ws_disable_connection_management() is called. */
void uvhttp_server_ws_add_connection(uvhttp_server_t* server,
                                     uvhttp_ws_connection_t* ws_conn,
                                     const char* path);

void uvhttp_server_ws_remove_connection(uvhttp_server_t* server,
                                        uvhttp_ws_connection_t* ws_conn);

void uvhttp_server_ws_update_activity(uvhttp_server_t* server,
                                      uvhttp_ws_connection_t* ws_conn);
#endif

/* TLSFunction (whenDisable) */
/* uvhttp_error_t uvhttp_tls_init(void); */
/* void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx); */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_SERVER_H */
