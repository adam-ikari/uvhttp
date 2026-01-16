#ifndef TEST_LOOP_HELPER_H
#define TEST_LOOP_HELPER_H

#include <uv.h>
#include "uvhttp_allocator.h"
#include "uvhttp_features.h"

/**
 * @brief 测试循环辅助类，确保每个测试使用独立的 libuv 循环
 * 
 * 单元测试原则：
 * 1. 隔离性：每个测试应该独立运行，不共享全局状态
 * 2. 可重复性：多次运行应该产生相同的结果
 * 3. 清理：测试结束后必须清理所有资源
 * 
 * 使用示例：
 * TEST(MyTest, TestCase) {
 *     TestLoop loop;
 *     uv_tcp_t client;
 *     uv_tcp_init(loop.get(), &client);
 *     // ... 测试逻辑
 *     // loop 析构时自动清理
 * }
 */
class TestLoop {
public:
    TestLoop() : initialized_(false) {
        loop_ = (uv_loop_t*)uvhttp_alloc(sizeof(uv_loop_t));
        if (loop_) {
            int result = uv_loop_init(loop_);
            if (result == 0) {
                initialized_ = true;
            } else {
                uvhttp_free(loop_);
                loop_ = nullptr;
            }
        }
    }
    
    ~TestLoop() {
        if (loop_ && initialized_) {
            uv_loop_close(loop_);
            uvhttp_free(loop_);
        }
    }
    
    /* 禁止拷贝和赋值 */
    TestLoop(const TestLoop&) = delete;
    TestLoop& operator=(const TestLoop&) = delete;
    
    /* 获取循环指针 */
    uv_loop_t* get() const { return loop_; }
    
    /* 运行循环（单次迭代） */
    int run_once() {
        if (!loop_) return -1;
        return uv_run(loop_, UV_RUN_ONCE);
    }
    
    /* 运行循环（直到没有更多活动） */
    int run() {
        if (!loop_) return -1;
        return uv_run(loop_, UV_RUN_DEFAULT);
    }
    
    /* 检查循环是否有效 */
    bool is_valid() const { return loop_ != nullptr && initialized_; }

private:
    uv_loop_t* loop_;
    bool initialized_;
};

#endif /* TEST_LOOP_HELPER_H */