/* UVHTTP 服务器模块 - 简单 API 覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_allocator.h"

/* 测试 uvhttp_server_enable_tls 函数 */
TEST(UvhttpServerSimpleAPITest, EnableTlsNullServer) {
    /* 测试 NULL 服务器 */
    uvhttp_tls_context_t* tls_ctx = NULL;
    int result = uvhttp_server_enable_tls(NULL, tls_ctx);
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_disable_tls 函数 */
TEST(UvhttpServerSimpleAPITest, DisableTlsNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_disable_tls(NULL);
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_run 函数 */
TEST(UvhttpServerSimpleAPITest, RunNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_run(NULL);
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_simple_free 函数 */
TEST(UvhttpServerSimpleAPITest, SimpleFreeNull) {
    /* 测试 NULL 服务器 */
    uvhttp_server_simple_free(NULL);
    /* 不应该崩溃 */
}

/* 测试 uvhttp_server_stop_simple 函数 */
TEST(UvhttpServerSimpleAPITest, StopSimpleNull) {
    /* 测试 NULL 服务器 */
    uvhttp_server_stop_simple(NULL);
    /* 不应该崩溃 */
}

/* 测试 uvhttp_serve 函数 */
TEST(UvhttpServerSimpleAPITest, ServeNullServer) {
    /* 测试 NULL loop */
    int result = uvhttp_serve(NULL, NULL, 8080);
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_ws_get_connection_count 函数 */
TEST(UvhttpServerSimpleAPITest, WsGetConnectionCountNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_ws_get_connection_count(NULL);
    /* 应该返回 0 */
    EXPECT_EQ(result, 0);
}

/* 测试 uvhttp_server_ws_get_connection_count_by_path 函数 */
TEST(UvhttpServerSimpleAPITest, WsGetConnectionCountByPathNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_ws_get_connection_count_by_path(NULL, "/ws");
    /* 应该返回 0 */
    EXPECT_EQ(result, 0);
}

/* 测试 uvhttp_server_ws_close_all 函数 */
TEST(UvhttpServerSimpleAPITest, WsCloseAllNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_ws_close_all(NULL, "Normal closure");
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_ws_broadcast 函数 */
TEST(UvhttpServerSimpleAPITest, WsBroadcastNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_ws_broadcast(NULL, "/ws", "Hello, World!", 13);
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_ws_send 函数 */
TEST(UvhttpServerSimpleAPITest, WsSendNullServer) {
    /* 测试 NULL 服务器 */
    int result = uvhttp_server_ws_send(NULL, "Hello, World!", 13);
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}

/* 测试 uvhttp_server_ws_close 函数 */
TEST(UvhttpServerSimpleAPITest, WsCloseNullServer) {
    /* 测试 NULL 连接 */
    int result = uvhttp_server_ws_close(NULL, 1000, "Normal closure");
    /* 应该返回错误 */
    EXPECT_NE(result, 0);
}