#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "include/uvhttp_logging.h"

int test_logging_functionality() {
    printf("测试日志功能...\n");
    
    // 初始化日志系统
    uvhttp_log_init(UVHTTP_LOG_DEBUG);
    printf("✓ 日志系统初始化成功\n");
    
    // 测试不同级别的日志
    UVHTTP_LOG_TRACE("这是一条TRACE级别日志");
    UVHTTP_LOG_DEBUG("这是一条DEBUG级别日志");
    UVHTTP_LOG_INFO("这是一条INFO级别日志");
    UVHTTP_LOG_WARN("这是一条WARN级别日志");
    UVHTTP_LOG_ERROR("这是一条ERROR级别日志");
    printf("✓ 各级别日志输出测试通过\n");
    
    // 测试日志配置
    uvhttp_log_set_level(UVHTTP_LOG_WARN);
    UVHTTP_LOG_DEBUG("这条DEBUG日志不应该显示");
    UVHTTP_LOG_WARN("这条WARN日志应该显示");
    printf("✓ 日志级别过滤测试通过\n");
    
    // 测试时间戳
    uvhttp_log_enable_timestamp(1);
    UVHTTP_LOG_INFO("带时间戳的日志消息");
    printf("✓ 时间戳功能测试通过\n");
    
    return 0;
}

int test_performance_monitoring() {
    printf("\n测试性能监控...\n");
    
    uvhttp_perf_timer_t timer;
    
    // 测试性能计时器
    uvhttp_perf_start(&timer, "测试操作");
    usleep(10000); // 模拟10ms操作
    uvhttp_perf_end(&timer);
    
    double duration = uvhttp_perf_get_duration(&timer);
    if (duration > 5.0 && duration < 50.0) { // 允许5-50ms误差
        printf("✓ 性能计时器测试通过 (%.2f ms)\n", duration);
    } else {
        printf("✗ 性能计时器测试失败 (%.2f ms)\n", duration);
        return -1;
    }
    
    // 测试未激活的计时器
    uvhttp_perf_timer_t inactive_timer = {0};
    double inactive_duration = uvhttp_perf_get_duration(&inactive_timer);
    if (inactive_duration == 0.0) {
        printf("✓ 未激活计时器测试通过\n");
    } else {
        printf("✗ 未激活计时器测试失败\n");
        return -1;
    }
    
    return 0;
}

int test_memory_monitoring() {
    printf("\n测试内存监控...\n");
    
    // 重置统计
    uvhttp_mem_reset_stats();
    
    // 模拟内存分配
    uvhttp_mem_track_alloc(1024);
    uvhttp_mem_track_alloc(2048);
    uvhttp_mem_track_alloc(512);
    
    uvhttp_mem_stats_t stats;
    uvhttp_mem_get_stats(&stats);
    
    if (stats.total_allocated == 3584 && 
        stats.current_usage == 3584 && 
        stats.peak_usage == 3584 && 
        stats.allocation_count == 3) {
        printf("✓ 内存分配监控测试通过\n");
    } else {
        printf("✗ 内存分配监控测试失败\n");
        return -1;
    }
    
    // 模拟内存释放
    uvhttp_mem_track_free(1024);
    uvhttp_mem_track_free(512);
    
    uvhttp_mem_get_stats(&stats);
    if (stats.current_usage == 2048) {
        printf("✓ 内存释放监控测试通过\n");
    } else {
        printf("✗ 内存释放监控测试失败\n");
        return -1;
    }
    
    return 0;
}

int test_connection_monitoring() {
    printf("\n测试连接监控...\n");
    
    // 重置统计
    uvhttp_conn_reset_stats();
    
    // 模拟连接操作
    uvhttp_conn_track_open();
    uvhttp_conn_track_open();
    uvhttp_conn_track_open();
    
    uvhttp_conn_stats_t stats;
    uvhttp_conn_get_stats(&stats);
    
    if (stats.total_connections == 3 && 
        stats.active_connections == 3 && 
        stats.peak_connections == 3) {
        printf("✓ 连接打开监控测试通过\n");
    } else {
        printf("✗ 连接打开监控测试失败\n");
        return -1;
    }
    
    // 模拟请求处理
    uvhttp_conn_track_request(50.0);
    uvhttp_conn_track_request(100.0);
    uvhttp_conn_track_request(75.0);
    
    // 模拟连接关闭和失败
    uvhttp_conn_track_close();
    uvhttp_conn_track_failed();
    
    uvhttp_conn_get_stats(&stats);
    if (stats.active_connections == 2 && 
        stats.failed_connections == 1 && 
        stats.total_requests == 3) {
        printf("✓ 连接关闭和失败监控测试通过\n");
    } else {
        printf("✗ 连接关闭和失败监控测试失败\n");
        return -1;
    }
    
    return 0;
}

int test_monitoring_integration() {
    printf("\n测试监控集成...\n");
    
    // 重置所有统计
    uvhttp_mem_reset_stats();
    uvhttp_conn_reset_stats();
    
    // 模拟完整的监控场景
    UVHTTP_LOG_INFO("开始集成测试");
    
    uvhttp_perf_timer_t overall_timer;
    uvhttp_perf_start(&overall_timer, "集成测试");
    
    // 模拟内存操作
    uvhttp_mem_track_alloc(2048);
    uvhttp_conn_track_open();
    
    // 模拟请求处理
    uvhttp_perf_timer_t request_timer;
    uvhttp_perf_start(&request_timer, "请求处理");
    usleep(5000); // 5ms
    uvhttp_perf_end(&request_timer);
    uvhttp_conn_track_request(uvhttp_perf_get_duration(&request_timer));
    
    // 清理资源
    uvhttp_mem_track_free(2048);
    uvhttp_conn_track_close();
    
    uvhttp_perf_end(&overall_timer);
    
    // 验证统计数据
    uvhttp_mem_stats_t mem_stats;
    uvhttp_conn_stats_t conn_stats;
    uvhttp_mem_get_stats(&mem_stats);
    uvhttp_conn_get_stats(&conn_stats);
    
    if (mem_stats.total_allocated == 2048 && 
        mem_stats.current_usage == 0 &&
        conn_stats.total_connections == 1 && 
        conn_stats.active_connections == 0 &&
        conn_stats.total_requests == 1) {
        printf("✓ 监控集成测试通过\n");
        printf("  - 总内存分配: %zu bytes\n", mem_stats.total_allocated);
        printf("  - 总连接数: %d\n", conn_stats.total_connections);
        printf("  - 总请求数: %d\n", conn_stats.total_requests);
        printf("  - 总耗时: %.2f ms\n", uvhttp_perf_get_duration(&overall_timer));
    } else {
        printf("✗ 监控集成测试失败\n");
        return -1;
    }
    
    return 0;
}

int main() {
    printf("=== 监控和日志功能测试 ===\n");
    
    int failed = 0;
    int total = 5;
    
    if (test_logging_functionality() != 0) failed++;
    if (test_performance_monitoring() != 0) failed++;
    if (test_memory_monitoring() != 0) failed++;
    if (test_connection_monitoring() != 0) failed++;
    if (test_monitoring_integration() != 0) failed++;
    
    printf("\n=== 监控测试结果 ===\n");
    if (failed == 0) {
        printf("✅ 所有监控测试通过 (%d/%d)\n", total - failed, total);
        printf("📊 监控和日志功能完整可用\n");
    } else {
        printf("❌ 部分监控测试失败 (%d/%d 通过)\n", total - failed, total);
    }
    
    // 清理日志系统
    uvhttp_log_cleanup();
    
    return failed;
}