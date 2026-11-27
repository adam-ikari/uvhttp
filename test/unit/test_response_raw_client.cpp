/*
 * uvhttp_response_send_raw takes a void* client. An embedder may pass its
 * own uv_tcp_t whose data points to application state, not to a
 * uvhttp_connection_t. The function must not cast that pointer and read
 * tls_enabled out of bounds just to decide how to encrypt.
 *
 * Under ASan the unfixed version reads past an 8-byte allocation.
 */

#include "uvhttp_features.h"

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_response.h"

#include <stdlib.h>
#include <string.h>
#include <uv.h>

namespace {

/* Deliberately tiny: reading uvhttp_connection_t::tls_enabled from here is
 * an out-of-bounds read that ASan catches. */
struct embedder_state {
    int marker;
};

TEST(ResponseRawClientTest, ForeignClientDataIsNotCastToConnection) {
    uv_loop_t* loop = uv_loop_new();
    ASSERT_NE(loop, nullptr);

    uv_tcp_t stream;
    ASSERT_EQ(uv_tcp_init(loop, &stream), 0);
    embedder_state* state = (embedder_state*)uvhttp_alloc(sizeof(*state));
    ASSERT_NE(state, nullptr);
    state->marker = 42;
    stream.data = state;

    static const char kRaw[] = "HTTP/1.1 200 OK\r\nContent-Length: 0\r\n\r\n";
    /* No assertion on the return code: an unconnected stream legitimately
     * fails uv_write. The observable is that no OOB read occurs. */
    (void)uvhttp_response_send_raw(kRaw, sizeof(kRaw) - 1, &stream, nullptr);

    EXPECT_EQ(state->marker, 42);
    uv_close((uv_handle_t*)&stream, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
    uvhttp_free(state);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

}  // namespace
