/*
 * RFC 6455 protocol-error handling in the frame parser.
 *
 * 1. Reserved opcodes (0x3-0x7, 0xB-0xF) used to fall through the dispatch
 *    chain and be consumed as if valid. They must fail the connection.
 * 2. A CLOSE reason used to be a raw pointer into recv_buffer, so an on_close
 *    handler calling strlen/%s read whatever bytes followed the frame.
 */

#include "uvhttp_features.h"

#if UVHTTP_FEATURE_WEBSOCKET

#    include "uvhttp_allocator.h"
#    include "uvhttp_error.h"
#    include "uvhttp_websocket.h"

#    include <gtest/gtest.h>
#    include <string.h>
#    include <string>
#    include <vector>

namespace {

std::string g_close_reason;
int g_close_calls = 0;

int OnClose(uvhttp_ws_connection_t* conn, int code, const char* reason) {
    (void)conn;
    (void)code;
    ++g_close_calls;
    /* An embedder's natural use of the reason: strlen/%s. A slice into
     * recv_buffer without a terminator makes this read adjacent bytes. */
    g_close_reason.assign(reason, strlen(reason));
    return 0;
}

/* Build a masked client frame. Zero mask keeps the payload readable. */
std::vector<unsigned char> MaskedFrame(unsigned char opcode,
                                       const std::vector<unsigned char>& body) {
    std::vector<unsigned char> f;
    f.push_back((unsigned char)(0x80 | opcode)); /* FIN + opcode */
    f.push_back((unsigned char)(0x80 | body.size()));
    f.push_back(0);
    f.push_back(0);
    f.push_back(0);
    f.push_back(0);
    f.insert(f.end(), body.begin(), body.end());
    return f;
}

class WebSocketProtocolErrorTest : public ::testing::Test {
   protected:
    void SetUp() override {
        g_close_reason.clear();
        g_close_calls = 0;
        conn_ = uvhttp_ws_connection_create(-1, NULL, 1, NULL);
        ASSERT_NE(conn_, nullptr);
        uvhttp_ws_set_callbacks(conn_, NULL, OnClose, NULL);
    }
    void TearDown() override {
        uvhttp_ws_connection_free(conn_);
        conn_ = nullptr;
    }
    uvhttp_ws_connection_t* conn_ = nullptr;
};

}  // namespace

TEST_F(WebSocketProtocolErrorTest, ReservedOpcodeFailsProcessing) {
    /* A rejected frame is NOT consumed from recv_buffer, so reusing one
     * connection would re-parse the first opcode six times and never test the
     * others. Use a fresh connection per case. */
    for (unsigned char opcode : {0x3, 0x5, 0x7, 0xB, 0xD, 0xF}) {
        uvhttp_ws_connection_t* fresh =
            uvhttp_ws_connection_create(-1, NULL, 1, NULL);
        ASSERT_NE(fresh, nullptr);
        std::vector<unsigned char> frame = MaskedFrame(opcode, {0x41});
        EXPECT_NE(uvhttp_ws_process_data(fresh, frame.data(), frame.size()),
                  UVHTTP_OK)
            << "reserved opcode 0x" << std::hex << (int)opcode;
        uvhttp_ws_connection_free(fresh);
    }
}


TEST_F(WebSocketProtocolErrorTest, CloseReasonIsNulTerminated) {
    /* CLOSE with code 1000 and reason "bye", followed by unrelated bytes in
     * the same read window. Without termination strlen reads those bytes. */
    std::vector<unsigned char> body = {0x03, 0xE8, 'b', 'y', 'e'};
    std::vector<unsigned char> frame = MaskedFrame(0x8, body);
    const char trailing[] = "TRAILING-JUNK";
    frame.insert(frame.end(), trailing, trailing + strlen(trailing));

    EXPECT_EQ(uvhttp_ws_process_data(conn_, frame.data(), frame.size()),
              UVHTTP_OK);
    EXPECT_EQ(g_close_calls, 1);
    EXPECT_EQ(g_close_reason, "bye");
}

#endif /* UVHTTP_FEATURE_WEBSOCKET */
