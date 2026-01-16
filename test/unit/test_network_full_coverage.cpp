/* uvhttp_network.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_network.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <stdlib.h>

TEST(UvhttpNetworkTest, InterfaceCreateNull) {
    /* 测试创建 NULL 接口 */
    uvhttp_network_interface_t* interface = uvhttp_network_interface_create(
        (uvhttp_network_type_t)999, NULL);
    EXPECT_EQ(interface, nullptr);
}

TEST(UvhttpNetworkTest, InterfaceDestroyNull) {
    /* 测试销毁 NULL 接口 */
    uvhttp_network_interface_destroy(nullptr);
}

TEST(UvhttpNetworkTest, LibuvNetworkCreate) {
    /* 测试创建 libuv 网络接口 */
    uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(nullptr);
    
    if (interface) {
        EXPECT_NE(interface, nullptr);
        EXPECT_EQ(interface->type, UVHTTP_NETWORK_LIBUV);
        EXPECT_NE(interface->write, nullptr);
        EXPECT_NE(interface->read_start, nullptr);
        EXPECT_NE(interface->read_stop, nullptr);
        EXPECT_NE(interface->close, nullptr);
        EXPECT_NE(interface->get_stats, nullptr);
        EXPECT_NE(interface->reset_stats, nullptr);
        EXPECT_NE(interface->set_error_simulation, nullptr);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkCreate) {
    /* 测试创建 mock 网络接口 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        EXPECT_NE(interface, nullptr);
        EXPECT_EQ(interface->type, UVHTTP_NETWORK_MOCK);
        EXPECT_NE(interface->write, nullptr);
        EXPECT_NE(interface->read_start, nullptr);
        EXPECT_NE(interface->read_stop, nullptr);
        EXPECT_NE(interface->close, nullptr);
        EXPECT_NE(interface->get_stats, nullptr);
        EXPECT_NE(interface->reset_stats, nullptr);
        EXPECT_NE(interface->set_error_simulation, nullptr);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkCreate) {
    /* 测试创建 benchmark 网络接口 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        EXPECT_NE(interface, nullptr);
        EXPECT_EQ(interface->type, UVHTTP_NETWORK_BENCHMARK);
        EXPECT_NE(interface->write, nullptr);
        EXPECT_NE(interface->read_start, nullptr);
        EXPECT_NE(interface->read_stop, nullptr);
        EXPECT_NE(interface->close, nullptr);
        EXPECT_NE(interface->get_stats, nullptr);
        EXPECT_NE(interface->reset_stats, nullptr);
        EXPECT_NE(interface->set_error_simulation, nullptr);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, InterfaceCreateAllTypes) {
    /* 测试创建所有类型的网络接口 */
    uvhttp_network_interface_t* libuv = uvhttp_network_interface_create(
        UVHTTP_NETWORK_LIBUV, nullptr);
    EXPECT_NE(libuv, nullptr);
    
    uvhttp_network_interface_t* mock = uvhttp_network_interface_create(
        UVHTTP_NETWORK_MOCK, nullptr);
    EXPECT_NE(mock, nullptr);
    
    uvhttp_network_interface_t* benchmark = uvhttp_network_interface_create(
        UVHTTP_NETWORK_BENCHMARK, nullptr);
    EXPECT_NE(benchmark, nullptr);
    
    uvhttp_network_interface_destroy(libuv);
    uvhttp_network_interface_destroy(mock);
    uvhttp_network_interface_destroy(benchmark);
}

TEST(UvhttpNetworkTest, InterfaceStats) {
    /* 测试接口统计 */
    uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(nullptr);
    
    if (interface) {
        /* 初始统计 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        /* 获取统计 */
        void* stats = interface->get_stats(interface);
        EXPECT_NE(stats, nullptr);
        EXPECT_EQ(stats, interface);
        
        /* 重置统计 */
        interface->reset_stats(interface);
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->error_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, InterfaceSetErrorSimulation) {
    /* 测试错误模拟 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        /* 设置错误模拟 */
        interface->set_error_simulation(interface, UV_ENOMEM);
        
        /* 重置错误模拟 */
        interface->set_error_simulation(interface, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkWrite) {
    /* 测试 mock 网络写入 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        const char* data = "Hello, World!";
        uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
        
        /* 写入数据 */
        int result = interface->write(interface, nullptr, &buf, 1, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_GT(interface->write_count, 0);
        EXPECT_GT(interface->bytes_sent, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkWriteError) {
    /* 测试 mock 网络写入错误 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        /* 设置错误模拟 */
        interface->set_error_simulation(interface, UV_ENOMEM);
        
        const char* data = "Hello, World!";
        uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
        
        /* 写入数据 - 应该失败 */
        int result = interface->write(interface, nullptr, &buf, 1, nullptr);
        EXPECT_EQ(result, UV_ENOMEM);
        
        /* 检查错误统计 */
        EXPECT_GT(interface->error_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkReadStart) {
    /* 测试 mock 网络读取启动 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        /* 启动读取 */
        int result = interface->read_start(interface, nullptr, nullptr, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_GT(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkReadStop) {
    /* 测试 mock 网络读取停止 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        /* 停止读取 */
        int result = interface->read_stop(interface, nullptr);
        EXPECT_EQ(result, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkClose) {
    /* 测试 mock 网络关闭 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        /* 关闭连接 */
        int result = interface->close(interface, nullptr, nullptr);
        EXPECT_EQ(result, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkWrite) {
    /* 测试 benchmark 网络写入 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        const char* data = "Hello, World!";
        uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
        
        /* 写入数据 */
        int result = interface->write(interface, nullptr, &buf, 1, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_GT(interface->write_count, 0);
        EXPECT_GT(interface->bytes_sent, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkReadStart) {
    /* 测试 benchmark 网络读取启动 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        /* 启动读取 */
        int result = interface->read_start(interface, nullptr, nullptr, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_GT(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkReadStop) {
    /* 测试 benchmark 网络读取停止 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        /* 停止读取 */
        int result = interface->read_stop(interface, nullptr);
        EXPECT_EQ(result, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkClose) {
    /* 测试 benchmark 网络关闭 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        /* 关闭连接 */
        int result = interface->close(interface, nullptr, nullptr);
        EXPECT_EQ(result, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkMultipleWrites) {
    /* 测试 mock 网络多次写入 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        const char* data1 = "Hello";
        const char* data2 = "World";
        uv_buf_t bufs[] = {
            uv_buf_init((char*)data1, strlen(data1)),
            uv_buf_init((char*)data2, strlen(data2))
        };
        
        /* 写入数据 */
        int result = interface->write(interface, nullptr, bufs, 2, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_EQ(interface->write_count, 1);
        EXPECT_EQ(interface->bytes_sent, strlen(data1) + strlen(data2));
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkResetStats) {
    /* 测试 mock 网络重置统计 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        const char* data = "Hello";
        uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
        
        /* 写入数据 */
        interface->write(interface, nullptr, &buf, 1, nullptr);
        
        /* 检查统计 */
        EXPECT_GT(interface->write_count, 0);
        EXPECT_GT(interface->bytes_sent, 0);
        
        /* 重置统计 */
        interface->reset_stats(interface);
        
        /* 检查统计已重置 */
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, NetworkTypeEnums) {
    /* 测试网络类型枚举 */
    EXPECT_EQ(UVHTTP_NETWORK_LIBUV, 0);
    EXPECT_EQ(UVHTTP_NETWORK_MOCK, 1);
    EXPECT_EQ(UVHTTP_NETWORK_BENCHMARK, 2);
}

TEST(UvhttpNetworkTest, InterfaceSizeValidation) {
    /* 测试接口结构体大小 */
    EXPECT_GE(sizeof(uvhttp_network_interface_t), 64);
}

TEST(UvhttpNetworkTest, GlobalInterfaceNull) {
    /* 测试全局接口为 NULL */
    EXPECT_EQ(g_uvhttp_network_interface, nullptr);
}

TEST(UvhttpNetworkTest, InterfaceNullPointers) {
    /* 测试接口空指针 */
    uvhttp_network_interface_t interface;
    memset(&interface, 0, sizeof(interface));
    
    /* 初始状态 */
    EXPECT_EQ(interface.write, nullptr);
    EXPECT_EQ(interface.read_start, nullptr);
    EXPECT_EQ(interface.read_stop, nullptr);
    EXPECT_EQ(interface.close, nullptr);
    EXPECT_EQ(interface.get_stats, nullptr);
    EXPECT_EQ(interface.reset_stats, nullptr);
    EXPECT_EQ(interface.set_error_simulation, nullptr);
    EXPECT_EQ(interface.handle, nullptr);
}

TEST(UvhttpNetworkTest, MockNetworkEmptyWrite) {
    /* 测试 mock 网络空写入 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        uv_buf_t buf = uv_buf_init(nullptr, 0);
        
        /* 写入空数据 */
        int result = interface->write(interface, nullptr, &buf, 1, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_GT(interface->write_count, 0);
        EXPECT_EQ(interface->bytes_sent, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkMultipleWrites) {
    /* 测试 benchmark 网络多次写入 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        const char* data1 = "Hello";
        const char* data2 = "World";
        uv_buf_t bufs[] = {
            uv_buf_init((char*)data1, strlen(data1)),
            uv_buf_init((char*)data2, strlen(data2))
        };
        
        /* 写入数据 */
        int result = interface->write(interface, nullptr, bufs, 2, nullptr);
        EXPECT_EQ(result, 0);
        
        /* 检查统计 */
        EXPECT_EQ(interface->write_count, 1);
        EXPECT_EQ(interface->bytes_sent, strlen(data1) + strlen(data2));
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkResetStats) {
    /* 测试 benchmark 网络重置统计 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        const char* data = "Hello";
        uv_buf_t buf = uv_buf_init((char*)data, strlen(data));
        
        /* 写入数据 */
        interface->write(interface, nullptr, &buf, 1, nullptr);
        
        /* 检查统计 */
        EXPECT_GT(interface->write_count, 0);
        EXPECT_GT(interface->bytes_sent, 0);
        
        /* 重置统计 */
        interface->reset_stats(interface);
        
        /* 检查统计已重置 */
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, LibuvNetworkStats) {
    /* 测试 libuv 网络统计 */
    uvhttp_network_interface_t* interface = uvhttp_libuv_network_create(nullptr);
    
    if (interface) {
        /* 初始统计 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        /* 重置统计 */
        interface->reset_stats(interface);
        
        /* 检查统计已重置 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, MockNetworkStats) {
    /* 测试 mock 网络统计 */
    uvhttp_network_interface_t* interface = uvhttp_mock_network_create(nullptr);
    
    if (interface) {
        /* 初始统计 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        /* 重置统计 */
        interface->reset_stats(interface);
        
        /* 检查统计已重置 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}

TEST(UvhttpNetworkTest, BenchmarkNetworkStats) {
    /* 测试 benchmark 网络统计 */
    uvhttp_network_interface_t* interface = uvhttp_benchmark_network_create(nullptr);
    
    if (interface) {
        /* 初始统计 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        /* 重置统计 */
        interface->reset_stats(interface);
        
        /* 检查统计已重置 */
        EXPECT_EQ(interface->bytes_sent, 0);
        EXPECT_EQ(interface->bytes_received, 0);
        EXPECT_EQ(interface->error_count, 0);
        EXPECT_EQ(interface->write_count, 0);
        EXPECT_EQ(interface->read_count, 0);
        
        uvhttp_network_interface_destroy(interface);
    }
}