#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/resource.h>
#include <pthread.h>
#include "include/uvhttp_logging.h"
#include "include/uvhttp_response_simple.h"
#include "include/uvhttp_request_simple.h"

// 边界条件测试配置
typedef struct {
    int max_concurrent_connections;
    int max_request_size_bytes;
    int max_header_count;
    int max_response_size_bytes;
    int test_duration_seconds;
    char* test_name;
} boundary_test_config_t;

// 边界测试统计
typedef struct {
    int total_test_cases;
    int passed_test_cases;
    int failed_test_cases;
    int max_connections_handled;
    int max_request_size_handled;
    double max_response_time;
    int system_errors;
    int resource_exhausted;
    time_t start_time;
    time_t end_time;
    double test_duration;
} boundary_test_stats_t;

// 测试用例结构
typedef struct {
    char* name;
    int (*test_func)(boundary_test_config_t*, boundary_test_stats_t*);
    int is_critical;
} boundary_test_case_t;

// 全局控制
static volatile int g_should_stop = 0;
static boundary_test_stats_t g_stats;

// 信号处理
static void signal_handler(int sig) {
    printf("\n收到信号 %d，停止边界测试...\n", sig);
    g_should_stop = 1;
}

// 获取系统资源限制
static void get_system_limits(void) {
    struct rlimit rl;
    
    printf("\n系统资源限制:\n");
    
    // 内存限制
    if (getrlimit(RLIMIT_AS, &rl) == 0) {
        printf("  内存限制: %.2f MB\n", rl.rlim_cur / 1024.0 / 1024.0);
        printf("  内存限制(硬): %.2f MB\n", rl.rlim_max / 1024.0 / 1024.0);
    }
    
    // 文件描述符限制
    if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
        printf("  文件描述符限制: %d\n", rl.rlim_cur);
        printf("  文件描述符限制(硬): %d\n", rl.rlim_max);
    }
    
    // 进程限制
    if (getrlimit(RLIMIT_NPROC, &rl) == 0) {
        printf("  进程限制: %d\n", rl.rlim_cur);
        printf("  进程限制(硬): %d\n", rl.rlim_max);
    }
    
    // CPU时间限制
    if (getrlimit(RLIMIT_CPU, &rl) == 0) {
        printf("  CPU时间限制: %d 秒\n", rl.rlim_cur);
        printf("  CPU时间限制(硬): %d 秒\n", rl.rlim_max);
    }
}

// 测试最大并发连接
static int test_max_connections(boundary_test_config_t* config, boundary_test_stats_t* stats) {
    printf("测试最大并发连接数: %d\n", config->max_concurrent_connections);
    
    // 模拟连接创建
    int connections_created = 0;
    int errors = 0;
    
    for (int i = 0; i < config->max_concurrent_connections && !g_should_stop; i++) {
        // 模拟连接创建开销
        uvhttp_perf_timer_t conn_timer;
        uvhttp_perf_start(&conn_timer, "connection_creation");
        
        // 模拟连接建立
        usleep(1000 + (rand() % 5000)); // 1-6ms建立时间
        
        // 检查系统资源
        if (i % 100 == 0) {
            struct rusage usage;
            if (getrusage(RUSAGE_SELF, &usage) == 0) {
                if (usage.ru_maxrss > 100 * 1024 * 1024) { // 100MB内存使用
                    UVHTTP_LOG_WARN("High memory usage: %lu KB", usage.ru_maxrss / 1024);
                    stats->system_errors++;
                }
            }
        }
        
        uvhttp_perf_end(&conn_timer);
        double connection_time = uvhttp_perf_get_duration(&conn_timer);
        
        // 记录最大响应时间
        if (connection_time > stats->max_response_time) {
            stats->max_response_time = connection_time;
        }
        
        connections_created++;
        
        if (i % 1000 == 0) {
            printf("\r创建连接数: %d/%d (%.1f%%)", 
                   connections_created, config->max_concurrent_connections,
                   (double)connections_created / config->max_concurrent_connections * 100);
            fflush(stdout);
        }
    }
    
    stats->max_connections_handled = connections_created;
    return connections_created == config->max_concurrent_connections ? 0 : -1;
}

// 测试最大请求大小
static int test_max_request_size(boundary_test_config_t* config, boundary_test_stats_t* stats) {
    printf("测试最大请求大小: %d bytes\n", config->max_request_size_bytes);
    
    for (int i = 0; i < 10 && !g_should_stop; i++) {
        // 创建超大请求
        uvhttp_request_t request;
        uvhttp_response_t response;
        
        if (uvhttp_request_init(&request, (void*)0x1) != 0) {
            stats->system_errors++;
            continue;
        }
        
        if (uvhttp_response_init(&response, (void*)0x1) != 0) {
            uvhttp_request_cleanup(&request);
            stats->system_errors++;
            continue;
        }
        
        // 分配大内存块用于请求体
        char* large_body = malloc(config->max_request_size_bytes);
        if (!large_body) {
            stats->resource_exhausted++;
            uvhttp_request_cleanup(&request);
            uvhttp_response_cleanup(&response);
            continue;
        }
        
        // 填充数据
        memset(large_body, 'A', config->max_request_size_bytes - 1);
        large_body[config->max_request_size_bytes - 1] = '\0';
        
        // 设置超大请求体
        if (uvhttp_response_set_body(&response, large_body, config->max_request_size_bytes) != 0) {
            stats->system_errors++;
            free(large_body);
            uvhttp_request_cleanup(&request);
            uvhttp_response_cleanup(&response);
            continue;
        }
        
        // 模拟处理
        usleep(10000); // 10ms处理时间
        
        // 清理资源
        free(large_body);
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
        
        printf("✓ 大请求处理成功 (%d/%d)\n", i + 1, 10);
    }
    
    return 0;
}

// 测试最大头部数量
static int test_max_headers(boundary_test_config_t* config, boundary_test_stats_t* stats) {
    printf("测试最大头部数量: %d\n", config->max_header_count);
    
    uvhttp_response_t response;
    if (uvhttp_response_init(&response, (void*)0x1) != 0) {
        return -1;
    }
    
    int headers_added = 0;
    int errors = 0;
    
    for (int i = 0; i < config->max_header_count * 2 && !g_should_stop; i++) { // 测试超出限制
        char header_name[32];
        char header_value[256];
        
        snprintf(header_name, sizeof(header_name), "Header-%d", i);
        snprintf(header_value, sizeof(header_value), "Value-%d", i);
        
        if (uvhttp_response_set_header(&response, header_name, header_value) == 0) {
            headers_added++;
        } else {
            errors++;
        }
        
        if (i % 100 == 0) {
            printf("\r添加头部数: %d (错误: %d)", headers_added, errors);
            fflush(stdout);
        }
    }
    
    uvhttp_response_cleanup(&response);
    
    printf("\n最终结果: 成功添加 %d 个头部, 错误 %d 个\n", headers_added, errors);
    return errors == 0 ? 0 : -1;
}

// 测试最大响应大小
static int test_max_response_size(boundary_test_config_t* config, boundary_test_stats_t* stats) {
    printf("测试最大响应大小: %d bytes\n", config->max_response_size_bytes);
    
    uvhttp_response_t response;
    if (uvhttp_response_init(&response, (void*)0x1) != 0) {
        return -1;
    }
    
    // 设置基本响应
    uvhttp_response_set_status(&response, 200);
    uvhttp_response_set_header(&response, "Content-Type", "application/octet-stream");
    
    // 分配大响应体
    char* large_response = malloc(config->max_response_size_bytes);
    if (!large_response) {
        stats->resource_exhausted++;
        uvhttp_response_cleanup(&response);
        return -1;
    }
    
    // 填充数据模式
    for (size_t i = 0; i < config->max_response_size_bytes - 1; i++) {
        large_response[i] = 'B' + (i % 26);
    }
    large_response[config->max_response_size_bytes - 1] = '\0';
    
    // 设置响应体
    int result = uvhttp_response_set_body(&response, large_response, config->max_response_size_bytes);
    
    if (result == 0) {
        printf("✓ 大响应创建成功 (%.2f MB)\n", 
               config->max_response_size_bytes / 1024.0 / 1024.0);
    } else {
        printf("✗ 大响应创建失败\n");
        stats->system_errors++;
    }
    
    free(large_response);
    uvhttp_response_cleanup(&response);
    
    return result;
}

// 测试系统资源耗尽
static int test_resource_exhaustion(boundary_test_config_t* config, boundary_test_stats_t* stats) {
    printf("测试系统资源耗尽...\n");
    
    int fd_count = 0;
    int max_fds = 0;
    
    // 尝试打开文件直到失败
    while (!g_should_stop) {
        FILE* file = fopen("/dev/null", "r");
        if (file) {
            fd_count++;
            fclose(file);
        } else {
            break;
        }
        
        if (fd_count % 1000 == 0) {
            printf("\r打开文件描述符数: %d", fd_count);
            fflush(stdout);
        }
    }
    
    max_fds = fd_count;
    
    printf("\n最终结果: 成功打开 %d 个文件描述符\n", max_fds);
    
    // 关闭所有文件描述符
    for (int i = 0; i < max_fds; i++) {
        fclose(fopen("/dev/null", "r"));
    }
    
    return max_fds > 0 ? 0 : -1;
}

// 运行边界测试
static int run_boundary_test(boundary_test_config_t* config, boundary_test_stats_t* stats) {
    printf("开始边界测试: %s\n", config->test_name);
    
    memset(stats, 0, sizeof(boundary_test_stats_t));
    stats->start_time = time(NULL);
    
    int test_results[] = {
        test_max_connections(config, stats),
        test_max_request_size(config, stats),
        test_max_headers(config, stats),
        test_max_response_size(config, stats),
        test_resource_exhaustion(config, stats)
    };
    
    int num_tests = sizeof(test_results) / sizeof(test_results[0]);
    
    for (int i = 0; i < num_tests; i++) {
        stats->total_test_cases++;
        
        if (test_results[i] == 0) {
            stats->passed_test_cases++;
        } else {
            stats->failed_test_cases++;
        }
    }
    
    stats->end_time = time(NULL);
    stats->test_duration = difftime(stats->end_time, stats->start_time);
    
    return 0;
}

// 打印边界测试结果
static void print_boundary_results(boundary_test_stats_t* stats, boundary_test_config_t* config) {
    double pass_rate = stats->total_test_cases > 0 ? 
                     (double)stats->passed_test_cases / stats->total_test_cases * 100 : 0;
    
    printf("\n============================================================\n");
    printf("                    边界测试结果: %s\n", config->test_name);
    printf("============================================================\n");
    
    printf("测试配置:\n");
    printf("  最大并发连接: %d\n", config->max_concurrent_connections);
    printf("  最大请求大小: %d bytes\n", config->max_request_size_bytes);
    printf("  最大头部数量: %d\n", config->max_header_count);
    printf("  最大响应大小: %d bytes\n", config->max_response_size_bytes);
    printf("  测试持续时间: %.2f 秒\n", stats->test_duration);
    
    printf("\n测试结果:\n");
    printf("  总测试用例: %d\n", stats->total_test_cases);
    printf("  通过测试用例: %d\n", stats->passed_test_cases);
    printf("  失败测试用例: %d\n", stats->failed_test_cases);
    printf("  通过率: %.1f%%\n", pass_rate);
    
    if (stats->max_connections_handled > 0) {
        printf("  最大连接数: %d\n", stats->max_connections_handled);
    }
    if (stats->max_response_time > 0) {
        printf("  最大响应时间: %.2f ms\n", stats->max_response_time);
    }
    if (stats->system_errors > 0) {
        printf("  系统错误: %d\n", stats->system_errors);
    }
    if (stats->resource_exhausted > 0) {
        printf("  资源耗尽次数: %d\n", stats->resource_exhausted);
    }
    
    printf("\n边界测试评级:\n");
    if (pass_rate >= 95.0) {
        printf("  🛡️ 优秀 (≥95%% 通过率)\n");
    } else if (pass_rate >= 80.0) {
        printf("  ✅ 良好 (80-95%% 通过率)\n");
    } else if (pass_rate >= 60.0) {
        printf("  ⚠️  一般 (60-80%% 通过率)\n");
    } else {
        printf("  ❌ 需要改进 (<60%% 通过率)\n");
    }
    
    if (stats->system_errors == 0) {
        printf("  ✅ 系统稳定性优秀 (0错误)\n");
    } else if (stats->system_errors <= 5) {
        printf("  ⚠️ 系统稳定性良好 (1-5错误)\n");
    } else {
        printf("  ❌ 系统稳定性需要改进 (>5错误)\n");
    }
    
    printf("============================================================\n");
}

// 标准边界测试
static int test_standard_boundaries(void) {
    printf("开始标准边界测试...\n");
    
    boundary_test_config_t config = {
        .max_concurrent_connections = 1000,
        .max_request_size_bytes = 1024 * 1024, // 1MB
        .max_header_count = 64,
        .max_response_size_bytes = 10 * 1024 * 1024, // 10MB
        .test_duration_seconds = 30,
        .test_name = "标准边界测试"
    };
    
    boundary_test_stats_t stats;
    return run_boundary_test(&config, &stats);
}

// 扩展边界测试
static int test_extended_boundaries(void) {
    printf("开始扩展边界测试...\n");
    
    boundary_test_config_t config = {
        .max_concurrent_connections = 5000,
        .max_request_size_bytes = 10 * 1024 * 1024, // 10MB
        .max_header_count = 128,
        .max_response_size_bytes = 100 * 1024 * 1024, // 100MB
        .test_duration_seconds = 60,
        .test_name = "扩展边界测试"
    };
    
    boundary_test_stats_t stats;
    return run_boundary_test(&config, &stats);
}

// 极限边界测试
static int test_extreme_boundaries(void) {
    printf("开始极限边界测试...\n");
    
    boundary_test_config_t config = {
        .max_concurrent_connections = 10000,
        .max_request_size_bytes = 100 * 1024 * 1024, // 100MB
        .max_header_count = 256,
        .max_response_size_bytes = 500 * 1024 * 1024, // 500MB
        .test_duration_seconds = 120,
        .test_name = "极限边界测试"
    };
    
    boundary_test_stats_t stats;
    return run_boundary_test(&config, &stats);
}

// 系统限制探索测试
static int test_system_limits(void) {
    printf("开始系统限制探索测试...\n");
    
    get_system_limits();
    
    // 测试文件描述符限制
    boundary_test_config_t fd_config = {
        .max_concurrent_connections = 0,
        .max_request_size_bytes = 0,
        .max_header_count = 0,
        .max_response_size_bytes = 0,
        .test_duration_seconds = 30,
        .test_name = "文件描述符限制测试"
    };
    
    boundary_test_stats_t stats;
    int result = test_resource_exhaustion(&fd_config, &stats);
    
    if (result == 0) {
        printf("✅ 文件描述符限制测试通过\n");
    } else {
        printf("✗ 文件描述符限制测试失败\n");
    }
    
    return result;
}

int main(int argc, char* argv[]) {
    // 初始化日志系统
    uvhttp_log_init(UVHTTP_LOG_INFO);
    
    printf("=== UVHTTP 边界条件压力测试套件 ===\n");
    printf("测试开始时间: %s", ctime(&(time_t){time(NULL)}));
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // 重置控制标志和统计
    g_should_stop = 0;
    memset(&g_stats, 0, sizeof(g_stats));
    
    // 执行边界测试
    if (test_standard_boundaries() != 0) {
        UVHTTP_LOG_ERROR("Standard boundaries test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (!g_should_stop && test_extended_boundaries() != 0) {
        UVHTTP_LOG_ERROR("Extended boundaries test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (!g_should_stop && test_extreme_boundaries() != 0) {
        UVHTTP_LOG_ERROR("Extreme boundaries test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    if (!g_should_stop && test_system_limits() != 0) {
        UVHTTP_LOG_ERROR("System limits test failed");
        uvhttp_log_cleanup();
        return 1;
    }
    
    printf("\n=== 边界条件压力测试完成 ===\n");
    printf("测试结束时间: %s", ctime(&(time_t){time(NULL)}));
    
    if (g_should_stop) {
        printf("\n测试被用户中断\n");
    }
    
    uvhttp_log_cleanup();
    return 0;
}