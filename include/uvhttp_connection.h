#ifndef UVHTTP_CONNECTION_H
#define UVHTTP_CONNECTION_H

#include "uvhttp_common.h"
#include "uvhttp_platform.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"

#include "llhttp.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

// Forward declaration (avoid circular references)
typedef struct uvhttp_connection uvhttp_connection_t;
typedef struct uvhttp_server uvhttp_server_t;

typedef enum {
    UVHTTP_CONN_STATE_NEW,
    UVHTTP_CONN_STATE_TLS_HANDSHAKE,
    UVHTTP_CONN_STATE_HTTP_READING,
    UVHTTP_CONN_STATE_HTTP_PROCESSING,
    UVHTTP_CONN_STATE_HTTP_WRITING,
    UVHTTP_CONN_STATE_CLOSING
} uvhttp_connection_state_t;

struct uvhttp_connection {
    /* ========== Cache line 1 (0-63 bytes): hot path fields - most frequently
     * accessed ========== */
    /* Frequently accessed in on_read, on_write, connection management */
    uvhttp_connection_state_t state; /* 4 bytes - Connectionstate */
    int parsing_complete;            /* 4 bytes - parsingusecompleted */
    int keepalive;                   /* 4 bytes - usekeepConnection */
    int chunked_encoding;            /* 4 bytes - useUsechunkedtransfer */
    int close_pending;               /* 4 bytes - pendingclose handle count */
    int need_restart_read;           /* 4 bytes - useneedrestartread */
    int tls_enabled;                 /* 4 bytes - TLS useEnable */
    int _padding1;                   /* 4 bytes - paddingto32bytes */
    size_t body_received;            /* 8 bytes - receive body length */
    size_t content_length;           /* 8 bytes - Content-Length */
    size_t read_buffer_used;         /* 8 bytes - BufferUse */
    size_t read_buffer_size;         /* 8 bytes - Buffersize */
    /* Cache line 1 total: 56 bytes (remaining 8 bytes padding) */

    /* ========== Cache line 2 (64-127 bytes): pointer fields - second most
     * frequently accessed ========== */
    /* Frequently accessed in connection creation, destruction, request handling
     */
    struct uvhttp_server* server; /* 8 bytes - ownedServer */
    uvhttp_request_t* request;    /* 8 bytes - Requestobject */
    uvhttp_response_t* response;  /* 8 bytes - Responseobject */
    void* ssl;                    /* 8 bytes - SSL/TLS context */
    char* read_buffer;            /* 8 bytes - Bufferpointer */
#if UVHTTP_FEATURE_WEBSOCKET
    void* ws_connection; /* 8 bytes - WebSocket connection */
#endif
    int current_header_is_important; /* 4 bytes - currentheaderuseimportant */
    int parsing_header_field; /* 4 bytes - useparsingparsingheaderField */
    int last_error;           /* 4 bytes - lastError code */
#if UVHTTP_FEATURE_WEBSOCKET
    int is_websocket; /* 4 bytes - useto WebSocket connection */
#endif
    int _padding2; /* 4 bytes - paddingto64bytes */
    /* Cache line 2 total: approximately 64 bytes (depends on whether WebSocket
     * is enabled) */

    /* ========== Cache line 3 (128-191 bytes): libuv handles ========== */
    /* libuv internal structure, fixed size */
    uv_tcp_t tcp_handle;      /* approximately40-48bytes */
    uv_idle_t idle_handle;    /* approximately24-32bytes */
    uv_timer_t timeout_timer; /* approximately24-32bytes */
    /* Cache line 3 total: approximately 88-112 bytes */

    /* ========== Cache line 4 (192-255 bytes): HTTP parsing state ========== */
    /* Frequently accessed during HTTP parsing */
    size_t current_header_field_len; /* 8 bytes - currentheaderFieldlength */
    int _padding3[14];               /* 56bytes - paddingto64bytes */
    /* Cache line 4 total: 64 bytes */

    /* ========== Cache line 5+ (256+ bytes): large buffers ========== */
    /* Placed at the end to avoid affecting cache locality of hot path fields */
    char current_header_field[UVHTTP_MAX_HEADER_NAME_SIZE]; /* blockmemory */
};

/* ========== Memory Layout Verification Static Assertions ========== */

/* Verify pointer alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, server, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, request, UVHTTP_POINTER_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, response, UVHTTP_POINTER_ALIGNMENT);

/* Verify size_t alignment (platform adaptive) */
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, content_length,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, body_received,
                       UVHTTP_SIZE_T_ALIGNMENT);
UVHTTP_CHECK_ALIGNMENT(uvhttp_connection_t, read_buffer_size,
                       UVHTTP_SIZE_T_ALIGNMENT);

/* Verify large buffers are at the end of structure */
UVHTTP_STATIC_ASSERT(offsetof(uvhttp_connection_t, current_header_field) >= 64,
                     "current_header_field should be after first 64 bytes");

/* Connection management functions */
/**
 * @brief createnewConnectionobject
 * @param server Serverobject
 * @param conn outputParameter, Used forreceiveConnectionpointer
 * @return UVHTTP_OK Success, othervaluerepresentsFailure
 * @note Successwhen, *conn issettovalidConnectionobject, mustUse
 * uvhttp_connection_free release
 * @note Failurewhen, *conn issetto NULL
 */
uvhttp_error_t uvhttp_connection_new(struct uvhttp_server* server,
                                     uvhttp_connection_t** conn);
void uvhttp_connection_free(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t* conn);
void uvhttp_connection_close(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_restart_read(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_schedule_restart_read(
    uvhttp_connection_t* conn);

/* TLShandleFunction */
uvhttp_error_t uvhttp_connection_start_tls_handshake(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_tls_read(uvhttp_connection_t* conn);
uvhttp_error_t uvhttp_connection_tls_write(uvhttp_connection_t* conn,
                                           const void* data, size_t len);
uvhttp_error_t uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn);
void uvhttp_connection_tls_cleanup(uvhttp_connection_t* conn);

/* statemanage */
void uvhttp_connection_set_state(uvhttp_connection_t* conn,
                                 uvhttp_connection_state_t state);
const char* uvhttp_connection_get_state_string(uvhttp_connection_state_t state);

/* WebSockethandleFunction(internal) */
#if UVHTTP_FEATURE_WEBSOCKET
uvhttp_error_t uvhttp_connection_handle_websocket_handshake(
    uvhttp_connection_t* conn, const char* ws_key);
void uvhttp_connection_switch_to_websocket(uvhttp_connection_t* conn);
void uvhttp_connection_websocket_read(uv_stream_t* stream, ssize_t nread,
                                      const uv_buf_t* buf);
void uvhttp_connection_websocket_close(uvhttp_connection_t* conn);

/**
 * @brief ConnectionTimeoutwhen
 *
 * toConnectionTimeoutwhen, UseDefaultTimeoutwhen。
 * ifConnectionTimeoutwhenno, AutomaticcloseConnection。
 *
 * @param conn Connectionobject
 * @return int SuccessReturn UVHTTP_OK, FailureReturnnegativeError code
 *
 * @note Timeoutwhen conn->server->config->connection_timeout read,
 *       if config to NULL, Use UVHTTP_CONNECTION_TIMEOUT_DEFAULT
 * @note Functionstoprestartexistingwhen(if)
 */
uvhttp_error_t uvhttp_connection_start_timeout(uvhttp_connection_t* conn);

/**
 * @brief ConnectionTimeoutwhen(customTimeoutwhen)
 *
 * toConnectionTimeoutwhen, UsespecifiedTimeoutwhen。
 * ifConnectionTimeoutwhenno, AutomaticcloseConnection。
 *
 * @param conn Connectionobject
 * @param timeout_seconds Timeoutwhen(seconds), scope: 5-300
 * @return uvhttp_error_t SuccessReturn UVHTTP_OK, FailureReturnnegativeError
 * code
 *
 * @note Timeoutwhenmust UVHTTP_CONNECTION_TIMEOUT_MIN and
 *       UVHTTP_CONNECTION_TIMEOUT_MAX
 * @note Functionstoprestartexistingwhen(if)
 * @note ifTimeoutwhencauseoverflow, Return UVHTTP_ERROR_INVALID_PARAM
 */
uvhttp_error_t uvhttp_connection_start_timeout_custom(uvhttp_connection_t* conn,
                                                      int timeout_seconds);

#endif /* UVHTTP_FEATURE_WEBSOCKET */

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_CONNECTION_H */