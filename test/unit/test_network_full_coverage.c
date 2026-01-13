/* UVHTTP 网络层抽象接口完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_network.h"
#include "uvhttp_constants.h"

/* 测试 libuv 网络接口创建 */
void test_libuv_network_create(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(loop);
    
    if (interface != NULL) {
        assert(interface->write != NULL);
        assert(interface->read_start != NULL);
        assert(interface->read_stop != NULL);
        assert(interface->close != NULL);
        assert(interface->get_stats != NULL);
        assert(interface->reset_stats != NULL);
        assert(interface->set_error_simulation != NULL);
        uvhttp_network_interface_destroy(interface);
    }
    
    printf("test_libuv_network_create: PASSED\n");
}

/* 测试 mock 网络接口创建 */
void test_mock_network_create(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(loop);
    
    if (interface != NULL) {
        assert(interface->write != NULL);
        assert(interface->read_start != NULL);
        assert(interface->read_stop != NULL);
        assert(interface->close != NULL);
        assert(interface->get_stats != NULL);
        assert(interface->reset_stats != NULL);
        assert(interface->set_error_simulation != NULL);
        uvhttp_network_interface_destroy(interface);
    }
    
    printf("test_mock_network_create: PASSED\n");
}

/* 测试 benchmark 网络接口创建 */
void test_benchmark_network_create(void) {
    uv_loop_t* loop = uv_default_loop();
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(loop);
    
    if (interface != NULL) {
        assert(interface->write != NULL);
        assert(interface->read_start != NULL);
        assert(interface->read_stop != NULL);
        assert(interface->close != NULL);
        assert(interface->get_stats != NULL);
        assert(interface->reset_stats != NULL);
        assert(interface->set_error_simulation != NULL);
        uvhttp_network_interface_destroy(interface);
    }
    
    printf("test_benchmark_network_create: PASSED\n");
}

/* 测试通用接口创建函数 */
void test_network_interface_create(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 LIBUV 类型 */
    uvhttp_network_interface_t* libuv_interface = 
        uvhttp_network_interface_create(UVHTTP_NETWORK_LIBUV, loop);
    if (libuv_interface != NULL) {
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 MOCK 类型 */
    uvhttp_network_interface_t* mock_interface = 
        uvhttp_network_interface_create(UVHTTP_NETWORK_MOCK, loop);
    if (mock_interface != NULL) {
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 BENCHMARK 类型 */
    uvhttp_network_interface_t* benchmark_interface = 
        uvhttp_network_interface_create(UVHTTP_NETWORK_BENCHMARK, loop);
    if (benchmark_interface != NULL) {
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    /* 测试无效类型 */
    uvhttp_network_interface_t* invalid_interface = 
        uvhttp_network_interface_create((uvhttp_network_type_t)999, loop);
    assert(invalid_interface == NULL);
    
    printf("test_network_interface_create: PASSED\n");
}

/* 测试 NULL 参数处理 */
void test_network_null_params(void) {
    /* 测试 NULL 参数处理 */
    uvhttp_network_interface_destroy(NULL);
    
    /* 测试 NULL loop 参数 */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(NULL);
    if (libuv_interface != NULL) {
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(NULL);
    if (mock_interface != NULL) {
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(NULL);
    if (benchmark_interface != NULL) {
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_null_params: PASSED\n");
}

/* 测试网络接口销毁 */
void test_network_interface_destroy(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 创建并销毁多次 */
    for (int i = 0; i < 5; i++) {
        uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(loop);
        if (interface != NULL) {
            uvhttp_network_interface_destroy(interface);
        }
    }
    
    for (int i = 0; i < 5; i++) {
        uvhttp_network_interface_t* interface = uvhttp_mock_network_create(loop);
        if (interface != NULL) {
            uvhttp_network_interface_destroy(interface);
        }
    }
    
    for (int i = 0; i < 5; i++) {
        uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(loop);
        if (interface != NULL) {
            uvhttp_network_interface_destroy(interface);
        }
    }
    
    printf("test_network_interface_destroy: PASSED\n");
}

/* 测试统计字段初始化 */
void test_network_stats_init(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 libuv 接口统计初始化 */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(loop);
    if (libuv_interface != NULL) {
        assert(libuv_interface->bytes_sent == 0);
        assert(libuv_interface->bytes_received == 0);
        assert(libuv_interface->error_count == 0);
        assert(libuv_interface->write_count == 0);
        assert(libuv_interface->read_count == 0);
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 mock 接口统计初始化 */
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(loop);
    if (mock_interface != NULL) {
        assert(mock_interface->bytes_sent == 0);
        assert(mock_interface->bytes_received == 0);
        assert(mock_interface->error_count == 0);
        assert(mock_interface->write_count == 0);
        assert(mock_interface->read_count == 0);
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 benchmark 接口统计初始化 */
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(loop);
    if (benchmark_interface != NULL) {
        assert(benchmark_interface->bytes_sent == 0);
        assert(benchmark_interface->bytes_received == 0);
        assert(benchmark_interface->error_count == 0);
        assert(benchmark_interface->write_count == 0);
        assert(benchmark_interface->read_count == 0);
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_stats_init: PASSED\n");
}

/* 测试 get_stats 函数 */
void test_network_get_stats(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 libuv 接口 get_stats */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(loop);
    if (libuv_interface != NULL) {
        void* stats = libuv_interface->get_stats(libuv_interface);
        assert(stats != NULL);
        assert(stats == libuv_interface);
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 mock 接口 get_stats */
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(loop);
    if (mock_interface != NULL) {
        void* stats = mock_interface->get_stats(mock_interface);
        assert(stats != NULL);
        assert(stats == mock_interface);
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 benchmark 接口 get_stats */
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(loop);
    if (benchmark_interface != NULL) {
        void* stats = benchmark_interface->get_stats(benchmark_interface);
        assert(stats != NULL);
        assert(stats == benchmark_interface);
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_get_stats: PASSED\n");
}

/* 测试 reset_stats 函数 */
void test_network_reset_stats(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 libuv 接口 reset_stats */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(loop);
    if (libuv_interface != NULL) {
        /* 修改统计字段 */
        libuv_interface->bytes_sent = 100;
        libuv_interface->write_count = 10;
        
        /* 重置统计 */
        libuv_interface->reset_stats(libuv_interface);
        
        /* 验证重置 */
        assert(libuv_interface->bytes_sent == 0);
        assert(libuv_interface->bytes_received == 0);
        assert(libuv_interface->error_count == 0);
        assert(libuv_interface->write_count == 0);
        assert(libuv_interface->read_count == 0);
        
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 mock 接口 reset_stats */
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(loop);
    if (mock_interface != NULL) {
        /* 修改统计字段 */
        mock_interface->bytes_sent = 200;
        mock_interface->write_count = 20;
        
        /* 重置统计 */
        mock_interface->reset_stats(mock_interface);
        
        /* 验证重置 */
        assert(mock_interface->bytes_sent == 0);
        assert(mock_interface->bytes_received == 0);
        assert(mock_interface->error_count == 0);
        assert(mock_interface->write_count == 0);
        assert(mock_interface->read_count == 0);
        
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 benchmark 接口 reset_stats */
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(loop);
    if (benchmark_interface != NULL) {
        /* 修改统计字段 */
        benchmark_interface->bytes_sent = 300;
        benchmark_interface->write_count = 30;
        
        /* 重置统计 */
        benchmark_interface->reset_stats(benchmark_interface);
        
        /* 验证重置 */
        assert(benchmark_interface->bytes_sent == 0);
        assert(benchmark_interface->bytes_received == 0);
        assert(benchmark_interface->error_count == 0);
        assert(benchmark_interface->write_count == 0);
        assert(benchmark_interface->read_count == 0);
        
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_reset_stats: PASSED\n");
}

/* 测试 set_error_simulation 函数 */
void test_network_set_error_simulation(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 libuv 接口 set_error_simulation（生产环境不支持错误模拟） */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(loop);
    if (libuv_interface != NULL) {
        libuv_interface->set_error_simulation(libuv_interface, UV_ENOMEM);
        /* 生产环境不支持错误模拟，所以不会有任何效果 */
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 mock 接口 set_error_simulation */
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(loop);
    if (mock_interface != NULL) {
        /* 设置错误模拟 */
        mock_interface->set_error_simulation(mock_interface, UV_ENOMEM);
        
        /* 重置错误模拟 */
        mock_interface->set_error_simulation(mock_interface, 0);
        
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 benchmark 接口 set_error_simulation */
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(loop);
    if (benchmark_interface != NULL) {
        benchmark_interface->set_error_simulation(benchmark_interface, UV_ENOMEM);
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_set_error_simulation: PASSED\n");
}

/* 测试多次创建和销毁 */
void test_network_multiple_cycles(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试多次创建和销毁 */
    for (int i = 0; i < 10; i++) {
        uvhttp_network_interface_t* interface = uvhttp_network_interface_create(
            (uvhttp_network_type_t)(i % 3), loop);
        if (interface != NULL) {
            uvhttp_network_interface_destroy(interface);
        }
    }
    
    printf("test_network_multiple_cycles: PASSED\n");
}

/* 测试 handle 字段 */
void test_network_handle_field(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 libuv 接口 handle 字段 */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(loop);
    if (libuv_interface != NULL) {
        assert(libuv_interface->handle == NULL);
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 mock 接口 handle 字段 */
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(loop);
    if (mock_interface != NULL) {
        /* mock 接口的 handle 字段是 NULL */
        assert(mock_interface->handle == NULL);
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 benchmark 接口 handle 字段 */
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(loop);
    if (benchmark_interface != NULL) {
        assert(benchmark_interface->handle == NULL);
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_handle_field: PASSED\n");
}

/* 测试函数指针一致性 */
void test_network_function_pointers(void) {
    uv_loop_t* loop = uv_default_loop();
    
    /* 测试 libuv 接口函数指针 */
    uvhttp_network_interface_t* libuv_interface = uvhttp_libuv_network_create(loop);
    if (libuv_interface != NULL) {
        assert(libuv_interface->write != NULL);
        assert(libuv_interface->read_start != NULL);
        assert(libuv_interface->read_stop != NULL);
        assert(libuv_interface->close != NULL);
        assert(libuv_interface->get_stats != NULL);
        assert(libuv_interface->reset_stats != NULL);
        assert(libuv_interface->set_error_simulation != NULL);
        uvhttp_network_interface_destroy(libuv_interface);
    }
    
    /* 测试 mock 接口函数指针 */
    uvhttp_network_interface_t* mock_interface = uvhttp_mock_network_create(loop);
    if (mock_interface != NULL) {
        assert(mock_interface->write != NULL);
        assert(mock_interface->read_start != NULL);
        assert(mock_interface->read_stop != NULL);
        assert(mock_interface->close != NULL);
        assert(mock_interface->get_stats != NULL);
        assert(mock_interface->reset_stats != NULL);
        assert(mock_interface->set_error_simulation != NULL);
        uvhttp_network_interface_destroy(mock_interface);
    }
    
    /* 测试 benchmark 接口函数指针 */
    uvhttp_network_interface_t* benchmark_interface = uvhttp_benchmark_network_create(loop);
    if (benchmark_interface != NULL) {
        assert(benchmark_interface->write != NULL);
        assert(benchmark_interface->read_start != NULL);
        assert(benchmark_interface->read_stop != NULL);
        assert(benchmark_interface->close != NULL);
        assert(benchmark_interface->get_stats != NULL);
        assert(benchmark_interface->reset_stats != NULL);
        assert(benchmark_interface->set_error_simulation != NULL);
        uvhttp_network_interface_destroy(benchmark_interface);
    }
    
    printf("test_network_function_pointers: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_network.c 完整覆盖率测试 ===\n\n");

    test_libuv_network_create();
    test_mock_network_create();
    test_benchmark_network_create();
    test_network_interface_create();
    test_network_null_params();
    test_network_interface_destroy();
    test_network_stats_init();
    test_network_get_stats();
    test_network_reset_stats();
    test_network_set_error_simulation();
    test_network_multiple_cycles();
    test_network_handle_field();
    test_network_function_pointers();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}