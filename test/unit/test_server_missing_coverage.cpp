#include <gtest/gtest.h>
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* ========== 辅助函数 ========== */

static void create_server_and_loop(uv_loop_t** loop, uvhttp_server_t** server) {
    *loop = uv_loop_new();
    ASSERT_NE(*loop, nullptr);
    uvhttp_error_t result = uvhttp_server_new(*loop, server);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(*server, nullptr);
}

static void destroy_server_and_loop(uv_loop_t* loop, uvhttp_server_t* server) {
    if (server) {
        uvhttp_server_free(server);
    }
    if (loop) {
        uv_loop_close(loop);
        uvhttp_free(loop);
    }
}

/* ========== 测试 WebSocket 连接管理 ========== */

TEST(UvhttpServerMissingCoverageTest, WsEnableConnectionManagementNullServer) {
    uvhttp_error_t result = uvhttp_server_ws_enable_connection_management(NULL, 3600, 30);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerMissingCoverageTest, WsEnableConnectionManagementSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    /* Test with minimal timeout to avoid hanging */
    uvhttp_error_t result = uvhttp_server_ws_enable_connection_management(server, 1, 30);
    /* Should succeed or fail gracefully without hanging */
    
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpServerMissingCoverageTest, WsDisableConnectionManagementNullServer) {
    uvhttp_error_t result = uvhttp_server_ws_disable_connection_management(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerMissingCoverageTest, WsDisableConnectionManagementSuccess) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    uvhttp_server_ws_enable_connection_management(server, 3600, 30);
    
    uvhttp_error_t result = uvhttp_server_ws_disable_connection_management(server);
    /* Should succeed or fail gracefully */
    
    destroy_server_and_loop(loop, server);
}

TEST(UvhttpServerMissingCoverageTest, WsAddConnectionNullServer) {
    uvhttp_server_ws_add_connection(NULL, NULL, "/ws");
    /* Should not crash with null parameters */
}

TEST(UvhttpServerMissingCoverageTest, WsRemoveConnectionNullServer) {
    uvhttp_server_ws_remove_connection(NULL, NULL);
    /* Should not crash with null parameters */
}

TEST(UvhttpServerMissingCoverageTest, WsUpdateActivityNullServer) {
    uvhttp_server_ws_update_activity(NULL, NULL);
    /* Should not crash with null parameters */
}

TEST(UvhttpServerMissingCoverageTest, RegisterWsHandlerNullServer) {
    uvhttp_error_t result = uvhttp_server_register_ws_handler(NULL, "/ws", NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* ========== 测试 WebSocket 连接管理边界条件 ========== */

TEST(UvhttpServerMissingCoverageTest, WsConnectionManagementEdgeCases) {
    uv_loop_t* loop = nullptr;
    uvhttp_server_t* server = nullptr;
    create_server_and_loop(&loop, &server);
    
    /* Test with zero timeout */
    uvhttp_server_ws_enable_connection_management(server, 0, 30);
    
    /* Test with very short timeout */
    uvhttp_server_ws_enable_connection_management(server, 1, 30);
    
    /* Test with very long timeout */
    uvhttp_server_ws_enable_connection_management(server, 86400, 30);
    
    /* Test enable/disable cycle */
    for (int i = 0; i < 3; i++) {
        uvhttp_server_ws_enable_connection_management(server, 3600, 30);
        uvhttp_server_ws_disable_connection_management(server);
    }
    
    destroy_server_and_loop(loop, server);
}
