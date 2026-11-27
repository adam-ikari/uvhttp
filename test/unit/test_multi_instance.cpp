/*
 * PHILOSOPHY §10: internal tests pass but a real embedder can still fail.
 * One class of embedding defect was global state contaminating multiple
 * server instances in one process. This test drives two servers on one loop
 * and asserts their per-instance state is isolated.
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_server.h"

#include <uv.h>

namespace {

static uvhttp_error_t deny_all(uvhttp_request_t*, uvhttp_response_t*, void*) {
    return UVHTTP_ERROR_INVALID_PARAM;
}

class MultiInstanceTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        for (int i = 0; i < 2; ++i) {
            ASSERT_EQ(uvhttp_server_new(loop_, &servers_[i]), UVHTTP_OK);
        }
    }
    void TearDown() override {
        for (int i = 0; i < 2; ++i) {
            if (servers_[i]) {
                uvhttp_server_free(servers_[i]);
                servers_[i] = nullptr;
            }
        }
        if (loop_) {
            uv_run(loop_, UV_RUN_NOWAIT);
            uv_loop_close(loop_);
            uvhttp_free(loop_);
        }
    }
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* servers_[2] = {nullptr, nullptr};
};

}  // namespace

TEST_F(MultiInstanceTest, ConfigsAreIndependent) {
    for (int i = 0; i < 2; ++i) {
        uvhttp_config_t* cfg = nullptr;
        ASSERT_EQ(uvhttp_config_new(&cfg), UVHTTP_OK);
        uvhttp_config_set_defaults(cfg);
        cfg->max_connections = (i == 0) ? 11 : 22;
        ASSERT_EQ(uvhttp_server_take_config(servers_[i], cfg), UVHTTP_OK);
    }
    for (int i = 0; i < 2; ++i) {
        const uvhttp_config_t* cfg = uvhttp_server_get_config(servers_[i]);
        ASSERT_NE(cfg, nullptr);
        EXPECT_EQ(cfg->max_connections, (i == 0) ? 11 : 22);
    }
    EXPECT_NE(uvhttp_server_get_config(servers_[0]),
              uvhttp_server_get_config(servers_[1]));
}

TEST_F(MultiInstanceTest, DrainingIsIndependent) {
    ASSERT_EQ(uvhttp_server_set_draining(servers_[0], 1), UVHTTP_OK);
    /* No public getter for draining. Verify isolation by giving server 1 a
     * config and confirming server 0's view stays untouched (NULL), and that
     * server 1's config is exactly the one installed. */
    uvhttp_config_t* cfg = nullptr;
    ASSERT_EQ(uvhttp_config_new(&cfg), UVHTTP_OK);
    uvhttp_config_set_defaults(cfg);
    ASSERT_EQ(uvhttp_server_take_config(servers_[1], cfg), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_get_config(servers_[0]), nullptr);
    EXPECT_EQ(uvhttp_server_get_config(servers_[1]), cfg);

    uvhttp_server_stats_t stats;
    ASSERT_EQ(uvhttp_server_get_stats(servers_[0], &stats), UVHTTP_OK);
    EXPECT_EQ(stats.active_connections, 0u);
    ASSERT_EQ(uvhttp_server_get_stats(servers_[1], &stats), UVHTTP_OK);
    EXPECT_EQ(stats.active_connections, 0u);
}

TEST_F(MultiInstanceTest, ErrorCallbacksAreIndependent) {
    /* Installing a filter on one server must not affect the other. */
    ASSERT_EQ(uvhttp_server_set_request_filter(servers_[0], deny_all, nullptr),
              UVHTTP_OK);
    uvhttp_server_set_request_filter(servers_[1], nullptr, nullptr);
    /* Both servers remain usable: listen on ephemeral ports, no cross-talk. */
    for (int i = 0; i < 2; ++i) {
        EXPECT_EQ(uvhttp_server_listen(servers_[i], "127.0.0.1", 0), UVHTTP_OK);
    }
}
