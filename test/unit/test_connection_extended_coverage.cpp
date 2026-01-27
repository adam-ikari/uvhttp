/* uvhttp_connection.c 扩展覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include <uv.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"
#include "test_loop_helper.h"

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionTlsHandshakeFunc) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());

    uvhttp_server_t* server = (uvhttp_server_t*)uvhttp_alloc(sizeof(uvhttp_server_t));
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop.get();

    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    ASSERT_NE(conn, nullptr);

    EXPECT_EQ(uvhttp_connection_tls_handshake_func(conn), -1);

    // 手动关闭句柄
    uv_idle_stop(&conn->idle_handle);
    uv_close((uv_handle_t*)&conn->idle_handle, NULL);
    uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
    
    // 运行循环以处理关闭回调
    loop.run_once();
    
    // 清理请求和响应数据
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
    
    // 释放连接内存
    uvhttp_free(conn);
    uvhttp_free(server);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionStateTransitions) {
    TestLoop loop;
    ASSERT_TRUE(loop.is_valid());

    uvhttp_server_t* server = (uvhttp_server_t*)uvhttp_alloc(sizeof(uvhttp_server_t));
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop.get();

    uvhttp_connection_t* conn = uvhttp_connection_new(server);
    ASSERT_NE(conn, nullptr);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_READING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_READING);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_HTTP_PROCESSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_HTTP_PROCESSING);

    uvhttp_connection_set_state(conn, UVHTTP_CONN_STATE_CLOSING);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_CLOSING);

    // 手动关闭句柄
    uv_idle_stop(&conn->idle_handle);
    uv_close((uv_handle_t*)&conn->idle_handle, NULL);
    uv_close((uv_handle_t*)&conn->tcp_handle, NULL);
    
    // 运行循环以处理关闭回调
    loop.run_once();
    
    // 清理请求和响应数据
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
    
    // 释放连接内存
    uvhttp_free(conn);
    uvhttp_free(server);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionRestartReadWithNull) {
    EXPECT_EQ(uvhttp_connection_restart_read(nullptr), -1);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionScheduleRestartReadWithNull) {
    EXPECT_EQ(uvhttp_connection_schedule_restart_read(nullptr), -1);
}

TEST(UvhttpConnectionExtendedCoverageTest, ConnectionStartWithNull) {
    EXPECT_EQ(uvhttp_connection_start(nullptr), -1);
}