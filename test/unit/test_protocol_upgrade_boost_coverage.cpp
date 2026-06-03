/**
 * @file test_protocol_upgrade_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_protocol_upgrade module
 *
 * Tests protocol registration/unregistration, null checks, and error paths.
 * Connection-level functions (transfer_ownership, get_fd, get_peer_address)
 * require real libuv handles so only null checks are tested for those.
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_protocol_upgrade.h"
#include "uvhttp_server.h"
}

#include <string.h>

// Dummy detector and handler for testing
static int test_detector(uvhttp_request_t* request, char* protocol_name,
                         size_t protocol_name_len, const char* upgrade_header,
                         const char* connection_header) {
    (void)request;
    (void)upgrade_header;
    (void)connection_header;
    if (protocol_name && protocol_name_len > 0) {
        strncpy(protocol_name, "test-proto", protocol_name_len - 1);
        protocol_name[protocol_name_len - 1] = '\0';
    }
    return 1;
}

static uvhttp_error_t test_handler(uvhttp_connection_t* conn,
                                   const char* protocol_name, void* user_data) {
    (void)conn;
    (void)protocol_name;
    (void)user_data;
    return UVHTTP_OK;
}

class ProtocolUpgradeTest : public ::testing::Test {
protected:
    uv_loop_t loop;
    uvhttp_server_t* server = nullptr;

    void SetUp() override {
        int err = uv_loop_init(&loop);
        ASSERT_EQ(err, 0);
        uvhttp_error_t serr = uvhttp_server_new(&loop, &server);
        ASSERT_EQ(serr, UVHTTP_OK);
        ASSERT_NE(server, nullptr);
    }

    void TearDown() override {
        if (server) {
            // Clean up protocol registry before freeing server
            if (server->protocol_registry) {
                uvhttp_protocol_registry_t* reg =
                    (uvhttp_protocol_registry_t*)server->protocol_registry;
                uvhttp_protocol_info_t* p = reg->protocols;
                while (p) {
                    uvhttp_protocol_info_t* next = p->next;
                    uvhttp_free(p);
                    p = next;
                }
                uvhttp_free(reg);
                server->protocol_registry = nullptr;
            }
            uvhttp_server_free(server);
            server = nullptr;
        }
        uv_loop_close(&loop);
    }
};

// ========== uvhttp_server_register_protocol_upgrade ==========

TEST_F(ProtocolUpgradeTest, Register_NullServer_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  nullptr, "test", "websocket", test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NullName_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, nullptr, "websocket", test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NullDetector_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "test", "websocket", nullptr, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NullHandler_ReturnsError) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "test", "websocket", test_detector, nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_NameTooLong_ReturnsError) {
    char long_name[64];
    memset(long_name, 'a', sizeof(long_name) - 1);
    long_name[sizeof(long_name) - 1] = '\0';
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, long_name, nullptr, test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_UpgradeHeaderTooLong_ReturnsError) {
    char long_header[128];
    memset(long_header, 'b', sizeof(long_header) - 1);
    long_header[sizeof(long_header) - 1] = '\0';
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "test", long_header, test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Register_Valid_CreatesRegistry) {
    // Server pre-registers websocket, so use a different name
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "custom-proto", "custom-upgrade", test_detector, test_handler, nullptr),
              UVHTTP_OK);
    EXPECT_NE(server->protocol_registry, nullptr);
}

TEST_F(ProtocolUpgradeTest, Register_NullUpgradeHeader_Success) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "custom", nullptr, test_detector, test_handler, nullptr),
              UVHTTP_OK);
}

TEST_F(ProtocolUpgradeTest, Register_DuplicateName_ReturnsAlreadyExists) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "myproto", "myupgrade", test_detector, test_handler, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "myproto", "myupgrade", test_detector, test_handler, nullptr),
              UVHTTP_ERROR_ALREADY_EXISTS);
}

TEST_F(ProtocolUpgradeTest, Register_NameNormalizedToLowercase) {
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "MyCustomProto", "MyUpgradeHeader", test_detector, test_handler, nullptr),
              UVHTTP_OK);

    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    uvhttp_protocol_info_t* info = reg->protocols;
    ASSERT_NE(info, nullptr);
    EXPECT_STREQ(info->name, "mycustomproto");
    EXPECT_STREQ(info->upgrade_header, "myupgradeheader");
}

TEST_F(ProtocolUpgradeTest, Register_MultipleProtocols_InsertedAtHead) {
    // Server already has websocket registered, so count starts at 1
    size_t initial_count = 0;
    {
        uvhttp_protocol_registry_t* reg =
            (uvhttp_protocol_registry_t*)server->protocol_registry;
        if (reg) initial_count = reg->protocol_count;
    }

    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "proto1", "upgrade1", test_detector, test_handler, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "proto2", "upgrade2", test_detector, test_handler, (void*)0x42),
              UVHTTP_OK);

    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    EXPECT_EQ(reg->protocol_count, initial_count + 2);

    // Newest protocol is at head
    EXPECT_STREQ(reg->protocols->name, "proto2");
    EXPECT_EQ(reg->protocols->user_data, (void*)0x42);
}

TEST_F(ProtocolUpgradeTest, Register_MaxProtocols_ReturnsError) {
    // Server already has websocket registered (1), register 9 more to reach 10
    for (int i = 0; i < 9; i++) {
        char name[32];
        snprintf(name, sizeof(name), "proto%d", i);
        EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                      server, name, nullptr, test_detector, test_handler, nullptr),
                  UVHTTP_OK);
    }
    // 11th total (10 + 1 existing) should fail
    EXPECT_EQ(uvhttp_server_register_protocol_upgrade(
                  server, "proto_extra", nullptr, test_detector, test_handler, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_server_unregister_protocol_upgrade ==========

TEST_F(ProtocolUpgradeTest, Unregister_NullServer_ReturnsError) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(nullptr, "test"),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Unregister_NullName_ReturnsError) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, Unregister_NoRegistry_ReturnsNotFound) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "test"),
              UVHTTP_ERROR_NOT_FOUND);
}

TEST_F(ProtocolUpgradeTest, Unregister_NotFound_ReturnsNotFound) {
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "nonexistent"),
              UVHTTP_ERROR_NOT_FOUND);
}

TEST_F(ProtocolUpgradeTest, Unregister_Valid_RemovesProtocol) {
    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    size_t initial = reg->protocol_count;

    uvhttp_server_register_protocol_upgrade(
        server, "tempproto", "tempupgrade", test_detector, test_handler, nullptr);
    EXPECT_EQ(reg->protocol_count, initial + 1);

    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "tempproto"),
              UVHTTP_OK);
    EXPECT_EQ(reg->protocol_count, initial);
}

TEST_F(ProtocolUpgradeTest, Unregister_MiddleOfList_Works) {
    uvhttp_server_register_protocol_upgrade(
        server, "aaa", nullptr, test_detector, test_handler, nullptr);
    uvhttp_server_register_protocol_upgrade(
        server, "bbb", nullptr, test_detector, test_handler, nullptr);
    uvhttp_server_register_protocol_upgrade(
        server, "ccc", nullptr, test_detector, test_handler, nullptr);

    // List is: ccc -> bbb -> aaa -> (existing protocols)
    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    size_t count_before = reg->protocol_count;

    // Remove middle
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "bbb"),
              UVHTTP_OK);
    EXPECT_EQ(reg->protocol_count, count_before - 1);
    EXPECT_STREQ(reg->protocols->name, "ccc");
}

TEST_F(ProtocolUpgradeTest, Unregister_HeadOfList_Works) {
    uvhttp_server_register_protocol_upgrade(
        server, "first", nullptr, test_detector, test_handler, nullptr);
    uvhttp_server_register_protocol_upgrade(
        server, "second", nullptr, test_detector, test_handler, nullptr);

    uvhttp_protocol_registry_t* reg =
        (uvhttp_protocol_registry_t*)server->protocol_registry;
    size_t count_before = reg->protocol_count;

    // Remove head (second, which was inserted last)
    EXPECT_EQ(uvhttp_server_unregister_protocol_upgrade(server, "second"),
              UVHTTP_OK);
    EXPECT_EQ(reg->protocol_count, count_before - 1);
    EXPECT_STREQ(reg->protocols->name, "first");
}

// ========== uvhttp_connection_set_lifecycle ==========

TEST_F(ProtocolUpgradeTest, SetLifecycle_NullConn_ReturnsError) {
    uvhttp_connection_lifecycle_t lifecycle = {nullptr, nullptr};
    EXPECT_EQ(uvhttp_connection_set_lifecycle(nullptr, &lifecycle),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, SetLifecycle_NullLifecycle_ReturnsError) {
    // Need a non-null conn pointer - use stack address as dummy
    // This will crash if the function tries to access conn fields
    // so we only test the null lifecycle path
    // Actually, the function checks !conn first, so we need a valid conn
    // Just test null lifecycle
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    EXPECT_EQ(uvhttp_connection_set_lifecycle(&dummy_conn, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_get_fd ==========

TEST_F(ProtocolUpgradeTest, GetFd_NullConn_ReturnsError) {
    int fd = 0;
    EXPECT_EQ(uvhttp_connection_get_fd(nullptr, &fd), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, GetFd_NullFd_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    EXPECT_EQ(uvhttp_connection_get_fd(&dummy_conn, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_get_peer_address ==========

TEST_F(ProtocolUpgradeTest, GetPeerAddress_NullConn_ReturnsError) {
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);
    EXPECT_EQ(uvhttp_connection_get_peer_address(nullptr, &addr, &len),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, GetPeerAddress_NullAddr_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    socklen_t len = sizeof(struct sockaddr_storage);
    EXPECT_EQ(uvhttp_connection_get_peer_address(&dummy_conn, nullptr, &len),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, GetPeerAddress_NullLen_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    struct sockaddr_storage addr;
    EXPECT_EQ(uvhttp_connection_get_peer_address(&dummy_conn, &addr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_connection_transfer_ownership ==========

TEST_F(ProtocolUpgradeTest, TransferOwnership_NullConn_ReturnsError) {
    EXPECT_EQ(uvhttp_connection_transfer_ownership(nullptr, nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ProtocolUpgradeTest, TransferOwnership_NullCallback_ReturnsError) {
    uvhttp_connection_t dummy_conn;
    memset(&dummy_conn, 0, sizeof(dummy_conn));
    EXPECT_EQ(uvhttp_connection_transfer_ownership(&dummy_conn, nullptr, nullptr),
              UVHTTP_ERROR_INVALID_PARAM);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
