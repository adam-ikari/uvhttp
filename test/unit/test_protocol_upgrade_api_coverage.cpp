/**
 * @file test_protocol_upgrade_api_coverage.cpp
 * @brief API coverage tests for uvhttp_protocol_upgrade module
 *
 * This test file focuses on testing the public API functions without
 * accessing internal structures, ensuring proper error handling and
 * parameter validation.
 */

#include <gtest/gtest.h>
#include <uvhttp_protocol_upgrade.h>
#include <uvhttp_server.h>
#include <uvhttp_connection.h>
#include <string.h>

class UvhttpProtocolUpgradeAPICoverageTest : public ::testing::Test {
protected:
    uv_loop_t* loop;
    uvhttp_server_t* server;

    void SetUp() override {
        loop = uv_default_loop();
        uvhttp_error_t result = uvhttp_server_new(loop, &server);
        ASSERT_EQ(result, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            uvhttp_server_free(server);
        }
    }

    // Mock protocol detector
    static int mock_detector(uvhttp_request_t* request,
                            char* protocol_name,
                            size_t protocol_name_len,
                            const char* upgrade_header,
                            const char* connection_header) {
        (void)request;
        (void)protocol_name;
        (void)protocol_name_len;
        (void)upgrade_header;
        (void)connection_header;
        return 0;
    }

    // Mock protocol upgrade handler
    static uvhttp_error_t mock_handler(uvhttp_connection_t* conn,
                                       const char* protocol_name,
                                       void* user_data) {
        (void)conn;
        (void)protocol_name;
        (void)user_data;
        return UVHTTP_OK;
    }

    // Mock ownership transfer callback
    static void mock_ownership_callback(uv_tcp_t* tcp_handle, int fd, void* user_data) {
        (void)tcp_handle;
        (void)fd;
        (void)user_data;
    }

    // Mock lifecycle callback
    static void mock_lifecycle_callback(void* user_data) {
        (void)user_data;
    }
};

/* ========== Protocol Registration API Tests ========== */

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolSuccess) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", "Test-Protocol", mock_detector, mock_handler,
        nullptr);

    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolNullServer) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        nullptr, "test-protocol", "Test-Protocol", mock_detector, mock_handler,
        nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolNullName) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, nullptr, "Test-Protocol", mock_detector, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolNullDetector) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", "Test-Protocol", nullptr, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolNullHandler) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", "Test-Protocol", mock_detector, nullptr, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolLongName) {
    char long_name[100];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';

    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, long_name, "Test-Protocol", mock_detector, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolLongUpgradeHeader) {
    char long_header[100];
    memset(long_header, 'b', sizeof(long_header) - 1);
    long_header[sizeof(long_header) - 1] = '\0';

    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", long_header, mock_detector, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolNullUpgradeHeader) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", nullptr, mock_detector, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolWithUserData) {
    int user_data = 42;
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", "Test-Protocol", mock_detector, mock_handler,
        &user_data);

    EXPECT_EQ(result, UVHTTP_OK);
}

/* ========== Protocol Unregistration API Tests ========== */

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, UnregisterProtocolSuccess) {
    uvhttp_server_register_protocol_upgrade(server, "test-protocol",
                                            "Test-Protocol", mock_detector,
                                            mock_handler, nullptr);

    uvhttp_error_t result = uvhttp_server_unregister_protocol_upgrade(
        server, "test-protocol");

    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, UnregisterProtocolNullServer) {
    uvhttp_error_t result = uvhttp_server_unregister_protocol_upgrade(
        nullptr, "test-protocol");

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, UnregisterProtocolNullName) {
    uvhttp_error_t result = uvhttp_server_unregister_protocol_upgrade(
        server, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, UnregisterProtocolNotFound) {
    uvhttp_error_t result = uvhttp_server_unregister_protocol_upgrade(
        server, "nonexistent-protocol");

    EXPECT_EQ(result, UVHTTP_ERROR_NOT_FOUND);
}

/* ========== Connection Ownership Transfer API Tests ========== */

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, TransferOwnershipNullConnection) {
    uvhttp_error_t result = uvhttp_connection_transfer_ownership(
        nullptr, mock_ownership_callback, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, TransferOwnershipNullCallback) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)malloc(sizeof(uvhttp_connection_t));
    memset(conn, 0, sizeof(uvhttp_connection_t));
    conn->state = UVHTTP_CONN_STATE_HTTP_PROCESSING;

    uvhttp_error_t result = uvhttp_connection_transfer_ownership(
        conn, nullptr, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    free(conn);
}

/* ========== Connection Lifecycle API Tests ========== */

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, SetLifecycleNullConnection) {
    uvhttp_connection_lifecycle_t lifecycle;
    lifecycle.user_data = nullptr;
    lifecycle.on_close = mock_lifecycle_callback;

    uvhttp_error_t result = uvhttp_connection_set_lifecycle(nullptr, &lifecycle);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, SetLifecycleNullLifecycle) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)malloc(sizeof(uvhttp_connection_t));
    memset(conn, 0, sizeof(uvhttp_connection_t));

    uvhttp_error_t result = uvhttp_connection_set_lifecycle(conn, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    free(conn);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, SetLifecycleValid) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)malloc(sizeof(uvhttp_connection_t));
    memset(conn, 0, sizeof(uvhttp_connection_t));

    uvhttp_connection_lifecycle_t lifecycle;
    lifecycle.user_data = (void*)42;
    lifecycle.on_close = mock_lifecycle_callback;

    uvhttp_error_t result = uvhttp_connection_set_lifecycle(conn, &lifecycle);

    EXPECT_EQ(result, UVHTTP_OK);

    free(conn);
}

/* ========== Helper Function Tests ========== */

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, GetFdNullConnection) {
    int fd;
    uvhttp_error_t result = uvhttp_connection_get_fd(nullptr, &fd);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, GetFdNullFd) {
    uvhttp_connection_t* conn = (uvhttp_connection_t*)malloc(sizeof(uvhttp_connection_t));
    memset(conn, 0, sizeof(uvhttp_connection_t));

    uvhttp_error_t result = uvhttp_connection_get_fd(conn, nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);

    free(conn);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, GetPeerAddressNullConnection) {
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);

    uvhttp_error_t result =
        uvhttp_connection_get_peer_address(nullptr, &addr, &addr_len);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, GetPeerAddressNullAddr) {
    socklen_t addr_len = sizeof(struct sockaddr_storage);

    uvhttp_error_t result = uvhttp_connection_get_peer_address(
        (uvhttp_connection_t*)malloc(sizeof(uvhttp_connection_t)), nullptr,
        &addr_len);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, GetPeerAddressNullAddrLen) {
    struct sockaddr_storage addr;

    uvhttp_error_t result = uvhttp_connection_get_peer_address(
        (uvhttp_connection_t*)malloc(sizeof(uvhttp_connection_t)), &addr,
        nullptr);

    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

/* ========== Edge Case Tests ========== */

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolEmptyName) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "", "Test-Protocol", mock_detector, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, RegisterProtocolEmptyUpgradeHeader) {
    uvhttp_error_t result = uvhttp_server_register_protocol_upgrade(
        server, "test-protocol", "", mock_detector, mock_handler, nullptr);

    EXPECT_EQ(result, UVHTTP_OK);
}

TEST_F(UvhttpProtocolUpgradeAPICoverageTest, UnregisterProtocolEmptyName) {
    uvhttp_error_t result = uvhttp_server_unregister_protocol_upgrade(
        server, "");

    EXPECT_EQ(result, UVHTTP_ERROR_NOT_FOUND);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}