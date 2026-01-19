#include "test_time_mock.h"

/* 全局时间 Mock 指针 */
TimeMock* g_time_mock = nullptr;

/* 设置时间 Mock */
void set_time_mock(TimeMock* mock) {
    g_time_mock = mock;
}

/* 获取当前时间（使用 Mock 或真实时间） */
extern "C" time_t get_current_time() {
    if (g_time_mock) {
        return g_time_mock->get_time();
    }
    return time(NULL);
}

/* 推进时间（用于测试） */
void advance_time(time_t seconds) {
    if (g_time_mock) {
        g_time_mock->advance_time(seconds);
    }
}