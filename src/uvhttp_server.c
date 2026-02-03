/*
 * UVHTTP server module
 *
 * Provides core HTTP server functionality including connection management,
 * request routing, and response processing
 * Implements high-performance asynchronous I/O based on libuv event-driven
 * architecture
 */

#include "uvhttp_server.h"

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_logging.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_tls.h"
#include "uvhttp_utils.h"

#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <uv.h>

#if UVHTTP_FEATURE_WEBSOCKET
#    include "uvhttp_websocket.h"
#endif

// WebSocket route entry forward declaration
#if UVHTTP_FEATURE_WEBSOCKET
typedef struct ws_route_entry {
    char* path;
    uvhttp_ws_handler_t handler;
    struct ws_route_entry* next;
} ws_route_entry_t;
#endif

/**
 * 503 response write completion callback function
 *
 * Handle cleanup after sending 503 Service Unavailable response
 *
 * @param req write request object
 * @param status write operation state
 */
static void write_503_response_cb(uv_write_t* req, int status) {
    uvhttp_handle_write_error(req, status, "503_response");
}

/**
 * Single-threaded event-driven connection processing callback
 *
 * This is the core callback function of libuv event loop, processes all new
 * connections Single-threaded model advantages: no locks needed, data access is
 * safe, execution stream is predictable
 *
 * @param server_handle server handle
 * @param status connection state
 */
static void on_connection(uv_stream_t* server_handle, int status) {
    UVHTTP_LOG_DEBUG("on_connection called with status: %d\n", status);

    if (status < 0) {
        uvhttp_log_safe_error(status, "connection_accept", NULL);
        return;
    }

    if (!server_handle || !server_handle->data) {
        UVHTTP_LOG_ERROR("Invalid server handle or data\n");
        return;
    }

    uvhttp_server_t* server = (uvhttp_server_t*)server_handle->data;
    UVHTTP_LOG_DEBUG("Server TLS enabled: %d\n", server->tls_enabled);

    /* Single-threaded connection count check - use server specific config */
    size_t max_connections = UVHTTP_MAX_CONNECTIONS_DEFAULT;  // defaultvalue
    if (server->config) {
        max_connections = server->config->max_connections;
    } else {
        // Fall back to global config (use server->context)
        uvhttp_context_t* context = server->context;
        const uvhttp_config_t* global_config =
            uvhttp_config_get_current(context);
        if (global_config) {
            max_connections = global_config->max_connections;
        }
    }

    if (server->active_connections >= max_connections) {
        UVHTTP_LOG_WARN("Connection limit reached: %zu/%zu\n",
                        server->active_connections, (size_t)max_connections);
        /* Create temporary connection to send 503 response */
        uv_tcp_t* temp_client = uvhttp_alloc(sizeof(uv_tcp_t));
        if (!temp_client) {
            uvhttp_handle_memory_failure("temporary_client_allocation", NULL,
                                         NULL);
            return;
        }

        if (uv_tcp_init(server->loop, temp_client) != 0) {
            UVHTTP_LOG_ERROR("Failed to initialize temporary client\n");
            uvhttp_free(temp_client);
            return;
        }

        if (uv_accept(server_handle, (uv_stream_t*)temp_client) == 0) {
            /* Send HTTP 503 response - use static constants to avoid repeated
             * allocation */
            static const char response_503[] = UVHTTP_VERSION_1_1
                " 503 Service Unavailable\r\n"
                "Content-Type: text/plain\r\n"
                "Content-Length: " UVHTTP_STRINGIFY(
                    UVHTTP_503_RESPONSE_CONTENT_LENGTH) "\r\n"
                                                        "Connection: close\r\n"
                                                        "\r\n"
                                                        "Service Unavailable";

            uv_write_t* write_req = uvhttp_alloc(sizeof(uv_write_t));
            if (write_req) {
                uv_buf_t buf =
                    uv_buf_init((char*)response_503, sizeof(response_503) - 1);

                int write_result =
                    uv_write(write_req, (uv_stream_t*)temp_client, &buf, 1,
                             write_503_response_cb);
                if (write_result < 0) {
                    UVHTTP_LOG_ERROR("Failed to send 503 response: %s\n",
                                     uv_strerror(write_result));
                    // If write failure, immediately release write_req and close
                    // connection
                    uvhttp_free(write_req);
                    uv_close((uv_handle_t*)temp_client,
                             (uv_close_cb)uvhttp_free);
                    return;
                }
            } else {
                UVHTTP_LOG_ERROR(
                    "Failed to allocate write request for 503 response\n");
                uv_close((uv_handle_t*)temp_client, (uv_close_cb)uvhttp_free);
                return;
            }
        } else {
            UVHTTP_LOG_ERROR("Failed to accept temporary connection\n");
            uvhttp_free(temp_client);
        }

        return;
    }

    /* Create new connection object - single-threaded allocation, no
     * synchronization needed */
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t conn_result = uvhttp_connection_new(server, &conn);
    if (conn_result != UVHTTP_OK) {
        return;
    }

    /* acceptconnection */
    int accept_result =
        uv_accept(server_handle, (uv_stream_t*)&conn->tcp_handle);

    if (accept_result != 0) {
        UVHTTP_LOG_ERROR("Failed to accept connection: %d\n", accept_result);
        uvhttp_connection_free(conn);
        return;
    }

    UVHTTP_LOG_DEBUG("Connection accepted, TLS enabled: %d\n",
                     conn->tls_enabled);

    /* Request and response objects have been initialized when connection was
     * created */

    /* Single-threaded safe connection count increment */
    server->active_connections++;

    /* Start connection process (TLS handshake or HTTP read)
     * All subsequent processes are done asynchronously through libuv callback
     * in event loop */
    UVHTTP_LOG_DEBUG("Starting connection...\n");
    int start_result = uvhttp_connection_start(conn);
    UVHTTP_LOG_DEBUG("Connection start result: %d\n", start_result);
    if (start_result == 0) {
        uvhttp_connection_start_timeout(conn);
    }

    if (start_result != 0) {
        uvhttp_connection_close(conn);
        return;
    }
}

/* Create single-threaded event-driven HTTP server
 * loop: libuv event loop (must be provided by application layer)
 * return: server object, all operations are done in single event loop thread
 * Single-threaded design advantages:
 * 1. No lock mechanism needed, avoid deadlocks and race conditions
 * 2. Memory access is safer, no atomic operations needed
 * 3. Performance is predictable, avoid thread switching overhead
 * 4. Debug is simple, execution stream is clear */
uvhttp_error_t uvhttp_server_new(uv_loop_t* loop, uvhttp_server_t** server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!loop) {
        UVHTTP_LOG_ERROR("loop parameter is required - must be provided by "
                         "application layer");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *server = NULL;

/* Initialize TLS module (if not yet initialized) */
#if UVHTTP_FEATURE_TLS
    UVHTTP_LOG_DEBUG("Initializing TLS module...");
    /* Use global variable to keep backward compatibility */
    /* New projects should use uvhttp_context for TLS config */
    UVHTTP_LOG_DEBUG("TLS module initialization skipped (using global "
                     "variables for backward compatibility)");
#endif
    UVHTTP_LOG_DEBUG("Allocating uvhttp_server_t, size=%zu",
                     sizeof(uvhttp_server_t));
    uvhttp_server_t* s = uvhttp_alloc(sizeof(uvhttp_server_t));
    if (!s) {
        UVHTTP_LOG_ERROR("Failed to allocate uvhttp_server_t");
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    UVHTTP_LOG_DEBUG("uvhttp_alloc success, server=%p", (void*)s);
    memset(s, 0, sizeof(uvhttp_server_t));

    // initializeconnectionlimitdefaultvalue
    s->max_connections =
        UVHTTP_MAX_CONNECTIONS_MAX;              // default max connection count
    s->max_message_size = UVHTTP_MAX_BODY_SIZE;  // default max message size 1MB
// Initialize WebSocket router table
#if UVHTTP_FEATURE_WEBSOCKET
    s->ws_routes = NULL;
    s->ws_connection_manager = NULL;
#endif

#if UVHTTP_FEATURE_RATE_LIMIT
    // Initialize rate limit function field
    s->rate_limit_enabled = 0;
    s->rate_limit_max_requests = 0;
    s->rate_limit_window_seconds = 0;
    s->rate_limit_request_count = 0;
    s->rate_limit_window_start_time = 0;
    s->rate_limit_whitelist = NULL;
    s->rate_limit_whitelist_count = 0;
#endif

    // Loop must be provided by application layer
    s->loop = loop;
    s->owns_loop = 0;

    if (uv_tcp_init(s->loop, &s->tcp_handle) != 0) {
        uvhttp_free(s);
        return UVHTTP_ERROR_IO_ERROR;
    }
    s->tcp_handle.data = s;
    s->active_connections = 0;
#if UVHTTP_FEATURE_TLS
    s->tls_enabled = 0;
    s->tls_ctx = NULL;
#endif

    *server = s;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* close TCP handle */
    if (!uv_is_closing((uv_handle_t*)&server->tcp_handle)) {
        uv_close((uv_handle_t*)&server->tcp_handle, NULL);
    }

    /* Run loop multiple times to process close callback
     * Fix: Regardless of whether owning loop, need to run loop to process close
     * callback Use UV_RUN_ONCE instead of UV_RUN_NOWAIT to ensure callback is
     * executed */
    if (server->loop) {
        for (int index = 0; index < UVHTTP_SERVER_CLEANUP_LOOP_ITERATIONS;
             index++) {
            uv_run(server->loop, UV_RUN_ONCE);
        }
    }

    /* Clean connection pool */
    if (server->router) {
        uvhttp_router_free(server->router);
    }
#if UVHTTP_FEATURE_TLS
    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
    }
#endif
    if (server->config) {
        uvhttp_config_free(server->config);
    }

    /* cleancontext */
    if (server->context) {
        uvhttp_context_destroy(server->context);
        server->context = NULL;
    }

// Release WebSocket router table
#if UVHTTP_FEATURE_WEBSOCKET
    if (server->ws_routes) {
        ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
        while (current) {
            ws_route_entry_t* next = current->next;
            if (current->path) {
                uvhttp_free(current->path);
            }
            uvhttp_free(current);
            current = next;
        }
        server->ws_routes = NULL;
    }
#endif

#if UVHTTP_FEATURE_RATE_LIMIT
    // Clean rate limit whitelist
    if (server->rate_limit_whitelist) {
        for (size_t i = 0; i < server->rate_limit_whitelist_count; i++) {
            if (server->rate_limit_whitelist[i]) {
                uvhttp_free(server->rate_limit_whitelist[i]);
            }
        }
        uvhttp_free(server->rate_limit_whitelist);
        server->rate_limit_whitelist = NULL;
        server->rate_limit_whitelist_count = 0;
    }

    // Clean whitelist hash table
    struct whitelist_item *current, *tmp;
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wunused-value"
    HASH_ITER(hh, server->rate_limit_whitelist_hash, current, tmp) {
        HASH_DEL(server->rate_limit_whitelist_hash, current);
        uvhttp_free(current);
    }
#    pragma GCC diagnostic pop
    server->rate_limit_whitelist_hash = NULL;

    // Rate limit state has been embedded in struct, no need for extra cleanup
#endif

    uvhttp_free(server);
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host,
                                    int port) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!host) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    struct sockaddr_in addr;
    uv_ip4_addr(host, port, &addr);

    /* Nginx optimize: bindport */
    int ret =
        uv_tcp_bind(&server->tcp_handle, (const struct sockaddr*)&addr, 0);
    if (ret != 0) {
        UVHTTP_LOG_DEBUG("uv_tcp_bind failed with code: %d (%s)\n", ret,
                         uv_strerror(ret));
        UVHTTP_LOG_ERROR("uv_tcp_bind failed: %s\n", uv_strerror(ret));
        return UVHTTP_ERROR_SERVER_LISTEN;
    }

    /* TCP optimization: set TCP_NODELAY and TCP_KEEPALIVE */
    int enable = 1;
    uv_tcp_nodelay(&server->tcp_handle, enable);

    /* setkeepalive */
    unsigned int keepalive_timeout = server->config
                                         ? server->config->tcp_keepalive_timeout
                                         : UVHTTP_TCP_KEEPALIVE_TIMEOUT;
    uv_tcp_keepalive(&server->tcp_handle, enable, keepalive_timeout);

    /* performanceoptimize: set TCP buffersize */
    int sockfd;
    if (uv_fileno((uv_handle_t*)&server->tcp_handle, &sockfd) == 0) {
        /* setsendbuffersize */
        int send_buf_size = UVHTTP_SOCKET_SEND_BUF_SIZE;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, &send_buf_size,
                   sizeof(send_buf_size));

        /* setreceivebuffersize */
        int recv_buf_size = UVHTTP_SOCKET_RECV_BUF_SIZE;
        setsockopt(sockfd, SOL_SOCKET, SO_RCVBUF, &recv_buf_size,
                   sizeof(recv_buf_size));

        /* Set TCP_CORK (latency send to optimize small packets) - only used
         * when sending large files */
        int cork = 0; /* Default disabled, enable when sending large files */
        setsockopt(sockfd, IPPROTO_TCP, TCP_CORK, &cork, sizeof(cork));
    }

    /* Use config system's backlog setting */
    // Use server->context instead of loop->data, avoid monopolizing loop->data
    uvhttp_context_t* context = server->context;
    const uvhttp_config_t* config = NULL;

    if (context) {
        config = uvhttp_config_get_current(context);
    }

    int backlog = UVHTTP_BACKLOG;
    if (config && config->backlog > 0) {
        backlog = config->backlog;
    }

    ret = uv_listen((uv_stream_t*)&server->tcp_handle, backlog, on_connection);
    if (ret != 0) {
        UVHTTP_LOG_DEBUG("uv_listen failed with code: %d (%s)\n", ret,
                         uv_strerror(ret));
        UVHTTP_LOG_ERROR("uv_listen failed: %s\n", uv_strerror(ret));
        return UVHTTP_ERROR_SERVER_LISTEN;
    }

    server->is_listening = 1;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server,
                                         uvhttp_request_handler_t handler) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->handler = handler;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server,
                                        uvhttp_router_t* router) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->router = router;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_set_context(uvhttp_server_t* server,
                                         struct uvhttp_context* context) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->context = context;
    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (server->is_listening) {
        uv_close((uv_handle_t*)&server->tcp_handle, NULL);
        server->is_listening = 0;
        return UVHTTP_OK;
    }

    return UVHTTP_ERROR_SERVER_STOP;
}

#if UVHTTP_FEATURE_TLS
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server,
                                        uvhttp_tls_context_t* tls_ctx) {
    if (!server || !tls_ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
    }

    server->tls_ctx = tls_ctx;
    server->tls_enabled = 1;

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (server->tls_ctx) {
        uvhttp_tls_context_free(server->tls_ctx);
        server->tls_ctx = NULL;
    }

    server->tls_enabled = 0;

    return UVHTTP_OK;
}

int uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    return server ? server->tls_enabled : 0;
}
#else
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server,
                                        void* tls_ctx) {
    (void)server;
    (void)tls_ctx;
    return UVHTTP_ERROR_INVALID_PARAM;
}

uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server) {
    (void)server;
    return UVHTTP_ERROR_INVALID_PARAM;
}

int uvhttp_server_is_tls_enabled(uvhttp_server_t* server) {
    (void)server;
    return 0;
}
#endif

// ========== Unified API Implementation ==========

// Internal auxiliary function
static uvhttp_error_t create_simple_server_internal(
    uv_loop_t* loop, const char* host, int port,
    uvhttp_server_builder_t** server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    uvhttp_server_builder_t* simple =
        uvhttp_alloc(sizeof(uvhttp_server_builder_t));
    if (!simple) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(simple, 0, sizeof(uvhttp_server_builder_t));

    // Loop must be provided by application layer
    if (!loop) {
        UVHTTP_LOG_ERROR("loop parameter is required - must be provided by "
                         "application layer");
        uvhttp_free(simple);
        *server = NULL;
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    simple->loop = loop;

    // Create server
    uvhttp_error_t server_result =
        uvhttp_server_new(simple->loop, &simple->server);
    if (server_result != UVHTTP_OK) {
        uvhttp_free(simple);
        *server = NULL;  // set to NULL to avoid double release
        return server_result;
    }

    // Create router
    uvhttp_error_t router_result = uvhttp_router_new(&simple->router);
    if (router_result != UVHTTP_OK) {
        // Before calling uvhttp_server_free, set config to NULL
        simple->server->config = NULL;
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        *server = NULL;  // set to NULL to avoid double release
        return router_result;
    }

    // Create and set default config
    uvhttp_error_t result = uvhttp_config_new(&simple->config);
    if (result != UVHTTP_OK) {
        // Before calling uvhttp_server_free, set config and router to NULL
        simple->server->config = NULL;
        simple->server->router = NULL;
        uvhttp_router_free(simple->router);
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        *server = NULL;  // set to NULL to avoid double release
        return result;
    }

    simple->server->config = simple->config;
    simple->server->router = simple->router;
    simple->auto_cleanup = 1;

    // startlisten
    if (uvhttp_server_listen(simple->server, host, port) != UVHTTP_OK) {
        UVHTTP_LOG_ERROR("Failed to start server on %s:%d\n", host, port);
        // Before calling uvhttp_server_free, set config and router to NULL
        // Because they will be released in uvhttp_server_free
        // simple->server->config = NULL;
        simple->server->router = NULL;
        uvhttp_server_free(simple->server);
        uvhttp_free(simple);
        *server = NULL;  // set to NULL to avoid double release
        return UVHTTP_ERROR_SERVER_LISTEN;
    }

    *server = simple;
    return UVHTTP_OK;
}

// Quick create and start server
uvhttp_error_t uvhttp_server_create(uv_loop_t* loop, const char* host, int port,
                                    uvhttp_server_builder_t** server) {
    return create_simple_server_internal(loop, host, port, server);
}

// routeraddauxiliaryfunction
static uvhttp_server_builder_t* add_route_internal(
    uvhttp_server_builder_t* server, const char* path, uvhttp_method_t method,
    uvhttp_request_handler_t handler) {
    if (!server || !path || !handler)
        return server;

    uvhttp_router_add_route_method(server->router, path, method, handler);
    return server;
}

// Chained router API
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_GET, handler);
}

uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server,
                                     const char* path,
                                     uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_POST, handler);
}

uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_PUT, handler);
}

uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server,
                                       const char* path,
                                       uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_DELETE, handler);
}

uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler) {
    return add_route_internal(server, path, UVHTTP_ANY, handler);
}

// Simplified config API
uvhttp_server_builder_t* uvhttp_set_max_connections(
    uvhttp_server_builder_t* server, int max_conn) {
    if (server && server->config) {
        server->config->max_connections = max_conn;
    }
    return server;
}

uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server,
                                            int timeout) {
    if (server && server->config) {
        server->config->request_timeout = timeout;
        server->config->keepalive_timeout = timeout;
    }
    return server;
}

uvhttp_server_builder_t* uvhttp_set_max_body_size(
    uvhttp_server_builder_t* server, size_t size) {
    if (server && server->config) {
        server->config->max_body_size = size;
    }
    return server;
}

// Convenient request parameter get
const char* uvhttp_get_param(uvhttp_request_t* request, const char* name) {
    return uvhttp_request_get_query_param(request, name);
}

const char* uvhttp_get_header(uvhttp_request_t* request, const char* name) {
    return uvhttp_request_get_header(request, name);
}

const char* uvhttp_get_body(uvhttp_request_t* request) {
    return uvhttp_request_get_body(request);
}

// Server run and cleanup
int uvhttp_server_run(uvhttp_server_builder_t* server) {
    if (!server || !server->loop)
        return -1;
    return uv_run(server->loop, UV_RUN_DEFAULT);
}

void uvhttp_server_stop_simple(uvhttp_server_builder_t* server) {
    if (server && server->server) {
        uvhttp_server_stop(server->server);
    }
}

void uvhttp_server_simple_free(uvhttp_server_builder_t* server) {
    if (!server)
        return;

    if (server->server) {
        uvhttp_server_free(server->server);
    }

    // Note: router and config are released by server, do not release repeatedly

    uvhttp_free(server);
}

// default handler (for one-key start)
static int default_handler(uvhttp_request_t* request,
                           uvhttp_response_t* response) {
    const char* method = uvhttp_request_get_method(request);
    const char* url = uvhttp_request_get_url(request);

    char response_body[512];
    snprintf(response_body, sizeof(response_body),
             "UVHTTP unified API server\n\n"
             "requestinfo:\n"
             "- method: %s\n"
             "- URL: %s\n"
             "- time: %ld\n"
             "\nWelcome to UVHTTP unified API!",
             method, url, time(NULL));

    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, response_body, strlen(response_body));
    uvhttp_response_send(response);
    return 0;
}

// One-key start function (simplest API)
int uvhttp_serve(uv_loop_t* loop, const char* host, int port) {
    // Parameter verify
    if (!loop) {
        fprintf(stderr, "error: loop parameter is required - must be provided "
                        "by application layer\n");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (port < 1 || port > 65535) {
        fprintf(stderr, "error: port number must be in 1-65535 range\n");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!host) {
        fprintf(stderr,
                "warn: host parameter is NULL, use default value 0.0.0.0\n");
    }

    uvhttp_server_builder_t* server = NULL;
    uvhttp_error_t create_result =
        uvhttp_server_create(loop, host, port, &server);
    if (create_result != UVHTTP_OK)
        return create_result;

    // adddefaultrouter
    uvhttp_any(server, "/", default_handler);

    printf("UVHTTP server running on http://%s:%d\n", host ? host : "0.0.0.0",
           port);
    printf("Press Ctrl+C to stop server\n");

    int run_result = uvhttp_server_run(server);

    // only release after successfully creating server
    if (server) {
        uvhttp_server_simple_free(server);
    }

    return run_result;
}

// ========== WebSocket implement ==========

#if UVHTTP_FEATURE_WEBSOCKET

// WebSocket handshake verification (single-thread safe)
// register WebSocket handler (add to server's router table)
uvhttp_error_t uvhttp_server_register_ws_handler(uvhttp_server_t* server,
                                                 const char* path,
                                                 uvhttp_ws_handler_t* handler) {
    if (!server || !path || !handler) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // create new router entry
    ws_route_entry_t* entry =
        (ws_route_entry_t*)uvhttp_alloc(sizeof(ws_route_entry_t));
    if (!entry) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // allocate and copy path (use uvhttp_alloc to avoid mixing allocators)
    size_t path_len = strlen(path);
    entry->path = (char*)uvhttp_alloc(path_len + 1);
    if (!entry->path) {
        uvhttp_free(entry);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(entry->path, path, path_len + 1);

    // copyhandler
    memcpy(&entry->handler, handler, sizeof(uvhttp_ws_handler_t));
    entry->next = NULL;

    // add to server's WebSocket router table (single-thread safe)
    if (!server->ws_routes) {
        server->ws_routes = entry;
    } else {
        ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
        while (current->next) {
            current = current->next;
        }
        current->next = entry;
    }

    return UVHTTP_OK;
}

// find WebSocket handler (by path)
uvhttp_ws_handler_t* uvhttp_server_find_ws_handler(uvhttp_server_t* server,
                                                   const char* path) {
    if (!server || !path) {
        return NULL;
    }

    // traverse WebSocket router table
    ws_route_entry_t* current = (ws_route_entry_t*)server->ws_routes;
    while (current) {
        if (current->path && strcmp(current->path, path) == 0) {
            // found matching path, return handler pointer
            return &current->handler;
        }
        current = current->next;
    }

    // no matching handler found
    return NULL;
}

// sendWebSocketmessage
uvhttp_error_t uvhttp_server_ws_send(uvhttp_ws_connection_t* ws_conn,
                                     const char* data, size_t len) {
    if (!ws_conn || !data) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // get context
    uvhttp_context_t* context = NULL;
    if (ws_conn->ssl) {
        // TLS connection
        uvhttp_connection_t* conn = (uvhttp_connection_t*)ws_conn->user_data;
        if (conn && conn->server && conn->server->context) {
            context = conn->server->context;
        }
    }

    // call native WebSocket API to send text message
    int result = uvhttp_ws_send_text(context, ws_conn, data, len);
    if (result != 0) {
        return UVHTTP_ERROR_WEBSOCKET_FRAME;
    }

    return UVHTTP_OK;
}

// closeWebSocketconnection
uvhttp_error_t uvhttp_server_ws_close(uvhttp_ws_connection_t* ws_conn, int code,
                                      const char* reason) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // get context
    uvhttp_context_t* context = NULL;
    if (ws_conn->ssl) {
        // TLS connection
        uvhttp_connection_t* conn = (uvhttp_connection_t*)ws_conn->user_data;
        if (conn && conn->server && conn->server->context) {
            context = conn->server->context;
        }
    }

    // call native WebSocket API to close connection
    int result = uvhttp_ws_close(context, ws_conn, code, reason);
    if (result != 0) {
        return UVHTTP_ERROR_WEBSOCKET_FRAME;
    }

    return UVHTTP_OK;
}

#endif  // UVHTTP_FEATURE_WEBSOCKET

// ========== rate limiting function implementation (core function) ==========

#if UVHTTP_FEATURE_RATE_LIMIT
// ========== rate limiting function implementation ==========

// rate limiting parameter limit
#    define MAX_RATE_LIMIT_REQUESTS 1000000  // maximum request count: 1 million
#    define MAX_RATE_LIMIT_WINDOW_SECONDS \
        86400  // maximum time window: 24 hours

// enable rate limiting function
uvhttp_error_t uvhttp_server_enable_rate_limit(uvhttp_server_t* server,
                                               int max_requests,
                                               int window_seconds) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (max_requests <= 0 || max_requests > MAX_RATE_LIMIT_REQUESTS) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (window_seconds <= 0 || window_seconds > MAX_RATE_LIMIT_WINDOW_SECONDS) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // initialize rate limiting state
    server->rate_limit_enabled = 1;
    server->rate_limit_max_requests = max_requests;
    server->rate_limit_window_seconds = window_seconds;
    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = 0;

    return UVHTTP_OK;
}

// disable rate limiting function
uvhttp_error_t uvhttp_server_disable_rate_limit(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->rate_limit_enabled = 0;
    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = 0;

    return UVHTTP_OK;
}

// check rate limiting state
uvhttp_error_t uvhttp_server_check_rate_limit(uvhttp_server_t* server) {
    if (!server || !server->rate_limit_enabled) {
        return UVHTTP_OK;  // rate limiting not enabled, allow request
    }

    // get current time (milliseconds)
    uint64_t current_time = uv_hrtime() / 1000000;
    uint64_t window_duration = server->rate_limit_window_seconds * 1000;

    // check if time window has expired
    if (current_time - server->rate_limit_window_start_time >=
        window_duration) {
        // reset counter
        server->rate_limit_request_count = 0;
        server->rate_limit_window_start_time = current_time;
    }

    // check if exceeds limit
    if (server->rate_limit_request_count >= server->rate_limit_max_requests) {
        return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
    }

    // increase count
    server->rate_limit_request_count++;

    return UVHTTP_OK;
}

// add rate limiting whitelist IP address
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(uvhttp_server_t* server,
                                                      const char* client_ip) {
    if (!server || !client_ip) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // verifyIPaddressformat
    if (!uvhttp_is_valid_ip_address(client_ip)) {
        return UVHTTP_ERROR_INVALID_PARAM;  // invalid IP address
    }

    // check if already exists in hash table (avoid duplicate add)
    struct whitelist_item* existing_item;
    HASH_FIND_STR(server->rate_limit_whitelist_hash, client_ip, existing_item);
    if (existing_item) {
        return UVHTTP_OK;  // already exists, no need to add again
    }

    // reallocate whitelist array
    size_t new_count = server->rate_limit_whitelist_count + 1;
    void** new_whitelist =
        uvhttp_realloc(server->rate_limit_whitelist, sizeof(void*) * new_count);
    if (!new_whitelist) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    server->rate_limit_whitelist = new_whitelist;
    server->rate_limit_whitelist_count = new_count;

    // copyIPaddress
    size_t ip_len = strlen(client_ip) + 1;
    char* ip_copy = uvhttp_alloc(ip_len);
    if (!ip_copy) {
        // fallback: resume original array size
        server->rate_limit_whitelist_count = new_count - 1;
        void** old_whitelist = uvhttp_realloc(server->rate_limit_whitelist,
                                              sizeof(void*) * (new_count - 1));
        if (old_whitelist) {
            server->rate_limit_whitelist = old_whitelist;
        }
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    memcpy(ip_copy, client_ip, ip_len);
    server->rate_limit_whitelist[new_count - 1] = ip_copy;

    // add to hash table (for O(1) lookup)
    struct whitelist_item* hash_item =
        uvhttp_alloc(sizeof(struct whitelist_item));
    if (!hash_item) {
        // fallback: clean allocated IP string
        uvhttp_free(ip_copy);
        server->rate_limit_whitelist_count = new_count - 1;
        void** old_whitelist = uvhttp_realloc(server->rate_limit_whitelist,
                                              sizeof(void*) * (new_count - 1));
        if (old_whitelist) {
            server->rate_limit_whitelist = old_whitelist;
        }
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    strncpy(hash_item->ip, client_ip, INET_ADDRSTRLEN - 1);
    hash_item->ip[INET_ADDRSTRLEN - 1] = '\0';
    HASH_ADD_STR(server->rate_limit_whitelist_hash, ip, hash_item);

    return UVHTTP_OK;
}

// get client rate limiting state
uvhttp_error_t uvhttp_server_get_rate_limit_status(uvhttp_server_t* server,
                                                   const char* client_ip,
                                                   int* remaining,
                                                   uint64_t* reset_time) {
    if (!server || !client_ip || !remaining) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!server->rate_limit_enabled) {
        *remaining = -1;  // rate limiting not enabled
        return UVHTTP_OK;
    }

    *remaining =
        server->rate_limit_max_requests - server->rate_limit_request_count;

    if (reset_time) {
        uint64_t window_duration = server->rate_limit_window_seconds * 1000;
        *reset_time = server->rate_limit_window_start_time + window_duration;
    }

    return UVHTTP_OK;
}

// clear all rate limiting states
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = 0;

    return UVHTTP_OK;
}

// reset client rate limiting state
uvhttp_error_t uvhttp_server_reset_rate_limit_client(uvhttp_server_t* server,
                                                     const char* client_ip) {
    if (!server || !client_ip) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // simplified implementation: reset entire server's rate limiting counter
    server->rate_limit_request_count = 0;
    server->rate_limit_window_start_time = uv_hrtime() / 1000000;

    return UVHTTP_OK;
}
#endif /* UVHTTP_FEATURE_RATE_LIMIT */

#if !UVHTTP_FEATURE_TLS
// null TLS function definition, used when TLS is disabled for linking
void uvhttp_tls_context_free(void* ctx) {
    (void)ctx;
}
#endif

// ========== WebSocket connectionmanageimplement ==========

#if UVHTTP_FEATURE_WEBSOCKET

/**
 * timeout detection timer callback
 * check all connections' activity time, close timeout connections
 */
static void ws_timeout_timer_callback(uv_timer_t* handle) {
    if (!handle || !handle->data) {
        return;
    }

    ws_connection_manager_t* manager = (ws_connection_manager_t*)handle->data;
    uint64_t current_time = uv_hrtime() / 1000000; /* convert to milliseconds */
    uint64_t timeout_ms = manager->timeout_seconds * 1000;

    ws_connection_node_t* current = manager->connections;
    ws_connection_node_t* prev = NULL;

    while (current) {
        ws_connection_node_t* next = current->next;

        /* check if connection has timed out */
        if (current_time - current->last_activity > timeout_ms) {
            UVHTTP_LOG_WARN("WebSocket connection timeout, closing...\n");

            /* closetimeoutconnection */
            if (current->ws_conn) {
                uvhttp_ws_close(NULL, current->ws_conn, 1000,
                                "Connection timeout");
            }

            /* remove from list */
            if (prev) {
                prev->next = next;
            } else {
                manager->connections = next;
            }

            /* release node */
            uvhttp_free(current);
            manager->connection_count--;
        } else {
            prev = current;
        }

        current = next;
    }
}

/**
 * heartbeat detection timer callback
 * periodically send Ping frame to detect connection active state
 */
static void ws_heartbeat_timer_callback(uv_timer_t* handle) {
    if (!handle || !handle->data) {
        return;
    }

    ws_connection_manager_t* manager = (ws_connection_manager_t*)handle->data;
    uint64_t current_time = uv_hrtime() / 1000000; /* convert to milliseconds */

    ws_connection_node_t* current = manager->connections;

    while (current) {
        if (current->ws_conn &&
            current->ws_conn->state == UVHTTP_WS_STATE_OPEN) {
            /* check if need to send Ping */
            if (!current->ping_pending) {
                /* send Ping frame */
                if (uvhttp_ws_send_ping(NULL, current->ws_conn, NULL, 0) == 0) {
                    current->last_ping_sent = current_time;
                    current->ping_pending = 1;
                }
            } else {
                /* check if Ping has timed out (no Pong response received) */
                if (current_time - current->last_ping_sent >
                    manager->ping_timeout_ms) {
                    UVHTTP_LOG_WARN(
                        "WebSocket ping timeout, closing connection...\n");

                    /* close connection without response */
                    uvhttp_ws_close(NULL, current->ws_conn, 1000,
                                    "Ping timeout");
                }
            }
        }

        current = current->next;
    }
}

/**
 * enable WebSocket connectionmanage
 *
 * @param server serverinstance
 * @param timeout_seconds timeout time (seconds), range: 10-3600
 * @param heartbeat_interval heartbeat interval (seconds), range: 5-300
 * @return UVHTTP_OK success, other values indicate failure
 */
uvhttp_error_t uvhttp_server_ws_enable_connection_management(
    uvhttp_server_t* server, int timeout_seconds, int heartbeat_interval) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* parameterverify */
    if (timeout_seconds < 10 || timeout_seconds > 3600) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (heartbeat_interval < 5 || heartbeat_interval > 300) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* if already enabled, disable first */
    if (server->ws_connection_manager) {
        uvhttp_error_t result =
            uvhttp_server_ws_disable_connection_management(server);
        if (result != UVHTTP_OK) {
            return result;
        }
    }

    /* create connection manager */
    ws_connection_manager_t* manager =
        uvhttp_alloc(sizeof(ws_connection_manager_t));
    if (!manager) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(manager, 0, sizeof(ws_connection_manager_t));
    manager->connections = NULL;
    manager->connection_count = 0;
    manager->timeout_seconds = timeout_seconds;
    manager->heartbeat_interval = heartbeat_interval;
    manager->ping_timeout_ms = 10000; /* default 10 seconds Ping timeout */
    manager->enabled = 1;

    /* initialize timeout detection timer */
    int ret = uv_timer_init(server->loop, &manager->timeout_timer);
    if (ret != 0) {
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }
    manager->timeout_timer.data = manager;

    /* initialize heartbeat detection timer */
    ret = uv_timer_init(server->loop, &manager->heartbeat_timer);
    if (ret != 0) {
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }
    manager->heartbeat_timer.data = manager;

    /* start timer */
    ret = uv_timer_start(&manager->timeout_timer, ws_timeout_timer_callback,
                         timeout_seconds * 1000, timeout_seconds * 1000);
    if (ret != 0) {
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
        uv_close((uv_handle_t*)&manager->heartbeat_timer, NULL);
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }

    ret = uv_timer_start(&manager->heartbeat_timer, ws_heartbeat_timer_callback,
                         heartbeat_interval * 1000, heartbeat_interval * 1000);
    if (ret != 0) {
        uv_timer_stop(&manager->timeout_timer);
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
        uv_close((uv_handle_t*)&manager->heartbeat_timer, NULL);
        uvhttp_free(manager);
        return UVHTTP_ERROR_SERVER_INIT;
    }

    server->ws_connection_manager = manager;

    UVHTTP_LOG_INFO(
        "WebSocket connection management enabled: timeout=%ds, heartbeat=%ds\n",
        timeout_seconds, heartbeat_interval);

    return UVHTTP_OK;
}

/**
 * disable WebSocket connectionmanage
 *
 * @param server serverinstance
 * @return UVHTTP_OK success, other values indicate failure
 */
uvhttp_error_t uvhttp_server_ws_disable_connection_management(
    uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager) {
        return UVHTTP_OK; /* not enabled, directly return success */
    }

    /* stop timer */
    if (!uv_is_closing((uv_handle_t*)&manager->timeout_timer)) {
        uv_timer_stop(&manager->timeout_timer);
        uv_close((uv_handle_t*)&manager->timeout_timer, NULL);
    }

    if (!uv_is_closing((uv_handle_t*)&manager->heartbeat_timer)) {
        uv_timer_stop(&manager->heartbeat_timer);
        uv_close((uv_handle_t*)&manager->heartbeat_timer, NULL);
    }

    /* close all connections */
    ws_connection_node_t* current = manager->connections;
    while (current) {
        ws_connection_node_t* next = current->next;

        if (current->ws_conn) {
            uvhttp_ws_close(NULL, current->ws_conn, 1000, "Server shutdown");
        }

        uvhttp_free(current);
        current = next;
    }

    manager->connections = NULL;
    manager->connection_count = 0;
    manager->enabled = 0;

    /* release manager */
    uvhttp_free(manager);
    server->ws_connection_manager = NULL;

    UVHTTP_LOG_INFO("WebSocket connection management disabled\n");

    return UVHTTP_OK;
}

/**
 * get total WebSocket connection count
 *
 * @param server serverinstance
 * @return connectioncount
 */
int uvhttp_server_ws_get_connection_count(uvhttp_server_t* server) {
    if (!server || !server->ws_connection_manager) {
        return 0;
    }

    return server->ws_connection_manager->connection_count;
}

/**
 * get WebSocket connection count for specified path
 *
 * @param server serverinstance
 * @param path path
 * @return connectioncount
 */
int uvhttp_server_ws_get_connection_count_by_path(uvhttp_server_t* server,
                                                  const char* path) {
    if (!server || !server->ws_connection_manager || !path) {
        return 0;
    }

    int count = 0;
    ws_connection_node_t* current = server->ws_connection_manager->connections;

    while (current) {
        if (strcmp(current->path, path) == 0) {
            count++;
        }
        current = current->next;
    }

    return count;
}

/**
 * broadcast message to all connections on specified path
 *
 * @param server serverinstance
 * @param path path (NULL means broadcast to all connections)
 * @param data messagedata
 * @param len messagelength
 * @return UVHTTP_OK success, other values indicate failure
 */
uvhttp_error_t uvhttp_server_ws_broadcast(uvhttp_server_t* server,
                                          const char* path, const char* data,
                                          size_t len) {
    if (!server || !server->ws_connection_manager) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    if (!data || len == 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    ws_connection_node_t* current = server->ws_connection_manager->connections;
    int sent_count = 0;

    while (current) {
        /* check if path matches (if path is specified) */
        if (!path || strcmp(current->path, path) == 0) {
            if (current->ws_conn &&
                current->ws_conn->state == UVHTTP_WS_STATE_OPEN) {
                uvhttp_ws_send_text(NULL, current->ws_conn, data, len);
                sent_count++;
            }
        }

        current = current->next;
    }

    UVHTTP_LOG_DEBUG("WebSocket broadcast: sent to %d connections\n",
                     sent_count);

    return UVHTTP_OK;
}

/**
 * close all connections on specified path
 *
 * @param server server instance
 * @param path path (NULL means close all connections)
 * @return UVHTTP_OK success, other values indicate failure
 */
uvhttp_error_t uvhttp_server_ws_close_all(uvhttp_server_t* server,
                                          const char* path) {
    if (!server || !server->ws_connection_manager) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    int closed_count = 0;
    ws_connection_node_t* current = server->ws_connection_manager->connections;
    ws_connection_node_t* prev = NULL;

    while (current) {
        ws_connection_node_t* next = current->next;

        /* check if path matches (if path is specified) */
        if (!path || strcmp(current->path, path) == 0) {
            /* closeconnection */
            if (current->ws_conn) {
                uvhttp_ws_close(NULL, current->ws_conn, 1000,
                                "Server closed connection");
            }

            /* remove from list */
            if (prev) {
                prev->next = next;
            } else {
                server->ws_connection_manager->connections = next;
            }

            /* release node */
            uvhttp_free(current);
            server->ws_connection_manager->connection_count--;
            closed_count++;
        } else {
            prev = current;
        }

        current = next;
    }

    UVHTTP_LOG_DEBUG("WebSocket close_all: closed %d connections\n",
                     closed_count);

    return UVHTTP_OK;
}

/**
 * internal function: add WebSocket connection to manager
 */
void uvhttp_server_ws_add_connection(uvhttp_server_t* server,
                                     uvhttp_ws_connection_t* ws_conn,
                                     const char* path) {
    if (!server || !ws_conn || !path) {
        return;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager || !manager->enabled) {
        return;
    }

    /* create connection node */
    ws_connection_node_t* node = uvhttp_alloc(sizeof(ws_connection_node_t));
    if (!node) {
        UVHTTP_LOG_ERROR("Failed to allocate WebSocket connection node\n");
        return;
    }

    memset(node, 0, sizeof(ws_connection_node_t));
    node->ws_conn = ws_conn;
    strncpy(node->path, path, sizeof(node->path) - 1);
    node->path[sizeof(node->path) - 1] = '\0';
    node->last_activity = uv_hrtime() / 1000000; /* convert to milliseconds */
    node->last_ping_sent = 0;
    node->ping_pending = 0;
    node->next = NULL;

    /* add to list header */
    node->next = manager->connections;
    manager->connections = node;
    manager->connection_count++;

    UVHTTP_LOG_DEBUG("WebSocket connection added: path=%s, total=%d\n", path,
                     manager->connection_count);
}

/**
 * internal function: remove WebSocket connection from manager
 */
void uvhttp_server_ws_remove_connection(uvhttp_server_t* server,
                                        uvhttp_ws_connection_t* ws_conn) {
    if (!server || !ws_conn) {
        return;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager || !manager->enabled) {
        return;
    }

    ws_connection_node_t* current = manager->connections;
    ws_connection_node_t* prev = NULL;

    while (current) {
        if (current->ws_conn == ws_conn) {
            /* remove from list */
            if (prev) {
                prev->next = current->next;
            } else {
                manager->connections = current->next;
            }

            /* release node */
            uvhttp_free(current);
            manager->connection_count--;

            UVHTTP_LOG_DEBUG("WebSocket connection removed: total=%d\n",
                             manager->connection_count);
            return;
        }

        prev = current;
        current = current->next;
    }
}

/**
 * internal function: update WebSocket connection activity time
 */
void uvhttp_server_ws_update_activity(uvhttp_server_t* server,
                                      uvhttp_ws_connection_t* ws_conn) {
    if (!server || !ws_conn) {
        return;
    }

    ws_connection_manager_t* manager = server->ws_connection_manager;
    if (!manager || !manager->enabled) {
        return;
    }

    ws_connection_node_t* current = manager->connections;

    while (current) {
        if (current->ws_conn == ws_conn) {
            current->last_activity =
                uv_hrtime() / 1000000; /* convert to milliseconds */
            current->ping_pending = 0; /* clear pending Ping flag */
            return;
        }

        current = current->next;
    }
}

/* ========== WebSocket authenticate API ========== */

#endif /* UVHTTP_FEATURE_WEBSOCKET */
uvhttp_error_t uvhttp_server_set_timeout_callback(
    uvhttp_server_t* server, uvhttp_timeout_callback_t callback,
    void* user_data) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    server->timeout_callback = callback;
    server->timeout_callback_user_data = user_data;

    return UVHTTP_OK;
}
