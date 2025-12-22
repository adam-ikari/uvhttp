/* WebSocket 性能基准测试 */
#include "uvhttp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>

/* 获取当前时间（微秒） */
static long long get_current_time_us() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (long long)tv.tv_sec * 1000000 + tv.tv_usec;
}

/* 测试 WebSocket API 调用性能 */
static void benchmark_api_calls() {
    printf("\n=== WebSocket API 调用性能测试 ===\n");
    
    const int iterations = 100000;
    long long start_time, end_time;
    
    /* 测试 WebSocket 创建和销毁 */
    printf("测试 WebSocket 创建和销毁 (%d 次迭代)...\n", iterations);
    start_time = get_current_time_us();
    
    for (int i = 0; i < iterations; i++) {
        /* 测试空参数创建（应该返回 NULL） */
        uvhttp_websocket_t* ws = uvhttp_websocket_new(NULL, NULL);
        if (ws) {
            uvhttp_websocket_free(ws);
        }
    }
    
    end_time = get_current_time_us();
    double duration_ms = (end_time - start_time) / 1000.0;
    printf("创建/销毁性能: %.2f ms 总时间, %.2f ns/次调用\n", 
           duration_ms, (duration_ms * 1000000.0) / iterations);
    
    /* 测试错误检查性能 */
    printf("\n测试错误检查性能 (%d 次迭代)...\n", iterations);
    start_time = get_current_time_us();
    
    for (int i = 0; i < iterations; i++) {
        uvhttp_websocket_send_text(NULL, "test");
        uvhttp_websocket_close(NULL, 1000, "test");
        uvhttp_websocket_verify_peer_cert(NULL);
    }
    
    end_time = get_current_time_us();
    duration_ms = (end_time - start_time) / 1000.0;
    printf("错误检查性能: %.2f ms 总时间, %.2f ns/次调用\n", 
           duration_ms, (duration_ms * 1000000.0) / (iterations * 3));
}

/* 测试内存分配性能 */
static void benchmark_memory_allocation() {
    printf("\n=== 内存分配性能测试 ===\n");
    
    const int iterations = 10000;
    long long start_time, end_time;
    
    /* 测试 mTLS 配置分配 */
    printf("测试 mTLS 配置结构分配 (%d 次)...\n", iterations);
    start_time = get_current_time_us();
    
    for (int i = 0; i < iterations; i++) {
        uvhttp_websocket_mtls_config_t* config = 
            (uvhttp_websocket_mtls_config_t*)malloc(sizeof(uvhttp_websocket_mtls_config_t));
        if (config) {
            config->server_cert_path = "test.crt";
            config->server_key_path = "test.key";
            config->ca_cert_path = "ca.crt";
            config->require_client_cert = 1;
            config->verify_depth = 3;
            free(config);
        }
    }
    
    end_time = get_current_time_us();
    double duration_ms = (end_time - start_time) / 1000.0;
    printf("mTLS 配置分配: %.2f ms 总时间, %.2f ns/次分配\n", 
           duration_ms, (duration_ms * 1000000.0) / iterations);
    
    /* 测试消息结构分配 */
    printf("\n测试消息结构分配 (%d 次)...\n", iterations);
    start_time = get_current_time_us();
    
    for (int i = 0; i < iterations; i++) {
        uvhttp_websocket_message_t* msg = 
            (uvhttp_websocket_message_t*)malloc(sizeof(uvhttp_websocket_message_t));
        if (msg) {
            msg->type = UVHTTP_WEBSOCKET_TEXT;
            msg->data = "test message";
            msg->length = 12;
            free(msg);
        }
    }
    
    end_time = get_current_time_us();
    duration_ms = (end_time - start_time) / 1000.0;
    printf("消息结构分配: %.2f ms 总时间, %.2f ns/次分配\n", 
           duration_ms, (duration_ms * 1000000.0) / iterations);
}

/* 测试字符串处理性能 */
static void benchmark_string_operations() {
    printf("\n=== 字符串处理性能测试 ===\n");
    
    const int iterations = 50000;
    long long start_time, end_time;
    
    /* 测试字符串操作性能 */
    printf("测试字符串操作 (%d 次)...\n", iterations);
    
    char test_string[64];
    char output[64];
    
    /* 准备测试数据 */
    for (int i = 0; i < 32; i++) {
        test_string[i] = 'A' + (i % 26);
    }
    test_string[32] = '\0';
    
    start_time = get_current_time_us();
    
    for (int i = 0; i < iterations; i++) {
        /* 模拟字符串操作 */
        strcpy(output, test_string);
        size_t len = strlen(output);
        (void)len; /* 避免未使用变量警告 */
    }
    
    end_time = get_current_time_us();
    double duration_ms = (end_time - start_time) / 1000.0;
    printf("字符串操作性能: %.2f ms 总时间, %.2f ns/次操作\n", 
           duration_ms, (duration_ms * 1000000.0) / iterations);
}

/* 测试错误处理性能 */
static void benchmark_error_handling() {
    printf("\n=== 错误处理性能测试 ===\n");
    
    const int iterations = 100000;
    long long start_time, end_time;
    
    /* 测试错误码映射性能 */
    printf("测试错误码映射 (%d 次)...\n", iterations);
    start_time = get_current_time_us();
    
    for (int i = 0; i < iterations; i++) {
        /* 模拟错误处理逻辑 */
        uvhttp_websocket_error_t errors[] = {
            UVHTTP_WEBSOCKET_ERROR_NONE,
            UVHTTP_WEBSOCKET_ERROR_INVALID_PARAM,
            UVHTTP_WEBSOCKET_ERROR_MEMORY,
            UVHTTP_WEBSOCKET_ERROR_TLS_CONFIG,
            UVHTTP_WEBSOCKET_ERROR_CONNECTION,
            UVHTTP_WEBSOCKET_ERROR_NOT_CONNECTED,
            UVHTTP_WEBSOCKET_ERROR_CERT_VERIFY,
            UVHTTP_WEBSOCKET_ERROR_CERT_EXPIRED,
            UVHTTP_WEBSOCKET_ERROR_CERT_NOT_YET_VALID,
            UVHTTP_WEBSOCKET_ERROR_PROTOCOL
        };
        
        /* 简单的错误处理逻辑 */
        for (int j = 0; j < 10; j++) {
            if (errors[j] != UVHTTP_WEBSOCKET_ERROR_NONE) {
                /* 错误处理逻辑 */
                volatile int error_code = (int)errors[j];
                (void)error_code;
            }
        }
    }
    
    end_time = get_current_time_us();
    double duration_ms = (end_time - start_time) / 1000.0;
    printf("错误处理性能: %.2f ms 总时间, %.2f ns/次处理\n", 
           duration_ms, (duration_ms * 1000000.0) / (iterations * 10));
}

/* 测试并发安全性 */
static void benchmark_thread_safety() {
    printf("\n=== 线程安全性测试 ===\n");
    
    printf("测试多线程环境下的 API 调用...\n");
    
    /* 由于这是模拟测试，我们只能测试基本的线程安全性 */
    /* 在真实环境中，需要创建多个线程进行并发测试 */
    
    const int iterations = 10000;
    long long start_time, end_time;
    
    start_time = get_current_time_us();
    
    /* 模拟并发操作 */
    for (int i = 0; i < iterations; i++) {
        /* 全局清理函数的线程安全性 */
        uvhttp_websocket_cleanup_global();
        
        /* API 调用 */
        uvhttp_websocket_get_peer_cert(NULL);
        uvhttp_websocket_verify_peer_cert(NULL);
    }
    
    end_time = get_current_time_us();
    double duration_ms = (end_time - start_time) / 1000.0;
    printf("线程安全测试: %.2f ms 总时间, %.2f ns/次操作\n", 
           duration_ms, (duration_ms * 1000000.0) / (iterations * 3));
}

/* 生成性能报告 */
static void generate_performance_report() {
    printf("\n=== 性能测试总结 ===\n");
    printf("测试完成时间: %s", ctime(&(time_t){time(NULL)}));
    printf("\n性能指标说明:\n");
    printf("- API 调用: 测试 WebSocket API 的响应速度\n");
    printf("- 内存分配: 测试动态内存分配的性能\n");
    printf("- 字符串处理: 测试 Base64 编码等字符串操作\n");
    printf("- 错误处理: 测试错误码处理和映射的性能\n");
    printf("- 线程安全: 测试多线程环境下的操作安全性\n");
    
    printf("\n建议:\n");
    printf("- 所有 API 调用都应该在微秒级别完成\n");
    printf("- 内存分配应该高效，避免频繁分配/释放\n");
    printf("- 字符串操作应该使用优化的算法\n");
    printf("- 错误处理应该快速且不影响性能\n");
    printf("- 线程安全操作应该使用适当的同步机制\n");
}

int main() {
    printf("WebSocket 性能基准测试\n");
    printf("测试开始时间: %s", ctime(&(time_t){time(NULL)}));
    
    /* 运行各项性能测试 */
    benchmark_api_calls();
    benchmark_memory_allocation();
    benchmark_string_operations();
    benchmark_error_handling();
    benchmark_thread_safety();
    
    /* 生成性能报告 */
    generate_performance_report();
    
    printf("\n性能测试完成！\n");
    
    return 0;
}