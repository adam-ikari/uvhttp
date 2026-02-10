/**
 * @file libuv_mock.h
 * @brief libuv Mock 库 - 用于单元测试
 * 
 * 这个库通过链接时符号替换（linker wrap）来 mock libuv 函数，
 * 实现零开销的测试隔离。
 * 
 * 使用方式：
 * 1. 编译测试时链接此库
 * 2. 使用 --wrap=uv_xxx 选项替换 libuv 函数
 * 3. 测试代码通过 mock 函数控制行为
 */

#ifndef LIBUV_MOCK_H
#define LIBUV_MOCK_H

#include <uv.h>
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ========== Mock 控制接口 ========== */

/**
 * @brief 重置所有 mock 状态
 */
void libuv_mock_reset(void);

/**
 * @brief 设置是否启用 mock
 * @param enabled true 启用 mock，false 使用真实 libuv
 */
void libuv_mock_set_enabled(bool enabled);

/**
 * @brief 设置是否记录所有调用
 * @param record true 记录调用，false 不记录
 */
void libuv_mock_set_record_calls(bool record);

/**
 * @brief 获取调用记录
 * @param func_name 函数名
 * @param call_count 返回调用次数
 */
void libuv_mock_get_call_count(const char* func_name, size_t* call_count);

/* ========== uv_loop_t Mock ========== */

/**
 * @brief 设置 uv_loop_init 的返回值
 */
void libuv_mock_set_uv_loop_init_result(int result);

/**
 * @brief 设置 uv_loop_close 的返回值
 */
void libuv_mock_set_uv_loop_close_result(int result);

/**
 * @brief 设置 uv_run 的返回值
 */
void libuv_mock_set_uv_run_result(int result);

/**
 * @brief 设置 uv_run 的运行模式
 */
void libuv_mock_set_uv_run_mode(uv_run_mode mode);

/* ========== uv_tcp_t Mock ========== */

/**
 * @brief 设置 uv_tcp_init 的返回值
 */
void libuv_mock_set_uv_tcp_init_result(int result);

/**
 * @brief 设置 uv_tcp_bind 的返回值
 */
void libuv_mock_set_uv_tcp_bind_result(int result);

/**
 * @brief 设置 uv_listen 的返回值
 */
void libuv_mock_set_uv_listen_result(int result);

/**
 * @brief 设置 uv_tcp_accept 的返回值
 */
void libuv_mock_set_uv_tcp_accept_result(int result);

/**
 * @brief 设置 uv_tcp_connect 的返回值
 */
void libuv_mock_set_uv_tcp_connect_result(int result);

/* ========== uv_stream_t Mock ========== */

/**
 * @brief 设置 uv_read_start 的返回值
 */
void libuv_mock_set_uv_read_start_result(int result);

/**
 * @brief 设置 uv_read_stop 的返回值
 */
void libuv_mock_set_uv_read_stop_result(int result);

/**
 * @brief 设置 uv_write 的返回值
 */
void libuv_mock_set_uv_write_result(int result);

/**
 * @brief 设置 uv_close 的回调延迟（毫秒）
 */
void libuv_mock_set_uv_close_delay(unsigned int delay_ms);

/* ========== uv_handle_t Mock ========== */

/**
 * @brief 设置 uv_is_active 的返回值
 */
void libuv_mock_set_uv_is_active_result(int result);

/**
 * @brief 设置 uv_is_closing 的返回值
 */
void libuv_mock_set_uv_is_closing_result(int result);

/* ========== uv_idle_t Mock ========== */

/**
 * @brief 设置 uv_idle_init 的返回值
 */
void libuv_mock_set_uv_idle_init_result(int result);

/* ========== uv_timer_t Mock ========== */

/**
 * @brief 设置 uv_timer_init 的返回值
 */
void libuv_mock_set_uv_timer_init_result(int result);

/**
 * @brief 设置 uv_timer_stop 的返回值
 */
void libuv_mock_set_uv_timer_stop_result(int result);

/**
 * @brief 设置 uv_timer_start 的返回值
 */
void libuv_mock_set_uv_timer_start_result(int result);

/* ========== uv_tcp_t Mock ========== */

/**
 * @brief 设置 uv_tcp_nodelay 的返回值
 */
void libuv_mock_set_uv_tcp_nodelay_result(int result);

/**
 * @brief 设置 uv_tcp_keepalive 的返回值
 */
void libuv_mock_set_uv_tcp_keepalive_result(int result);

/**
 * @brief 设置 uv_tcp_simultaneous_accepts 的返回值
 */
void libuv_mock_set_uv_tcp_simultaneous_accepts_result(int result);

/* ========== uv_stream_t Mock ========== */

/**
 * @brief 设置 uv_accept 的返回值
 */
void libuv_mock_set_uv_accept_result(int result);

/* ========== uv_handle_t Mock ========== */

/**
 * @brief 设置 uv_fileno 的返回值
 */
void libuv_mock_set_uv_fileno_result(int result);

/* ========== uv_util_t Mock ========== */

/**
 * @brief 设置 uv_ip4_addr 的返回值
 */
void libuv_mock_set_uv_ip4_addr_result(int result);

/**
 * @brief 设置 uv_ip6_addr 的返回值
 */
void libuv_mock_set_uv_ip6_addr_result(int result);

/**
 * @brief 设置 uv_strerror 返回的错误消息
 */
void libuv_mock_set_uv_strerror(const char* error_msg);

/**
 * @brief 设置 uv_hrtime 返回的时间戳
 */
void libuv_mock_set_uv_hrtime(uint64_t hrtime);

/**
 * @brief 设置 uv_buf_init 返回的缓冲区
 */
void libuv_mock_set_uv_buf_init_result(uv_buf_t buf);

/* ========== uv_buf_t Mock ========== */

/**
 * @brief 设置 alloc_cb 分配的缓冲区大小
 */
void libuv_mock_set_alloc_buffer_size(size_t size);

/**
 * @brief 设置是否触发 alloc_cb 回调
 */
void libuv_mock_set_trigger_alloc_cb(bool trigger);

/**
 * @brief 设置是否触发 read_cb 回调
 */
void libuv_mock_set_trigger_read_cb(bool trigger);

/**
 * @brief 设置 read_cb 返回的数据
 */
void libuv_mock_set_read_data(const char* data, size_t len);

/* ========== 回调控制 ========== */

/**
 * @brief 手动触发 connection_cb 回调
 */
void libuv_mock_trigger_connection_cb(uv_stream_t* server, int status);

/**
 * @brief 手动触发 read_cb 回调
 */
void libuv_mock_trigger_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf);

/**
 * @brief 手动触发 write_cb 回调
 */
void libuv_mock_trigger_write_cb(uv_write_t* req, int status);

/**
 * @brief 手动触发 close_cb 回调
 */
void libuv_mock_trigger_close_cb(uv_handle_t* handle);

/* ========== 错误模拟 ========== */

/**
 * @brief 设置下一次调用是否返回错误
 * @param error_code 错误码，0 表示成功
 */
void libuv_mock_set_next_error(int error_code);

/**
 * @brief 设置特定函数的返回值
 * @param func_name 函数名
 * @param result 返回值
 */
void libuv_mock_set_function_result(const char* func_name, int result);

#ifdef __cplusplus
}
#endif

#endif /* LIBUV_MOCK_H */