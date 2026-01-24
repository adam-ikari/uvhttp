/**
 * @file test_stress.cpp
 * @brief UVHTTP 压力测试（简化版）
 *
 * 压力测试用于验证系统在高负载下的稳定性和性能
 */

#include <gtest/gtest.h>
#include <uv.h>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include "uvhttp.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_response.h"
#include "uvhttp_config.h"
#include "uvhttp_allocator.h"

// 压力测试配置
#define STRESS_TEST_HOST "127.0.0.1"
#define STRESS_TEST_MAX_CONNECTIONS 100
#define STRESS_TEST_DURATION_SECONDS 5

// 全局变量用于压力测试
static std::atomic<int> g_request_count{0};
static std::atomic<int> g_error_count{0};
static uvhttp_server_t* g_stress_server = nullptr;
static uv_loop_t* g_stress_loop = nullptr;
static int g_stress_test_port = 0;

/**
 * @brief 获取动态端口
 */
static int get_dynamic_port() {
    // 使用固定端口范围，避免冲突
    static int port = 18888;
    return port++;
}

/**
 * @brief 简单的请求处理器
 */
static int simple_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    g_request_count++;
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "text/plain");
    uvhttp_response_set_body(res, "OK", 2);
    return uvhttp_response_send(res);
}

/**
 * @brief 设置压力测试服务器
 */
static void setup_stress_server() {
    g_stress_test_port = get_dynamic_port();

    g_stress_loop = uv_loop_new();
    ASSERT_NE(g_stress_loop, nullptr);

    g_stress_server = uvhttp_server_new(g_stress_loop);
    ASSERT_NE(g_stress_server, nullptr);

    uvhttp_router_t* router = uvhttp_router_new();
    ASSERT_NE(router, nullptr);

    // 添加路由
    uvhttp_router_add_route(router, "/", simple_handler);

    uvhttp_server_set_router(g_stress_server, router);

    uvhttp_error_t result = uvhttp_server_listen(g_stress_server, STRESS_TEST_HOST, g_stress_test_port);
    ASSERT_EQ(result, UVHTTP_OK);
}

/**
 * @brief 清理压力测试服务器
 */
static void teardown_stress_server() {
    if (g_stress_server) {
        uvhttp_server_free(g_stress_server);
        g_stress_server = nullptr;
    }

    if (g_stress_loop) {
        uv_loop_close(g_stress_loop);
        uvhttp_free(g_stress_loop);
        g_stress_loop = nullptr;
    }
}

/**
 * @brief 测试快速连接和断开
 */
TEST(StressTest, RapidConnectDisconnect) {
    g_request_count = 0;
    g_error_count = 0;

    setup_stress_server();

    // 启动服务器线程
    std::thread server_thread([]() {
        uv_run(g_stress_loop, UV_RUN_DEFAULT);
    });
    server_thread.detach();

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 快速创建和关闭连接
    const int num_connections = 50;

    for (int i = 0; i < num_connections; i++) {
        uv_loop_t* client_loop = uv_loop_new();

        uv_tcp_t* tcp = (uv_tcp_t*)uvhttp_alloc(sizeof(uv_tcp_t));
        uv_tcp_init(client_loop, tcp);

        struct sockaddr_in addr;
        uv_ip4_addr(STRESS_TEST_HOST, g_stress_test_port, &addr);

        uv_connect_t* connect = (uv_connect_t*)uvhttp_alloc(sizeof(uv_connect_t));
        uv_tcp_connect(connect, tcp, (const struct sockaddr*)&addr, [](uv_connect_t* req, int status) {
            if (status != 0) {
                g_error_count++;
            } else {
                g_request_count++;
            }
            uvhttp_free(req->handle);
            uvhttp_free(req);
        });

        uv_run(client_loop, UV_RUN_DEFAULT);
        uv_loop_close(client_loop);
        uvhttp_free(client_loop);
    }

    // 等待服务器处理完所有请求
    std::this_thread::sleep_for(std::chrono::seconds(1));

    teardown_stress_server();

    // 验证结果
    EXPECT_GE(g_request_count.load(), num_connections * 0.8);  // 允许 20% 的误差
}

/**
 * @brief 测试长时间运行
 */
TEST(StressTest, LongRunningStability) {
    g_request_count = 0;
    g_error_count = 0;

    setup_stress_server();

    // 启动服务器线程
    std::thread server_thread([]() {
        uv_run(g_stress_loop, UV_RUN_DEFAULT);
    });
    server_thread.detach();

    // 等待服务器启动
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 持续发送请求一段时间
    auto start_time = std::chrono::steady_clock::now();
    auto end_time = start_time + std::chrono::seconds(STRESS_TEST_DURATION_SECONDS);

    uv_loop_t* client_loop = uv_loop_new();

    while (std::chrono::steady_clock::now() < end_time) {
        uv_tcp_t* tcp = (uv_tcp_t*)uvhttp_alloc(sizeof(uv_tcp_t));
        uv_tcp_init(client_loop, tcp);

        struct sockaddr_in addr;
        uv_ip4_addr(STRESS_TEST_HOST, g_stress_test_port, &addr);

        uv_connect_t* connect = (uv_connect_t*)uvhttp_alloc(sizeof(uv_connect_t));
        uv_tcp_connect(connect, tcp, (const struct sockaddr*)&addr, [](uv_connect_t* req, int status) {
            if (status != 0) {
                g_error_count++;
            } else {
                g_request_count++;
            }
            uvhttp_free(req->handle);
            uvhttp_free(req);
        });

        uv_run(client_loop, UV_RUN_ONCE);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    uv_loop_close(client_loop);
    uvhttp_free(client_loop);

    // 等待服务器处理完所有请求
    std::this_thread::sleep_for(std::chrono::seconds(1));

    teardown_stress_server();

    // 验证结果
    EXPECT_GT(g_request_count.load(), 0);
    EXPECT_LT(g_error_count.load(), g_request_count.load() * 0.1);  // 错误率 < 10%
}

/**
 * @brief 主函数
 */
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}