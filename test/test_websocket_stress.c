/* WebSocket 压力测试 */
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <pthread.h>

/* 测试配置 */
#define NUM_CONNECTIONS 100
#define MESSAGES_PER_CONNECTION 1000
#define MESSAGE_SIZE 1024
#define TEST_DURATION_SECONDS 30

/* 统计数据 */
static volatile long total_messages_sent = 0;
static volatile long total_messages_received = 0;
static volatile long total_errors = 0;
static volatile int test_running = 1;

/* 性能计时 */
static struct timeval start_time, end_time;

/* 获取当前时间（毫秒） */
static long long get_current_time_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000 + tv.tv_usec / 1000;
}

/* WebSocket 消息处理器 */
static void stress_test_handler(uvhttp_websocket_t* ws, 
                               const uvhttp_websocket_message_t* msg, 
                               void* user_data) {
    __sync_fetch_and_add(&total_messages_received, 1);
}

/* 工作线程函数 */
static void* worker_thread(void* arg) {
    int thread_id = *(int*)arg;
    char message[MESSAGE_SIZE];
    
    /* 准备测试消息 */
    snprintf(message, sizeof(message), 
             "WebSocket 压力测试消息 - 线程 %d - 时间戳 %ld", 
             thread_id, time(NULL));
    
    /* 创建模拟的请求和响应 */
    typedef struct {
        char method[16];
        char url[256];
        char headers[1024];
    } mock_request_t;
    
    typedef struct {
        int status;
        char headers[1024];
        char body[4096];
    } mock_response_t;
    
    mock_request_t request = {
        .method = "GET",
        .url = "/ws",
        .headers = "Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
    };
    mock_response_t response = {0};
    
    /* 创建 WebSocket 连接 */
    uvhttp_websocket_t* ws = uvhttp_websocket_new(
        (uvhttp_request_t*)&request, 
        (uvhttp_response_t*)&response
    );
    
    if (!ws) {
        __sync_fetch_and_add(&total_errors, 1);
        return NULL;
    }
    
    /* 设置消息处理器 */
    uvhttp_websocket_set_handler(ws, stress_test_handler, NULL);
    
    /* 发送消息循环 */
    long messages_sent = 0;
    while (test_running && messages_sent < MESSAGES_PER_CONNECTION) {
        uvhttp_websocket_error_t result = uvhttp_websocket_send_text(ws, message);
        
        if (result == UVHTTP_WEBSOCKET_ERROR_NONE) {
            __sync_fetch_and_add(&total_messages_sent, 1);
            messages_sent++;
        } else {
            __sync_fetch_and_add(&total_errors, 1);
        }
        
        /* 短暂休眠以避免过度占用 CPU */
        usleep(1000); // 1ms
    }
    
    /* 关闭连接 */
    uvhttp_websocket_close(ws, 1000, "压力测试完成");
    uvhttp_websocket_free(ws);
    
    return NULL;
}

/* 打印性能统计 */
static void print_performance_stats() {
    long long duration_ms = (end_time.tv_sec - start_time.tv_sec) * 1000 + 
                           (end_time.tv_usec - start_time.tv_usec) / 1000;
    
    double duration_sec = duration_ms / 1000.0;
    
    printf("\n=== WebSocket 压力测试结果 ===\n");
    printf("测试持续时间: %.2f 秒\n", duration_sec);
    printf("发送消息总数: %ld\n", total_messages_sent);
    printf("接收消息总数: %ld\n", total_messages_received);
    printf("错误总数: %ld\n", total_errors);
    
    if (duration_sec > 0) {
        printf("发送速率: %.2f 消息/秒\n", total_messages_sent / duration_sec);
        printf("接收速率: %.2f 消息/秒\n", total_messages_received / duration_sec);
        printf("总吞吐量: %.2f 消息/秒\n", 
               (total_messages_sent + total_messages_received) / duration_sec);
    }
    
    printf("成功率: %.2f%%\n", 
           total_messages_sent > 0 ? 
           (100.0 * (total_messages_sent - total_errors) / total_messages_sent) : 0.0);
}

/* 单连接压力测试 */
static void test_single_connection_stress() {
    printf("\n=== 单连接压力测试 ===\n");
    
    gettimeofday(&start_time, NULL);
    test_running = 1;
    total_messages_sent = 0;
    total_messages_received = 0;
    total_errors = 0;
    
    /* 创建单个连接发送大量消息 */
    pthread_t thread;
    int thread_id = 0;
    
    if (pthread_create(&thread, NULL, worker_thread, &thread_id) == 0) {
        /* 运行指定时间 */
        sleep(TEST_DURATION_SECONDS);
        test_running = 0;
        
        /* 等待线程完成 */
        pthread_join(thread, NULL);
    }
    
    gettimeofday(&end_time, NULL);
    print_performance_stats();
}

/* 多连接并发测试 */
static void test_multi_connection_stress() {
    printf("\n=== 多连接并发压力测试 ===\n");
    printf("并发连接数: %d\n", NUM_CONNECTIONS);
    
    gettimeofday(&start_time, NULL);
    test_running = 1;
    total_messages_sent = 0;
    total_messages_received = 0;
    total_errors = 0;
    
    /* 创建多个工作线程 */
    pthread_t threads[NUM_CONNECTIONS];
    int thread_ids[NUM_CONNECTIONS];
    
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        thread_ids[i] = i;
        if (pthread_create(&threads[i], NULL, worker_thread, &thread_ids[i]) != 0) {
            printf("创建线程 %d 失败\n", i);
            thread_ids[i] = -1;
        }
    }
    
    /* 运行指定时间 */
    sleep(TEST_DURATION_SECONDS);
    test_running = 0;
    
    /* 等待所有线程完成 */
    for (int i = 0; i < NUM_CONNECTIONS; i++) {
        if (thread_ids[i] != -1) {
            pthread_join(threads[i], NULL);
        }
    }
    
    gettimeofday(&end_time, NULL);
    print_performance_stats();
}

/* 内存压力测试 */
static void test_memory_stress() {
    printf("\n=== 内存压力测试 ===\n");
    
    gettimeofday(&start_time, NULL);
    test_running = 1;
    total_messages_sent = 0;
    total_messages_received = 0;
    total_errors = 0;
    
    /* 创建大量连接并快速销毁，测试内存泄漏 */
    for (int cycle = 0; cycle < 1000 && test_running; cycle++) {
        typedef struct {
            char method[16];
            char url[256];
            char headers[1024];
        } mock_request_t;
        
        typedef struct {
            int status;
            char headers[1024];
            char body[4096];
        } mock_response_t;
        
        mock_request_t request = {
            .method = "GET",
            .url = "/ws",
            .headers = "Upgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n"
        };
        mock_response_t response = {0};
        
        /* 快速创建和销毁连接 */
        for (int i = 0; i < 10; i++) {
            uvhttp_websocket_t* ws = uvhttp_websocket_new(
                (uvhttp_request_t*)&request, 
                (uvhttp_response_t*)&response
            );
            
            if (ws) {
                /* 发送一条消息 */
                uvhttp_websocket_send_text(ws, "内存测试消息");
                __sync_fetch_and_add(&total_messages_sent, 1);
                
                /* 立即关闭 */
                uvhttp_websocket_close(ws, 1000, "内存测试");
                uvhttp_websocket_free(ws);
            } else {
                __sync_fetch_and_add(&total_errors, 1);
            }
        }
        
        /* 每100个周期报告一次 */
        if (cycle % 100 == 0) {
            printf("内存测试周期: %d/1000\n", cycle);
        }
    }
    
    gettimeofday(&end_time, NULL);
    print_performance_stats();
    
    /* 清理全局资源 */
    uvhttp_websocket_cleanup_global();
}

/* 错误处理测试 */
static void test_error_handling() {
    printf("\n=== 错误处理压力测试 ===\n");
    
    gettimeofday(&start_time, NULL);
    test_running = 1;
    total_messages_sent = 0;
    total_messages_received = 0;
    total_errors = 0;
    
    /* 测试各种错误条件 */
    for (int i = 0; i < 10000 && test_running; i++) {
        /* 测试空参数 */
        uvhttp_websocket_t* ws = uvhttp_websocket_new(NULL, NULL);
        if (ws != NULL) {
            __sync_fetch_and_add(&total_errors, 1);
        }
        
        /* 测试发送到空连接 */
        uvhttp_websocket_error_t result = uvhttp_websocket_send_text(NULL, "测试");
        if (result != UVHTTP_WEBSOCKET_ERROR_INVALID_PARAM) {
            __sync_fetch_and_add(&total_errors, 1);
        }
        
        /* 测试关闭空连接 */
        result = uvhttp_websocket_close(NULL, 1000, "测试");
        if (result != UVHTTP_WEBSOCKET_ERROR_INVALID_PARAM) {
            __sync_fetch_and_add(&total_errors, 1);
        }
        
        __sync_fetch_and_add(&total_messages_sent, 1);
    }
    
    gettimeofday(&end_time, NULL);
    print_performance_stats();
}

int main() {
    printf("WebSocket 压力测试开始...\n");
    printf("测试配置:\n");
    printf("- 并发连接数: %d\n", NUM_CONNECTIONS);
    printf("- 每连接消息数: %d\n", MESSAGES_PER_CONNECTION);
    printf("- 消息大小: %d 字节\n", MESSAGE_SIZE);
    printf("- 测试持续时间: %d 秒\n", TEST_DURATION_SECONDS);
    
    /* 运行各种压力测试 */
    test_single_connection_stress();
    test_multi_connection_stress();
    test_memory_stress();
    test_error_handling();
    
    printf("\n=== 压力测试完成 ===\n");
    printf("所有测试已完成，请查看上述结果。\n");
    
    return 0;
}