/* uvhttp_connection.c 简单 mock 测试 - 验证 mock 框架工作 */

#include <gtest/gtest.h>
#include "uvhttp_connection.h"
#include "uvhttp_server.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include "libuv_mock.h"
#include <string.h>

/* ========== Mock 控制辅助函数 ========== */

static void setup_mock_loop(void) {
    libuv_mock_reset();
    libuv_mock_set_enabled(true);
    
    /* 设置默认返回值 */
    libuv_mock_set_uv_loop_init_result(0);
    libuv_mock_set_uv_loop_close_result(0);
    libuv_mock_set_uv_run_result(0);
    libuv_mock_set_uv_tcp_init_result(0);
    libuv_mock_set_uv_tcp_bind_result(0);
    libuv_mock_set_uv_listen_result(0);
    libuv_mock_set_uv_read_start_result(0);
    libuv_mock_set_uv_read_stop_result(0);
    libuv_mock_set_uv_write_result(0);
    libuv_mock_set_uv_is_active_result(0);
    libuv_mock_set_uv_is_closing_result(0);
    libuv_mock_set_uv_idle_init_result(0);
    libuv_mock_set_uv_timer_init_result(0);
}

static uv_loop_t* create_mock_loop(void) {
    uv_loop_t* loop = (uv_loop_t*)uvhttp_alloc(sizeof(uv_loop_t));
    if (!loop) {
        return nullptr;
    }
    memset(loop, 0, sizeof(uv_loop_t));
    return loop;
}

static void destroy_mock_loop(uv_loop_t* loop) {
    if (loop) {
        uvhttp_free(loop);
    }
}

/* ========== 创建一个最小化的 server 用于测试 ========== */

static uvhttp_server_t* create_minimal_server(uv_loop_t* loop) {
    uvhttp_server_t* server = (uvhttp_server_t*)uvhttp_alloc(sizeof(uvhttp_server_t));
    if (!server) {
        return nullptr;
    }
    memset(server, 0, sizeof(uvhttp_server_t));
    server->loop = loop;
    return server;
}

/* ========== 测试连接创建和释放 ========== */

TEST(UvhttpConnectionSimpleMockTest, CreateAndDestroy) {
    setup_mock_loop();
    
    uv_loop_t* loop = create_mock_loop();
    ASSERT_NE(loop, nullptr);
    
    uvhttp_server_t* server = create_minimal_server(loop);
    ASSERT_NE(server, nullptr);
    
    uvhttp_connection_t* conn = nullptr;
    uvhttp_error_t result = uvhttp_connection_new(server, &conn);
    ASSERT_EQ(result, UVHTTP_OK);
    ASSERT_NE(conn, nullptr);
    
    /* 验证连接属性 */
    EXPECT_EQ(conn->server, server);
    EXPECT_EQ(conn->state, UVHTTP_CONN_STATE_NEW);
    
    uvhttp_connection_free(conn);
    uvhttp_free(server);
    destroy_mock_loop(loop);
}