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
            /* 关闭所有未关闭的句柄并驱动循环处理 close 回调，否则
             * uv_loop_close 会因 UV_EBUSY 失败，导致循环内部分配的内存
             * （uv_loop_init 分配的结构、句柄数组 realloc）泄漏。 */
            close_all_handles_();
            /* 处理挂起的 close 回调，使句柄真正被回收 */
            uv_run(loop_, UV_RUN_DEFAULT);
            /* 现在循环中已无活动句柄，可以安全关闭 */
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
    /* uv_walk 回调：关闭尚未进入关闭状态的句柄 */
    static void close_handle_cb_(uv_handle_t* handle, void* /*arg*/) {
        if (handle && !uv_is_closing(handle)) {
            uv_close(handle, /*close_cb*/ NULL);
        }
    }

    void close_all_handles_() {
        if (loop_) {
            uv_walk(loop_, &TestLoop::close_handle_cb_, NULL);
        }
    }

    uv_loop_t* loop_;
    bool initialized_;
};

#endif /* TEST_LOOP_HELPER_H */