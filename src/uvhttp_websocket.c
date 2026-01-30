/*
 * uvhttp WebSocket Native Implementation
 * 完全自主实现的 WebSocket 协议支持，基于 RFC 6455
 */

#include "uvhttp_websocket.h"

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_platform.h"
#include "uvhttp_server.h"

#include <errno.h>
#include <mbedtls/base64.h>
#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/sha1.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* WebSocket GUID (RFC 6455) */
#define WS_GUID "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"

/* 生成安全的随机数 */
static int uvhttp_ws_random_bytes(uvhttp_context_t* context, unsigned char* buf,
                                  size_t len) {
    /* 使用上下文中的 DRBG */
    if (context && context->ws_drbg_initialized) {
        return mbedtls_ctr_drbg_random(
            (mbedtls_ctr_drbg_context*)context->ws_drbg, buf, len);
    }

    /* DRBG 未初始化，返回错误而不是使用不安全的伪随机数 */
    UVHTTP_LOG_ERROR(
        "WebSocket DRBG not initialized, cannot generate secure random bytes");
    return UVHTTP_ERROR_INVALID_PARAM;
}

/* 创建 WebSocket 连接 */
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

    /* 设置配置 */
    if (config) {
        conn->config.max_frame_size = config->websocket_max_frame_size;
        conn->config.max_message_size = config->websocket_max_message_size;
        conn->config.ping_interval = config->websocket_ping_interval;
        conn->config.ping_timeout = config->websocket_ping_timeout;
    } else {
        /* 使用默认配置 */
        conn->config.max_frame_size = UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE;
        conn->config.max_message_size = UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE;
        conn->config.ping_interval = UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL;
        conn->config.ping_timeout = UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT;
    }

    /* 分配接收缓冲区 */
    conn->recv_buffer_size = UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE;
    conn->recv_buffer = uvhttp_alloc(conn->recv_buffer_size);
    if (!conn->recv_buffer) {
        uvhttp_free(conn);
        return NULL;
    }

    return conn;
}

/* 释放 WebSocket 连接 */
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

/* 解析帧头 */
uvhttp_error_t uvhttp_ws_parse_frame_header(const uint8_t* data, size_t len,
                                            uvhttp_ws_frame_header_t* header,
                                            size_t* header_size) {
    if (!data || !header || !header_size || len < 2) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(header, 0, sizeof(uvhttp_ws_frame_header_t));

    /* 解析第一个字节 */
    header->fin = (data[0] & 0x80) != 0;
    header->rsv1 = (data[0] & 0x40) != 0;
    header->rsv2 = (data[0] & 0x20) != 0;
    header->rsv3 = (data[0] & 0x10) != 0;
    header->opcode = data[0] & 0x0F;

    /* 解析第二个字节 */
    header->mask = (data[1] & 0x80) != 0;
    header->payload_len = data[1] & 0x7F;

    *header_size = 2;

    /* 解析扩展载荷长度 */
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

/* 应用掩码 */
void uvhttp_ws_apply_mask(uint8_t* data, size_t len,
                          const uint8_t* masking_key) {
    if (!data || !masking_key) {
        return;
    }

    for (size_t i = 0; i < len; i++) {
        data[i] ^= masking_key[i % 4];
    }
}

/* 构建 WebSocket 帧 */
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

    /* 构建第一个字节 */
    buffer[0] = (fin ? 0x80 : 0x00) | (opcode & 0x0F);

    /* 构建第二个字节和扩展载荷长度 */
    if (payload_len < 126) {
        buffer[1] = (mask ? 0x80 : 0x00) | payload_len;
    } else if (payload_len < 65536) {
        buffer[1] = (mask ? 0x80 : 0x00) | 126;
        buffer[2] = (payload_len >> 8) & 0xFF;
        buffer[3] = payload_len & 0xFF;
        header_size = 4;
    } else {
        buffer[1] = (mask ? 0x80 : 0x00) | 127;
        buffer[2] = (payload_len >> 56) & 0xFF;
        buffer[3] = (payload_len >> 48) & 0xFF;
        buffer[4] = (payload_len >> 40) & 0xFF;
        buffer[5] = (payload_len >> 32) & 0xFF;
        buffer[6] = (payload_len >> 24) & 0xFF;
        buffer[7] = (payload_len >> 16) & 0xFF;
        buffer[8] = (payload_len >> 8) & 0xFF;
        buffer[9] = payload_len & 0xFF;
        header_size = 10;
    }

    /* 添加掩码密钥（如果是客户端） */
    if (mask) {
        uint8_t masking_key[4];
        if (uvhttp_ws_random_bytes(context, masking_key, 4) != 0) {
            uvhttp_free(buffer);
            return UVHTTP_ERROR_INVALID_PARAM;
        }
        for (int i = 0; i < 4; i++) {
            buffer[header_size + i] = masking_key[i];
        }

        /* 复制并掩码载荷 */
        if (payload && payload_len > 0) {
            memcpy(buffer + header_size + 4, payload, payload_len);
            uvhttp_ws_apply_mask(buffer + header_size + 4, payload_len,
                                 masking_key);
        }
    } else {
        /* 复制载荷（无需掩码） */
        if (payload && payload_len > 0) {
            memcpy(buffer + header_size, payload, payload_len);
        }
    }

    return total_size;
}

/* 生成 Sec-WebSocket-Accept */
uvhttp_error_t uvhttp_ws_generate_accept(const char* key, char* accept,
                                         size_t accept_len) {
    if (!key || !accept || accept_len < 32) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 构建 key + GUID */
    char combined[256];
    snprintf(combined, sizeof(combined), "%s%s", key, WS_GUID);

    /* 计算 SHA-1 */
    unsigned char sha1[20];
    mbedtls_sha1((const unsigned char*)combined, strlen(combined), sha1);

    /* Base64 编码 */
    size_t olen;
    mbedtls_base64_encode((unsigned char*)accept, accept_len, &olen, sha1, 20);

    return UVHTTP_OK;
}

/* 验证 Sec-WebSocket-Accept */
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

/* 服务器端握手 */
uvhttp_error_t uvhttp_ws_handshake_server(struct uvhttp_ws_connection* conn,
                                          const char* request,
                                          size_t request_len, char* response,
                                          size_t* response_len) {
    if (!conn || !request || !response || !response_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    (void)request_len; /* 参数未使用（通过 strlen 计算） */

    /* 解析请求，获取 Sec-WebSocket-Key */
    const char* key_start = strstr(request, "Sec-WebSocket-Key:");
    if (!key_start) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    key_start += 19; /* 跳过 "Sec-WebSocket-Key:" */

    /* 跳过空白字符 */
    while (*key_start == ' ') {
        key_start++;
    }

    /* 提取 key */
    char key[64];
    size_t key_len = 0;
    while (key_start[key_len] != '\r' && key_start[key_len] != '\n' &&
           key_start[key_len] != '\0' && key_len < sizeof(key) - 1) {
        key[key_len] = key_start[key_len];
        key_len++;
    }
    key[key_len] = '\0';

    /* 生成 accept */
    char accept[64];
    if (uvhttp_ws_generate_accept(key, accept, sizeof(accept)) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 构建响应 */
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

/* 客户端握手 */
uvhttp_error_t uvhttp_ws_handshake_client(uvhttp_context_t* context,
                                          struct uvhttp_ws_connection* conn,
                                          const char* host, const char* path,
                                          char* request, size_t* request_len) {
    if (!conn || !host || !path || !request || !request_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 生成随机 key */
    unsigned char raw_key[16];
    if (uvhttp_ws_random_bytes(context, raw_key, 16) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Base64 编码 key */
    unsigned char base64_key[32];
    size_t olen;
    mbedtls_base64_encode(base64_key, sizeof(base64_key), &olen, raw_key, 16);
    base64_key[olen] = '\0';

    /* 保存 key 到连接（用于后续验证） */
    strncpy(conn->client_key, (char*)base64_key, sizeof(conn->client_key) - 1);
    conn->client_key[sizeof(conn->client_key) - 1] = '\0';

    /* 构建请求 */
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

/* 验证握手响应 */
uvhttp_error_t uvhttp_ws_verify_handshake_response(
    struct uvhttp_ws_connection* conn, const char* response,
    size_t response_len) {
    if (!conn || !response || !response_len) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查是否是 101 响应 */
    if (strncmp(response, "HTTP/1.1 101", 12) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 检查 Upgrade 头 */
    if (strstr(response, "Upgrade: websocket") == NULL) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 验证 Sec-WebSocket-Accept */
    const char* accept_start = strstr(response, "Sec-WebSocket-Accept:");
    if (!accept_start) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    accept_start += 22; /* 跳过 "Sec-WebSocket-Accept:" */

    /* 跳过空白字符 */
    while (*accept_start == ' ') {
        accept_start++;
    }

    /* 提取 accept */
    char accept[64];
    size_t accept_len = 0;
    while (
        accept_start[accept_len] != '\r' && accept_start[accept_len] != '\n' &&
        accept_start[accept_len] != '\0' && accept_len < sizeof(accept) - 1) {
        accept[accept_len] = accept_start[accept_len];
        accept_len++;
    }
    accept[accept_len] = '\0';

    /* 验证 accept */
    if (conn->client_key[0] != '\0') {
        /* 计算期望的 accept 值 */
        unsigned char hash[20];
        char expected_accept[32];

        /* key + GUID */
        char combined[128];
        snprintf(combined, sizeof(combined), "%s%s", conn->client_key, WS_GUID);

        /* SHA1 哈希 */
        mbedtls_sha1((unsigned char*)combined, strlen(combined), hash);

        /* Base64 编码 */
        size_t olen;
        mbedtls_base64_encode((unsigned char*)expected_accept,
                              sizeof(expected_accept), &olen, hash,
                              sizeof(hash));
        expected_accept[olen] = '\0';

        /* 比较 */
        if (strcmp(accept, expected_accept) != 0) {
            /* Accept 验证失败 */
            return UVHTTP_ERROR_INVALID_PARAM;
        }
    }

    conn->state = UVHTTP_WS_STATE_OPEN;

    return UVHTTP_OK;
}

/* 发送 WebSocket 帧 */
uvhttp_error_t uvhttp_ws_send_frame(uvhttp_context_t* context,
                                    struct uvhttp_ws_connection* conn,
                                    const uint8_t* data, size_t len,
                                    uvhttp_ws_opcode_t opcode) {
    if (!conn || conn->state != UVHTTP_WS_STATE_OPEN) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 分配发送缓冲区 */
    size_t buffer_size = 10 + len + 4; /* 最大帧头 + 载荷 + 掩码 */
    uint8_t* buffer = uvhttp_alloc(buffer_size);
    if (!buffer) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 构建帧（客户端需要掩码） */
    int frame_len =
        uvhttp_ws_build_frame(context, buffer, buffer_size, data, len, opcode,
                              conn->is_server ? 0 : 1, 1);
    if (frame_len < 0) {
        uvhttp_free(buffer);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 发送数据 */
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

/* 发送文本消息 */
uvhttp_error_t uvhttp_ws_send_text(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const char* text, size_t len) {
    return uvhttp_ws_send_frame(context, conn, (const uint8_t*)text, len,
                                UVHTTP_WS_OPCODE_TEXT);
}

/* 发送二进制消息 */
uvhttp_error_t uvhttp_ws_send_binary(uvhttp_context_t* context,
                                     struct uvhttp_ws_connection* conn,
                                     const uint8_t* data, size_t len) {
    return uvhttp_ws_send_frame(context, conn, data, len,
                                UVHTTP_WS_OPCODE_BINARY);
}

/* 发送 Ping */
uvhttp_error_t uvhttp_ws_send_ping(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const uint8_t* data, size_t len) {
    return uvhttp_ws_send_frame(context, conn, data, len,
                                UVHTTP_WS_OPCODE_PING);
}

/* 发送 Pong */
uvhttp_error_t uvhttp_ws_send_pong(uvhttp_context_t* context,
                                   struct uvhttp_ws_connection* conn,
                                   const uint8_t* data, size_t len) {
    return uvhttp_ws_send_frame(context, conn, data, len,
                                UVHTTP_WS_OPCODE_PONG);
}

/* 关闭连接 */
uvhttp_error_t uvhttp_ws_close(uvhttp_context_t* context,
                               struct uvhttp_ws_connection* conn, int code,
                               const char* reason) {
    if (!conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    conn->state = UVHTTP_WS_STATE_CLOSING;

    /* 构建关闭帧 */
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

/* 接收 WebSocket 帧 */
uvhttp_error_t uvhttp_ws_recv_frame(struct uvhttp_ws_connection* conn,
                                    uvhttp_ws_frame_t* frame) {
    if (!conn || !frame) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    memset(frame, 0, sizeof(uvhttp_ws_frame_t));

    /* 读取帧头 */
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

    /* 解析帧头 */
    size_t header_size;
    if (uvhttp_ws_parse_frame_header(header, ret, &frame->header,
                                     &header_size) != 0) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 读取扩展载荷长度（如果有） */
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

    /* 读取掩码密钥（如果有） */
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

    /* 读取载荷 */
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

        /* 应用掩码（如果有） */
        if (frame->header.mask) {
            uvhttp_ws_apply_mask(frame->payload, frame->header.payload_len,
                                 frame->masking_key);
        }
    }

    conn->bytes_received += frame->header.payload_len;
    conn->frames_received++;

    return UVHTTP_OK;
}

/* 处理接收到的数据 */
uvhttp_error_t uvhttp_ws_process_data(struct uvhttp_ws_connection* conn,
                                      const uint8_t* data, size_t len) {
    if (!conn || !data) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* 将数据添加到接收缓冲区 */
    if (conn->recv_buffer_pos + len > conn->recv_buffer_size) {
        /* 缓冲区不足，需要扩展 */
        size_t new_size = conn->recv_buffer_size;

        /* 检查是否超过最大限制 */
        if (new_size > SIZE_MAX / 2) {
            return UVHTTP_ERROR_INVALID_PARAM; /* 溢出保护 */
        }

        new_size *= 2;

        /* 检查是否满足需求 */
        while (conn->recv_buffer_pos + len > new_size) {
            if (new_size > SIZE_MAX / 2) {
                return UVHTTP_ERROR_INVALID_PARAM; /* 溢出保护 */
            }
            new_size *= 2;
        }

        /* 检查是否超过配置的最大大小 */
        if (new_size > (size_t)conn->config.max_frame_size) {
            new_size = (size_t)conn->config.max_frame_size;
            if (conn->recv_buffer_pos + len > new_size) {
                return UVHTTP_ERROR_INVALID_PARAM; /* 超过最大限制 */
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

    /* 处理完整的帧 */
    while (conn->recv_buffer_pos >= 2) {
        uvhttp_ws_frame_header_t header;
        size_t header_size;

        if (uvhttp_ws_parse_frame_header(conn->recv_buffer,
                                         conn->recv_buffer_pos, &header,
                                         &header_size) != 0) {
            break;
        }

        /* 检查是否有足够的数据 */
        size_t total_frame_size = header_size + header.payload_len;
        if (header.mask) {
            total_frame_size += 4;
        }

        if (conn->recv_buffer_pos < total_frame_size) {
            break;
        }

        /* 提取载荷 */
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

        /* 处理帧 */
        if (header.opcode == UVHTTP_WS_OPCODE_TEXT ||
            header.opcode == UVHTTP_WS_OPCODE_BINARY) {
            /* 处理分片消息 */
            if (!header.fin) {
                /* 分片开始或继续 */
                if (conn->fragmented_message == NULL) {
                    /* 新的分片消息 */
                    conn->fragmented_opcode = header.opcode;
                    conn->fragmented_capacity = header.payload_len * 2;
                    conn->fragmented_message =
                        uvhttp_alloc(conn->fragmented_capacity);
                    conn->fragmented_size = 0;
                }

                /* 扩展缓冲区（如果需要） */
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
                /* 最后一个分片或完整消息 */
                if (conn->fragmented_message != NULL) {
                    /* 完成分片消息 */
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
                    /* 完整消息 */
                    if (conn->on_message) {
                        conn->on_message(conn, (const char*)payload,
                                         header.payload_len, header.opcode);
                    }
                }
            }
        } else if (header.opcode == UVHTTP_WS_OPCODE_CLOSE) {
            /* 关闭帧 */
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
            /* 自动回复 Pong */
            /* 从 conn->user_data 获取 wrapper，然后获取 conn，再获取
             * server->context */
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
        /* PONG 帧通常不需要特殊处理 */

        /* 从缓冲区移除已处理的帧 */
        size_t remaining = conn->recv_buffer_pos - total_frame_size;
        if (remaining > 0) {
            memmove(conn->recv_buffer, conn->recv_buffer + total_frame_size,
                    remaining);
        }
        conn->recv_buffer_pos = remaining;
    }

    return UVHTTP_OK;
}

/* 设置回调函数 */
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

/* 触发消息回调 */
__attribute__((unused)) static void uvhttp_ws_trigger_message_callback(
    struct uvhttp_ws_connection* conn, const uint8_t* data, size_t len,
    uvhttp_ws_opcode_t opcode) {
    if (conn && conn->on_message) {
        conn->on_message(conn, (const char*)data, len, opcode);
    }
}

/* 触发关闭回调 */
__attribute__((unused)) static void uvhttp_ws_trigger_close_callback(
    struct uvhttp_ws_connection* conn, int code, const char* reason) {
    if (conn && conn->on_close) {
        conn->on_close(conn, code, reason);
    }
}

/* 触发错误回调 */
__attribute__((unused)) static void uvhttp_ws_trigger_error_callback(
    struct uvhttp_ws_connection* conn, int error_code, const char* error_msg) {
    if (conn && conn->on_error) {
        conn->on_error(conn, error_code, error_msg);
    }
}
