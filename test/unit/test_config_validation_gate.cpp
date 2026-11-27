/*
 * uvhttp_config_validate is now wired into uvhttp_server_listen.
 *
 * Before this, an invalid config was silently accepted by listen and only
 * surfaced later as a bogus uv_timer_start / uv_listen argument. These tests
 * lock in the choke-point behavior: the error is returned before any socket
 * is bound, and the error code names the real cause.
 */

#include <gtest/gtest.h>

#include "uvhttp_allocator.h"
#include "uvhttp_config.h"
#include "uvhttp_error.h"
#include "uvhttp_server.h"

#include <uv.h>

namespace {

class ConfigValidationGateTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
    }
    void TearDown() override {
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        uv_run(loop_, UV_RUN_NOWAIT);
        uv_loop_close(loop_);
        uvhttp_free(loop_);
    }
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
};

}  // namespace

TEST_F(ConfigValidationGateTest, RejectsInvalidMaxConnectionsBeforeBind) {
    uvhttp_config_t* config = nullptr;
    ASSERT_EQ(uvhttp_config_new(&config), UVHTTP_OK);
    config->max_connections = 0; /* below the documented minimum */
    ASSERT_EQ(uvhttp_server_take_config(server_, config), UVHTTP_OK);    /* server_free owns it */

    EXPECT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0),
              UVHTTP_ERROR_SERVER_INVALID_CONFIG);
}

TEST_F(ConfigValidationGateTest, TakeConfigInstallsSingleChannel) {
    EXPECT_EQ(uvhttp_server_get_config(nullptr), nullptr);
    EXPECT_EQ(uvhttp_server_get_config(server_), nullptr);
    EXPECT_EQ(uvhttp_server_take_config(nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);

    uvhttp_config_t* config = nullptr;
    ASSERT_EQ(uvhttp_config_new(&config), UVHTTP_OK);
    config->max_connections = 32;
    ASSERT_EQ(uvhttp_server_take_config(server_, config), UVHTTP_OK);
    EXPECT_NE(server_->context, nullptr);
    EXPECT_EQ(uvhttp_server_get_config(server_), config);

    /* NULL clears the one channel back to built-in defaults. */
    ASSERT_EQ(uvhttp_server_take_config(server_, nullptr), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_get_config(server_), nullptr);
}

TEST_F(ConfigValidationGateTest, TakeConfigReplacesOwnedConfig) {
    uvhttp_config_t* first = nullptr;
    uvhttp_config_t* second = nullptr;
    ASSERT_EQ(uvhttp_config_new(&first), UVHTTP_OK);
    ASSERT_EQ(uvhttp_config_new(&second), UVHTTP_OK);
    ASSERT_EQ(uvhttp_server_take_config(server_, first), UVHTTP_OK);
    /* Replacing must release the first config rather than leak or double-free
     * it. ASan checks the ownership side; this asserts the visible channel. */
    ASSERT_EQ(uvhttp_server_take_config(server_, second), UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_get_config(server_), second);
}

