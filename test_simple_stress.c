#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/resource.h>

// 简单的压力测试

typedef struct {
    int total_requests;
    int successful_requests;
    int failed_requests;
    double total_response_time;
    double min_response_time;
    double max_response_time;
    double start_time;
    double end_time;
} stress_stats_t;

static volatile int running = 1;

// 获取当前时间戳（毫秒）
static double get_time_ms(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec * 1000.0 + tv.tv_usec / 1000.0;
}

// 获取内存使用量（KB）
static size_t get_memory_usage(void) {
    struct rusage usage;
    getrusage(RUSAGE_SELF, &usage);
    return usage.ru_maxrss;
}

// 发送HTTP请求
static int send_http_request(const char* host, int port, stress_stats_t* stats) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        return -1;
    }
    
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host, &server_addr.sin_addr);
    
    double start_time = get_time_ms();
    
    // 连接服务器
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    
    double connect_time = get_time_ms();
    
    // 发送HTTP请求
    const char* request = 
        "GET / HTTP/1.1\r\n"
        "Host: localhost\r\n"
        "User-Agent: simple-stress-test/1.0\r\n"
        "Connection: close\r\n"
        "\r\n";
    
    if (send(sockfd, request, strlen(request), 0) < 0) {
        close(sockfd);
        return -1;
    }
    
    // 读取响应
    char buffer[4096];
    int bytes_received = 0;
    int header_complete = 0;
    
    while (running && !header_complete) {
        int n = recv(sockfd, buffer, sizeof(buffer), 0);
        if (n <= 0) {
            break;
        }
        
        bytes_received += n;
        
        // 检查HTTP头是否完整
        if (!header_complete && bytes_received >= 4) {
            for (int i = 0; i <= bytes_received - 4; i++) {
                if (memcmp(buffer + i, "\r\n\r\n", 4) == 0) {
                    header_complete = 1;
                    break;
                }
            }
        }
    }
    
    double end_time = get_time_ms();
    close(sockfd);
    
    // 更新统计
    double response_time = end_time - start_time;
    stats->total_requests++;
    
    if (header_complete) {
        stats->successful_requests++;
        stats->total_response_time += response_time;
        
        if (response_time < stats->min_response_time || stats->min_response_time == 0) {
            stats->min_response_time = response_time;
        }
        if (response_time > stats->max_response_time) {
            stats->max_response_time = response_time;
        }
    } else {
        stats->failed_requests++;
    }
    
    return header_complete ? 0 : -1;
}

// 信号处理
static void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        running = 0;
        printf("\n收到停止信号，正在停止测试...\n");
    }
}

int main() {
    printf("UVHTTP 简单压力测试\n");
    printf("===================\n");
    
    // 设置信号处理
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    printf("系统信息:\n");
    printf("  CPU核心数: %d\n", sysconf(_SC_NPROCESSORS_ONLN));
    printf("  总内存: %ld MB\n", sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGESIZE) / (1024 * 1024));
    
    stress_stats_t stats = {0};
    stats.start_time = get_time_ms();
    
    const char* host = "127.0.0.1";
    const int port = 8080;
    const int test_duration = 30; // 30秒
    const int target_rps = 1000;
    
    printf("\n开始压力测试:\n");
    printf("  目标服务器: %s:%d\n", host, port);
    printf("  测试持续时间: %d 秒\n", test_duration);
    printf("  目标RPS: %d\n", target_rps);
    printf("\n");
    
    size_t initial_memory = get_memory_usage();
    
    // 主测试循环
    double last_report_time = stats.start_time;
    int requests_this_second = 0;
    double test_start = stats.start_time;
    
    while (running) {
        double current_time = get_time_ms();
        double elapsed = (current_time - test_start) / 1000.0;
        
        // 检查是否达到测试时间
        if (elapsed >= test_duration) {
            break;
        }
        
        // 控制请求速率
        if (current_time - last_report_time >= 1000.0) {
            double actual_rps = requests_this_second;
            printf("\r[%6.2fs] 请求: %d (成功: %d, 失败: %d), RPS: %.0f, "
                   "响应时间: %.3f/%.3f/%.3f ms",
                   elapsed,
                   stats.total_requests,
                   stats.successful_requests,
                   stats.failed_requests,
                   actual_rps,
                   stats.min_response_time,
                   stats.successful_requests > 0 ? stats.total_response_time / stats.successful_requests : 0,
                   stats.max_response_time);
            fflush(stdout);
            
            last_report_time = current_time;
            requests_this_second = 0;
        }
        
        // 发送请求
        if (requests_this_second < target_rps) {
            send_http_request(host, port, &stats);
            requests_this_second++;
        } else {
            // 等待下一秒
            usleep(1000); // 1ms
        }
    }
    
    stats.end_time = get_time_ms();
    size_t final_memory = get_memory_usage();
    
    // 输出最终结果
    double total_duration = (stats.end_time - stats.start_time) / 1000.0;
    double actual_rps = stats.total_requests / total_duration;
    
    printf("\n\n--- 压力测试结果 ---\n");
    printf("测试持续时间: %.2f 秒\n", total_duration);
    printf("总请求数: %d\n", stats.total_requests);
    printf("成功请求: %d (%.1f%%)\n", stats.successful_requests,
           stats.total_requests > 0 ? (double)stats.successful_requests / stats.total_requests * 100.0 : 0.0);
    printf("失败请求: %d (%.1f%%)\n", stats.failed_requests,
           stats.total_requests > 0 ? (double)stats.failed_requests / stats.total_requests * 100.0 : 0.0);
    printf("目标RPS: %d\n", target_rps);
    printf("实际RPS: %.1f\n", actual_rps);
    printf("RPS达成率: %.1f%%\n", (double)actual_rps / target_rps * 100.0);
    printf("平均响应时间: %.3f ms\n", 
           stats.successful_requests > 0 ? stats.total_response_time / stats.successful_requests : 0.0);
    printf("最小响应时间: %.3f ms\n", stats.min_response_time);
    printf("最大响应时间: %.3f ms\n", stats.max_response_time);
    printf("内存使用变化: %ld KB\n", final_memory - initial_memory);
    
    printf("\n压力测试完成！\n");
    
    return 0;
}