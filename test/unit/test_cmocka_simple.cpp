#include <gtest/gtest.h>
#include <uv.h>

/* 简化的 mock 状态管理 */
struct MockState {
    int uv_tcp_init_result;
    int uv_tcp_bind_result;
    int uv_listen_result;
    size_t uv_tcp_init_count;
    size_t uv_tcp_bind_count;
    size_t uv_listen_count;
};

static MockState g_mock_state = {0, 0, 0, 0, 0, 0};

/* Mock 控制函数 */
void mock_reset() {
    g_mock_state = {0, 0, 0, 0, 0, 0};
}

void mock_set_uv_tcp_init_result(int result) {
    g_mock_state.uv_tcp_init_result = result;
}

void mock_set_uv_tcp_bind_result(int result) {
    g_mock_state.uv_tcp_bind_result = result;
}

void mock_set_uv_listen_result(int result) {
    g_mock_state.uv_listen_result = result;
}

size_t mock_get_uv_tcp_init_count() {
    return g_mock_state.uv_tcp_init_count;
}

/* Mock libuv 函数 */
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    g_mock_state.uv_tcp_init_count++;
    return g_mock_state.uv_tcp_init_result;
}

int __wrap_uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags) {
    g_mock_state.uv_tcp_bind_count++;
    return g_mock_state.uv_tcp_bind_result;
}

int __wrap_uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb) {
    g_mock_state.uv_listen_count++;
    return g_mock_state.uv_listen_result;
}

/* 测试用例 */
TEST(SimplifiedMock, TcpInitSuccess) {
    mock_reset();
    mock_set_uv_tcp_init_result(0);
    
    uv_loop_t loop;
    uv_tcp_t handle;
    int result = __wrap_uv_tcp_init(&loop, &handle);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(mock_get_uv_tcp_init_count(), 1);
}

TEST(SimplifiedMock, TcpInitFailure) {
    mock_reset();
    mock_set_uv_tcp_init_result(-1);
    
    uv_loop_t loop;
    uv_tcp_t handle;
    int result = __wrap_uv_tcp_init(&loop, &handle);
    
    EXPECT_EQ(result, -1);
    EXPECT_EQ(mock_get_uv_tcp_init_count(), 1);
}

TEST(SimplifiedMock, MultipleCalls) {
    mock_reset();
    mock_set_uv_tcp_init_result(0);
    
    uv_loop_t loop;
    uv_tcp_t handle1, handle2, handle3;
    
    __wrap_uv_tcp_init(&loop, &handle1);
    __wrap_uv_tcp_init(&loop, &handle2);
    __wrap_uv_tcp_init(&loop, &handle3);
    
    EXPECT_EQ(mock_get_uv_tcp_init_count(), 3);
}

TEST(SimplifiedMock, TcpBindSuccess) {
    mock_reset();
    mock_set_uv_tcp_bind_result(0);
    
    uv_tcp_t handle;
    struct sockaddr_in addr;
    int result = __wrap_uv_tcp_bind(&handle, (struct sockaddr*)&addr, 0);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_mock_state.uv_tcp_bind_count, 1);
}

TEST(SimplifiedMock, ListenSuccess) {
    mock_reset();
    mock_set_uv_listen_result(0);
    
    uv_tcp_t handle;
    int result = __wrap_uv_listen((uv_stream_t*)&handle, 128, NULL);
    
    EXPECT_EQ(result, 0);
    EXPECT_EQ(g_mock_state.uv_listen_count, 1);
}

TEST(SimplifiedMock, CombinedOperations) {
    mock_reset();
    mock_set_uv_tcp_init_result(0);
    mock_set_uv_tcp_bind_result(0);
    mock_set_uv_listen_result(0);
    
    uv_loop_t loop;
    uv_tcp_t handle;
    struct sockaddr_in addr;
    
    __wrap_uv_tcp_init(&loop, &handle);
    __wrap_uv_tcp_bind(&handle, (struct sockaddr*)&addr, 0);
    __wrap_uv_listen((uv_stream_t*)&handle, 128, NULL);
    
    EXPECT_EQ(mock_get_uv_tcp_init_count(), 1);
    EXPECT_EQ(g_mock_state.uv_tcp_bind_count, 1);
    EXPECT_EQ(g_mock_state.uv_listen_count, 1);
}