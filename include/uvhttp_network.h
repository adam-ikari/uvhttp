/* UVHTTP 网络层抽象接口 - 针对libuv测试模拟 */

#ifndef UVHTTP_NETWORK_H
#define UVHTTP_NETWORK_H

#include "uvhttp_common.h"
#include <uv.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* 网络层抽象接口结构 */
typedef struct uvhttp_network_interface {
    /* 核心 libuv 操作的抽象函数指针 */
    int (*write)(struct uvhttp_network_interface* self, 
                 uv_stream_t* stream, 
                 const uv_buf_t* bufs, 
                 unsigned int nbufs, 
                 uv_write_cb cb);
    
    int (*read_start)(struct uvhttp_network_interface* self,
                      uv_stream_t* stream,
                      uv_alloc_cb alloc_cb,
                      uv_read_cb read_cb);
    
    int (*read_stop)(struct uvhttp_network_interface* self,
                     uv_stream_t* stream);
    
    int (*close)(struct uvhttp_network_interface* self,
                 uv_handle_t* handle,
                 uv_close_cb close_cb);
    
    /* 测试专用接口 */
    void* (*get_stats)(struct uvhttp_network_interface* self);
    void (*reset_stats)(struct uvhttp_network_interface* self);
    void (*set_error_simulation)(struct uvhttp_network_interface* self, int error_code);
    
    /* 私有数据 */
    void* handle;
    
    /* 性能统计字段（测试时有用） */
    uint64_t bytes_sent;
    uint64_t bytes_received;
    uint32_t error_count;
    uint32_t write_count;
    uint32_t read_count;
    
} uvhttp_network_interface_t;

/* 网络接口类型枚举 */
typedef enum {
    UVHTTP_NETWORK_LIBUV,    /* 生产环境：直接使用libuv */
    UVHTTP_NETWORK_MOCK,     /* 测试环境：模拟libuv行为 */
    UVHTTP_NETWORK_BENCHMARK /* 性能测试：零开销模拟 */
} uvhttp_network_type_t;

/* 网络接口创建函数 */
uvhttp_network_interface_t* uvhttp_network_interface_create(
    uvhttp_network_type_t type,
    uv_loop_t* loop
);

/* 网络接口销毁函数 */
void uvhttp_network_interface_destroy(uvhttp_network_interface_t* interface);

/* 生产环境网络接口创建 */
uvhttp_network_interface_t* uvhttp_libuv_network_create(uv_loop_t* loop);

/* 测试环境网络接口创建 */
uvhttp_network_interface_t* uvhttp_mock_network_create(uv_loop_t* loop);

/* 性能测试网络接口创建 */
uvhttp_network_interface_t* uvhttp_benchmark_network_create(uv_loop_t* loop);

/* 全局网络接口声明 */
extern uvhttp_network_interface_t* g_uvhttp_network_interface;

/* 编译时宏控制 */
#ifdef UVHTTP_TEST_MODE
    /* 测试模式：使用网络接口 */
    #define UVHTTP_USE_NETWORK_INTERFACE 1
    
    #define uvhttp_network_write(stream, bufs, nbufs, cb) \
        g_uvhttp_network_interface->write(g_uvhttp_network_interface, stream, bufs, nbufs, cb)
    
    #define uvhttp_network_read_start(stream, alloc_cb, read_cb) \
        g_uvhttp_network_interface->read_start(g_uvhttp_network_interface, stream, alloc_cb, read_cb)
    
    #define uvhttp_network_read_stop(stream) \
        g_uvhttp_network_interface->read_stop(g_uvhttp_network_interface, stream)
    
    #define uvhttp_network_close(handle, close_cb) \
        g_uvhttp_network_interface->close(g_uvhttp_network_interface, handle, close_cb)
#else
    /* 生产模式：直接调用libuv，零开销 */
    #define UVHTTP_USE_NETWORK_INTERFACE 0
    
    /* 内联函数优化 - 编译器会内联这些调用 */
    static inline int uvhttp_network_write(uv_stream_t* stream, 
                                          const uv_buf_t* bufs, 
                                          unsigned int nbufs, 
                                          uv_write_cb cb) {
        return uv_write(&((uv_write_t){0}), stream, bufs, nbufs, cb);
    }
    
    static inline int uvhttp_network_read_start(uv_stream_t* stream,
                                               uv_alloc_cb alloc_cb,
                                               uv_read_cb read_cb) {
        return uv_read_start(stream, alloc_cb, read_cb);
    }
    
    static inline int uvhttp_network_read_stop(uv_stream_t* stream) {
        return uv_read_stop(stream);
    }
    
    static inline void uvhttp_network_close(uv_handle_t* handle, uv_close_cb close_cb) {
        uv_close(handle, close_cb);
    }
#endif

/* 测试辅助宏 */
#ifdef UVHTTP_TEST_MODE
    /* 测试统计访问 */
    #define uvhttp_network_get_write_count() \
        (g_uvhttp_network_interface ? g_uvhttp_network_interface->write_count : 0)
    
    #define uvhttp_network_get_bytes_sent() \
        (g_uvhttp_network_interface ? g_uvhttp_network_interface->bytes_sent : 0)
    
    #define uvhttp_network_get_error_count() \
        (g_uvhttp_network_interface ? g_uvhttp_network_interface->error_count : 0)
    
    /* 错误模拟 */
    #define uvhttp_network_simulate_error(error_code) \
        do { \
            if (g_uvhttp_network_interface) { \
                g_uvhttp_network_interface->set_error_simulation(g_uvhttp_network_interface, error_code); \
            } \
        } while(0)
    
    /* 统计重置 */
    #define uvhttp_network_reset_stats() \
        do { \
            if (g_uvhttp_network_interface) { \
                g_uvhttp_network_interface->reset_stats(g_uvhttp_network_interface); \
            } \
        } while(0)
#else
    /* 生产环境空宏 - 编译器会优化掉 */
    #define uvhttp_network_get_write_count() 0
    #define uvhttp_network_get_bytes_sent() 0
    #define uvhttp_network_get_error_count() 0
    #define uvhttp_network_simulate_error(error_code) do {} while(0)
    #define uvhttp_network_reset_stats() do {} while(0)
#endif

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_NETWORK_H */