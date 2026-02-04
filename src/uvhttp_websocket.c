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

#if UVHTTP_FEATURE_PROTOCOL_UPGRADE
#    include "uvhttp_protocol_upgrade.h"
#endif

#include <errno.h>

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

    *header_size = 2;

    /* parse extended payload length */
    if (header->payload_len == 126) {
        if (len < 4) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        header->payload_len = (data[2] << 8) | data[3];
        *header_size = 4;
    } else if (header->payload_len == 127) {
        if (len < 10) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        header->payload_len =
            ((uint64_t)data[2] << 56) | ((uint64_t)data[3] << 48) |
            ((uint64_t)data[4] << 40) | ((uint64_t)data[5] << 32) |
            ((uint64_t)data[6] << 24) | ((uint64_t)data[7] << 16) |
            ((uint64_t)data[8] << 8) | (uint64_t)data[9];
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

/* build WebSocket frame */
uvhttp_error_t uvhttp_ws_build_frame(uvhttp_context_t* context, uint8_t* buffer,
                                     size_t buffer_size, const uint8_t* payload,
                                     size_t payload_len,
                                     uvhttp_ws_opcode_t opcode, int mask,
                                     int fin) {
    if (!buffer) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    size_t header_size = 2;
    size_t total_size = header_size + payload_len;

    if (mask) {
        total_size += 4;
    }

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
        header_size = 4;
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
        header_size = 10;
    }

    /* add masking key (if client) */
    if (mask) {
        uint8_t masking_key[4];
        if (uvhttp_ws_random_bytes(context, masking_key, 4) != 0) {
            uvhttp_free(buffer);
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

    return total_size;
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

    /* parserequest, get Sec-WebSocket-Key */
    const char* key_start = strstr(request, "Sec-WebSocket-Key:");
    if (!key_start) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    key_start += 19; /* skip "Sec-WebSocket-Key:" */

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
    strncpy(conn->client_key, (char*)base64_key, sizeof(conn->client_key) - 1);
    conn->client_key[sizeof(conn->client_key) - 1] = '\0';

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

    /* check Upgrade header */
    if (strstr(response, "Upgrade: websocket") == NULL) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* verify Sec-WebSocket-Accept */
    const char* accept_start = strstr(response, "Sec-WebSocket-Accept:");
    if (!accept_start) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    accept_start += 22; /* skip "Sec-WebSocket-Accept:" */

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

    /* allocatesendbuffer */
    size_t buffer_size =
        10 + len + 4; /* maximum frame header + payload + masking */
    uint8_t* buffer = uvhttp_alloc(buffer_size);
    if (!buffer) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* build frame (client needs masking) */
    int frame_len =
        uvhttp_ws_build_frame(context, buffer, buffer_size, data, len, opcode,
                              conn->is_server ? 0 : 1, 1);
    if (frame_len < 0) {
        uvhttp_free(buffer);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* senddata */
    int ret;
    if (conn->ssl) {
        ret = mbedtls_ssl_write(conn->ssl, buffer, frame_len);
    } else {
        ret = send(conn->fd, buffer, frame_len, 0);
    }

    uvhttp_free(buffer);

    if (ret < 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    conn->bytes_sent += ret;
    conn->frames_sent++;

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

    conn->state = UVHTTP_WS_STATE_CLOSING;

    /* buildcloseframe */
    uint8_t payload[128];
    payload[0] = (code >> 8) & 0xFF;
    payload[1] = code & 0xFF;

    if (reason) {
        size_t reason_len = strlen(reason);
        if (reason_len > 125) {
            reason_len = 125;
        }
        memcpy(payload + 2, reason, reason_len);

        return uvhttp_ws_send_frame(context, conn, payload, 2 + reason_len,
                                    UVHTTP_WS_OPCODE_CLOSE);
    }

    return uvhttp_ws_send_frame(context, conn, payload, 2,
                                UVHTTP_WS_OPCODE_CLOSE);
}

/* receive WebSocket frame */
uvhttp_error_t uvhttp_ws_recv_frame(struct uvhttp_ws_connection* conn,
                                    uvhttp_ws_frame_t* frame) {
    if (!conn || !frame) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(frame, 0, sizeof(uvhttp_ws_frame_t));

    /* read frame header */
    uint8_t header[10];
    int ret;

    if (conn->ssl) {
        ret = mbedtls_ssl_read(conn->ssl, header, 2);
    } else {
        ret = recv(conn->fd, header, 2, 0);
    }

    if (ret <= 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* parse frame header */
    size_t header_size;
    if (uvhttp_ws_parse_frame_header(header, ret, &frame->header,
                                     &header_size) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* read extended payload length (if any) */
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
    if (frame->header.payload_len > 0) {
        if (frame->header.payload_len > conn->config.max_frame_size) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        frame->payload = uvhttp_alloc(frame->header.payload_len);
        if (!frame->payload) {
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        frame->payload_size = frame->header.payload_len;

        if (conn->ssl) {
            ret = mbedtls_ssl_read(conn->ssl, frame->payload,
                                   frame->header.payload_len);
        } else {
            ret = recv(conn->fd, frame->payload, frame->header.payload_len, 0);
        }

        if (ret != (int)frame->header.payload_len) {
            uvhttp_free(frame->payload);
            frame->payload = NULL;
            return UVHTTP_ERROR_INVALID_PARAM;
        }

        /* application masking (if any) */
        if (frame->header.mask) {
            uvhttp_ws_apply_mask(frame->payload, frame->header.payload_len,
                                 frame->masking_key);
        }
    }

    conn->bytes_received += frame->header.payload_len;
    conn->frames_received++;

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
            break;
        }

        /* check if has enough data */
        size_t total_frame_size = header_size + header.payload_len;
        if (header.mask) {
            total_frame_size += 4;
        }

        if (conn->recv_buffer_pos < total_frame_size) {
            break;
        }

        /* extract payload */
        uint8_t* payload = NULL;

        if (header.payload_len > 0) {
            payload = conn->recv_buffer + header_size;

            if (header.mask) {
                uint8_t masking_key[4] = {0};
                memcpy(masking_key, conn->recv_buffer + header_size, 4);
                payload += 4;
                uvhttp_ws_apply_mask(payload, header.payload_len, masking_key);
            }
        }

        /* processframe */
        if (header.opcode == UVHTTP_WS_OPCODE_TEXT ||
            header.opcode == UVHTTP_WS_OPCODE_BINARY) {
            /* process fragmented message */
            if (!header.fin) {
                /* fragment start or continue */
                if (conn->fragmented_message == NULL) {
                    /* new fragmented message */
                    conn->fragmented_opcode = header.opcode;
                    conn->fragmented_capacity = header.payload_len * 2;
                    conn->fragmented_message =
                        uvhttp_alloc(conn->fragmented_capacity);
                    conn->fragmented_size = 0;
                }

                /* expand buffer (if needed) */
                while (conn->fragmented_size + header.payload_len >
                       conn->fragmented_capacity) {
                    conn->fragmented_capacity *= 2;
                    conn->fragmented_message = uvhttp_realloc(
                        conn->fragmented_message, conn->fragmented_capacity);
                }

                memcpy(conn->fragmented_message + conn->fragmented_size,
                       payload, header.payload_len);
                conn->fragmented_size += header.payload_len;
            } else {
                /* last fragment or complete message */
                if (conn->fragmented_message != NULL) {
                    /* complete fragmented message */
                    while (conn->fragmented_size + header.payload_len >
                           conn->fragmented_capacity) {
                        conn->fragmented_capacity *= 2;
                        conn->fragmented_message =
                            uvhttp_realloc(conn->fragmented_message,
                                           conn->fragmented_capacity);
                    }

                    memcpy(conn->fragmented_message + conn->fragmented_size,
                           payload, header.payload_len);
                    conn->fragmented_size += header.payload_len;

                    if (conn->on_message) {
                        conn->on_message(
                            conn, (const char*)conn->fragmented_message,
                            conn->fragmented_size, conn->fragmented_opcode);
                    }

                    uvhttp_free(conn->fragmented_message);
                    conn->fragmented_message = NULL;
                    conn->fragmented_size = 0;
                    conn->fragmented_capacity = 0;
                } else {
                    /* complete message */
                    if (conn->on_message) {
                        conn->on_message(conn, (const char*)payload,
                                         header.payload_len, header.opcode);
                    }
                }
            }
        } else if (header.opcode == UVHTTP_WS_OPCODE_CLOSE) {
            /* closeframe */
            if (conn->on_close) {
                int code = 1000;
                const char* reason = "";

                if (header.payload_len >= 2) {
                    code = (payload[0] << 8) | payload[1];
                    if (header.payload_len > 2) {
                        reason = (const char*)(payload + 2);
                    }
                }

                conn->on_close(conn, code, reason);
            }

            conn->state = UVHTTP_WS_STATE_CLOSED;
        } else if (header.opcode == UVHTTP_WS_OPCODE_PING) {
            /* automatically reply Pong */
            /* get wrapper from conn->user_data, then get conn, then get */
            /* server->context */
            typedef struct {
                void* conn;
                void* user_handler;
            } uvhttp_ws_wrapper_t;

            uvhttp_ws_wrapper_t* wrapper =
                (uvhttp_ws_wrapper_t*)conn->user_data;
            if (wrapper && wrapper->conn) {
                uvhttp_connection_t* http_conn =
                    (uvhttp_connection_t*)wrapper->conn;
                if (http_conn && http_conn->server &&
                    http_conn->server->context) {
                    uvhttp_ws_send_pong(http_conn->server->context, conn,
                                        payload, header.payload_len);
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

    /* Check Connection header (may contain multiple values) */
    if (strstr(connection_header, UVHTTP_HEADER_UPGRADE) == NULL) {
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

#if UVHTTP_FEATURE_PROTOCOL_UPGRADE
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
#endif
