/*
 * UVHTTP WebSocket
 * complete WebSocket protocolSupport, memory RFC 6455
 */

#if UVHTTP_FEATURE_WEBSOCKET

#    ifndef UVHTTP_WEBSOCKET_H
#        define UVHTTP_WEBSOCKET_H

#        include "uvhttp_config.h"
#        include "uvhttp_connection.h"
#        include "uvhttp_error.h"
#        include "uvhttp_tls.h"

#        include <stddef.h>
#        include <stdint.h>

#        ifdef __cplusplus
extern "C" {
#        endif

/* WebSocket opcodes (RFC 6455) */
typedef enum {
    UVHTTP_WS_OPCODE_CONTINUATION = 0x0,
    UVHTTP_WS_OPCODE_TEXT = 0x1,
    UVHTTP_WS_OPCODE_BINARY = 0x2,
    UVHTTP_WS_OPCODE_CLOSE = 0x8,
    UVHTTP_WS_OPCODE_PING = 0x9,
    UVHTTP_WS_OPCODE_PONG = 0xA
} uvhttp_ws_opcode_t;

/* WebSocket states */
typedef enum {
    UVHTTP_WS_STATE_CONNECTING = 0,
    UVHTTP_WS_STATE_OPEN = 1,
    UVHTTP_WS_STATE_CLOSING = 2,
    UVHTTP_WS_STATE_CLOSED = 3
} uvhttp_ws_state_t;

/* WebSocket frame header (RFC 6455) */
typedef struct {
    uint8_t fin : 1;
    uint8_t rsv1 : 1;
    uint8_t rsv2 : 1;
    uint8_t rsv3 : 1;
    uint8_t opcode : 4;
    uint8_t mask : 1;
    uint8_t payload_len : 7;
} uvhttp_ws_frame_header_t;

/* WebSocket frame structure */
typedef struct {
    uvhttp_ws_frame_header_t header;
    uint64_t payload_length;
    uint8_t masking_key[4];
    uint8_t* payload;
    size_t payload_size;
} uvhttp_ws_frame_t;

/* WebSocket configuration */
typedef struct {
    int max_frame_size;
    int max_message_size;
    int ping_interval;
    int ping_timeout;
    int enable_compression;
} uvhttp_ws_config_t;

/* Forward declarations */
struct uvhttp_ws_connection;

/* Callback function types */
typedef int (*uvhttp_ws_on_message_callback)(struct uvhttp_ws_connection* conn,
                                             const char* data, size_t len,
                                             int opcode);
typedef int (*uvhttp_ws_on_close_callback)(struct uvhttp_ws_connection* conn,
                                           int code, const char* reason);
typedef int (*uvhttp_ws_on_error_callback)(struct uvhttp_ws_connection* conn,
                                           int error_code,
                                           const char* error_msg);

/* WebSocket connection */
typedef struct uvhttp_ws_connection {
    int fd;
    uvhttp_ws_state_t state;
    uvhttp_ws_config_t config;
    mbedtls_ssl_context* ssl;
    int is_server;

    /* Original key saved during client handshake (for accept verification) */
    char client_key[64];

    /* Receive buffer */
    uint8_t* recv_buffer;
    size_t recv_buffer_size;
    size_t recv_buffer_pos;

    /* Send buffer */
    uint8_t* send_buffer;
    size_t send_buffer_size;

    /* Fragment reassembly */
    uint8_t* fragmented_message;
    size_t fragmented_size;
    size_t fragmented_capacity;
    uvhttp_ws_opcode_t fragmented_opcode;

    /* Callback functions */
    uvhttp_ws_on_message_callback on_message;
    uvhttp_ws_on_close_callback on_close;
    uvhttp_ws_on_error_callback on_error;
    void* user_data;

    /* Statistics */
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint64_t frames_sent;
    uint64_t frames_received;
} uvhttp_ws_connection_t;

/* WebSocket API */

/**
 * create WebSocket connection
 */
struct uvhttp_ws_connection* uvhttp_ws_connection_create(
    int fd, mbedtls_ssl_context* ssl, int is_server,
    const uvhttp_config_t* config);

/**
 * release WebSocket connection
 */
void uvhttp_ws_connection_free(struct uvhttp_ws_connection* conn);

/**
 * execute WebSocket handshake (Server)
 */
uvhttp_error_t uvhttp_ws_handshake_server(struct uvhttp_ws_connection* conn,
                                          const char* request,
                                          size_t request_len, char* response,
                                          size_t* response_len);

/**
 * execute WebSocket handshake (Client)
 */
uvhttp_error_t uvhttp_ws_handshake_client(uvhttp_context_t* context,
                                          struct uvhttp_ws_connection* conn,
                                          const char* host, const char* path,
                                          char* request, size_t* request_len);

/**
 * validatehandshakeResponse (Client)
 */
uvhttp_error_t uvhttp_ws_verify_handshake_response(
    struct uvhttp_ws_connection* conn, const char* response,
    size_t response_len);

/**
 * receive WebSocket frame
 */
uvhttp_error_t uvhttp_ws_recv_frame(struct uvhttp_ws_connection* conn,
                                    uvhttp_ws_frame_t* frame);

/**
 * send WebSocket frame
 */
uvhttp_error_t uvhttp_ws_send_frame(uvhttp_context_t* context,
                                    struct uvhttp_ws_connection* conn,
                                    const uint8_t* data, size_t len,
                                    uvhttp_ws_opcode_t opcode);

/**
 * sendmessage
 */
uvhttp_error_t uvhttp_ws_send_text(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const char* text, size_t len);

/**
 * sendbinarymessage
 */
uvhttp_error_t uvhttp_ws_send_binary(uvhttp_context_t* context,
                                     struct uvhttp_ws_connection* conn,
                                     const uint8_t* data, size_t len);

/**
 * send Ping
 */
uvhttp_error_t uvhttp_ws_send_ping(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const uint8_t* data, size_t len);

/**
 * send Pong
 */
uvhttp_error_t uvhttp_ws_send_pong(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const uint8_t* data, size_t len);

/**
 * closeConnection
 */
uvhttp_error_t uvhttp_ws_close(uvhttp_context_t* context,
                               struct uvhttp_ws_connection* conn, int code,
                               const char* reason);

/**
 * handlereceiveto
 */
uvhttp_error_t uvhttp_ws_process_data(struct uvhttp_ws_connection* conn,
                                      const uint8_t* data, size_t len);

/**
 * setcallbackFunction
 */
void uvhttp_ws_set_callbacks(struct uvhttp_ws_connection* conn,
                             uvhttp_ws_on_message_callback on_message,
                             uvhttp_ws_on_close_callback on_close,
                             uvhttp_ws_on_error_callback on_error);

/* Frame processing functions */

/**
 * parsingframe header
 */
uvhttp_error_t uvhttp_ws_parse_frame_header(const uint8_t* data, size_t len,
                                            uvhttp_ws_frame_header_t* header,
                                            size_t* header_size);

/**
 * build frame
 */
uvhttp_error_t uvhttp_ws_build_frame(uvhttp_context_t* context, uint8_t* buffer,
                                     size_t buffer_size, const uint8_t* payload,
                                     size_t payload_len,
                                     uvhttp_ws_opcode_t opcode, int mask,
                                     int fin);

/**
 * apply mask
 */
void uvhttp_ws_apply_mask(uint8_t* data, size_t len,
                          const uint8_t* masking_key);

/**
 * generate Sec-WebSocket-Accept
 */
uvhttp_error_t uvhttp_ws_generate_accept(const char* key, char* accept,
                                         size_t accept_len);

/**
 * validate Sec-WebSocket-Accept
 */
uvhttp_error_t uvhttp_ws_verify_accept(const char* key, const char* accept);

/* Convenience macros */
#        define uvhttp_websocket_send_text(ctx, ws, text) \
            uvhttp_ws_send_text(ctx, ws, text, strlen(text))

#        define uvhttp_websocket_send_binary(ctx, ws, data, len) \
            uvhttp_ws_send_binary(ctx, ws, data, len)

#        ifdef __cplusplus
}
#        endif

#    endif /* UVHTTP_WEBSOCKET_H */
#endif     /* UVHTTP_FEATURE_WEBSOCKET */
