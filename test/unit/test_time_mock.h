#ifndef TEST_TIME_MOCK_H
#define TEST_TIME_MOCK_H

#include <gmock/gmock.h>
#include <time.h>

/* 时间 Mock 对象 */
class TimeMock {
public:
    virtual ~TimeMock() = default;
    virtual time_t get_time() const = 0;
    virtual void advance_time(time_t seconds) = 0;
};

/* 真实时间实现 */
class RealTime : public TimeMock {
public:
    time_t get_time() const override {
        return time(NULL);
    }
    
    void advance_time(time_t seconds) override {
        /* 真实时间无法手动推进 */
        (void)seconds;
    }
};

/* Mock 时间实现 */
class MockTime : public TimeMock {
public:
    MockTime() : current_time_(0) {}
    
    void set_current_time(time_t time) {
        current_time_ = time;
    }
    
    time_t get_time() const override {
        return current_time_;
    }
    
    void advance_time(time_t seconds) override {
        current_time_ += seconds;
    }
    
private:
    time_t current_time_;
};

/* 全局时间 Mock 指针 */
extern TimeMock* g_time_mock;

/* 设置时间 Mock */
void set_time_mock(TimeMock* mock);

/* 获取当前时间（使用 Mock 或真实时间） */
extern "C" time_t get_current_time();

/* 推进时间（用于测试） */
void advance_time(time_t seconds);

#endif /* TEST_TIME_MOCK_H */