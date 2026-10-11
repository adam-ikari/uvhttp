/**
 * @file test_config_full_coverage.cpp
 * @brief Comprehensive coverage tests for uvhttp_config module
 *
 * This test file aims to achieve 100% coverage for uvhttp_config.c by testing:
 * - NULL parameter handling
 * - Memory allocation failures
 * - All configuration fields
 * - All validation ranges
 * - Dynamic configuration updates
 * - Edge cases and boundary conditions
 */

#include "uvhttp_config.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <limits.h>

class UvhttpConfigTest : public ::testing::Test {
   protected:
    void SetUp() override {
        uv_error = uv_loop_init(&loop);
        ASSERT_EQ(uv_error, 0);

        uvhttp_error_t err = uvhttp_config_new(&config);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(config, nullptr);

        err = uvhttp_context_create(&loop, &context);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(context, nullptr);
    }

    void TearDown() override {
        if (context) {
            // Detach config before destroy to avoid double-free
            context->current_config = nullptr;
            uvhttp_context_destroy(context);
            context = nullptr;
        }
        if (config) {
            uvhttp_config_free(config);
            config = nullptr;
        }
        uv_loop_close(&loop);
    }

    uv_loop_t loop;
    int uv_error;

    uvhttp_config_t* config = nullptr;
    uvhttp_context_t* context = nullptr;
};

// Basic creation and destruction tests
TEST_F(UvhttpConfigTest, ConfigNewSuccess) {
    uvhttp_config_t* test_config = nullptr;
    uvhttp_error_t err = uvhttp_config_new(&test_config);

    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_NE(test_config, nullptr);

    uvhttp_config_free(test_config);
}

TEST_F(UvhttpConfigTest, ConfigNewNullOutput) {
    uvhttp_error_t err = uvhttp_config_new(nullptr);
    EXPECT_EQ(err, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigFreeNull) {
    // Should not crash
    uvhttp_config_free(nullptr);
}

TEST_F(UvhttpConfigTest, ConfigSetDefaultsNull) {
    // Should not crash
    uvhttp_config_set_defaults(nullptr);
}

TEST_F(UvhttpConfigTest, ConfigSetDefaultsValid) {
    uvhttp_config_set_defaults(config);

    // Pin every surviving field to its documented default. v2.10 deleted the 14
    // fields nothing read; this test is the inventory that must keep shrinking
    // or growing only together with the code that reads the field - never on
    // its own.
    EXPECT_EQ(config->max_connections, UVHTTP_DEFAULT_MAX_CONNECTIONS);
    EXPECT_EQ(config->backlog, UVHTTP_DEFAULT_BACKLOG);
    EXPECT_EQ(config->connection_timeout, UVHTTP_CONNECTION_TIMEOUT_DEFAULT);
    EXPECT_EQ(config->max_file_size, UVHTTP_DEFAULT_MAX_FILE_SIZE);
    EXPECT_EQ(config->trust_proxy_headers, 0);
    EXPECT_EQ(config->websocket_max_frame_size,
              UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE);
    EXPECT_EQ(config->websocket_max_message_size,
              UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE);
    EXPECT_EQ(config->websocket_ping_interval,
              UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL);
    EXPECT_EQ(config->websocket_ping_timeout,
              UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT);
    EXPECT_EQ(config->tcp_keepalive_timeout, UVHTTP_TCP_KEEPALIVE_TIMEOUT);
    EXPECT_EQ(config->sendfile_timeout_ms, UVHTTP_SENDFILE_TIMEOUT_MS);
    EXPECT_EQ(config->sendfile_max_retry, UVHTTP_SENDFILE_DEFAULT_MAX_RETRY);

    EXPECT_EQ(uvhttp_config_validate(config), UVHTTP_OK);
}

// Configuration validation tests
TEST_F(UvhttpConfigTest, ConfigValidateNull) {
    int result = uvhttp_config_validate(nullptr);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateDefault) {
    uvhttp_config_set_defaults(config);
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpConfigTest, ConfigValidateMinConnections) {
    config->max_connections = 0;  // Below minimum
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMaxConnections) {
    config->max_connections = 65536;  // Above maximum
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateValidConnections) {
    config->max_connections = 1000;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

// WebSocket configuration validation
TEST_F(UvhttpConfigTest, ConfigValidateMinWebSocketFrameSize) {
    config->websocket_max_frame_size = 0;  // Below minimum
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMaxWebSocketFrameSize) {
    config->websocket_max_frame_size =
        300 * 1024 * 1024;  // Above maximum (256MB)
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMinWebSocketMessageSize) {
    config->websocket_max_message_size = 0;  // Below minimum
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMaxWebSocketMessageSize) {
    config->websocket_max_message_size =
        2ULL * 1024 * 1024 * 1024;  // Above maximum (1GB)
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMessageSizeLessThanFrameSize) {
    config->websocket_max_frame_size = 16 * 1024 * 1024;
    config->websocket_max_message_size =
        8 * 1024 * 1024;  // Less than frame size
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateValidWebSocketConfig) {
    config->websocket_max_frame_size = 16 * 1024 * 1024;
    config->websocket_max_message_size = 64 * 1024 * 1024;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

// TCP Keep-Alive validation
TEST_F(UvhttpConfigTest, ConfigValidateMinTcpKeepaliveTimeout) {
    config->tcp_keepalive_timeout = 0;  // Below minimum
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMaxTcpKeepaliveTimeout) {
    config->tcp_keepalive_timeout = 7201;  // Above maximum (7200)
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

// Sendfile validation
TEST_F(UvhttpConfigTest, ConfigValidateMinSendfileTimeout) {
    config->sendfile_timeout_ms = 0;  // Below minimum
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigValidateMaxSendfileTimeout) {
    config->sendfile_timeout_ms = 400000;  // Above maximum (300000)
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

// Cache configuration validation
// Rate limiting validation
// Configuration print tests
TEST_F(UvhttpConfigTest, ConfigPrintNull) {
    // Should not crash
    uvhttp_config_print(nullptr);
}

TEST_F(UvhttpConfigTest, ConfigPrintValid) {
    // Should not crash
    uvhttp_config_print(config);
}

// Get current configuration tests
TEST_F(UvhttpConfigTest, ConfigGetCurrentNullContext) {
    const uvhttp_config_t* result = uvhttp_config_get_current(nullptr);
    EXPECT_EQ(result, nullptr);
}

TEST_F(UvhttpConfigTest, ConfigGetCurrentNoConfig) {
    const uvhttp_config_t* result = uvhttp_config_get_current(context);
    EXPECT_EQ(result, nullptr);
}

TEST_F(UvhttpConfigTest, ConfigGetCurrentValid) {
    uvhttp_config_set_current(context, config);
    const uvhttp_config_t* result = uvhttp_config_get_current(context);
    EXPECT_EQ(result, config);
}

// Set current configuration tests
TEST_F(UvhttpConfigTest, ConfigSetCurrentNullContext) {
    // Should not crash
    uvhttp_config_set_current(nullptr, config);
}

TEST_F(UvhttpConfigTest, ConfigSetCurrentValid) {
    uvhttp_config_set_current(context, config);
    EXPECT_EQ(context->current_config, config);
}

// Dynamic configuration update tests
TEST_F(UvhttpConfigTest, ConfigUpdateMaxConnectionsNullContext) {
    int result = uvhttp_config_update_max_connections(nullptr, 1000);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigUpdateMaxConnectionsMinValue) {
    int result = uvhttp_config_update_max_connections(context, 0);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigUpdateMaxConnectionsMaxValue) {
    int result = uvhttp_config_update_max_connections(context, 10001);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigUpdateMaxConnectionsNoConfig) {
    int result = uvhttp_config_update_max_connections(context, 1000);
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpConfigTest, ConfigUpdateMaxConnectionsValid) {
    uvhttp_config_set_current(context, config);
    int result = uvhttp_config_update_max_connections(context, 500);
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(config->max_connections, 500);
}

// Boundary value tests
TEST_F(UvhttpConfigTest, ConfigBoundaryConnectionsMin) {
    config->max_connections = 1;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpConfigTest, ConfigBoundaryConnectionsMax) {
    config->max_connections = 65535;
    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

// All configuration fields tests
TEST_F(UvhttpConfigTest, ConfigAllServerFields) {
    config->max_connections = 1000;
    config->backlog = 128;
    config->connection_timeout = 60;

    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpConfigTest, ConfigAllPerformanceFields) {
    config->max_file_size = 100 * 1024 * 1024;

    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpConfigTest, ConfigAllSecurityFields) {
    config->trust_proxy_headers = 1;

    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpConfigTest, ConfigAllWebSocketFields) {
    config->websocket_max_frame_size = 16 * 1024 * 1024;
    config->websocket_max_message_size = 64 * 1024 * 1024;
    config->websocket_ping_interval = 30;
    config->websocket_ping_timeout = 10;

    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpConfigTest, ConfigAllNetworkFields) {
    config->tcp_keepalive_timeout = 60;
    config->sendfile_timeout_ms = 5000;
    config->sendfile_max_retry = 3;

    int result = uvhttp_config_validate(config);
    EXPECT_EQ(result, UVHTTP_OK);
}

// Multiple updates test
TEST_F(UvhttpConfigTest, ConfigMultipleUpdates) {
    uvhttp_config_set_current(context, config);

    uvhttp_config_update_max_connections(context, 500);
    EXPECT_EQ(config->max_connections, 500);
}

// Complete workflow test
TEST_F(UvhttpConfigTest, ConfigCompleteWorkflow) {
    // Create config
    uvhttp_config_t* workflow_config = nullptr;
    uvhttp_error_t err = uvhttp_config_new(&workflow_config);
    ASSERT_EQ(err, UVHTTP_OK);
    ASSERT_NE(workflow_config, nullptr);

    // Set defaults
    uvhttp_config_set_defaults(workflow_config);

    // Validate
    int result = uvhttp_config_validate(workflow_config);
    EXPECT_EQ(result, UVHTTP_OK);

    // Set to context
    uvhttp_config_set_current(context, workflow_config);

    // Get current
    const uvhttp_config_t* current = uvhttp_config_get_current(context);
    EXPECT_EQ(current, workflow_config);

    // Update values
    uvhttp_config_update_max_connections(context, 800);

    // Verify updates
    EXPECT_EQ(workflow_config->max_connections, 800);

    // Clean up
    uvhttp_config_free(workflow_config);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}