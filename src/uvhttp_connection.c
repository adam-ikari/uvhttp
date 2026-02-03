#include "uvhttp_connection.h"

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_error_handler.h"
#include "uvhttp_error_helpers.h"
#include "uvhttp_logging.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#include "uvhttp_tls.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <uv.h>

#if UVHTTP_FEATURE_TLS
#    include <mbedtls/error.h>
#    include <mbedtls/net_sockets.h>
#    include <mbedtls/ssl.h>
#    include <mbedtls/ssl_ciphersuites.h>
#endif

/* ========== Compile-time validation ========== */
/* Validate structure size to ensure memory layout optimization is not broken */
#ifdef __cplusplus
#    define UVHTTP_STATIC_ASSERT(cond, msg) static_assert(cond, msg)
#else
#    define UVHTTP_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#endif

/* ========== Structure size validation ========== */

/* Validate structure size is within reasonable range (allows user custom
 * configuration) */

/* Note: Structure size depends on UVHTTP_INLINE_HEADERS_CAPACITY and other
 * configurable constants */

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_request_t) >= 65536,
                     "uvhttp_request_t size too small");

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_request_t) < 2 * 1024 * 1024,
                     "uvhttp_request_t size exceeds 2MB limit, consider "
                     "reducing UVHTTP_INLINE_HEADERS_CAPACITY");

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_response_t) >= 65536,
                     "uvhttp_response_t size too small");

UVHTTP_STATIC_ASSERT(sizeof(uvhttp_response_t) < 2 * 1024 * 1024,

                     "uvhttp_response_t size exceeds 2MB limit, consider "
                     "reducing UVHTTP_INLINE_HEADERS_CAPACITY");

// Idle callback for safe connection reuse
static void on_idle_restart_read(uv_idle_t* handle);

/* connection pool get function implementation */
static void on_alloc_buffer(uv_handle_t* handle, size_t suggested_size,
                            uv_buf_t* buf) {
    (void)suggested_size;
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn || !conn->read_buffer) {
        buf->base = NULL;
        buf->len = 0;
        return;
    }

    size_t remaining = conn->read_buffer_size - conn->read_buffer_used;

    buf->base = conn->read_buffer + conn->read_buffer_used;
    buf->len = remaining;
}

/* single-threaded event-driven read callback
 * This function is called in libuv event loop thread, processes all incoming
 * data Single-threaded model advantages: no locks needed, data access is safe,
 * execution stream is predictable
 */
/* Custom BIO callbacks for TLS integration with libuv */
static int mbedtls_bio_recv(void* ctx, unsigned char* buf, size_t len) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)ctx;

    if (!conn || !conn->read_buffer) {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    /* Check if we have data in the read buffer */
    if (conn->read_buffer_used == 0) {
        return MBEDTLS_ERR_SSL_WANT_READ;
    }

    /* Validate buffer state to prevent overflow */
    if (conn->read_buffer_used > conn->read_buffer_size) {
        UVHTTP_LOG_ERROR("Buffer overflow detected: used=%zu, size=%zu\n",
                         conn->read_buffer_used, conn->read_buffer_size);
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    /* Copy data from read buffer to SSL buffer */
    size_t copy_len =
        (len < conn->read_buffer_used) ? len : conn->read_buffer_used;
    memcpy(buf, conn->read_buffer, copy_len);

    /* Shift remaining data in buffer */
    if (copy_len < conn->read_buffer_used) {
        memmove(conn->read_buffer, conn->read_buffer + copy_len,
                conn->read_buffer_used - copy_len);
    }

    conn->read_buffer_used -= copy_len;

    return (int)copy_len;
}

static int mbedtls_bio_send(void* ctx, const unsigned char* buf, size_t len) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)ctx;

    if (!conn) {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    /* Check if TCP handle is valid */
    if (uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    /* Write encrypted data using libuv write */
    uv_buf_t uv_buf = uv_buf_init((char*)buf, len);
    int result = uv_try_write((uv_stream_t*)&conn->tcp_handle, &uv_buf, 1);

    if (result < 0) {
        /* Check for EAGAIN/EWOULDBLOCK using system error codes */
        if (result == -EAGAIN) {
            return MBEDTLS_ERR_SSL_WANT_WRITE;
        }
        return MBEDTLS_ERR_SSL_INTERNAL_ERROR;
    }

    return result;
}

static void on_read(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (!conn || !conn->request) {
        UVHTTP_LOG_ERROR("on_read: conn or conn->request is NULL\n");
        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            uvhttp_log_safe_error(nread, "connection_read", NULL);
        }
        /* async close connection - safe to execute in event loop */
        uvhttp_connection_close(conn);
        return;
    }

    if (nread == 0) {
        return;
    }

    /* check buffer validity */
    if (!buf || !buf->base) {
        UVHTTP_LOG_ERROR("Invalid buffer in on_read\n");
        uvhttp_connection_close(conn);
        return;
    }

    /* check buffer boundary, prevent overflow */
    if (conn->read_buffer_used + (size_t)nread > conn->read_buffer_size) {
        UVHTTP_LOG_ERROR("Read buffer overflow: %zu + %zd > %zu\n",
                         conn->read_buffer_used, nread, conn->read_buffer_size);
        uvhttp_connection_close(conn);
        return;
    }

    /* Copy received data to read buffer */
    memcpy(conn->read_buffer + conn->read_buffer_used, buf->base, nread);
    conn->read_buffer_used += nread;

    /* For TLS connections, handle handshake or decrypt data */
    if (conn->tls_enabled && conn->ssl) {
        /* Check if TLS handshake is in progress using connection state */
        if (conn->state == UVHTTP_CONN_STATE_TLS_HANDSHAKE) {
            /* Continue TLS handshake */
            int ret = mbedtls_ssl_handshake((mbedtls_ssl_context*)conn->ssl);
            if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
                ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
                /* Handshake in progress, wait for more data */
                return;
            } else if (ret != 0) {
                char error_buf[256];
                mbedtls_strerror(ret, error_buf, sizeof(error_buf));
                UVHTTP_LOG_ERROR("TLS handshake failed: %s\n", error_buf);
                uvhttp_connection_close(conn);
                return;
            }
            /* Handshake completed successfully */
            UVHTTP_LOG_DEBUG("TLS handshake completed\n");
            uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
        }

        /* Decrypt data from TLS */
        int ret = mbedtls_ssl_read((mbedtls_ssl_context*)conn->ssl,
                                   (unsigned char*)conn->read_buffer,
                                   conn->read_buffer_size);

        if (ret == MBEDTLS_ERR_SSL_WANT_READ ||
            ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
            /* Need more data, wait for next read callback */
            return;
        } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
            uvhttp_connection_close(conn);
            return;
        } else if (ret < 0) {
            char error_buf[256];
            mbedtls_strerror(ret, error_buf, sizeof(error_buf));
            UVHTTP_LOG_ERROR("TLS read failed: %s\n", error_buf);
            uvhttp_connection_close(conn);
            return;
        }

        /* Update read buffer with decrypted data */
        conn->read_buffer_used = ret;
    }

    /* single-threaded HTTP parse - no synchronization needed */
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        UVHTTP_LOG_DEBUG("on_read: Parsing %zu bytes\n",
                         conn->read_buffer_used);
        UVHTTP_LOG_DEBUG("on_read: parser->data = %p, conn = %p\n",
                         parser->data, conn);
        enum llhttp_errno err =
            llhttp_execute(parser, conn->read_buffer, conn->read_buffer_used);

        if (err != HPE_OK) {
            const char* err_name = llhttp_errno_name(err);
            UVHTTP_LOG_ERROR("HTTP parse error: %d (%s)\n", err,
                             err_name ? err_name : "unknown");
            UVHTTP_LOG_ERROR("HTTP parse error reason: %s\n",
                             llhttp_get_error_reason(parser));
            uvhttp_log_safe_error(err, "http_parse", err_name);
            /* async close connection on parse error */
            uvhttp_connection_close(conn);
            return;
        }

        UVHTTP_LOG_DEBUG(
            "on_read: llhttp_execute success, parsing_complete = %d\n",
            conn->parsing_complete);
    } else {
        UVHTTP_LOG_ERROR("on_read: parser is NULL\n");
    }

    /* Clear read buffer after parsing */
    conn->read_buffer_used = 0;
}

/* restart read for new request - used for keep-alive connection */
uvhttp_error_t uvhttp_connection_restart_read(uvhttp_connection_t* conn) {
    if (!conn || !conn->request || !conn->response || !conn->request->parser ||
        !conn->request->parser_settings) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check connection state, ensure connection is not in close process */
    if (conn->state == UVHTTP_CONN_STATE_CLOSING) {
        return UVHTTP_ERROR_CONNECTION_CLOSE;
    }

    /* optimize: stop current read first (if in progress) */
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);

    /* performance optimization: only reset necessary fields, avoid zeroing
     * entire struct (280KB)
     *
     * Optimization principle:
     * - Original: memset(conn->request, 0, sizeof(uvhttp_request_t)) zeros
     * 280KB
     * - New: only reset 10 fields, about 80 bytes total
     * - Performance improvement: saves 279,920 bytes of memory operation per
     * connection reuse
     *
     * Notes:
     * - Must ensure all state fields are correctly reset
     * - Pointer fields require special processing (keep unchanged or release)
     * - Large block memory (headers array) doesn't need zeroing, as
     * header_count has been reset
     *
     * Field reset list:
     * - Hot path fields: method, parsing_complete, header_count
     * - Pointer fields: path, query, body, user_data (set to NULL)
     * - Buffer fields: url (zero first byte)
     * - Large block memory: headers array (marked invalid via header_count)
     */

    /* reset hot path fields of request object */
    conn->request->method = UVHTTP_ANY;
    conn->request->parsing_complete = 0;
    conn->request->header_count = 0;
    conn->request->path = NULL;
    conn->request->query = NULL;
    conn->request->body = NULL;
    conn->request->body_length = 0;
    conn->request->body_capacity = 0;
    conn->request->user_data = NULL;

    /* resetURLbuffer */
    conn->request->url[0] = '\0';

    /* reset headers array (only reset used parts) */
    /* Note: no need to zero entire headers array, as header_count has already
     * been reset to 0 */
    /* reset HTTP parser */
    llhttp_t* parser = (llhttp_t*)conn->request->parser;
    if (parser) {
        llhttp_reset(parser);
        parser->data = conn;
    }

    /* performance optimization: only reset hot path fields of response object,
     * avoid zeroing entire struct (278KB)
     *
     * Optimization principle:
     * - Original: memset(conn->response, 0, sizeof(uvhttp_response_t)) zeros
     * 278KB
     * - New: only reset 10 fields, about 80 bytes total
     * - Performance improvement: saves 277,920 bytes of memory operation per
     * connection reuse
     */

    /* reset hot path fields of response object */
    conn->response->status_code = 0;
    conn->response->headers_sent = 0;
    conn->response->sent = 0;
    conn->response->finished = 0;
    conn->response->keepalive = 0;
    conn->response->compress = 0;
    conn->response->cache_ttl = 0;
    conn->response->header_count = 0;
    conn->response->body_length = 0;
    conn->response->cache_expires = 0;

    /* resetresponsebody */
    if (conn->response->body) {
        uvhttp_free(conn->response->body);
        conn->response->body = NULL;
    }

    /* reset HTTP/1.1 state flags of connection */
    conn->parsing_complete = 0;
    conn->content_length = 0;
    conn->body_received = 0;
    conn->keepalive = 1;        /* continue keep-alive */
    conn->chunked_encoding = 0; /* reset chunked transmission encoding flag */
    conn->current_header_is_important = 0;
    conn->parsing_header_field = 0;
    conn->need_restart_read = 0;

    /* reset current header field */
    conn->current_header_field_len = 0;

    /* updateconnectionstate */
    conn->state = UVHTTP_CONN_STATE_HTTP_READING;

    /* restart read to receive new request */
    int result = uv_read_start((uv_stream_t*)&conn->tcp_handle, on_alloc_buffer,
                               on_read);

    if (result != 0) {
        UVHTTP_LOG_ERROR("Failed to restart reading on connection: %s\n",
                         uv_strerror(result));
    }

    return result;
}

/* create new HTTP connection object (single-threaded event-driven)
 * server: HTTP server that owns this connection
 * return: connection object, all operations are processed in event loop thread
 *
 * Single-threaded connection management characteristics:
 * 1. No connection pool lock mechanism needed
 * 2. Memory allocation is done in single thread, safe and reliable
 * 3. All state changes are serialized in event loop
 */
uvhttp_error_t uvhttp_connection_new(struct uvhttp_server* server,
                                     uvhttp_connection_t** conn) {
    if (!server || !conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    *conn = NULL;

    /* single-threaded safe memory allocation */
    uvhttp_connection_t* c = uvhttp_alloc(sizeof(uvhttp_connection_t));
    if (!c) {
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    memset(c, 0, sizeof(uvhttp_connection_t));

    c->server = server;
    c->state = UVHTTP_CONN_STATE_NEW;
    c->tls_enabled = server->tls_enabled;  // Use server's TLS setting
    c->need_restart_read = 0;  // initialize to 0, no need to restart read
    // initialize idle handle for safe connection reuse
    if (uv_idle_init(server->loop, &c->idle_handle) != 0) {
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }
    c->idle_handle.data = c;

    // initialize timeout timer
    if (uv_timer_init(server->loop, &c->timeout_timer) != 0) {
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }
    c->timeout_timer.data = c;

    // HTTP/1.1optimize: initializedefaultvalue
    c->keepalive = 1;         /* HTTP/1.1defaultkeepconnection */
    c->chunked_encoding = 0;  /* default: no chunked transmission */
    c->close_pending = 0;     /* initialize pending close handle count */
    c->content_length = 0;    /* default: no content length */
    c->body_received = 0;     // received body length
    c->parsing_complete = 0;  // parsing not complete
    c->current_header_is_important = 0;  // current header is not a key field
    c->read_buffer_used = 0;             // reset read buffer usage
    // HTTPparsestateinitialize
    memset(c->current_header_field, 0, sizeof(c->current_header_field));
    c->current_header_field_len = 0;
    c->parsing_header_field = 0;

    // TCP initialize - complete implementation
    if (uv_tcp_init(server->loop, &c->tcp_handle) != 0) {
        /* Note: uv_close is async, cannot be used here
         * For initialization failure cases, just release memory directly
         * Because these handles have not been added to event loop yet */
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }
    c->tcp_handle.data = c;

    // TCP options are set uniformly at server level (TCP_NODELAY and
    // TCP_KEEPALIVE) Avoid duplicate settings to improve performance allocate
    // read buffer
    c->read_buffer_size = UVHTTP_READ_BUFFER_SIZE;
    c->read_buffer = uvhttp_alloc(c->read_buffer_size);
    if (!c->read_buffer) {
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    c->read_buffer_used = 0;

    // create request and response objects
    c->request = uvhttp_alloc(sizeof(uvhttp_request_t));
    if (!c->request) {
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // correctly initialize request object (contains HTTP parser)
    if (uvhttp_request_init(c->request, &c->tcp_handle) != 0) {
        uvhttp_free(c->request);
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }

    c->response = uvhttp_alloc(sizeof(uvhttp_response_t));
    if (!c->response) {
        uvhttp_request_cleanup(c->request);
        uvhttp_free(c->request);
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }

    // correctly initialize response object
    if (uvhttp_response_init(c->response, &c->tcp_handle) != 0) {
        uvhttp_request_cleanup(c->request);
        uvhttp_free(c->request);
        uvhttp_free(c->response);  // release directly, no cleanup needed (due
                                   // to initialization failure)
        uvhttp_free(c->read_buffer);
        uvhttp_free(c);
        return UVHTTP_ERROR_IO_ERROR;
    }

    // set parser's data pointer to connection object
    llhttp_t* parser = (llhttp_t*)c->request->parser;
    if (parser) {
        parser->data = c;
    }

    *conn = c;
    return UVHTTP_OK;
}

void uvhttp_connection_free(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    // clean request and response data
    if (conn->request) {
        uvhttp_request_cleanup(conn->request);
        uvhttp_free(conn->request);
    }

    if (conn->response) {
        uvhttp_response_cleanup(conn->response);
        uvhttp_free(conn->response);
    }

    if (conn->read_buffer) {
        uvhttp_free(conn->read_buffer);
    }

#if UVHTTP_FEATURE_TLS
    // Clean up SSL context to prevent memory leak
    if (conn->ssl) {
        uvhttp_connection_tls_cleanup(conn);
    }
#endif

    // releaseconnectionmemory
    uvhttp_free(conn);
}

uvhttp_error_t uvhttp_connection_start(uvhttp_connection_t* conn) {
    if (!conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* start HTTP read - complete implementation */
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle,
                      (uv_alloc_cb)on_alloc_buffer, (uv_read_cb)on_read) != 0) {
        UVHTTP_LOG_ERROR("Failed to start reading on connection\n");
        uvhttp_connection_close(conn);
        return UVHTTP_ERROR_CONNECTION_START;
    }

    /* TLSprocess - start TLS handshake if enabled */
    if (conn->tls_enabled) {
        uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_TLS_HANDSHAKE);
        uvhttp_error_t result = uvhttp_connection_tls_handshake_func(conn);
        if (result == UVHTTP_ERROR_TLS_WANT_READ ||
            result == UVHTTP_ERROR_TLS_WANT_WRITE) {
            /* TLS handshake in progress, wait for more data */
            UVHTTP_LOG_DEBUG("TLS handshake in progress, waiting for data\n");
            return UVHTTP_OK;
        } else if (result != UVHTTP_OK) {
            UVHTTP_LOG_ERROR("TLS handshake failed\n");
            uvhttp_connection_close(conn);
            return UVHTTP_ERROR_CONNECTION_START;
        }
        /* TLS handshake completed successfully */
        uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
        UVHTTP_LOG_DEBUG("TLS handshake completed\n");
    } else {
        uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    }

    return UVHTTP_OK;
}

/* Handle close callback (general) */
static void on_handle_close(uv_handle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn) {
        return;
    }

    /* decrease pending close handle count */
    conn->close_pending--;

    /* release connection when all handles are closed */
    if (conn->close_pending == 0) {
        /* single-threaded safe connection count decrement */
        if (conn->server) {
            conn->server->active_connections--;
        }
        /* release connection resources - safe to execute in event loop thread
         */
        uvhttp_connection_free(conn);
    }
}

void uvhttp_connection_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);

    /* initialize pending close handle count */
    conn->close_pending = 0;

    /* stop idle handle (if running) */
    if (!uv_is_closing((uv_handle_t*)&conn->idle_handle)) {
        uv_idle_stop(&conn->idle_handle);
        uv_close((uv_handle_t*)&conn->idle_handle, on_handle_close);
        conn->close_pending++;
    }

    /* stop timeout timer (if running) */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
        uv_close((uv_handle_t*)&conn->timeout_timer, on_handle_close);
        conn->close_pending++;
    }

    /* close TCP handle */
    if (!uv_is_closing((uv_handle_t*)&conn->tcp_handle)) {
        uv_close((uv_handle_t*)&conn->tcp_handle, on_handle_close);
        conn->close_pending++;
    }
}

void uvhttp_connection_set_state(uvhttp_connection_t* conn,
                                 uvhttp_connection_state_t state) {
    if (conn) {
        conn->state = state;
    }
}

uvhttp_error_t uvhttp_connection_tls_handshake_func(uvhttp_connection_t* conn) {
    if (!conn || !conn->server || !conn->server->tls_ctx) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Initialize SSL context if not already initialized */
    if (!conn->ssl) {
        /* Create SSL context from TLS context */
        mbedtls_ssl_context* ssl = uvhttp_tls_create_ssl(conn->server->tls_ctx);
        if (!ssl) {
            return UVHTTP_ERROR_TLS_INIT;
        }

        /* Setup SSL with custom BIO callbacks */
        mbedtls_ssl_set_bio(ssl, conn, mbedtls_bio_send, mbedtls_bio_recv,
                            NULL);

        conn->ssl = ssl;
    }

    /* Perform TLS handshake */
    int ret = mbedtls_ssl_handshake((mbedtls_ssl_context*)conn->ssl);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return UVHTTP_ERROR_TLS_WANT_READ;
    } else if (ret != 0) {
        char error_buf[256];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        UVHTTP_LOG_ERROR("TLS handshake failed: %s\n", error_buf);
        return UVHTTP_ERROR_TLS_HANDSHAKE;
    }

    return UVHTTP_OK;
}

uvhttp_error_t uvhttp_connection_tls_read(uvhttp_connection_t* conn) {
    if (!conn || !conn->ssl || !conn->read_buffer) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Read decrypted data from TLS */
    int ret = mbedtls_ssl_read((mbedtls_ssl_context*)conn->ssl,
                               (unsigned char*)conn->read_buffer,
                               conn->read_buffer_size);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return UVHTTP_ERROR_TLS_WANT_READ;
    } else if (ret == MBEDTLS_ERR_SSL_PEER_CLOSE_NOTIFY) {
        return UVHTTP_ERROR_CONNECTION_CLOSE;
    } else if (ret < 0) {
        char error_buf[256];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        UVHTTP_LOG_ERROR("TLS read failed: %s\n", error_buf);
        return UVHTTP_ERROR_TLS_READ;
    }

    conn->read_buffer_used = ret;
    return UVHTTP_OK;
}

void uvhttp_connection_tls_cleanup(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    /* Free SSL context */
    if (conn->ssl) {
        mbedtls_ssl_free((mbedtls_ssl_context*)conn->ssl);
        uvhttp_free(conn->ssl);
        conn->ssl = NULL;
    }
}

uvhttp_error_t uvhttp_connection_start_tls_handshake(
    uvhttp_connection_t* conn) {
    return uvhttp_connection_tls_handshake_func(conn);
}

uvhttp_error_t uvhttp_connection_tls_write(uvhttp_connection_t* conn,
                                           const void* data, size_t len) {
    if (!conn || !conn->ssl || !data) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* Write data through TLS */
    int ret = mbedtls_ssl_write((mbedtls_ssl_context*)conn->ssl, data, len);
    if (ret == MBEDTLS_ERR_SSL_WANT_READ || ret == MBEDTLS_ERR_SSL_WANT_WRITE) {
        return UVHTTP_ERROR_TLS_WANT_WRITE;
    } else if (ret < 0) {
        char error_buf[256];
        mbedtls_strerror(ret, error_buf, sizeof(error_buf));
        UVHTTP_LOG_ERROR("TLS write failed: %s\n", error_buf);
        return UVHTTP_ERROR_TLS_WRITE;
    }

    return UVHTTP_OK;
}

/* idle callback function for safe read restart

 * Execute in next event loop, avoid direct state manipulation in write complete
 callback */
static void on_idle_restart_read(uv_idle_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn) {
        return;
    }

    // stopidlehandle
    uv_idle_stop(handle);

    // checkconnectionstate
    if (conn->state == UVHTTP_CONN_STATE_CLOSING) {
        return;
    }

    // execute connection restart
    if (uvhttp_connection_restart_read(conn) != 0) {
        // restart failed, close connection
        uvhttp_connection_close(conn);
    }
}

// start safe connection reuse
uvhttp_error_t uvhttp_connection_schedule_restart_read(
    uvhttp_connection_t* conn) {
    if (!conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    // use idle handle to safely restart read in next event loop
    conn->idle_handle.data = conn;

    if (uv_idle_start(&conn->idle_handle, on_idle_restart_read) != 0) {
        UVHTTP_LOG_ERROR(
            "Failed to start idle handle for connection restart\n");
        return UVHTTP_ERROR_IO_ERROR;
    }

    return UVHTTP_OK;
}

#if UVHTTP_FEATURE_WEBSOCKET

/* WebSocket data read callback
 * Use this callback to process WebSocket framed data after WebSocket handshake
 * succeeds
 */
static void on_websocket_read(uv_stream_t* stream, ssize_t nread,
                              const uv_buf_t* buf) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)stream->data;
    if (!conn || !conn->ws_connection) {
        return;
    }

    if (nread < 0) {
        if (nread != UV_EOF) {
            UVHTTP_LOG_ERROR("WebSocket read error: %s\n", uv_strerror(nread));
        }
        uvhttp_connection_websocket_close(conn);
        return;
    }

    if (nread == 0) {
        return;
    }

    /* processWebSocketframedata */
    uvhttp_ws_connection_t* ws_conn =
        (uvhttp_ws_connection_t*)conn->ws_connection;
    int result =
        uvhttp_ws_process_data(ws_conn, (const uint8_t*)buf->base, nread);
    if (result != 0) {
        UVHTTP_LOG_ERROR("WebSocket data processing failed: %d\n", result);
        uvhttp_connection_websocket_close(conn);
    }
}

/* WebSocket connection wrapper - used to store user handler and connection
 * object */
typedef struct {
    uvhttp_connection_t* conn;
    uvhttp_ws_handler_t* user_handler;
} uvhttp_ws_wrapper_t;

/* WebSocketconnectionclosecallback */
static int on_websocket_close(uvhttp_ws_connection_t* ws_conn, int code,
                              const char* reason) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* get connection object and user handler from wrapper */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (!wrapper || !wrapper->conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* call user-registered close callback */
    if (wrapper->user_handler && wrapper->user_handler->on_close) {
        int result = wrapper->user_handler->on_close(ws_conn);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_close callback failed: %d\n", result);
        }
    }

    /* release wrapper */
    uvhttp_free(wrapper);
    ws_conn->user_data = NULL;

    (void)code;
    (void)reason;
    return UVHTTP_OK;
}

/* WebSocketerrorcallback */
static int on_websocket_error(uvhttp_ws_connection_t* ws_conn, int error_code,
                              const char* error_msg) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    UVHTTP_LOG_ERROR("WebSocket error: %s (code: %d)\n", error_msg, error_code);

    /* get user handler from wrapper */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (wrapper && wrapper->user_handler) {
        /* call user-registered error callback */
        if (wrapper->user_handler->on_error) {
            int result =
                wrapper->user_handler->on_error(ws_conn, error_code, error_msg);
            if (result != 0) {
                UVHTTP_LOG_ERROR("User on_error callback failed: %d\n", result);
            }
            return result;
        }
    }

    return UVHTTP_ERROR_IO_ERROR;
}

/* WebSocketmessagecallback */
static int on_websocket_message(uvhttp_ws_connection_t* ws_conn,
                                const char* data, size_t len, int opcode) {
    if (!ws_conn) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* update connection activity time */
    uvhttp_ws_wrapper_t* wrapper = (uvhttp_ws_wrapper_t*)ws_conn->user_data;
    if (wrapper && wrapper->conn && wrapper->conn->server) {
        uvhttp_server_ws_update_activity(wrapper->conn->server, ws_conn);
    }

    /* get user handler from wrapper */
    if (!wrapper || !wrapper->user_handler) {
        /* no user handler, ignore message */
        return UVHTTP_OK;
    }

    /* call user-registered message callback */
    if (wrapper->user_handler->on_message) {
        int result =
            wrapper->user_handler->on_message(ws_conn, data, len, opcode);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_message callback failed: %d\n", result);
            return result;
        }
    }

    return UVHTTP_OK;
}

/* processWebSockethandshake
 * call after handshake response send, create WebSocket connection object and
 * set callback
 */
uvhttp_error_t uvhttp_connection_handle_websocket_handshake(
    uvhttp_connection_t* conn, const char* ws_key) {
    if (!conn || !ws_key) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* getrequestpath */
    const char* path = conn->request ? conn->request->url : NULL;
    if (!path) {
        UVHTTP_LOG_ERROR(
            "Failed to get request path for WebSocket handshake\n");
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* getclient IP address */
    char client_ip[UVHTTP_CLIENT_IP_BUFFER_SIZE] = {0};
    struct sockaddr_in addr;
    int addr_len = sizeof(addr);
    if (uv_tcp_getpeername(&conn->tcp_handle, (struct sockaddr*)&addr,
                           &addr_len) == 0) {
        uv_ip4_name(&addr, client_ip, sizeof(client_ip));
    }

    /* get Token from query parameter or header */
    char token[256] = {0};
    if (conn->request) {
        /* attempt to get from query parameter */
        const char* query = conn->request->query;
        if (query && strstr(query, "token=")) {
            const char* token_start = strstr(query, "token=") + 6;
            const char* token_end = strchr(token_start, '&');
            if (token_end) {
                size_t token_len = token_end - token_start;
                if (token_len < sizeof(token)) {
                    /* use safe string copy function */
                    if (uvhttp_safe_strcpy(token, token_len + 1, token_start) !=
                        0) {
                        token[0] = '\0';
                    } else {
                        token[token_len] = '\0';
                    }
                }
            } else {
                /* use safe string copy function */
                if (uvhttp_safe_strcpy(token, sizeof(token), token_start) !=
                    0) {
                    token[0] = '\0';
                }
            }
        }
    }

    /* find user-registered WebSocket handler */
    uvhttp_ws_handler_t* user_handler = NULL;
    if (conn->server) {
        user_handler = uvhttp_server_find_ws_handler(conn->server, path);
    }

    if (!user_handler) {
        UVHTTP_LOG_WARN("No WebSocket handler found for path: %s\n", path);
        /* continue creating connection, but use default callback */
    }

    /* get TCP file descriptor */
    int fd = 0;
    if (uv_fileno((uv_handle_t*)&conn->tcp_handle, &fd) != 0) {
        UVHTTP_LOG_ERROR(
            "Failed to get file descriptor for WebSocket connection\n");
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* create WebSocket connection object */
    uvhttp_ws_connection_t* ws_conn =
        uvhttp_ws_connection_create(fd, NULL, 1, conn->server->config);
    if (!ws_conn) {
        UVHTTP_LOG_ERROR("Failed to create WebSocket connection object\n");
        return UVHTTP_ERROR_IO_ERROR;
    }

    /* save WebSocket Key (for verification) */
    strncpy(ws_conn->client_key, ws_key, sizeof(ws_conn->client_key) - 1);
    ws_conn->client_key[sizeof(ws_conn->client_key) - 1] = '\0';

    /* create wrapper to save connection object and user handler */
    uvhttp_ws_wrapper_t* wrapper = uvhttp_alloc(sizeof(uvhttp_ws_wrapper_t));
    if (!wrapper) {
        UVHTTP_LOG_ERROR("Failed to allocate WebSocket wrapper\n");
        uvhttp_ws_connection_free(ws_conn);
        return UVHTTP_ERROR_OUT_OF_MEMORY;
    }
    wrapper->conn = conn;
    wrapper->user_handler = user_handler;

    /* set wrapper as WebSocket connection's user_data */
    ws_conn->user_data = wrapper;

    /* set callback function (internal callback will call user callback) */
    uvhttp_ws_set_callbacks(ws_conn, on_websocket_message, on_websocket_close,
                            on_websocket_error);

    /* save to connection object */
    conn->ws_connection = ws_conn;
    conn->is_websocket = 1;

    /* call user-registered connection callback */
    if (user_handler && user_handler->on_connect) {
        int result = user_handler->on_connect(ws_conn);
        if (result != 0) {
            UVHTTP_LOG_ERROR("User on_connect callback failed: %d\n", result);
            /* connectioncallbackfailure, closeconnection */
            uvhttp_connection_websocket_close(conn);
            return UVHTTP_ERROR_IO_ERROR;
        }
    }

    /* switch to WebSocket data read pattern */
    uvhttp_connection_switch_to_websocket(conn);

    /* add to connection manager */
    if (conn->server) {
        uvhttp_server_ws_add_connection(conn->server, ws_conn, path);
    }

    UVHTTP_LOG_DEBUG("WebSocket handshake completed for path: %s\n", path);
    return UVHTTP_OK;
}

/* switch to WebSocket data process pattern
 * stopHTTPread, startWebSocketframeread
 */
void uvhttp_connection_switch_to_websocket(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    /* stopHTTPread */
    uv_read_stop((uv_stream_t*)&conn->tcp_handle);

    /* updateconnectionstate */
    conn->state = UVHTTP_CONN_STATE_HTTP_PROCESSING;

    /* startWebSocketdataread */
    if (uv_read_start((uv_stream_t*)&conn->tcp_handle, on_alloc_buffer,
                      on_websocket_read) != 0) {
        UVHTTP_LOG_ERROR("Failed to start WebSocket reading\n");
        uvhttp_connection_close(conn);
        return;
    }

    UVHTTP_LOG_DEBUG("Switched to WebSocket mode for connection\n");
}

/* closeWebSocketconnection */
void uvhttp_connection_websocket_close(uvhttp_connection_t* conn) {
    if (!conn) {
        return;
    }

    /* remove from connection manager */
    if (conn->server && conn->ws_connection) {
        uvhttp_server_ws_remove_connection(
            conn->server, (uvhttp_ws_connection_t*)conn->ws_connection);
    }

    /* release WebSocket connection object */
    if (conn->ws_connection) {
        uvhttp_ws_connection_free((uvhttp_ws_connection_t*)conn->ws_connection);
        conn->ws_connection = NULL;
    }

    conn->is_websocket = 0;

    /* close underlying TCP connection */
    uvhttp_connection_close(conn);
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */
/* connectiontimeoutcallbackfunction */
static void connection_timeout_cb(uv_timer_t* handle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)handle->data;
    if (!conn || !conn->server) {
        return;
    }

    /* get timeout time, if config is NULL then use default value */
    int timeout_ms = UVHTTP_CONNECTION_TIMEOUT_DEFAULT * 1000;
    if (conn->server->config) {
        timeout_ms = conn->server->config->connection_timeout * 1000;
    }

    /* trigger application layer timeout statistics callback */
    if (conn->server->timeout_callback) {
        conn->server->timeout_callback(
            conn->server, conn, timeout_ms,
            conn->server->timeout_callback_user_data);
    }

    UVHTTP_LOG_WARN("Connection timeout, closing connection...\n");
    uvhttp_connection_close(conn);
}

/* start connection timeout timer */
uvhttp_error_t uvhttp_connection_start_timeout(uvhttp_connection_t* conn) {
    if (!conn || !conn->server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* stop old timer (if exists) */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }

    /* get timeout time, if config is NULL then use default value */
    int timeout_ms = UVHTTP_CONNECTION_TIMEOUT_DEFAULT * 1000;
    if (conn->server->config) {
        timeout_ms = conn->server->config->connection_timeout * 1000;
    }

    /* start timer */
    if (uv_timer_start(&conn->timeout_timer, connection_timeout_cb, timeout_ms,
                       0) != 0) {
        UVHTTP_LOG_ERROR("Failed to start connection timeout timer\n");
        return UVHTTP_ERROR_CONNECTION_TIMEOUT;
    }

    return UVHTTP_OK;
}

/* start connection timeout timer (custom timeout time) */
uvhttp_error_t uvhttp_connection_start_timeout_custom(uvhttp_connection_t* conn,
                                                      int timeout_seconds) {
    if (!conn || !conn->server) {
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* verifytimeouttimerange */
    if (timeout_seconds < UVHTTP_CONNECTION_TIMEOUT_MIN ||
        timeout_seconds > UVHTTP_CONNECTION_TIMEOUT_MAX) {
        UVHTTP_LOG_ERROR("Invalid timeout value: %d seconds\n",
                         timeout_seconds);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* check integer overflow */
    if (timeout_seconds > INT_MAX / 1000) {
        UVHTTP_LOG_ERROR("Timeout value too large: %d seconds\n",
                         timeout_seconds);
        return UVHTTP_ERROR_INVALID_PARAM;
    }

    /* stop old timer (if exists) */
    if (!uv_is_closing((uv_handle_t*)&conn->timeout_timer)) {
        uv_timer_stop(&conn->timeout_timer);
    }

    /* start timer */
    if (uv_timer_start(&conn->timeout_timer, connection_timeout_cb,
                       timeout_seconds * 1000, 0) != 0) {
        UVHTTP_LOG_ERROR("Failed to start connection timeout timer\n");
        return UVHTTP_ERROR_CONNECTION_TIMEOUT;
    }

    return UVHTTP_OK;
}
