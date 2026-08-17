/*
 * uvhttp WebSocket Native Implementation
 * Fully self-implemented WebSocket protocol support based on RFC 6455
 */

#include "uvhttp_websocket.h"

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_logging.h"
#include "uvhttp_platform.h"
#include "uvhttp_server.h"
#include "uvhttp_utils.h"

#include "uvhttp_protocol_upgrade.h"

#include <errno.h>
#include <time.h>

#if UVHTTP_FEATURE_TLS
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/sha1.h>
#else
#error "WebSocket requires TLS support (BUILD_WITH_HTTPS=ON)"
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Case-insensitive substring search. HTTP header names (and the Upgrade
 * token) are case-insensitive per RFC 7230 §3.2 — review M4. */
static const char* uvhttp_ws_strcasestr(const char* haystack,
                                        const char* needle) {
    if (!haystack || !needle || !*needle) {
        return NULL;
    }
    size_t nlen = strlen(needle);
    for (const char* p = haystack; *p; ++p) {
        if (strncasecmp(p, needle, nlen) == 0) {
            return p;
        }
    }
    return NULL;
}

/* WebSocket GUID (RFC 6455) */
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* Generate secure random bytes */
static int uvhttp_ws_random_bytes(uvhttp_context_t* context, unsigned char* buf,
                                  size_t len) {
    /* Use DRBG from context */
    if (context && context->ws_drbg_initialized) {
        return mbedtls_ctr_drbg_random(
            (mbedtls_ctr_drbg_context*)context->ws_drbg, buf, len);
    }

    /* DRBG not initialized, return error instead of using insecure
     * pseudo-random */
    UVHTTP_LOG_ERROR(
        "WebSocket DRBG not initialized, cannot generate secure random bytes");
    return UVHTTP_ERROR_INVALID_PARAM;
}

/* Create WebSocket connection */
struct uvhttp_ws_connection* uvhttp_ws_connection_create(
    int fd, mbedtls_ssl_context* ssl, int is_server,
    const uvhttp_config_t* config) {
    struct uvhttp_ws_connection* conn =
        uvhttp_calloc(1, sizeof(uvhttp_ws_connection_t));
    if (!conn) {
        return NULL;
    }

    conn->fd = fd;
    conn->ssl = ssl;
    conn->is_server = is_server;
    conn->state = UVHTTP_WS_STATE_CONNECTING;

    /* setconfig */
    if (config) {
        conn->config.max_frame_size = config->websocket_max_frame_size;
        conn->config.max_message_size = config->websocket_max_message_size;
        conn->config.ping_interval = config->websocket_ping_interval;
        conn->config.ping_timeout = config->websocket_ping_timeout;
    } else {
        /* use default config */
        conn->config.max_frame_size = UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE;
        conn->config.max_message_size =
            UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE;
        conn->config.ping_interval = UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL;
        conn->config.ping_timeout = UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT;
    }

    /* allocatereceivebuffer */
    conn->recv_buffer_size = UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE;
    conn->recv_buffer = uvhttp_alloc(conn->recv_buffer_size);
    if (!conn->recv_buffer) {
        uvhttp_free(conn);
        return NULL;
    }

    return conn;
}

/* release WebSocket connection */
void uvhttp_ws_connection_free(struct uvhttp_ws_connection* conn) {
    if (!conn) {
        return;
    }

    if (conn->recv_buffer) {
        uvhttp_free(conn->recv_buffer);
    }

    if (conn->send_buffer) {
        uvhttp_free(conn->send_buffer);
    }

    if (conn->fragmented_message) {
        uvhttp_free(conn->fragmented_message);
    }

    uvhttp_free(conn);
}

/* parse frame header */
uvhttp_error_t uvhttp_ws_parse_frame_header(const uint8_t* data, size_t len,
                                            uvhttp_ws_frame_header_t* header,
                                            size_t* header_size) {
    if (!data || !header || !header_size || len < 2) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(header, 0, sizeof(uvhttp_ws_frame_header_t));

    /* parse first byte */
    header->fin = (data[0] & 0x80) != 0;
    header->rsv1 = (data[0] & 0x40) != 0;
    header->rsv2 = (data[0] & 0x20) != 0;
    header->rsv3 = (data[0] & 0x10) != 0;
    header->opcode = data[0] & 0x0F;

    /* parse second byte */
    header->mask = (data[1] & 0x80) != 0;
    header->payload_len = data[1] & 0x7F;
    header->payload_length = header->payload_len;

    *header_size = 2;

    /* parse extended payload length. NOTE: payload_len is a 7-bit bitfield
     * (the raw length code), so extended lengths MUST go into
     * payload_length — writing them into payload_len silently truncates
     * (e.g. 200 -> 72). */
    if (header->payload_len == 126) {
        if (len < 4) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        header->payload_length = ((uint64_t)data[2] << 8) | data[3];
        *header_size = 4;
    } else if (header->payload_len == 127) {
        if (len < 10) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        header->payload_length =
            ((uint64_t)data[2] << 56) | ((uint64_t)data[3] << 48) |
            ((uint64_t)data[4] << 40) | ((uint64_t)data[5] << 32) |
            ((uint64_t)data[6] << 24) | ((uint64_t)data[7] << 16) |
            ((uint64_t)data[8] << 8) | (uint64_t)data[9];
        /* RFC 6455 §5.2: the most significant bit of a 64-bit length
         * (length code 127) MUST be 0; a value >= 2^63 is a protocol
         * error (review M3). */
        if (header->payload_length & ((uint64_t)1 << 63)) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        *header_size = 10;
    }

    return UVHTTP_OK;
}

/* application masking */
void uvhttp_ws_apply_mask(uint8_t* data, size_t len,
                          const uint8_t* masking_key) {
    if (!data || !masking_key) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        data[i] ^= masking_key[i % 4];
    }
}

/* build WebSocket frame
 * Returns the total wire size of the built frame on success (>= 0), or a
 * negative error code on failure. A separate return type (not
 * uvhttp_error_t) is used because the size can exceed INT_MAX on
 * 64-bit payloads — review L3. */
long uvhttp_ws_build_frame(uvhttp_context_t* context, uint8_t* buffer,
                           size_t buffer_size, const uint8_t* payload,
                           size_t payload_len, uvhttp_ws_opcode_t opcode,
                           int mask, int fin) {
    if (!buffer) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Determine real header size first so total_size matches the actual frame
     * (126/127 extended length bytes are part of the header). */
    size_t header_size;
    if (payload_len < 126) {
        header_size = 2;
    } else if (payload_len < 65536) {
        header_size = 4;
    } else {
        header_size = 10;
    }
    size_t total_size;
    if (payload_len > SIZE_MAX - header_size - (mask ? 4 : 0)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    total_size = header_size + payload_len + (mask ? 4 : 0);

    if (buffer_size < total_size) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* build first byte */
    buffer[0] = (fin ? 0x80 : 0x00) | (opcode & 0x0F);

    /* build second byte and extended payload length */
    if (payload_len < 126) {
        buffer[1] = (mask ? 0x80 : 0x00) | payload_len;
    } else if (payload_len < 65536) {
        buffer[1] = (mask ? 0x80 : 0x00) | 126;
        buffer[2] = (payload_len >> 8) & 0xFF;
        buffer[3] = payload_len & 0xFF;
    } else {
        /* use uint64_t to avoid shift warning on 32-bit systems */
        uint64_t len = (uint64_t)payload_len;
        buffer[1] = (mask ? 0x80 : 0x00) | 127;
        buffer[2] = (len >> 56) & 0xFF;
        buffer[3] = (len >> 48) & 0xFF;
        buffer[4] = (len >> 40) & 0xFF;
        buffer[5] = (len >> 32) & 0xFF;
        buffer[6] = (len >> 24) & 0xFF;
        buffer[7] = (len >> 16) & 0xFF;
        buffer[8] = (len >> 8) & 0xFF;
        buffer[9] = len & 0xFF;
    }

    /* add masking key (if client) */
    if (mask) {
        uint8_t masking_key[4];
        if (uvhttp_ws_random_bytes(context, masking_key, 4) != 0) {
            /* NOTE: do NOT free(buffer) here — the buffer is owned by the
             * caller (uvhttp_ws_send_frame), which frees it exactly once on
             * error. Freeing it here caused a double-free (see fix/ws-rfc-
             * compliance review S1). All other error paths in this function
             * also leave the buffer untouched. */
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        for (int i = 0; i < 4; i++) {
            buffer[header_size + i] = masking_key[i];
        }

        /* copy and mask payload */
        if (payload && payload_len > 0) {
            memcpy(buffer + header_size + 4, payload, payload_len);
            uvhttp_ws_apply_mask(buffer + header_size + 4, payload_len,
                                 masking_key);
        }
    } else {
        /* copy payload (no masking needed) */
        if (payload && payload_len > 0) {
            memcpy(buffer + header_size, payload, payload_len);
        }
    }

    return (long)total_size;
}

/* generate Sec-WebSocket-Accept */
uvhttp_error_t uvhttp_ws_generate_accept(const char* key, char* accept,
                                         size_t accept_len) {
    if (!key || !accept || accept_len < 32) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* build key + GUID */
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_GUID);

    /* calculate SHA-1 */
    unsigned char sha1[20];
    mbedtls_sha1((const unsigned char*)combined, strlen(combined), sha1);

    /* Base64 encoding */
    size_t olen;
    mbedtls_base64_encode((unsigned char*)accept, accept_len, &olen, sha1, 20);

    return UVHTTP_OK;
}

/* verify Sec-WebSocket-Accept */
uvhttp_error_t uvhttp_ws_verify_accept(const char* key, const char* accept) {
    if (!key || !accept) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    char expected[64];
    if (uvhttp_ws_generate_accept(key, expected, sizeof(expected)) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    return strcmp(expected, accept) == 0 ? 0 : -1;
}

/* server-side handshake */
uvhttp_error_t uvhttp_ws_handshake_server(struct uvhttp_ws_connection* conn,
                                          const char* request,
                                          size_t request_len, char* response,
                                          size_t* response_len) {
    if (!conn || !request || !response || !response_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    (void)request_len; /* parameter not used (calculated via strlen) */

    /* parserequest, get Sec-WebSocket-Key (header names are
     * case-insensitive per RFC 7230 §3.2 — review M4) */
    const char* key_start = uvhttp_ws_strcasestr(request, UVHTTP_HEADER_WEBSOCKET_KEY);
    if (!key_start) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* UVHTTP_HEADER_WEBSOCKET_KEY is "Sec-WebSocket-Key" without the
     * trailing colon; skip the colon itself so key_start points at the
     * header value (review M4). */
    key_start += strlen(UVHTTP_HEADER_WEBSOCKET_KEY) + 1;

    /* skip null whitespace */
    while (*key_start == ' ') {
        key_start++;
    }

    /* extract key */
    char key[64];
    size_t key_len = 0;
    while (key_start[key_len] != '\r' && key_start[key_len] != '\n' &&
           key_start[key_len] != '\0' && key_len < sizeof(key) - 1) {
        key[key_len] = key_start[key_len];
        key_len++;
    }
    key[key_len] = '\0';

    /* generate accept */
    char accept[64];
    if (uvhttp_ws_generate_accept(key, accept, sizeof(accept)) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* buildresponse */
    int len = snprintf(response, *response_len,
                       "HTTP/1.1 101 Switching Protocols\r\n"
                       "Upgrade: websocket\r\n"
                       "Connection: Upgrade\r\n"
                       "Sec-WebSocket-Accept: %s\r\n"
                       "\r\n",
                       accept);

    if (len < 0 || (size_t)len >= *response_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *response_len = len;
    conn->state = UVHTTP_WS_STATE_OPEN;

    return UVHTTP_OK;
}

/* clienthandshake */
uvhttp_error_t uvhttp_ws_handshake_client(uvhttp_context_t* context,
                                          struct uvhttp_ws_connection* conn,
                                          const char* host, const char* path,
                                          char* request, size_t* request_len) {
    if (!conn || !host || !path || !request || !request_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* generate random key */
    unsigned char raw_key[16];
    if (uvhttp_ws_random_bytes(context, raw_key, 16) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Base64 encoding key */
    unsigned char base64_key[32];
    size_t olen;
    mbedtls_base64_encode(base64_key, sizeof(base64_key), &olen, raw_key, 16);
    base64_key[olen] = '\0';

    /* save key to connection (for subsequent verification) */
    uvhttp_safe_strncpy(conn->client_key, (char*)base64_key, sizeof(conn->client_key));

    /* buildrequest */
    int len = snprintf(request, *request_len,
                       "GET %s HTTP/1.1\r\n"
                       "Host: %s\r\n"
                       "Upgrade: websocket\r\n"
                       "Connection: Upgrade\r\n"
                       "Sec-WebSocket-Key: %s\r\n"
                       "Sec-WebSocket-Version: 13\r\n"
                       "\r\n",
                       path, host, (char*)base64_key);

    if (len < 0 || (size_t)len >= *request_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *request_len = len;

    return UVHTTP_OK;
}

/* verifyhandshakeresponse */
uvhttp_error_t uvhttp_ws_verify_handshake_response(
    struct uvhttp_ws_connection* conn, const char* response,
    size_t response_len) {
    if (!conn || !response || !response_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check if is 101 response */
    if (strncmp(response, "HTTP/1.1 101", 12) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check Upgrade header (case-insensitive — review M4) */
    if (uvhttp_ws_strcasestr(response, "Upgrade: websocket") == NULL) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* verify Sec-WebSocket-Accept (header names are case-insensitive) */
    const char* accept_start =
        uvhttp_ws_strcasestr(response, UVHTTP_HEADER_WEBSOCKET_ACCEPT);
    if (!accept_start) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* UVHTTP_HEADER_WEBSOCKET_ACCEPT is "Sec-WebSocket-Accept" without the
     * trailing colon; skip the colon itself so accept_start points at the
     * header value (review M4). */
    accept_start += strlen(UVHTTP_HEADER_WEBSOCKET_ACCEPT) + 1;

    /* skip null whitespace */
    while (*accept_start == ' ') {
        accept_start++;
    }

    /* extract accept */
    char accept[64];
    size_t accept_len = 0;
    while (
        accept_start[accept_len] != '\r' && accept_start[accept_len] != '\n' &&
        accept_start[accept_len] != '\0' && accept_len < sizeof(accept) - 1) {
        accept[accept_len] = accept_start[accept_len];
        accept_len++;
    }
    accept[accept_len] = '\0';

    /* verify accept */
    if (conn->client_key[0] != '\0') {
        /* calculate expected accept value */
        unsigned char hash[20];
        char expected_accept[32];

        /* key + GUID */
        char combined[128];
        snprintf(combined, sizeof(combined), "%s%s", conn->client_key, WS_GUID);

        /* SHA1 hash */
        mbedtls_sha1((unsigned char*)combined, strlen(combined), hash);

        /* Base64 encoding */
        size_t olen;
        mbedtls_base64_encode((unsigned char*)expected_accept,
                              sizeof(expected_accept), &olen, hash,
                              sizeof(hash));
        expected_accept[olen] = '\0';

        /* compare */
        if (strcmp(accept, expected_accept) != 0) {
            /* Accept verifyfailure */
            return UVHTTP_ERROR_INVALID_PARAM;
        }
    }

    conn->state = UVHTTP_WS_STATE_OPEN;

    return UVHTTP_OK;
}

/* send WebSocket frame */
uvhttp_error_t uvhttp_ws_send_frame(uvhttp_context_t* context,
                                    struct uvhttp_ws_connection* conn,
                                    const uint8_t* data, size_t len,
                                    uvhttp_ws_opcode_t opcode) {
    if (!conn || conn->state != UVHTTP_WS_STATE_OPEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* allocatesendbuffer (guard the 10+len+4 sum against overflow) */
    if (len > SIZE_MAX - 14) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    size_t buffer_size =
        10 + len + 4; /* maximum frame header + payload + masking */
    uint8_t* buffer = uvhttp_alloc(buffer_size);
    if (!buffer) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* build frame (client needs masking) */
    long frame_len_long =
        uvhttp_ws_build_frame(context, buffer, buffer_size, data, len, opcode,
                              conn->is_server ? 0 : 1, 1);
    if (frame_len_long < 0) {
        uvhttp_free(buffer);
        return UVHTTP_ERROR_INVALID_PARAM;
    }
    /* frame_len fits in int here because buffer_size is capped by the
     * allocator and len is bounded by SIZE_MAX - 14; keep the arithmetic
     * below in size_t where possible. */
    size_t frame_len = (size_t)frame_len_long;

    /* senddata */
    int ret;
    if (conn->ssl) {
        /* mbedtls_ssl_write emits at most one TLS record (~16KB) per call;
         * loop until the whole frame is flushed. Without this, frames larger
         * than one record are truncated on the wire (clients stall waiting
         * for the missing tail). */
        size_t sent = 0;
        int want_retries = 0;
        while (sent < (size_t)frame_len) {
            ret = mbedtls_ssl_write(conn->ssl, buffer + sent,
                                    (size_t)frame_len - sent);
            if (ret > 0) {
                sent += (size_t)ret;
                want_retries = 0;
                continue;
            }
            if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
                ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                if (++want_retries > 100) {
                    break;
                }
                struct timespec ts;
                ts.tv_sec = 0;
                ts.tv_nsec = 1000000; /* 1ms */
                nanosleep(&ts, NULL);
                continue;
            }
            break;
        }
        if (sent == 0) {
            uvhttp_free(buffer);
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        ret = (int)sent;
    } else {
        ret = send(conn->fd, buffer, frame_len, 0);
        if (ret < 0) {
            uvhttp_free(buffer);
            return UVHTTP_ERROR_CONNECTION_BROKEN;
        }
    }

    conn->bytes_sent += ret;
    conn->frames_sent++;

    /* Release the send buffer on the success path too. Previously the
     * buffer leaked on every successful send (review S2); only the two
     * error paths freed it. */
    uvhttp_free(buffer);
    return UVHTTP_OK;
}

/* sendtextmessage */
uvhttp_error_t uvhttp_ws_send_text(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const char* text, size_t len) {
    return uvhttp_ws_send_frame(context, conn, (const uint8_t*)text, len,
                                UVHTTP_WS_OPCODE_TEXT);
}

/* sendbinarymessage */
uvhttp_error_t uvhttp_ws_send_binary(uvhttp_context_t* context,
                                     struct uvhttp_ws_connection* conn,
                                     const uint8_t* data, size_t len) {
    return uvhttp_ws_send_frame(context, conn, data, len,
                                UVHTTP_WS_OPCODE_BINARY);
}

/* send Ping */
uvhttp_error_t uvhttp_ws_send_ping(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const uint8_t* data, size_t len) {
    return uvhttp_ws_send_frame(context, conn, data, len,
                                UVHTTP_WS_OPCODE_PING);
}

/* send Pong */
uvhttp_error_t uvhttp_ws_send_pong(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const uint8_t* data, size_t len) {
    return uvhttp_ws_send_frame(context, conn, data, len,
                                UVHTTP_WS_OPCODE_PONG);
}

/* closeconnection */
uvhttp_error_t uvhttp_ws_close(uvhttp_context_t* context,
                               struct uvhttp_ws_connection* conn, int code,
                               const char* reason) {
    if (!conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* send_frame requires state == OPEN; move to CLOSING only after the
     * close frame has been handed to the send path (setting it first made
     * this function always fail with UVHTTP_ERROR_INVALID_PARAM) */

    /* buildcloseframe — use heap to avoid ASan false positive where the
     * conn parameter parameter is misidentified as underflowing a stack
     * array (ASan confuses the two due to stack frame reuse) */
    uint8_t* payload = (uint8_t*)uvhttp_alloc(128);
    if (!payload) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    payload[0] = (code >> 8) & 0xFF;
    payload[1] = code & 0xFF;

    uvhttp_error_t ret;
    if (reason) {
        size_t reason_len = strlen(reason);
        if (reason_len > 125) {
            reason_len = 125;
        }
        memcpy(payload + 2, reason, reason_len);

        ret = uvhttp_ws_send_frame(context, conn, payload, 2 + reason_len,
                                   UVHTTP_WS_OPCODE_CLOSE);
    } else {
        ret = uvhttp_ws_send_frame(context, conn, payload, 2,
                                   UVHTTP_WS_OPCODE_CLOSE);
    }

    conn->state = UVHTTP_WS_STATE_CLOSING;
    uvhttp_free(payload);
    return ret;
}

/* receive WebSocket frame */
uvhttp_error_t uvhttp_ws_recv_frame(struct uvhttp_ws_connection* conn,
                                    uvhttp_ws_frame_t* frame) {
    if (!conn || !frame) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(frame, 0, sizeof(uvhttp_ws_frame_t));

    /* read first two bytes of the header */
    uint8_t header[10];
    int ret;

    if (conn->ssl) {
        ret = mbedtls_ssl_read(conn->ssl, header, 2);
    } else {
        ret = recv(conn->fd, header, 2, 0);
    }

    if (ret != 2) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Determine the full header size from the length code before parsing:
     * parsing with only the first 2 bytes would reject 126/127 frames
     * outright (uvhttp_ws_parse_frame_header requires len >= 4/10). */
    size_t header_size = 2;
    uint8_t len_code = header[1] & 0x7F;
    if (len_code == 126) {
        header_size = 4;
    } else if (len_code == 127) {
        header_size = 10;
    }

    /* read the extended payload length, if any */
    if (header_size > 2) {
        if (conn->ssl) {
            ret = mbedtls_ssl_read(conn->ssl, header + 2, header_size - 2);
        } else {
            ret = recv(conn->fd, header + 2, header_size - 2, 0);
        }

        if (ret != (int)(header_size - 2)) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
    }

    /* parse frame header */
    if (uvhttp_ws_parse_frame_header(header, header_size, &frame->header,
                                     &header_size) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* read masking key (if any) */
    if (frame->header.mask) {
        if (conn->ssl) {
            ret = mbedtls_ssl_read(conn->ssl, frame->masking_key, 4);
        } else {
            ret = recv(conn->fd, frame->masking_key, 4, 0);
        }

        if (ret != 4) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
    }

    /* read payload */
    if (frame->header.payload_length > 0) {
        if (frame->header.payload_length > (uint64_t)conn->config.max_frame_size) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        frame->payload = uvhttp_alloc((size_t)frame->header.payload_length);
        if (!frame->payload) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        frame->payload_size = (size_t)frame->header.payload_length;

        if (conn->ssl) {
            ret = mbedtls_ssl_read(conn->ssl, frame->payload,
                                   frame->header.payload_length);
        } else {
            ret = recv(conn->fd, frame->payload, frame->header.payload_length,
                       0);
        }

        if (ret != (int)frame->header.payload_length) {
            uvhttp_free(frame->payload);
            frame->payload = NULL;
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        /* application masking (if any) */
        if (frame->header.mask) {
            uvhttp_ws_apply_mask(frame->payload, frame->header.payload_length,
                                 frame->masking_key);
        }
    }

    conn->bytes_received += frame->header.payload_length;
    conn->frames_received++;

    return UVHTTP_OK;
}

/* Append payload_len bytes to the in-progress fragmented message.
 * Enforces config.max_message_size on the accumulated size, guards the
 * doubling growth against size_t overflow, and checks allocation failures
 * (review S4: previously the cumulative size was unbounded, the doubling
 * could overflow, and realloc NULL results were memcpy'd into).
 * Returns UVHTTP_OK on success, UVHTTP_ERROR_INVALID_PARAM otherwise — the
 * caller must treat that as a fatal protocol error and close the connection. */
static uvhttp_error_t uvhttp_ws_fragment_append(
    struct uvhttp_ws_connection* conn, const uint8_t* payload,
    size_t payload_len) {
    /* max_message_size enforcement: the accumulated message must never
     * exceed the configured cap. */
    if ((size_t)conn->config.max_message_size > 0 &&
        (conn->fragmented_size > (size_t)conn->config.max_message_size ||
         payload_len > (size_t)conn->config.max_message_size -
                            conn->fragmented_size)) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Grow only when needed; double with overflow guards. */
    if (payload_len > conn->fragmented_capacity - conn->fragmented_size) {
        size_t required = conn->fragmented_size + payload_len;
        size_t new_capacity = conn->fragmented_capacity;
        if (new_capacity == 0) {
            /* first fragment: start at the exact required size, otherwise
             * doubling 0 stays 0 forever (infinite loop) */
            new_capacity = required;
        } else {
            while (new_capacity < required) {
                if (new_capacity > SIZE_MAX / 2) {
                    return UVHTTP_ERROR_INVALID_PARAM;
                }
                new_capacity *= 2;
            }
        }
        uint8_t* new_buf =
            uvhttp_realloc(conn->fragmented_message, new_capacity);
        if (!new_buf) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        conn->fragmented_message = new_buf;
        conn->fragmented_capacity = new_capacity;
    }

    memcpy(conn->fragmented_message + conn->fragmented_size, payload,
           payload_len);
    conn->fragmented_size += payload_len;
    return UVHTTP_OK;
}

/* process received data */
uvhttp_error_t uvhttp_ws_process_data(struct uvhttp_ws_connection* conn,
                                      const uint8_t* data, size_t len) {
    if (!conn || !data) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* add data to receive buffer */
    if (conn->recv_buffer_pos + len > conn->recv_buffer_size) {
        /* buffer insufficient, need to expand */
        size_t new_size = conn->recv_buffer_size;

        /* check if exceeds maximum limit */
        if (new_size > SIZE_MAX / 2) {
            return UVHTTP_ERROR_INVALID_PARAM; /* overflow protection */
        }

        new_size *= 2;

        /* check if meets requirement */
        while (conn->recv_buffer_pos + len > new_size) {
            if (new_size > SIZE_MAX / 2) {
                return UVHTTP_ERROR_INVALID_PARAM; /* overflow protection */
            }
            new_size *= 2;
        }

        /* check if exceeds config's maximum size */
        if (new_size > (size_t)conn->config.max_frame_size) {
            new_size = (size_t)conn->config.max_frame_size;
            if (conn->recv_buffer_pos + len > new_size) {
                return UVHTTP_ERROR_INVALID_PARAM; /* exceeds maximum limit */
            }
        }

        uint8_t* new_buffer = uvhttp_realloc(conn->recv_buffer, new_size);
        if (!new_buffer) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        conn->recv_buffer = new_buffer;
        conn->recv_buffer_size = new_size;
    }

    memcpy(conn->recv_buffer + conn->recv_buffer_pos, data, len);
    conn->recv_buffer_pos += len;

    /* process complete frame */
    while (conn->recv_buffer_pos >= 2) {
        uvhttp_ws_frame_header_t header;
        size_t header_size;

        if (uvhttp_ws_parse_frame_header(conn->recv_buffer,
                                         conn->recv_buffer_pos, &header,
                                         &header_size) != 0) {
            /* Distinguish "need more data" (partial header buffered) from a
             * genuine protocol error (e.g. the length-code-127 high bit being
             * set — review M3). With a partial header we must wait for the
             * rest of the frame; otherwise the frame is malformed and the
             * connection must be closed rather than silently skipping it. */
            uint8_t len_code = conn->recv_buffer[1] & 0x7F;
            size_t need =
                (len_code == 126) ? 4 : (len_code == 127) ? 10 : 2;
            if (conn->recv_buffer_pos < need) {
                break;
            }
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        /* RFC 6455 §5.2: when no extension is in use, a nonzero RSV bit is
         * a protocol error and the connection MUST be closed (review M1). */
        if (header.rsv1 || header.rsv2 || header.rsv3) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        /* RFC 6455 §5.5/§5.6: control frames (CLOSE/PING/PONG) MUST have a
         * payload <= 125 bytes and MUST NOT be fragmented (FIN must be set);
         * violations are protocol errors (review M2). */
        if (header.opcode >= UVHTTP_WS_OPCODE_CLOSE) {
            if (header.payload_length > 125 || !header.fin) {
                return UVHTTP_ERROR_INVALID_PARAM;
            }
        }

        /* RFC 6455 §5.1: client-to-server frames MUST be masked; a server
         * receiving an unmasked frame MUST close the connection. */
        if (conn->is_server && !header.mask) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        /* Reject frames whose declared payload exceeds the configured limit.
         * Without this, a 64-bit (length code 127) wire length such as
         * 2^64-1 makes header_size + payload_length wrap around size_t, the
         * "enough data" check below passes with only the header buffered,
         * and uvhttp_ws_apply_mask runs an out-of-bounds write. */
        if (header.payload_length > (uint64_t)conn->config.max_frame_size) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        /* check if has enough data (payload_length is bounded by
         * max_frame_size above, so this sum cannot overflow) */
        size_t total_frame_size = header_size + (size_t)header.payload_length;
        if (header.mask) {
            total_frame_size += 4;
        }

        if (conn->recv_buffer_pos < total_frame_size) {
            break;
        }

        /* extract payload */
        uint8_t* payload = NULL;

        if (header.payload_length > 0) {
            payload = conn->recv_buffer + header_size;

            if (header.mask) {
                uint8_t masking_key[4] = {0};
                memcpy(masking_key, conn->recv_buffer + header_size, 4);
                payload += 4;
                uvhttp_ws_apply_mask(payload, header.payload_length,
                                     masking_key);
            }
        }

        /* processframe */
        if (header.opcode == UVHTTP_WS_OPCODE_TEXT ||
            header.opcode == UVHTTP_WS_OPCODE_BINARY ||
            header.opcode == UVHTTP_WS_OPCODE_CONTINUATION) {
            /* RFC 6455 §5.4 fragment state machine (review S3: CONTINUATION
             * frames used to be silently dropped, so multi-frame messages
             * never completed):
             * - fresh TEXT/BINARY with FIN=1 and no pending fragment is a
             *   complete message;
             * - fresh TEXT/BINARY with FIN=0 starts a fragmented message;
             * - CONTINUATION frames extend the pending message; FIN=1
             *   completes it;
             * - CONTINUATION with no pending message, or a fresh TEXT/BINARY
             *   while a fragment is pending, is a protocol error (§5.6) and
             *   the connection must be closed. */
            if (conn->fragmented_message == NULL) {
                if (header.opcode == UVHTTP_WS_OPCODE_CONTINUATION) {
                    /* continuation with no initial fragment — protocol
                     * violation */
                    return UVHTTP_ERROR_INVALID_PARAM;
                }
                if (!header.fin) {
                    /* start a new fragmented message */
                    conn->fragmented_opcode = header.opcode;
                    conn->fragmented_size = 0;
                    conn->fragmented_capacity = 0;
                    conn->fragmented_message = NULL;
                    if (uvhttp_ws_fragment_append(
                            conn, payload,
                            (size_t)header.payload_length) != UVHTTP_OK) {
                        return UVHTTP_ERROR_INVALID_PARAM;
                    }
                } else {
                    /* complete message */
                    if (conn->on_message) {
                        conn->on_message(conn, (const char*)payload,
                                         (size_t)header.payload_length,
                                         header.opcode);
                    }
                }
            } else {
                /* a fragmented message is in progress; only CONTINUATION
                 * frames may extend it */
                if (header.opcode != UVHTTP_WS_OPCODE_CONTINUATION) {
                    /* a fresh data frame interrupting a fragmented message
                     * — protocol violation */
                    return UVHTTP_ERROR_INVALID_PARAM;
                }
                if (uvhttp_ws_fragment_append(
                        conn, payload,
                        (size_t)header.payload_length) != UVHTTP_OK) {
                    return UVHTTP_ERROR_INVALID_PARAM;
                }
                if (header.fin) {
                    /* message complete — deliver and reset */
                    if (conn->on_message) {
                        conn->on_message(
                            conn, (const char*)conn->fragmented_message,
                            conn->fragmented_size,
                            conn->fragmented_opcode);
                    }
                    uvhttp_free(conn->fragmented_message);
                    conn->fragmented_message = NULL;
                    conn->fragmented_size = 0;
                    conn->fragmented_capacity = 0;
                }
            }
        } else if (header.opcode == UVHTTP_WS_OPCODE_CLOSE) {
            /* closeframe */
            int close_code = 1000;
            const char* close_reason = "";

            if (header.payload_length >= 2) {
                close_code = (payload[0] << 8) | payload[1];
                if (header.payload_length > 2) {
                    close_reason = (const char*)(payload + 2);
                }
            }

            if (conn->on_close) {
                conn->on_close(conn, close_code, close_reason);
            }

            /* Echo a close frame back (RFC 6455 §5.5.1) so the peer is not
             * left waiting for the close handshake; best effort, only when
             * the connection is wired to a server context. */
            {
                uvhttp_ws_wrapper_t* wrapper =
                    (uvhttp_ws_wrapper_t*)conn->user_data;
                if (wrapper && wrapper->conn) {
                    uvhttp_connection_t* http_conn = wrapper->conn;
                    if (http_conn && http_conn->server &&
                        http_conn->server->context) {
                        uint8_t close_payload[2 + 125];
                        size_t close_len = 0;
                        if (header.payload_length >= 2) {
                            close_payload[0] = payload[0];
                            close_payload[1] = payload[1];
                            close_len = 2;
                            size_t reason_len =
                                (size_t)header.payload_length - 2;
                            if (reason_len > 125) {
                                reason_len = 125;
                            }
                            if (reason_len > 0) {
                                memcpy(close_payload + 2, payload + 2,
                                       reason_len);
                                close_len = 2 + reason_len;
                            }
                        }
                        uvhttp_ws_send_frame(http_conn->server->context, conn,
                                             close_payload, close_len,
                                             UVHTTP_WS_OPCODE_CLOSE);
                    }
                }
            }

            conn->state = UVHTTP_WS_STATE_CLOSED;
        } else if (header.opcode == UVHTTP_WS_OPCODE_PING) {
            /* automatically reply Pong */
            /* get wrapper from conn->user_data, then get conn, then get */
            /* server->context */
            uvhttp_ws_wrapper_t* wrapper =
                (uvhttp_ws_wrapper_t*)conn->user_data;
            if (wrapper && wrapper->conn) {
                uvhttp_connection_t* http_conn = wrapper->conn;
                if (http_conn && http_conn->server &&
                    http_conn->server->context) {
                    uvhttp_ws_send_pong(http_conn->server->context, conn,
                                        payload, header.payload_length);
                }
            }
        }
        /* PONG frame usually does not need special processing */

        /* remove processed frame from buffer */
        size_t remaining = conn->recv_buffer_pos - total_frame_size;
        if (remaining > 0) {
            memmove(conn->recv_buffer, conn->recv_buffer + total_frame_size,
                    remaining);
        }
        conn->recv_buffer_pos = remaining;
    }

    return UVHTTP_OK;
}

/* setcallbackfunction */
void uvhttp_ws_set_callbacks(struct uvhttp_ws_connection* conn,
                             uvhttp_ws_on_message_callback on_message,
                             uvhttp_ws_on_close_callback on_close,
                             uvhttp_ws_on_error_callback on_error) {
    if (!conn) {
        return;
    }

    conn->on_message = on_message;
    conn->on_close = on_close;
    conn->on_error = on_error;
}

/* triggermessagecallback */
__attribute__((unused)) static void uvhttp_ws_trigger_message_callback(
    struct uvhttp_ws_connection* conn, const uint8_t* data, size_t len,
    uvhttp_ws_opcode_t opcode) {
    if (conn && conn->on_message) {
        conn->on_message(conn, (const char*)data, len, opcode);
    }
}

/* triggerclosecallback */
__attribute__((unused)) static void uvhttp_ws_trigger_close_callback(
    struct uvhttp_ws_connection* conn, int code, const char* reason) {
    if (conn && conn->on_close) {
        conn->on_close(conn, code, reason);
    }
}

/* triggererrorcallback */
__attribute__((unused)) static void uvhttp_ws_trigger_error_callback(
    struct uvhttp_ws_connection* conn, int error_code, const char* error_msg) {
    if (conn && conn->on_error) {
        conn->on_error(conn, error_code, error_msg);
    }
}

/* ========== Protocol upgrade framework integration ========== */

/**
 * @brief WebSocket protocol detector
 */
static int websocket_protocol_detector(uvhttp_request_t* request,
                                       char* protocol_name,
                                       size_t protocol_name_len,
                                       const char* upgrade_header,
                                       const char* connection_header) {
    (void)upgrade_header; /* Use pre-fetched value below */

    /* Check required headers */
    if (!upgrade_header || !connection_header) {
        return 0;
    }

    const char* ws_key =
        uvhttp_request_get_header(request, UVHTTP_HEADER_WEBSOCKET_KEY);
    if (!ws_key) {
        return 0;
    }

    /* Check Upgrade header (case-insensitive) */
    if (strcasecmp(upgrade_header, UVHTTP_VALUE_WEBSOCKET) != 0) {
        return 0;
    }

    /* Check Connection header (may contain multiple values); header names
     * and the token are case-insensitive — review M4 */
    if (uvhttp_ws_strcasestr(connection_header, "upgrade") == NULL) {
        return 0;
    }

    /* WebSocket protocol detected */
    strncpy(protocol_name, "websocket", protocol_name_len);
    return 1;
}

/**
 * @brief WebSocket upgrade handler
 */
static uvhttp_error_t websocket_upgrade_handler(uvhttp_connection_t* conn,
                                                const char* protocol_name,
                                                void* user_data) {
    (void)protocol_name; /* Unused parameter */
    (void)user_data;     /* Unused parameter */

    const char* ws_key =
        uvhttp_request_get_header(conn->request, UVHTTP_HEADER_WEBSOCKET_KEY);
    if (!ws_key) {
        uvhttp_response_set_status(conn->response, 400);
        uvhttp_response_set_header(conn->response, UVHTTP_HEADER_CONTENT_TYPE,
                                   UVHTTP_CONTENT_TYPE_TEXT);
        uvhttp_response_set_body(conn->response, UVHTTP_MESSAGE_WS_KEY_MISSING,
                                 strlen(UVHTTP_MESSAGE_WS_KEY_MISSING));
        uvhttp_response_send(conn->response);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Send 101 Switching Protocols response */
    uvhttp_response_set_status(conn->response, 101);
    uvhttp_response_set_header(conn->response, UVHTTP_HEADER_UPGRADE,
                               UVHTTP_VALUE_WEBSOCKET);
    uvhttp_response_set_header(conn->response, UVHTTP_HEADER_CONNECTION,
                               UVHTTP_HEADER_UPGRADE);

    /* Generate and set Sec-WebSocket-Accept header */
    char accept[64];
    if (uvhttp_ws_generate_accept(ws_key, accept, sizeof(accept)) != 0) {
        uvhttp_response_set_status(conn->response, 500);
        uvhttp_response_set_header(conn->response, UVHTTP_HEADER_CONTENT_TYPE,
                                   UVHTTP_CONTENT_TYPE_TEXT);
        uvhttp_response_set_body(conn->response,
                                 UVHTTP_MESSAGE_WS_HANDSHAKE_FAILED,
                                 strlen(UVHTTP_MESSAGE_WS_HANDSHAKE_FAILED));
        uvhttp_response_send(conn->response);
        return UVHTTP_ERROR_IO_ERROR;
    }

    uvhttp_response_set_header(conn->response, UVHTTP_HEADER_WEBSOCKET_ACCEPT,
                               accept);
    uvhttp_response_send(conn->response);

    /* Call WebSocket handshake handling */
    int ws_result = uvhttp_connection_handle_websocket_handshake(conn, ws_key);
    if (ws_result != 0) {
        UVHTTP_LOG_ERROR("Failed to handle WebSocket handshake: %d\n",
                         ws_result);
        uvhttp_connection_close(conn);
        return UVHTTP_ERROR_CONNECTION_INIT;
    }

    return UVHTTP_OK;
}

/**
 * @brief Register WebSocket protocol upgrade
 *
 * This function should be called after server creation to enable
 * WebSocket protocol upgrade support
 */
uvhttp_error_t uvhttp_server_register_websocket_upgrade(
    uvhttp_server_t* server) {
    if (!server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    return uvhttp_server_register_protocol_upgrade(
        server, "websocket", UVHTTP_VALUE_WEBSOCKET,
        websocket_protocol_detector, websocket_upgrade_handler, NULL);
}
