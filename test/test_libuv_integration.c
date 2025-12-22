#include "../deps/googletest/gtest_fixed.h"
#include "../include/uvhttp.h"
#include <uv.h>
#include <stdlib.h>
#include <string.h>

// libuv事件循环测试夹具
typedef struct {
    uv_loop_t* loop;
    int setup_completed;
    int callback_called;
} LibuvTest;

// 定义libuv测试夹具类
DEFINE_TEST_F(LibuvTest);

void LibuvTest_setup(LibuvTest* fixture) {
    fixture->loop = malloc(sizeof(uv_loop_t));
    if (fixture->loop) {
        uv_loop_init(fixture->loop);
        fixture->setup_completed = 1;
        fixture->callback_called = 0;
    }
}

void LibuvTest_teardown(LibuvTest* fixture) {
    if (fixture->loop) {
        uv_loop_close(fixture->loop);
        free(fixture->loop);
        fixture->loop = NULL;
    }
    fixture->setup_completed = 0;
}

// 回调函数用于测试
static void dummy_callback(uv_async_t* handle) {
    LibuvTest* test = (LibuvTest*)handle->data;
    test->callback_called = 1;
    uv_close((uv_handle_t*)handle, NULL);
}

// 测试libuv事件循环
TEST_F(LibuvTest, EventLoopBasic) {
    EXPECT_NOTNULL_PTR(fixture->loop);
    EXPECT_EQ(fixture->setup_completed, 1);
    
TEST_CLEANUP_LABEL:
    return;
}

TEST_F(LibuvTest, AsyncHandleTest) {
    uv_async_t async_handle;
    async_handle.data = fixture;
    
    int ret = uv_async_init(fixture->loop, &async_handle, dummy_callback);
    EXPECT_EQ(ret, 0);
    
    // 触发异步回调
    uv_async_send(&async_handle);
    
    // 运行事件循环直到回调被调用
    while (!fixture->callback_called) {
        uv_run(fixture->loop, UV_RUN_NOWAIT);
    }
    
    EXPECT_EQ(fixture->callback_called, 1);
    
TEST_CLEANUP_LABEL:
    return;
}

// 测试HTTP服务器的libuv集成
TEST_F(LibuvTest, ServerCreation) {
    uvhttp_server_t* server = uvhttp_server_new(fixture->loop);
    EXPECT_NOTNULL_PTR(server);
    EXPECT_EQ(server->loop, fixture->loop);
    EXPECT_EQ(server->is_listening, 0);
    
    // 设置处理函数
    void test_handler(uvhttp_request_t* request, uvhttp_response_t* response) {
        // 简单的测试处理函数
        uvhttp_response_set_status(response, 200);
        uvhttp_response_set_header(response, "Content-Type", "text/plain");
        uvhttp_response_set_body(response, "OK", 2);
    }
    
    uvhttp_server_set_handler(server, test_handler);
    EXPECT_EQ(server->handler, test_handler);
    
    // 清理
    uvhttp_server_free(server);
    
TEST_CLEANUP_LABEL:
    return;
}

// 测试连接管理
TEST_F(LibuvTest, ConnectionManagement) {
    uvhttp_server_t* server = uvhttp_server_new(fixture->loop);
    EXPECT_NOTNULL_PTR(server);
    
    // 测试连接计数
    EXPECT_EQ(server->active_connections, 0);
    
    uvhttp_server_free(server);
    
TEST_CLEANUP_LABEL:
    return;
}

// 普通测试（不使用夹具）
TEST(LibuvTestWithoutFixture, VersionCheck) {
    // 测试libuv版本
    EXPECT_EQ(uv_version(), 13759); // libuv 1.x.x的版本号
    
TEST_CLEANUP_LABEL:
    return;
}

RUN_ALL_TESTS()