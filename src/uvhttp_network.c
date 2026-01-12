/* UVHTTP 网络层抽象接口实现 */

#include "uvhttp_network.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <stdlib.h>
#include <string.h>

/* 全局网络接口实例 */
uvhttp_network_interface_t* g_uvhttp_network_interface = NULL;

/* ============ 生产环境：libuv 直接实现 ============ */

static int libuv_write_impl(uvhttp_network_interface_t* self, 
                           uv_stream_t* stream, 
                           const uv_buf_t* bufs, 
                           unsigned int nbufs, 
                           uv_write_cb cb) {
    (void)self; /* 避免未使用参数警告 */
    
    /* 创建写请求（在栈上分配，调用者负责生命周期） */
    uv_write_t* req = (uv_write_t*)uvhttp_malloc(sizeof(uv_write_t));
    if (!req) {
        return UV_ENOMEM;
    }
    
    memset(req, 0, sizeof(uv_write_t));
    
    /* 调用libuv写操作 */
    int result = uv_write(req, stream, bufs, nbufs, cb);
    
    if (result < 0) {
        uvhttp_free(req);
        return result;
    }
    
    /* 更新统计 */
    self->write_count++;
    for (unsigned int i = 0; i < nbufs; i++) {
        self->bytes_sent += bufs[i].len;
    }
    
    return 0;
}

static int libuv_read_start_impl(uvhttp_network_interface_t* self,
                                 uv_stream_t* stream,
                                 uv_alloc_cb alloc_cb,
                                 uv_read_cb read_cb) {
    (void)self; /* 避免未使用参数警告 */
    int result = uv_read_start(stream, alloc_cb, read_cb);
    if (result == 0) {
        self->read_count++;
    }
    return result;
}

static int libuv_read_stop_impl(uvhttp_network_interface_t* self,
                                uv_stream_t* stream) {
    (void)self; /* 避免未使用参数警告 */
    return uv_read_stop(stream);
}

static int libuv_close_impl(uvhttp_network_interface_t* self,
                            uv_handle_t* handle,
                            uv_close_cb close_cb) {
    (void)self; /* 避免未使用参数警告 */
    uv_close(handle, close_cb);
    return 0;
}

static void* libuv_get_stats_impl(uvhttp_network_interface_t* self) {
    return self; /* 返回自身作为统计对象 */
}

static void libuv_reset_stats_impl(uvhttp_network_interface_t* self) {
    self->bytes_sent = 0;
    self->bytes_received = 0;
    self->error_count = 0;
    self->write_count = 0;
    self->read_count = 0;
}

static void libuv_set_error_simulation_impl(uvhttp_network_interface_t* self, int error_code) {
    (void)self;
    (void)error_code;
    /* 生产环境不支持错误模拟 */
}

/* 创建生产环境网络接口 */
uvhttp_network_interface_t* uvhttp_libuv_network_create(uv_loop_t* loop) {
    (void)loop; /* 避免未使用参数警告 */
    
    uvhttp_network_interface_t* interface = 
        (uvhttp_network_interface_t*)uvhttp_malloc(sizeof(uvhttp_network_interface_t));
    if (!interface) {
        return NULL;
    }
    
    memset(interface, 0, sizeof(uvhttp_network_interface_t));
    
    /* 设置libuv实现函数 */
    interface->write = libuv_write_impl;
    interface->read_start = libuv_read_start_impl;
    interface->read_stop = libuv_read_stop_impl;
    interface->close = libuv_close_impl;
    interface->get_stats = libuv_get_stats_impl;
    interface->reset_stats = libuv_reset_stats_impl;
    interface->set_error_simulation = libuv_set_error_simulation_impl;
    
    interface->handle = NULL; /* libuv不需要额外handle */
    
    return interface;
}

/* ============ 测试环境：模拟libuv实现 ============ */

typedef struct {
    uvhttp_network_interface_t base;
    uv_loop_t* loop;
    int simulate_error;
    uint32_t delayed_writes;
    char* captured_data;
    size_t captured_length;
} uvhttp_mock_network_t;

static int mock_write_impl(uvhttp_network_interface_t* self, 
                          uv_stream_t* stream, 
                          const uv_buf_t* bufs,
                          unsigned int nbufs,
                          uv_write_cb cb) {
    (void)stream; // 避免未使用参数警告
    uvhttp_mock_network_t* mock = (uvhttp_mock_network_t*)((char*)self - offsetof(uvhttp_mock_network_t, base));

    /* 错误模拟 */
    if (mock->simulate_error != 0) {
        self->error_count++;
        return mock->simulate_error;
    }
    
    /* 捕获写入数据用于测试验证 */
    size_t total_length = 0;
    for (unsigned int i = 0; i < nbufs; i++) {
        total_length += bufs[i].len;
    }
    
    if (total_length > 0) {
        /* 重新分配捕获缓冲区 */
        char* new_data = (char*)uvhttp_realloc(mock->captured_data, 
                                               mock->captured_length + total_length);
        if (!new_data) {
            self->error_count++;
            return UV_ENOMEM;
        }
        
        mock->captured_data = new_data;
        
        /* 复制数据 */
        size_t offset = mock->captured_length;
        for (unsigned int i = 0; i < nbufs; i++) {
            memcpy(mock->captured_data + offset, bufs[i].base, bufs[i].len);
            offset += bufs[i].len;
        }
        
        mock->captured_length = offset;
    }
    
    /* 更新统计 */
    self->write_count++;
    self->bytes_sent += total_length;
    
    /* 模拟异步回调 - 在下一个事件循环中执行 */
    if (cb) {
        /* 这里简化处理，直接调用回调 */
        cb((uv_write_t*)0, 0);
    }
    
    return 0;
}

static int mock_read_start_impl(uvhttp_network_interface_t* self,
                                uv_stream_t* stream,
                                uv_alloc_cb alloc_cb,
                                uv_read_cb read_cb) {
    (void)self;
    (void)stream;
    (void)alloc_cb;
    (void)read_cb;
    /* 测试环境通常不需要模拟读取 */
    self->read_count++;
    return 0;
}

static int mock_read_stop_impl(uvhttp_network_interface_t* self,
                               uv_stream_t* stream) {
    (void)self;
    (void)stream;
    return 0;
}

static int mock_close_impl(uvhttp_network_interface_t* self,
                           uv_handle_t* handle,
                           uv_close_cb close_cb) {
    (void)self;
    if (close_cb) {
        close_cb(handle);
    }
    return 0;
}

static void* mock_get_stats_impl(uvhttp_network_interface_t* self) {
    return self;
}

static void mock_reset_stats_impl(uvhttp_network_interface_t* self) {
    uvhttp_mock_network_t* mock = (uvhttp_mock_network_t*)((char*)self - offsetof(uvhttp_mock_network_t, base));

    self->bytes_sent = 0;
    self->bytes_received = 0;
    self->error_count = 0;
    self->write_count = 0;
    self->read_count = 0;

    mock->simulate_error = 0;
    mock->delayed_writes = 0;

    if (mock->captured_data) {
        uvhttp_free(mock->captured_data);
        mock->captured_data = NULL;
        mock->captured_length = 0;
    }
}

static void mock_set_error_simulation_impl(uvhttp_network_interface_t* self, int error_code) {
    uvhttp_mock_network_t* mock = (uvhttp_mock_network_t*)((char*)self - offsetof(uvhttp_mock_network_t, base));
    mock->simulate_error = error_code;
}

/* 创建测试环境网络接口 */
uvhttp_network_interface_t* uvhttp_mock_network_create(uv_loop_t* loop) {
    uvhttp_mock_network_t* mock = 
        (uvhttp_mock_network_t*)uvhttp_malloc(sizeof(uvhttp_mock_network_t));
    if (!mock) {
        return NULL;
    }
    
    memset(mock, 0, sizeof(uvhttp_mock_network_t));
    
    /* 设置模拟实现函数 */
    mock->base.write = mock_write_impl;
    mock->base.read_start = mock_read_start_impl;
    mock->base.read_stop = mock_read_stop_impl;
    mock->base.close = mock_close_impl;
    mock->base.get_stats = mock_get_stats_impl;
    mock->base.reset_stats = mock_reset_stats_impl;
    mock->base.set_error_simulation = mock_set_error_simulation_impl;
    
    mock->loop = loop;
    mock->simulate_error = 0;
    
    return &mock->base;
}

/* ============ 性能测试：零开销模拟 ============ */

static int benchmark_write_impl(uvhttp_network_interface_t* self, 
                               uv_stream_t* stream, 
                               const uv_buf_t* bufs, 
                               unsigned int nbufs, 
                               uv_write_cb cb) {
    (void)stream;
    
    /* 只计算统计，不进行实际I/O */
    size_t total_length = 0;
    for (unsigned int i = 0; i < nbufs; i++) {
        total_length += bufs[i].len;
    }
    
    self->write_count++;
    self->bytes_sent += total_length;
    
    /* 立即回调成功 */
    if (cb) {
        cb((uv_write_t*)0, 0);
    }
    
    return 0;
}

static int benchmark_read_start_impl(uvhttp_network_interface_t* self,
                                     uv_stream_t* stream,
                                     uv_alloc_cb alloc_cb,
                                     uv_read_cb read_cb) {
    (void)self;
    (void)stream;
    (void)alloc_cb;
    (void)read_cb;
    self->read_count++;
    return 0;
}

static int benchmark_read_stop_impl(uvhttp_network_interface_t* self,
                                    uv_stream_t* stream) {
    (void)self;
    (void)stream;
    return 0;
}

static int benchmark_close_impl(uvhttp_network_interface_t* self,
                                uv_handle_t* handle,
                                uv_close_cb close_cb) {
    (void)self;
    if (close_cb) {
        close_cb(handle);
    }
    return 0;
}

/* 创建性能测试网络接口 */
uvhttp_network_interface_t* uvhttp_benchmark_network_create(uv_loop_t* loop) {
    (void)loop; /* 避免未使用参数警告 */
    
    uvhttp_network_interface_t* interface = 
        (uvhttp_network_interface_t*)uvhttp_malloc(sizeof(uvhttp_network_interface_t));
    if (!interface) {
        return NULL;
    }
    
    memset(interface, 0, sizeof(uvhttp_network_interface_t));
    
    /* 设置性能测试实现函数 */
    interface->write = benchmark_write_impl;
    interface->read_start = benchmark_read_start_impl;
    interface->read_stop = benchmark_read_stop_impl;
    interface->close = benchmark_close_impl;
    interface->get_stats = libuv_get_stats_impl;
    interface->reset_stats = libuv_reset_stats_impl;
    interface->set_error_simulation = libuv_set_error_simulation_impl;
    
    return interface;
}

/* ============ 通用接口函数 ============ */

uvhttp_network_interface_t* uvhttp_network_interface_create(
    uvhttp_network_type_t type,
    uv_loop_t* loop) {
    
    switch (type) {
        case UVHTTP_NETWORK_LIBUV:
            return uvhttp_libuv_network_create(loop);
            
        case UVHTTP_NETWORK_MOCK:
            return uvhttp_mock_network_create(loop);
            
        case UVHTTP_NETWORK_BENCHMARK:
            return uvhttp_benchmark_network_create(loop);
            
        default:
            return NULL;
    }
}

void uvhttp_network_interface_destroy(uvhttp_network_interface_t* interface) {
    if (!interface) {
        return;
    }

    /* 通过函数指针判断网络接口类型 */
    /* libuv 网络接口的 write 函数是 libuv_write_impl */
    if (interface->write == libuv_write_impl) {
        /* libuv 网络接口直接释放 */
        uvhttp_free(interface);
        return;
    }

    /* benchmark 网络接口的 write 函数是 benchmark_write_impl */
    if (interface->write == benchmark_write_impl) {
        /* benchmark 网络接口也直接释放 */
        uvhttp_free(interface);
        return;
    }

    /* mock 网络接口 */
    if (interface->write == mock_write_impl) {
        uvhttp_mock_network_t* mock = (uvhttp_mock_network_t*)((char*)interface - offsetof(uvhttp_mock_network_t, base));
        if (mock->captured_data) {
            uvhttp_free(mock->captured_data);
        }
        uvhttp_free(mock);
        return;
    }

    /* 未知类型，直接释放 */
    uvhttp_free(interface);
}

/* 测试辅助函数 */
#ifdef UVHTTP_TEST_MODE

/* 初始化测试网络接口 */
int uvhttp_test_network_init(uv_loop_t* loop, uvhttp_network_type_t type) {
    if (g_uvhttp_network_interface) {
        uvhttp_network_interface_destroy(g_uvhttp_network_interface);
    }
    
    g_uvhttp_network_interface = uvhttp_network_interface_create(type, loop);
    return g_uvhttp_network_interface ? 0 : -1;
}

/* 清理测试网络接口 */
void uvhttp_test_network_cleanup(void) {
    if (g_uvhttp_network_interface) {
        uvhttp_network_interface_destroy(g_uvhttp_network_interface);
        g_uvhttp_network_interface = NULL;
    }
}

/* 获取捕获的数据（仅测试模式） */
const char* uvhttp_test_get_captured_data(size_t* length) {
    if (!g_uvhttp_network_interface) {
        if (length) *length = 0;
        return NULL;
    }
    
    uvhttp_mock_network_t* mock = (uvhttp_mock_network_t*)g_uvhttp_network_interface;
    if (length) *length = mock->captured_length;
    return mock->captured_data;
}

#endif /* UVHTTP_TEST_MODE */