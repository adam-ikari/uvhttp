/*
 * 使用 cmocka 进行 libuv 函数 mock 的示例
 * 
 * 这个示例展示了如何使用 cmocka 来 mock libuv 函数，
 * 比手动实现 mock 库更简洁、更强大。
 */

#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

/* Mock libuv 函数声明 */
int __wrap_uv_loop_init(uv_loop_t* loop);
int __wrap_uv_loop_close(uv_loop_t* loop);
int __wrap_uv_run(uv_loop_t* loop, uv_run_mode mode);
int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle);
int __wrap_uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags);
int __wrap_uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb);
int __wrap_uv_tcp_accept(uv_tcp_t* server, uv_tcp_t* client);
int __wrap_uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb);
int __wrap_uv_read_stop(uv_stream_t* stream);
int __wrap_uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb);
void __wrap_uv_close(uv_handle_t* handle, uv_close_cb close_cb);
int __wrap_uv_is_active(const uv_handle_t* handle);
int __wrap_uv_is_closing(const uv_handle_t* handle);

/* 测试用例：使用 cmocka mock libuv 函数 */
static void test_uv_tcp_server_with_cmocka(void** state) {
    (void)state;
    
    /* 设置 mock 返回值 */
    will_return(__wrap_uv_loop_init, 0);
    will_return(__wrap_uv_tcp_init, 0);
    will_return(__wrap_uv_tcp_bind, 0);
    will_return(__wrap_uv_listen, 0);
    will_return(__wrap_uv_run, 0);
    will_return(__wrap_uv_loop_close, 0);
    
    /* 设置参数期望 */
    expect_value(__wrap_uv_listen, backlog, 128);
    
    /* 运行测试代码 */
    uv_loop_t loop;
    uv_tcp_t server;
    
    /* 这些调用会被 cmocka 拦截 */
    uv_loop_init(&loop);
    uv_tcp_init(&loop, &server);
    
    struct sockaddr_in addr;
    uv_tcp_bind(&server, (struct sockaddr*)&addr, 0);
    uv_listen((uv_stream_t*)&server, 128, NULL);
    
    uv_run(&loop, UV_RUN_DEFAULT);
    uv_loop_close(&loop);
}

/* 测试用例：验证函数调用次数 */
static void test_function_call_count(void** state) {
    (void)state;
    
    /* 设置 mock 返回值 */
    will_return(__wrap_uv_tcp_init, 0);
    will_return(__wrap_uv_tcp_init, 0);
    
    /* 运行测试代码 */
    uv_loop_t loop;
    uv_tcp_t server1, server2;
    
    uv_tcp_init(&loop, &server1);
    uv_tcp_init(&loop, &server2);
    
    /* cmocka 会自动验证调用次数 */
}

/* 测试用例：模拟错误 */
static void test_error_simulation(void** state) {
    (void)state;
    
    /* 设置 mock 返回错误 */
    will_return(__wrap_uv_tcp_init, UV_ENOMEM);
    
    /* 运行测试代码 */
    uv_loop_t loop;
    uv_tcp_t server;
    
    int result = uv_tcp_init(&loop, &server);
    
    /* 验证返回值 */
    assert_int_equal(result, UV_ENOMEM);
}

/* 测试用例：验证回调参数 */
static void test_callback_parameter(void** state) {
    (void)state;
    
    /* 设置 mock 返回值 */
    will_return(__wrap_uv_listen, 0);
    
    /* 期望回调参数不为 NULL */
    expect_check(__wrap_uv_listen, cb, mock_assert_not_null, NULL);
    
    /* 运行测试代码 */
    uv_loop_t loop;
    uv_tcp_t server;
    
    uv_tcp_init(&loop, &server);
    uv_listen((uv_stream_t*)&server, 128, some_connection_callback);
}

/* cmocka 测试组 */
int main(void) {
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_uv_tcp_server_with_cmocka),
        cmocka_unit_test(test_function_call_count),
        cmocka_unit_test(test_error_simulation),
        cmocka_unit_test(test_callback_parameter),
    };
    
    return cmocka_run_group_tests(tests, NULL, NULL);
}

/* Mock 函数实现 - cmocka 会自动生成这些函数的包装器 */
int __wrap_uv_loop_init(uv_loop_t* loop) {
    (void)loop;
    return mock_type(int);
}

int __wrap_uv_loop_close(uv_loop_t* loop) {
    (void)loop;
    return mock_type(int);
}

int __wrap_uv_run(uv_loop_t* loop, uv_run_mode mode) {
    (void)loop;
    (void)mode;
    return mock_type(int);
}

int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    (void)loop;
    (void)handle;
    return mock_type(int);
}

int __wrap_uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags) {
    (void)handle;
    (void)addr;
    (void)flags;
    return mock_type(int);
}

int __wrap_uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb) {
    (void)stream;
    (void)cb;
    check_expected(backlog);
    return mock_type(int);
}

int __wrap_uv_tcp_accept(uv_tcp_t* server, uv_tcp_t* client) {
    (void)server;
    (void)client;
    return mock_type(int);
}

int __wrap_uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb) {
    (void)stream;
    (void)alloc_cb;
    (void)read_cb;
    return mock_type(int);
}

int __wrap_uv_read_stop(uv_stream_t* stream) {
    (void)stream;
    return mock_type(int);
}

int __wrap_uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t bufs[], unsigned int nbufs, uv_write_cb cb) {
    (void)req;
    (void)handle;
    (void)bufs;
    (void)nbufs;
    (void)cb;
    return mock_type(int);
}

void __wrap_uv_close(uv_handle_t* handle, uv_close_cb close_cb) {
    (void)handle;
    (void)close_cb;
    mock();
}

int __wrap_uv_is_active(const uv_handle_t* handle) {
    (void)handle;
    return mock_type(int);
}

int __wrap_uv_is_closing(const uv_handle_t* handle) {
    (void)handle;
    return mock_type(int);
}