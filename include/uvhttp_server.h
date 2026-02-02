#ifndef UVHTTP_SERVER_H
#define UVHTTP_SERVER_H

#include "uvhttp_allocator.h"
#include "uvhttp_common.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"
#include "uvhttp_platform.h"

#include <uv.h>

/* Include uthash header for hash table implementation */
#include "uthash.h"

/* Whitelist hash table item */
struct whitelist_item {
    char ip[INET_ADDRSTRLEN];
    UT_hash_handle hh;
};
#include "uvhttp_features.h"

// Forward declarations
typedef struct uvhttp_request uvhttp_request_t;
typedef struct uvhttp_response uvhttp_response_t;
typedef struct uvhttp_router uvhttp_router_t;
typedef struct uvhttp_connection uvhttp_connection_t;

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

#define MAX_CONNECTIONS 1000

typedef struct uvhttp_server uvhttp_server_t;

/* Server builder structure (unified API) */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uvhttp_config_t* config;
    uv_loop_t* loop;
    int auto_cleanup;
} uvhttp_server_builder_t;
/* ========== Timeout Statistics Callback ========== */
/**
 * @brief Timeoutstatisticscallback functionsclass
 *
 * , Used forstatisticsandrecordConnectionTimeoutEvent
 */
typedef void (*uvhttp_timeout_callback_t)(uvhttp_server_t* server,
                                          uvhttp_connection_t* conn,
                                          uint64_t timeout_ms, void* user_data);

struct uvhttp_server {
    /* ========== Cache line 1 (0-63 bytes): hot path fields - most frequently
     * accessed ========== */
    /* Frequently accessed in on_connection, connection management */
    int is_listening;                 /* 4 bytes - useparsinglisten */
    int owns_loop;                    /* 4 bytes - useloop */
    int _padding1[2];                 /* 8 bytes - paddingto16bytes */
    size_t active_connections;        /* 8 bytes - Connection */
    size_t max_connections;           /* 8 bytes - Connection */
    size_t max_message_size;          /* 8 bytes - messagesize */
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
    uvhttp_config_t* config;        /* 8 bytes -  */
    struct uvhttp_context* context; /* 8 bytes -  */
    void* user_data;                /* 8 bytes -  */
#if UVHTTP_FEATURE_TLS
    uvhttp_tls_context_t* tls_ctx; /* 8 bytes - TLS context */
    int tls_enabled;               /* 4 bytes - TLS useEnable */
    int _padding2[3];              /* 12bytes - paddingto16bytes */
#else
    int _padding2[4];  /* 16bytes - paddingto64bytes */
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

    /* ========== Cache line 5-6 (256-383 bytes): rate limiting functionality
     * ========== */
#if UVHTTP_FEATURE_RATE_LIMIT
    int rate_limit_enabled;                /* 4 bytes - Rate limituseEnable */
    int rate_limit_max_requests;           /* 4 bytes - Request */
    int rate_limit_window_seconds;         /* 4 bytes - whenwindow(seconds) */
    int rate_limit_request_count;          /* 4 bytes - currentRequest */
    uint64_t rate_limit_window_start_time; /* 8 bytes - windowwhen */
    void** rate_limit_whitelist;           /* 8 bytes - Whitelist */
    size_t rate_limit_whitelist_count;     /* 8 bytes - Whitelistquantity */
    struct whitelist_item*
        rate_limit_whitelist_hash; /* 8 bytes - Whitelisthash */
    int _padding5[8];              /* 32bytes - paddingto64bytes */
#else
    int _padding5[16]; /* 64bytes - paddingto64bytes */
#endif
    /* Cache line 5 total: 64 bytes */
};

/* ========== Memory Layout Verification Static Assertions ========== */

/* Verify pointer alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, loop, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, router, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_server_t, config, UVHTTP_POINTER_ALIGNMENT);

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
uvhttp_error_t uvhttp_server_listen(uvhttp_server_t* server, const char* host,
                                    int port);
uvhttp_error_t uvhttp_server_stop(uvhttp_server_t* server);
#if UVHTTP_FEATURE_TLS
uvhttp_error_t uvhttp_server_enable_tls(uvhttp_server_t* server,
                                        uvhttp_tls_context_t* tls_ctx);
uvhttp_error_t uvhttp_server_disable_tls(uvhttp_server_t* server);
#endif
uvhttp_error_t uvhttp_server_free(uvhttp_server_t* server);
uvhttp_error_t uvhttp_server_set_handler(uvhttp_server_t* server,
                                         uvhttp_request_handler_t handler);
uvhttp_error_t uvhttp_server_set_router(uvhttp_server_t* server,
                                        uvhttp_router_t* router);
uvhttp_error_t uvhttp_server_set_context(uvhttp_server_t* server,
                                         struct uvhttp_context* context);

#if UVHTTP_FEATURE_RATE_LIMIT
/* ========== Rate Limiting API (Core Functionality) ========== */

/**
 * EnableServerRate limitfunction
 *
 * @param server Server
 * @param max_requests whenwindowallowRequest(scope: 1-1000000)
 * @param window_seconds whenwindow(seconds, scope: 1-86400)
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 *
 * Note:
 * - Rate limitfunctionpairofRequest(ServerRate limit)
 * - Rate limitstateServermanage, ofClientcounter
 * - Used forprevent DDoS , Used forClientRate limit
 * -  uvhttp_server_listen
 * - Usewindow
 */
uvhttp_error_t uvhttp_server_enable_rate_limit(uvhttp_server_t* server,
                                               int max_requests,
                                               int window_seconds);

/**
 * DisableRate limitfunction
 *
 * @param server Server
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 */
uvhttp_error_t uvhttp_server_disable_rate_limit(uvhttp_server_t* server);

/**
 * Rate limitstate(internalUse)
 *
 * @param server Server
 * @return UVHTTP_OK allowRequest, UVHTTP_ERROR_RATE_LIMIT_EXCEEDED exceedRate
 * limit
 */
uvhttp_error_t uvhttp_server_check_rate_limit(uvhttp_server_t* server);

/**
 * addRate limitWhitelistIPaddress(Rate limit)
 *
 * @param server Server
 * @param client_ip ClientIPaddress( "127.0.0.1")
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 */
uvhttp_error_t uvhttp_server_add_rate_limit_whitelist(uvhttp_server_t* server,
                                                      const char* client_ip);

/**
 * getServerRate limitstate
 *
 * @param server Server
 * @param client_ip ClientIPaddress(currentUse, Parameterextend)
 * @param remaining remainingRequest(output)
 * @param reset_time Timestamp(output, seconds)
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 *
 * Note:
 * - currenttoServerRate limit, client_ip ParameterUse
 * - ReturnServerRate limitstate, Clientstate
 */
uvhttp_error_t uvhttp_server_get_rate_limit_status(uvhttp_server_t* server,
                                                   const char* client_ip,
                                                   int* remaining,
                                                   uint64_t* reset_time);

/**
 * ServerRate limitstate
 *
 * @param server Server
 * @param client_ip ClientIPaddress(currentUse, Parameterextend)
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 *
 * Note:
 * - currenttoServerRate limit, client_ip ParameterUse
 * - ServerRate limitcounter, Clientcounter
 */
uvhttp_error_t uvhttp_server_reset_rate_limit_client(uvhttp_server_t* server,
                                                     const char* client_ip);

/**
 * nullofRate limitstate
 *
 * @param server Server
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 */
uvhttp_error_t uvhttp_server_clear_rate_limit_all(uvhttp_server_t* server);
#endif /* UVHTTP_FEATURE_RATE_LIMIT */

/* ========== Unified API Functions ========== */

/* Quick create and start server */
/**
 * Create simple server (API)
 *
 * @param loop libuv event loop (must be provided by application layer)
 * @param host listen address
 * @param port listen port
 * @param server output parameter, return created server
 * @return UVHTTP_OK success, other value represents error
 */
uvhttp_error_t uvhttp_server_create(uv_loop_t* loop, const char* host, int port,
                                    uvhttp_server_builder_t** server);

/* Chained routing API */
uvhttp_server_builder_t* uvhttp_get(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_post(uvhttp_server_builder_t* server,
                                     const char* path,
                                     uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_put(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_delete(uvhttp_server_builder_t* server,
                                       const char* path,
                                       uvhttp_request_handler_t handler);
uvhttp_server_builder_t* uvhttp_any(uvhttp_server_builder_t* server,
                                    const char* path,
                                    uvhttp_request_handler_t handler);

/* Simplified configuration API */
uvhttp_server_builder_t* uvhttp_set_max_connections(
    uvhttp_server_builder_t* server, int max_conn);
uvhttp_server_builder_t* uvhttp_set_timeout(uvhttp_server_builder_t* server,
                                            int timeout);
uvhttp_server_builder_t* uvhttp_set_max_body_size(
    uvhttp_server_builder_t* server, size_t size);

/* Convenient request parameter access */
const char* uvhttp_get_param(uvhttp_request_t* request, const char* name);
const char* uvhttp_get_header(uvhttp_request_t* request, const char* name);
const char* uvhttp_get_body(uvhttp_request_t* request);

/* Server runtime and cleanup */
int uvhttp_server_run(uvhttp_server_builder_t* server);
void uvhttp_server_stop_simple(uvhttp_server_builder_t* server);
void uvhttp_server_simple_free(uvhttp_server_builder_t* server);

/* one key function (API) */
int uvhttp_serve(uv_loop_t* loop, const char* host, int port);

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

/* internalFunction( uvhttp_connection ) */
void uvhttp_server_ws_add_connection(uvhttp_server_t* server,
                                     uvhttp_ws_connection_t* ws_conn,
                                     const char* path);

void uvhttp_server_ws_remove_connection(uvhttp_server_t* server,
                                        uvhttp_ws_connection_t* ws_conn);

void uvhttp_server_ws_update_activity(uvhttp_server_t* server,
                                      uvhttp_ws_connection_t* ws_conn);
#endif

/* internalFunction */
uvhttp_error_t uvhttp_request_init(uvhttp_request_t* request, uv_tcp_t* client);
void uvhttp_request_cleanup(uvhttp_request_t* request);

/* TLSFunction (whenDisable) */
/* uvhttp_error_t uvhttp_tls_init(void); */
/* void uvhttp_tls_context_free(uvhttp_tls_context_t* ctx); */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_SERVER_H */
