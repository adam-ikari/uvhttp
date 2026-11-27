/*
 * Backpressure decision, driven explicitly by queue size so the test does
 * not depend on a real socket backlog. The observable is read_paused and the
 * server's pause counter. resume keeps parser state (uv_read_start directly,
 * not restart_read) — verified by the same function restoring on a low queue.
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_connection.h"
#include "uvhttp_constants.h"
#include "uvhttp_error.h"
#include "uvhttp_features.h"
#include "uvhttp_server.h"

#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <uv.h>

namespace {

class BackpressureTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_config_t* config = nullptr;
        ASSERT_EQ(uvhttp_config_new(&config), UVHTTP_OK);
        config->write_queue_high_water = 1024;
        ASSERT_EQ(uvhttp_server_take_config(server_, config), UVHTTP_OK);
        ASSERT_EQ(uvhttp_connection_new(server_, &conn_), UVHTTP_OK);
        conn_->state = UVHTTP_CONN_STATE_HTTP_READING;

        /* uv_read_start needs a connected handle. Set up a local listener and
         * connect conn_'s tcp handle to it. */
        ASSERT_EQ(uv_tcp_init(loop_, &listener_), 0);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = 0;
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        ASSERT_EQ(uv_tcp_bind(&listener_, (const struct sockaddr*)&addr, 0), 0);
        ASSERT_EQ(uv_listen((uv_stream_t*)&listener_, 4, on_connected), 0);
        int port = 0;
        struct sockaddr_in bound;
        int len = sizeof(bound);
        ASSERT_EQ(uv_tcp_getsockname(&listener_, (struct sockaddr*)&bound, &len), 0);
        port = ntohs(bound.sin_port);
        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)port);
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        ASSERT_EQ(uv_tcp_connect(&connect_req_, (uv_tcp_t*)&conn_->tcp_handle,
                                 (const struct sockaddr*)&addr, nullptr), 0);
        for (int i = 0; i < 20; ++i) {
            uv_run(loop_, UV_RUN_NOWAIT);
        }
    }
    void TearDown() override {
        if (conn_) {
            uvhttp_connection_close(conn_);
            for (int i = 0; i < 10; ++i) {
                uv_run(loop_, UV_RUN_NOWAIT);
            }
        }
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        if (!uv_is_closing((uv_handle_t*)&listener_)) {
            uv_close((uv_handle_t*)&listener_, NULL);
        }
        if (loop_) {
            uv_run(loop_, UV_RUN_NOWAIT);
            uv_loop_close(loop_);
            uvhttp_free(loop_);
        }
    }
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
    uvhttp_connection_t* conn_ = nullptr;
    uv_tcp_t listener_{};
    uv_connect_t connect_req_{};

    static void on_connected(uv_stream_t* server, int status) {
        (void)server;
        (void)status;
    }
};

}  // namespace

TEST_F(BackpressureTest, HighQueuePausesReadAndCountsOnce) {
    EXPECT_EQ(conn_->read_paused, 0);
    uvhttp_connection_apply_backpressure(conn_, 2048);
    EXPECT_EQ(conn_->read_paused, 1);
    EXPECT_EQ(server_->backpressure_pauses, 1u);

    /* Repeated high queue must not double-count. */
    uvhttp_connection_apply_backpressure(conn_, 4096);
    EXPECT_EQ(server_->backpressure_pauses, 1u);
}

TEST_F(BackpressureTest, DrainedQueueResumesRead) {
    uvhttp_connection_apply_backpressure(conn_, 2048);
    ASSERT_EQ(conn_->read_paused, 1);
    uvhttp_connection_apply_backpressure(conn_, 512);
    EXPECT_EQ(conn_->read_paused, 0);
}

TEST_F(BackpressureTest, ZeroHighWaterIsNoop) {
    uvhttp_config_t* config = (uvhttp_config_t*)uvhttp_server_get_config(server_);
    config->write_queue_high_water = 0;
    uvhttp_connection_apply_backpressure(conn_, 1024 * 1024);
    EXPECT_EQ(conn_->read_paused, 0);
    EXPECT_EQ(server_->backpressure_pauses, 0u);
}
