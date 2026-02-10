/**
 * @file libuv_mock.c
 * @brief libuv Mock 库实现
 * 
 * 使用链接时符号替换（linker wrap）来 mock libuv 函数
 */

#include "libuv_mock.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ========== 全局状态 ========== */

typedef struct {
    bool enabled;
    bool record_calls;
    
    /* 调用计数 */
    size_t uv_loop_init_count;
    size_t uv_loop_close_count;
    size_t uv_run_count;
    size_t uv_tcp_init_count;
    size_t uv_tcp_bind_count;
    size_t uv_listen_count;
    size_t uv_tcp_accept_count;
    size_t uv_tcp_connect_count;
    size_t uv_read_start_count;
    size_t uv_read_stop_count;
    size_t uv_write_count;
    size_t uv_close_count;
    size_t uv_is_active_count;
    size_t uv_is_closing_count;
    
    /* 返回值设置 */
    int uv_loop_init_result;
    int uv_loop_close_result;
    int uv_run_result;
    uv_run_mode uv_run_mode;
    int uv_tcp_init_result;
    int uv_tcp_bind_result;
    int uv_listen_result;
    int uv_tcp_accept_result;
    int uv_tcp_connect_result;
    int uv_read_start_result;
    int uv_read_stop_result;
    int uv_write_result;
    int uv_is_active_result;
    int uv_is_closing_result;
    int uv_idle_init_result;
    int uv_timer_init_result;
    int uv_timer_stop_result;
    int uv_timer_start_result;
    int uv_tcp_nodelay_result;
    int uv_tcp_keepalive_result;
    int uv_tcp_simultaneous_accepts_result;
    int uv_accept_result;
    int uv_fileno_result;
    int uv_ip4_addr_result;
    int uv_ip6_addr_result;
    unsigned int uv_close_delay_ms;
    
    /* 缓冲区设置 */
    size_t alloc_buffer_size;
    bool trigger_alloc_cb;
    bool trigger_read_cb;
    char* read_data;
    size_t read_data_len;
    
    /* 下一次错误 */
    int next_error;
    
    /* 回调保存 */
    uv_connection_cb connection_cb;
    uv_alloc_cb alloc_cb;
    uv_read_cb read_cb;
    uv_write_cb write_cb;
    uv_close_cb close_cb;
    
    /* 保存的句柄 */
    uv_tcp_t* last_tcp;
    uv_stream_t* last_stream;
    uv_loop_t* last_loop;
    
} libuv_mock_state_t;

static libuv_mock_state_t g_mock_state = {
    .enabled = true,
    .record_calls = true,
    .uv_loop_init_result = 0,
    .uv_loop_close_result = 0,
    .uv_run_result = 0,
    .uv_run_mode = UV_RUN_DEFAULT,
    .uv_tcp_init_result = 0,
    .uv_tcp_bind_result = 0,
    .uv_listen_result = 0,
    .uv_tcp_accept_result = 0,
    .uv_tcp_connect_result = 0,
    .uv_read_start_result = 0,
    .uv_read_stop_result = 0,
    .uv_write_result = 0,
    .uv_is_active_result = 1,
    .uv_is_closing_result = 0,
    .uv_idle_init_result = 0,
    .uv_timer_init_result = 0,
    .uv_timer_stop_result = 0,
    .uv_timer_start_result = 0,
    .uv_tcp_nodelay_result = 0,
    .uv_tcp_keepalive_result = 0,
    .uv_tcp_simultaneous_accepts_result = 0,
    .uv_accept_result = 0,
    .uv_fileno_result = 0,
    .uv_ip4_addr_result = 0,
    .uv_ip6_addr_result = 0,
    .uv_close_delay_ms = 0,
    .alloc_buffer_size = 4096,
    .trigger_alloc_cb = true,
    .trigger_read_cb = false,
    .read_data = NULL,
    .read_data_len = 0,
    .next_error = 0,
    .connection_cb = NULL,
    .alloc_cb = NULL,
    .read_cb = NULL,
    .write_cb = NULL,
    .close_cb = NULL,
    .last_tcp = NULL,
    .last_stream = NULL,
    .last_loop = NULL,
};

/* ========== Mock 控制函数 ========== */

void libuv_mock_reset(void) {
    memset(&g_mock_state, 0, sizeof(g_mock_state));
    g_mock_state.enabled = true;
    g_mock_state.record_calls = true;
    g_mock_state.alloc_buffer_size = 4096;
    g_mock_state.trigger_alloc_cb = true;
    g_mock_state.trigger_read_cb = false;
}

void libuv_mock_set_enabled(bool enabled) {
    g_mock_state.enabled = enabled;
}

void libuv_mock_set_record_calls(bool record) {
    g_mock_state.record_calls = record;
}

void libuv_mock_get_call_count(const char* func_name, size_t* call_count) {
    if (!call_count) return;
    
    if (strcmp(func_name, "uv_loop_init") == 0) {
        *call_count = g_mock_state.uv_loop_init_count;
    } else if (strcmp(func_name, "uv_loop_close") == 0) {
        *call_count = g_mock_state.uv_loop_close_count;
    } else if (strcmp(func_name, "uv_run") == 0) {
        *call_count = g_mock_state.uv_run_count;
    } else if (strcmp(func_name, "uv_tcp_init") == 0) {
        *call_count = g_mock_state.uv_tcp_init_count;
    } else if (strcmp(func_name, "uv_tcp_bind") == 0) {
        *call_count = g_mock_state.uv_tcp_bind_count;
    } else if (strcmp(func_name, "uv_listen") == 0) {
        *call_count = g_mock_state.uv_listen_count;
    } else if (strcmp(func_name, "uv_tcp_accept") == 0) {
        *call_count = g_mock_state.uv_tcp_accept_count;
    } else if (strcmp(func_name, "uv_tcp_connect") == 0) {
        *call_count = g_mock_state.uv_tcp_connect_count;
    } else if (strcmp(func_name, "uv_read_start") == 0) {
        *call_count = g_mock_state.uv_read_start_count;
    } else if (strcmp(func_name, "uv_read_stop") == 0) {
        *call_count = g_mock_state.uv_read_stop_count;
    } else if (strcmp(func_name, "uv_write") == 0) {
        *call_count = g_mock_state.uv_write_count;
    } else if (strcmp(func_name, "uv_close") == 0) {
        *call_count = g_mock_state.uv_close_count;
    } else if (strcmp(func_name, "uv_is_active") == 0) {
        *call_count = g_mock_state.uv_is_active_count;
    } else if (strcmp(func_name, "uv_is_closing") == 0) {
        *call_count = g_mock_state.uv_is_closing_count;
    } else {
        *call_count = 0;
    }
}

/* ========== 返回值设置函数 ========== */

void libuv_mock_set_uv_loop_init_result(int result) {
    g_mock_state.uv_loop_init_result = result;
}

void libuv_mock_set_uv_loop_close_result(int result) {
    g_mock_state.uv_loop_close_result = result;
}

void libuv_mock_set_uv_run_result(int result) {
    g_mock_state.uv_run_result = result;
}

void libuv_mock_set_uv_run_mode(uv_run_mode mode) {
    g_mock_state.uv_run_mode = mode;
}

void libuv_mock_set_uv_tcp_init_result(int result) {
    g_mock_state.uv_tcp_init_result = result;
}

void libuv_mock_set_uv_tcp_bind_result(int result) {
    g_mock_state.uv_tcp_bind_result = result;
}

void libuv_mock_set_uv_listen_result(int result) {
    g_mock_state.uv_listen_result = result;
}

void libuv_mock_set_uv_tcp_accept_result(int result) {
    g_mock_state.uv_tcp_accept_result = result;
}

void libuv_mock_set_uv_tcp_connect_result(int result) {
    g_mock_state.uv_tcp_connect_result = result;
}

void libuv_mock_set_uv_read_start_result(int result) {
    g_mock_state.uv_read_start_result = result;
}

void libuv_mock_set_uv_read_stop_result(int result) {
    g_mock_state.uv_read_stop_result = result;
}

void libuv_mock_set_uv_write_result(int result) {
    g_mock_state.uv_write_result = result;
}

void libuv_mock_set_uv_is_active_result(int result) {
    g_mock_state.uv_is_active_result = result;
}

void libuv_mock_set_uv_is_closing_result(int result) {
    g_mock_state.uv_is_closing_result = result;
}

void libuv_mock_set_uv_idle_init_result(int result) {
    g_mock_state.uv_idle_init_result = result;
}

void libuv_mock_set_uv_timer_init_result(int result) {
    g_mock_state.uv_timer_init_result = result;
}

void libuv_mock_set_uv_close_delay(unsigned int delay_ms) {
    g_mock_state.uv_close_delay_ms = delay_ms;
}

void libuv_mock_set_alloc_buffer_size(size_t size) {
    g_mock_state.alloc_buffer_size = size;
}

void libuv_mock_set_trigger_alloc_cb(bool trigger) {
    g_mock_state.trigger_alloc_cb = trigger;
}

void libuv_mock_set_trigger_read_cb(bool trigger) {
    g_mock_state.trigger_read_cb = trigger;
}

void libuv_mock_set_read_data(const char* data, size_t len) {
    if (g_mock_state.read_data) {
        free(g_mock_state.read_data);
    }
    if (data && len > 0) {
        g_mock_state.read_data = (char*)malloc(len);
        memcpy(g_mock_state.read_data, data, len);
        g_mock_state.read_data_len = len;
    } else {
        g_mock_state.read_data = NULL;
        g_mock_state.read_data_len = 0;
    }
}

void libuv_mock_set_next_error(int error_code) {
    g_mock_state.next_error = error_code;
}

void libuv_mock_set_function_result(const char* func_name, int result) {
    if (strcmp(func_name, "uv_loop_init") == 0) {
        g_mock_state.uv_loop_init_result = result;
    } else if (strcmp(func_name, "uv_loop_close") == 0) {
        g_mock_state.uv_loop_close_result = result;
    } else if (strcmp(func_name, "uv_run") == 0) {
        g_mock_state.uv_run_result = result;
    } else if (strcmp(func_name, "uv_tcp_init") == 0) {
        g_mock_state.uv_tcp_init_result = result;
    } else if (strcmp(func_name, "uv_tcp_bind") == 0) {
        g_mock_state.uv_tcp_bind_result = result;
    } else if (strcmp(func_name, "uv_listen") == 0) {
        g_mock_state.uv_listen_result = result;
    } else if (strcmp(func_name, "uv_tcp_accept") == 0) {
        g_mock_state.uv_tcp_accept_result = result;
    } else if (strcmp(func_name, "uv_tcp_connect") == 0) {
        g_mock_state.uv_tcp_connect_result = result;
    } else if (strcmp(func_name, "uv_read_start") == 0) {
        g_mock_state.uv_read_start_result = result;
    } else if (strcmp(func_name, "uv_read_stop") == 0) {
        g_mock_state.uv_read_stop_result = result;
    } else if (strcmp(func_name, "uv_write") == 0) {
        g_mock_state.uv_write_result = result;
    } else if (strcmp(func_name, "uv_is_active") == 0) {
        g_mock_state.uv_is_active_result = result;
    } else if (strcmp(func_name, "uv_is_closing") == 0) {
        g_mock_state.uv_is_closing_result = result;
    }
}

/* ========== 回调触发函数 ========== */

void libuv_mock_trigger_connection_cb(uv_stream_t* server, int status) {
    if (g_mock_state.connection_cb) {
        g_mock_state.connection_cb(server, status);
    }
}

void libuv_mock_trigger_read_cb(uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
    if (g_mock_state.read_cb) {
        g_mock_state.read_cb(stream, nread, buf);
    }
}

void libuv_mock_trigger_write_cb(uv_write_t* req, int status) {
    if (g_mock_state.write_cb) {
        g_mock_state.write_cb(req, status);
    }
}

void libuv_mock_trigger_close_cb(uv_handle_t* handle) {
    if (g_mock_state.close_cb) {
        g_mock_state.close_cb(handle);
    }
}

/* ========== libuv 函数 Mock 实现 ========== */

/* 真实的 libuv 函数指针 */
static int (*real_uv_loop_init)(uv_loop_t*) = NULL;
static int (*real_uv_loop_close)(uv_loop_t*) = NULL;
static int (*real_uv_run)(uv_loop_t*, uv_run_mode) = NULL;

/* 初始化真实函数指针（通过 dlsym 或直接链接）*/
static void init_real_functions(void) {
    if (!real_uv_loop_init) {
        /* 这里使用 __wrap 前缀的函数名 */
        extern int __real_uv_loop_init(uv_loop_t*);
        extern int __real_uv_loop_close(uv_loop_t*);
        extern int __real_uv_run(uv_loop_t*, uv_run_mode);
        
        real_uv_loop_init = __real_uv_loop_init;
        real_uv_loop_close = __real_uv_loop_close;
        real_uv_run = __real_uv_run;
    }
}

/* uv_loop_t 函数 */

int __wrap_uv_loop_init(uv_loop_t* loop) {
    if (!g_mock_state.enabled) {
        init_real_functions();
        return real_uv_loop_init(loop);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_loop_init_count++;
    }
    
    g_mock_state.last_loop = loop;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_loop_init_result;
}

int __wrap_uv_loop_close(uv_loop_t* loop) {
    if (!g_mock_state.enabled) {
        init_real_functions();
        return real_uv_loop_close(loop);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_loop_close_count++;
    }
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_loop_close_result;
}

int __wrap_uv_run(uv_loop_t* loop, uv_run_mode mode) {
    if (!g_mock_state.enabled) {
        init_real_functions();
        return real_uv_run(loop, mode);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_run_count++;
    }
    
    g_mock_state.last_loop = loop;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_run_result;
}

/* uv_tcp_t 函数 */

int __wrap_uv_tcp_init(uv_loop_t* loop, uv_tcp_t* handle) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_tcp_init(uv_loop_t*, uv_tcp_t*);
        return __real_uv_tcp_init(loop, handle);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_tcp_init_count++;
    }
    
    g_mock_state.last_tcp = handle;
    g_mock_state.last_loop = loop;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_tcp_init_result;
}

int __wrap_uv_tcp_bind(uv_tcp_t* handle, const struct sockaddr* addr, unsigned int flags) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_tcp_bind(uv_tcp_t*, const struct sockaddr*, unsigned int);
        return __real_uv_tcp_bind(handle, addr, flags);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_tcp_bind_count++;
    }
    
    (void)addr;
    (void)flags;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_tcp_bind_result;
}

int __wrap_uv_listen(uv_stream_t* stream, int backlog, uv_connection_cb cb) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_listen(uv_stream_t*, int, uv_connection_cb);
        return __real_uv_listen(stream, backlog, cb);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_listen_count++;
    }
    
    g_mock_state.last_stream = stream;
    g_mock_state.connection_cb = cb;
    (void)backlog;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_listen_result;
}

int __wrap_uv_tcp_accept(uv_tcp_t* server, uv_tcp_t* client) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_tcp_accept(uv_tcp_t*, uv_tcp_t*);
        return __real_uv_tcp_accept(server, client);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_tcp_accept_count++;
    }
    
    (void)server;
    (void)client;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_tcp_accept_result;
}

/* uv_stream_t 函数 */

int __wrap_uv_read_start(uv_stream_t* stream, uv_alloc_cb alloc_cb, uv_read_cb read_cb) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_read_start(uv_stream_t*, uv_alloc_cb, uv_read_cb);
        return __real_uv_read_start(stream, alloc_cb, read_cb);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_read_start_count++;
    }
    
    g_mock_state.last_stream = stream;
    g_mock_state.alloc_cb = alloc_cb;
    g_mock_state.read_cb = read_cb;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_read_start_result;
}

int __wrap_uv_read_stop(uv_stream_t* stream) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_read_stop(uv_stream_t*);
        return __real_uv_read_stop(stream);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_read_stop_count++;
    }
    
    g_mock_state.last_stream = stream;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_read_stop_result;
}

int __wrap_uv_write(uv_write_t* req, uv_stream_t* handle, const uv_buf_t* bufs, unsigned int nbufs, uv_write_cb cb) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_write(uv_write_t*, uv_stream_t*, const uv_buf_t*, unsigned int, uv_write_cb);
        return __real_uv_write(req, handle, bufs, nbufs, cb);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_write_count++;
    }
    
    g_mock_state.last_stream = handle;
    g_mock_state.write_cb = cb;
    (void)req;
    (void)bufs;
    (void)nbufs;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_write_result;
}

/* uv_handle_t 函数 */

void __wrap_uv_close(uv_handle_t* handle, uv_close_cb cb) {
    if (!g_mock_state.enabled) {
        extern void __real_uv_close(uv_handle_t*, uv_close_cb);
        __real_uv_close(handle, cb);
        return;
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_close_count++;
    }
    
    g_mock_state.close_cb = cb;
    
    /* 立即触发回调 */
    if (cb) {
        cb(handle);
    }
}

int __wrap_uv_is_active(const uv_handle_t* handle) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_is_active(const uv_handle_t*);
        return __real_uv_is_active(handle);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_is_active_count++;
    }
    
    (void)handle;
    
    return g_mock_state.uv_is_active_result;
}

int __wrap_uv_is_closing(const uv_handle_t* handle) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_is_closing(const uv_handle_t*);
        return __real_uv_is_closing(handle);
    }
    
    if (g_mock_state.record_calls) {
        g_mock_state.uv_is_closing_count++;
    }
    
    (void)handle;
    
    return g_mock_state.uv_is_closing_result;
}

/* uv_idle_t 函数 */

int __wrap_uv_idle_init(uv_loop_t* loop, uv_idle_t* handle) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_idle_init(uv_loop_t*, uv_idle_t*);
        return __real_uv_idle_init(loop, handle);
    }
    
    g_mock_state.last_loop = loop;
    (void)handle;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_idle_init_result;
}

/* uv_timer_t 函数 */

int __wrap_uv_timer_init(uv_loop_t* loop, uv_timer_t* handle) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_timer_init(uv_loop_t*, uv_timer_t*);
        return __real_uv_timer_init(loop, handle);
    }
    
    g_mock_state.last_loop = loop;
    (void)handle;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_timer_init_result;
}

int __wrap_uv_timer_stop(uv_timer_t* handle) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_timer_stop(uv_timer_t*);
        return __real_uv_timer_stop(handle);
    }
    
    (void)handle;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_timer_stop_result;
}

int __wrap_uv_timer_start(uv_timer_t* handle, uv_timer_cb cb, uint64_t timeout, uint64_t repeat) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_timer_start(uv_timer_t*, uv_timer_cb, uint64_t, uint64_t);
        return __real_uv_timer_start(handle, cb, timeout, repeat);
    }
    
    (void)handle;
    (void)cb;
    (void)timeout;
    (void)repeat;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_timer_start_result;
}

/* uv_tcp_t 函数 */

int __wrap_uv_tcp_nodelay(uv_tcp_t* handle, int enable) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_tcp_nodelay(uv_tcp_t*, int);
        return __real_uv_tcp_nodelay(handle, enable);
    }
    
    (void)handle;
    (void)enable;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_tcp_nodelay_result;
}

int __wrap_uv_tcp_keepalive(uv_tcp_t* handle, int enable, unsigned int delay) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_tcp_keepalive(uv_tcp_t*, int, unsigned int);
        return __real_uv_tcp_keepalive(handle, enable, delay);
    }
    
    (void)handle;
    (void)enable;
    (void)delay;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_tcp_keepalive_result;
}

int __wrap_uv_tcp_simultaneous_accepts(uv_tcp_t* handle, int enable) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_tcp_simultaneous_accepts(uv_tcp_t*, int);
        return __real_uv_tcp_simultaneous_accepts(handle, enable);
    }
    
    (void)handle;
    (void)enable;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_tcp_simultaneous_accepts_result;
}

/* uv_stream_t 函数 */

int __wrap_uv_accept(uv_stream_t* server, uv_stream_t* client) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_accept(uv_stream_t*, uv_stream_t*);
        return __real_uv_accept(server, client);
    }
    
    g_mock_state.last_stream = server;
    (void)client;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_accept_result;
}

/* uv_handle_t 函数 */

int __wrap_uv_fileno(const uv_handle_t* handle, uv_os_fd_t* fd) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_fileno(const uv_handle_t*, uv_os_fd_t*);
        return __real_uv_fileno(handle, fd);
    }
    
    (void)handle;
    (void)fd;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_fileno_result;
}

/* uv_util_t 函数 */

int __wrap_uv_ip4_addr(const char* ip, int port, struct sockaddr_in* addr) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_ip4_addr(const char*, int, struct sockaddr_in*);
        return __real_uv_ip4_addr(ip, port, addr);
    }
    
    (void)ip;
    (void)port;
    (void)addr;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_ip4_addr_result;
}

int __wrap_uv_ip6_addr(const char* ip, int port, struct sockaddr_in6* addr) {
    if (!g_mock_state.enabled) {
        extern int __real_uv_ip6_addr(const char*, int, struct sockaddr_in6*);
        return __real_uv_ip6_addr(ip, port, addr);
    }
    
    (void)ip;
    (void)port;
    (void)addr;
    
    if (g_mock_state.next_error != 0) {
        int err = g_mock_state.next_error;
        g_mock_state.next_error = 0;
        return err;
    }
    
    return g_mock_state.uv_ip6_addr_result;
}

/* 全局变量用于 uv_strerror 和 uv_hrtime */

static char g_uv_strerror_msg[256] = "Unknown error";
static uint64_t g_uv_hrtime_value = 0;
static uv_buf_t g_uv_buf_init_value = {0, 0};

const char* __wrap_uv_strerror(int err) {
    if (!g_mock_state.enabled) {
        extern const char* __real_uv_strerror(int);
        return __real_uv_strerror(err);
    }
    
    (void)err;
    return g_uv_strerror_msg;
}

uint64_t __wrap_uv_hrtime(void) {
    if (!g_mock_state.enabled) {
        extern uint64_t __real_uv_hrtime(void);
        return __real_uv_hrtime();
    }
    
    return g_uv_hrtime_value;
}

uv_buf_t __wrap_uv_buf_init(char* base, size_t len) {
    if (!g_mock_state.enabled) {
        extern uv_buf_t __real_uv_buf_init(char*, size_t);
        return __real_uv_buf_init(base, len);
    }
    
    (void)base;
    (void)len;
    return g_uv_buf_init_value;
}

/* ========== Setter 函数 ========== */

void libuv_mock_set_uv_timer_stop_result(int result) {
    g_mock_state.uv_timer_stop_result = result;
}

void libuv_mock_set_uv_timer_start_result(int result) {
    g_mock_state.uv_timer_start_result = result;
}

void libuv_mock_set_uv_tcp_nodelay_result(int result) {
    g_mock_state.uv_tcp_nodelay_result = result;
}

void libuv_mock_set_uv_tcp_keepalive_result(int result) {
    g_mock_state.uv_tcp_keepalive_result = result;
}

void libuv_mock_set_uv_tcp_simultaneous_accepts_result(int result) {
    g_mock_state.uv_tcp_simultaneous_accepts_result = result;
}

void libuv_mock_set_uv_accept_result(int result) {
    g_mock_state.uv_accept_result = result;
}

void libuv_mock_set_uv_fileno_result(int result) {
    g_mock_state.uv_fileno_result = result;
}

void libuv_mock_set_uv_ip4_addr_result(int result) {
    g_mock_state.uv_ip4_addr_result = result;
}

void libuv_mock_set_uv_ip6_addr_result(int result) {
    g_mock_state.uv_ip6_addr_result = result;
}

void libuv_mock_set_uv_strerror(const char* error_msg) {
    if (error_msg) {
        strncpy(g_uv_strerror_msg, error_msg, sizeof(g_uv_strerror_msg) - 1);
        g_uv_strerror_msg[sizeof(g_uv_strerror_msg) - 1] = '\0';
    }
}

void libuv_mock_set_uv_hrtime(uint64_t hrtime) {
    g_uv_hrtime_value = hrtime;
}

void libuv_mock_set_uv_buf_init_result(uv_buf_t buf) {
    g_uv_buf_init_value = buf;
}