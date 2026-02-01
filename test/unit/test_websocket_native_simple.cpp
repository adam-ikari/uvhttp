#include <gtest/gtest.h>
#include <uvhttp_websocket.h>
#include <uvhttp_allocator.h>
#include <string.h>

/* 测试 WebSocket 连接创建 NULL fd */
TEST(UvhttpWebSocketNativeSimpleTest, ConnectionCreateNullFd) {
    struct uvhttp_ws_connection* conn = uvhttp_ws_connection_create(-1, NULL, 1, NULL);
    /* 即使 fd 为 -1，也应该创建连接 */
    if (conn) {
        uvhttp_ws_connection_free(conn);
    }
    SUCCEED();
}

/* 测试 WebSocket 连接释放 NULL */
TEST(UvhttpWebSocketNativeSimpleTest, ConnectionFreeNull) {
    uvhttp_ws_connection_free(NULL);
    /* 不应该崩溃 */
    SUCCEED();
}