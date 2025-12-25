/* UVHTTP 测试辅助函数和模拟对象支持实现 */

#include "uvhttp_test_helpers.h"
#include "uvhttp_features.h"
#include "uvhttp_allocator.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>

/* ============ 全局变量 ============ */

/* 内存跟踪器 */
uvhttp_memory_tracker_t* g_uvhttp_memory_tracker = NULL;

/* ============ 内存跟踪实现 ============ */

void uvhttp_test_memory_tracker_init(void) {
    if (g_uvhttp_memory_tracker) {
        return; /* 已经初始化 */
    }
    
    g_uvhttp_memory_tracker = (uvhttp_memory_tracker_t*)malloc(sizeof(uvhttp_memory_tracker_t));
    if (!g_uvhttp_memory_tracker) {
        fprintf(stderr, "Failed to allocate memory tracker\n");
        abort();
    }
    
    memset(g_uvhttp_memory_tracker, 0, sizeof(uvhttp_memory_tracker_t));
    g_uvhttp_memory_tracker->leak_detection_enabled = 1;
}

void uvhttp_test_memory_tracker_cleanup(void) {
    if (!g_uvhttp_memory_tracker) {
        return;
    }
    
    if (g_uvhttp_memory_tracker->leak_detection_enabled && 
        g_uvhttp_memory_tracker->records) {
        uvhttp_test_memory_tracker_print_leaks();
    }
    
    /* 清理所有记录 */
    uvhttp_memory_record_t* record = g_uvhttp_memory_tracker->records;
    while (record) {
        uvhttp_memory_record_t* next = record->next;
        free(record);
        record = next;
    }
    
    free(g_uvhttp_memory_tracker);
    g_uvhttp_memory_tracker = NULL;
}

void uvhttp_test_memory_tracker_enable_leak_detection(int enable) {
    if (g_uvhttp_memory_tracker) {
        g_uvhttp_memory_tracker->leak_detection_enabled = enable;
    }
}

void uvhttp_test_memory_tracker_add_record(void* ptr, size_t size, const char* file, int line) {
    if (!g_uvhttp_memory_tracker || !ptr) {
        return;
    }
    
    uvhttp_memory_record_t* record = (uvhttp_memory_record_t*)malloc(sizeof(uvhttp_memory_record_t));
    if (!record) {
        fprintf(stderr, "Failed to allocate memory record\n");
        abort();
    }
    
    record->ptr = ptr;
    record->size = size;
    record->file = file;
    record->line = line;
    record->next = g_uvhttp_memory_tracker->records;
    
    g_uvhttp_memory_tracker->records = record;
    g_uvhttp_memory_tracker->total_allocated += size;
    g_uvhttp_memory_tracker->allocation_count++;
    
    if (g_uvhttp_memory_tracker->total_allocated > g_uvhttp_memory_tracker->peak_allocated) {
        g_uvhttp_memory_tracker->peak_allocated = g_uvhttp_memory_tracker->total_allocated;
    }
}

void uvhttp_test_memory_tracker_remove_record(void* ptr) {
    if (!g_uvhttp_memory_tracker || !ptr) {
        return;
    }
    
    uvhttp_memory_record_t** current = &g_uvhttp_memory_tracker->records;
    while (*current) {
        if ((*current)->ptr == ptr) {
            uvhttp_memory_record_t* to_remove = *current;
            *current = to_remove->next;
            
            g_uvhttp_memory_tracker->total_allocated -= to_remove->size;
            g_uvhttp_memory_tracker->allocation_count--;
            
            free(to_remove);
            return;
        }
        current = &(*current)->next;
    }
}

size_t uvhttp_test_memory_tracker_get_allocated_bytes(void) {
    return g_uvhttp_memory_tracker ? g_uvhttp_memory_tracker->total_allocated : 0;
}

size_t uvhttp_test_memory_tracker_get_allocation_count(void) {
    return g_uvhttp_memory_tracker ? g_uvhttp_memory_tracker->allocation_count : 0;
}

int uvhttp_test_memory_tracker_has_leaks(void) {
    return g_uvhttp_memory_tracker && g_uvhttp_memory_tracker->records != NULL;
}

void uvhttp_test_memory_tracker_print_leaks(void) {
    if (!g_uvhttp_memory_tracker || !g_uvhttp_memory_tracker->records) {
        printf("No memory leaks detected\n");
        return;
    }
    
    printf("Memory leaks detected:\n");
    uvhttp_memory_record_t* record = g_uvhttp_memory_tracker->records;
    size_t total_leaked = 0;
    int leak_count = 0;
    
    while (record) {
        printf("  Leak %d: %zu bytes at %p allocated at %s:%d\n", 
               ++leak_count, record->size, record->ptr, record->file, record->line);
        total_leaked += record->size;
        record = record->next;
    }
    
    printf("Total leaked: %zu bytes in %d allocations\n", total_leaked, leak_count);
}

/* 内存跟踪版本的分配函数 */
void* uvhttp_test_malloc(size_t size, const char* file, int line) {
    void* ptr = malloc(size);
    if (ptr) {
        uvhttp_test_memory_tracker_add_record(ptr, size, file, line);
    }
    return ptr;
}

void uvhttp_test_free(void* ptr, const char* file, int line) {
    (void)file; (void)line; /* 避免未使用参数警告 */
    if (ptr) {
        uvhttp_test_memory_tracker_remove_record(ptr);
        free(ptr);
    }
}

void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line) {
    if (ptr) {
        uvhttp_test_memory_tracker_remove_record(ptr);
    }
    
    void* new_ptr = realloc(ptr, size);
    if (new_ptr) {
        uvhttp_test_memory_tracker_add_record(new_ptr, size, file, line);
    }
    
    return new_ptr;
}

/* ============ 模拟对象实现 ============ */

/* 模拟客户端 */
uvhttp_mock_client_t* uvhttp_mock_client_create(uv_loop_t* loop) {
    uvhttp_mock_client_t* client = (uvhttp_mock_client_t*)malloc(sizeof(uvhttp_mock_client_t));
    if (!client) {
        return NULL;
    }
    
    memset(client, 0, sizeof(uvhttp_mock_client_t));
    
    /* 初始化TCP句柄 */
    if (uv_tcp_init(loop, &client->tcp_handle) != 0) {
        free(client);
        return NULL;
    }
    
    client->tcp_handle.data = client;
    client->send_result = 0;
    client->capacity = 1024;
    client->received_data = (char*)malloc(client->capacity);
    if (!client->received_data) {
        free(client);
        return NULL;
    }
    
    return client;
}

void uvhttp_mock_client_destroy(uvhttp_mock_client_t* client) {
    if (!client) {
        return;
    }
    
    if (client->received_data) {
        free(client->received_data);
    }
    
    uv_close((uv_handle_t*)&client->tcp_handle, NULL);
    free(client);
}

void uvhttp_mock_client_set_send_result(uvhttp_mock_client_t* client, int result) {
    if (client) {
        client->send_result = result;
    }
}

void uvhttp_mock_client_clear_received(uvhttp_mock_client_t* client) {
    if (client) {
        client->received_length = 0;
    }
}

const char* uvhttp_mock_client_get_received_data(uvhttp_mock_client_t* client, size_t* length) {
    if (!client) {
        if (length) *length = 0;
        return NULL;
    }
    
    if (length) {
        *length = client->received_length;
    }
    
    return client->received_data;
}

int uvhttp_mock_client_was_closed(uvhttp_mock_client_t* client) {
    return client ? client->close_called : 0;
}

/* 模拟请求 */
uvhttp_mock_request_t* uvhttp_mock_request_create(uvhttp_mock_client_t* client) {
    uvhttp_mock_request_t* request = (uvhttp_mock_request_t*)malloc(sizeof(uvhttp_mock_request_t));
    if (!request) {
        return NULL;
    }
    
    memset(request, 0, sizeof(uvhttp_mock_request_t));
    
    /* 初始化基础请求对象 */
    if (uvhttp_request_init(&request->base, &client->tcp_handle) != 0) {
        free(request);
        return NULL;
    }
    
    return request;
}

void uvhttp_mock_request_destroy(uvhttp_mock_request_t* request) {
    if (!request) {
        return;
    }
    
    /* 清理字符串字段 */
    if (request->method) free(request->method);
    if (request->url) free(request->url);
    if (request->body) free(request->body);
    
    for (size_t i = 0; i < request->header_count; i++) {
        if (request->headers[i]) free(request->headers[i]);
        if (request->header_values[i]) free(request->header_values[i]);
    }
    
    uvhttp_request_cleanup(&request->base);
    free(request);
}

void uvhttp_mock_request_set_method(uvhttp_mock_request_t* request, const char* method) {
    if (request && method) {
        if (request->method) free(request->method);
        request->method = strdup(method);
    }
}

void uvhttp_mock_request_set_url(uvhttp_mock_request_t* request, const char* url) {
    if (request && url) {
        if (request->url) free(request->url);
        request->url = strdup(url);
    }
}

void uvhttp_mock_request_add_header(uvhttp_mock_request_t* request, const char* name, const char* value) {
    if (!request || !name || !value || request->header_count >= MAX_HEADERS) {
        return;
    }
    
    size_t index = request->header_count++;
    request->headers[index] = strdup(name);
    request->header_values[index] = strdup(value);
}

void uvhttp_mock_request_set_body(uvhttp_mock_request_t* request, const char* body, size_t length) {
    if (!request) {
        return;
    }
    
    if (request->body) {
        free(request->body);
    }
    
    if (body && length > 0) {
        request->body = (char*)malloc(length);
        if (request->body) {
            memcpy(request->body, body, length);
            request->body_length = length;
        }
    } else {
        request->body = NULL;
        request->body_length = 0;
    }
}

void uvhttp_mock_request_set_parse_error(uvhttp_mock_request_t* request, int error) {
    if (request) {
        request->parse_error = error;
    }
}

/* 模拟响应 */
uvhttp_mock_response_t* uvhttp_mock_response_create(uvhttp_mock_client_t* client) {
    uvhttp_mock_response_t* response = (uvhttp_mock_response_t*)malloc(sizeof(uvhttp_mock_response_t));
    if (!response) {
        return NULL;
    }
    
    memset(response, 0, sizeof(uvhttp_mock_response_t));
    
    /* 初始化基础响应对象 */
    if (uvhttp_response_init(&response->base, &client->tcp_handle) != 0) {
        free(response);
        return NULL;
    }
    
    response->send_result = 0;
    
    return response;
}

void uvhttp_mock_response_destroy(uvhttp_mock_response_t* response) {
    if (!response) {
        return;
    }
    
    if (response->sent_data) {
        free(response->sent_data);
    }
    
    uvhttp_response_cleanup(&response->base);
    free(response);
}

int uvhttp_mock_response_was_sent(uvhttp_mock_response_t* response) {
    return response ? response->send_called : 0;
}

const char* uvhttp_mock_response_get_sent_data(uvhttp_mock_response_t* response, size_t* length) {
    if (!response) {
        if (length) *length = 0;
        return NULL;
    }
    
    if (length) {
        *length = response->sent_length;
    }
    
    return response->sent_data;
}

void uvhttp_mock_response_set_send_result(uvhttp_mock_response_t* response, int result) {
    if (response) {
        response->send_result = result;
    }
}

/* 模拟连接 */
uvhttp_mock_connection_t* uvhttp_mock_connection_create(uv_loop_t* loop) {
    uvhttp_mock_connection_t* connection = (uvhttp_mock_connection_t*)malloc(sizeof(uvhttp_mock_connection_t));
    if (!connection) {
        return NULL;
    }
    
    memset(connection, 0, sizeof(uvhttp_mock_connection_t));
    
    /* 创建模拟客户端 */
    connection->mock_client = uvhttp_mock_client_create(loop);
    if (!connection->mock_client) {
        free(connection);
        return NULL;
    }
    
    /* 初始化基础连接对象 */
    if (uvhttp_connection_new(NULL) == NULL) { /* 传入NULL，我们手动设置 */
        uvhttp_mock_client_destroy(connection->mock_client);
        free(connection);
        return NULL;
    }
    
    return connection;
}

void uvhttp_mock_connection_destroy(uvhttp_mock_connection_t* connection) {
    if (!connection) {
        return;
    }
    
    if (connection->mock_client) {
        uvhttp_mock_client_destroy(connection->mock_client);
    }
    
    uvhttp_connection_free(&connection->base);
    free(connection);
}

int uvhttp_mock_connection_is_closed(uvhttp_mock_connection_t* connection) {
    return connection ? connection->close_called : 0;
}

/* ============ 测试场景模拟 ============ */

void uvhttp_test_simulate_network_error(int error_code) {
    uvhttp_network_simulate_error(error_code);
}

void uvhttp_test_simulate_connection_timeout(void) {
    uvhttp_network_simulate_error(UV_ETIMEDOUT);
}

void uvhttp_test_simulate_connection_reset(void) {
    uvhttp_network_simulate_error(UV_ECONNRESET);
}

void uvhttp_test_simulate_memory_exhaustion(void) {
    /* 这里可以通过设置分配器失败标志来模拟 */
    /* 具体实现取决于分配器提供者的设计 */
}

/* ============ 性能测试辅助 ============ */

uint64_t uvhttp_test_get_timestamp(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + (uint64_t)ts.tv_nsec / 1000000; /* 毫秒 */
}

void uvhttp_test_sleep_ms(uint64_t ms) {
    usleep(ms * 1000);
}

/* ============ 负载测试统计 ============ */

void uvhttp_load_test_stats_init(uvhttp_load_test_stats_t* stats) {
    if (stats) {
        memset(stats, 0, sizeof(uvhttp_load_test_stats_t));
    }
}

void uvhttp_load_test_stats_start(uvhttp_load_test_stats_t* stats) {
    if (stats) {
        stats->start_time = uvhttp_test_get_timestamp();
    }
}

void uvhttp_load_test_stats_end(uvhttp_load_test_stats_t* stats) {
    if (stats) {
        stats->end_time = uvhttp_test_get_timestamp();
    }
}

void uvhttp_load_test_stats_record_request(uvhttp_load_test_stats_t* stats, int success, 
                                          size_t bytes_sent, size_t bytes_received) {
    if (!stats) {
        return;
    }
    
    stats->request_count++;
    if (success) {
        stats->successful_requests++;
    } else {
        stats->failed_requests++;
    }
    
    stats->total_bytes_sent += bytes_sent;
    stats->total_bytes_received += bytes_received;
}

void uvhttp_load_test_stats_print(const uvhttp_load_test_stats_t* stats) {
    if (!stats) {
        return;
    }
    
    uint64_t duration = stats->end_time - stats->start_time;
    double rps = duration > 0 ? (double)stats->request_count * 1000.0 / duration : 0.0;
    
    printf("Load Test Statistics:\n");
    printf("  Duration: %lu ms\n", (unsigned long)duration);
    printf("  Total requests: %d\n", stats->request_count);
    printf("  Successful: %d\n", stats->successful_requests);
    printf("  Failed: %d\n", stats->failed_requests);
    printf("  Requests per second: %.2f\n", rps);
    printf("  Bytes sent: %zu\n", stats->total_bytes_sent);
    printf("  Bytes received: %zu\n", stats->total_bytes_received);
}

/* ============ 测试环境设置和清理 ============ */

int uvhttp_test_env_init(void) {
    /* 初始化内存跟踪 */
    uvhttp_test_memory_tracker_init();
    
    /* 初始化测试上下文 */
    uv_loop_t* loop = uv_default_loop();
    if (uvhttp_test_context_setup(loop) != 0) {
        uvhttp_test_memory_tracker_cleanup();
        return -1;
    }
    
    return 0;
}

void uvhttp_test_env_cleanup(void) {
    /* 清理测试上下文 */
    uvhttp_test_context_teardown();
    
    /* 清理内存跟踪 */
    uvhttp_test_memory_tracker_cleanup();
}

int uvhttp_test_setup(uv_loop_t** loop) {
    if (!loop) {
        return -1;
    }
    
    *loop = uv_default_loop();
    if (!*loop) {
        return -1;
    }
    
    return uvhttp_test_context_setup(*loop);
}

void uvhttp_test_teardown(uv_loop_t* loop) {
    (void)loop; /* 避免未使用参数警告 */
    uvhttp_test_context_teardown();
}

int uvhttp_test_context_setup(uv_loop_t* loop) {
    return uvhttp_test_context_init(loop);
}

void uvhttp_test_context_teardown(void) {
    uvhttp_test_context_cleanup();
}

int uvhttp_test_network_setup(uv_loop_t* loop, uvhttp_network_type_t type) {
    return uvhttp_test_network_init(loop, type);
}

void uvhttp_test_network_teardown(void) {
    uvhttp_test_network_cleanup();
}

/* ============ 测试数据生成 ============ */

char* uvhttp_test_generate_http_request(const char* method, const char* path, 
                                        const char* headers[], const char* body, 
                                        size_t* out_length) {
    if (!method || !path || !out_length) {
        return NULL;
    }
    
    /* 计算所需空间 */
    size_t total_length = strlen(method) + 1 + strlen(path) + 11 + 2; /* " HTTP/1.1\r\n" */
    
    /* 添加headers */
    if (headers) {
        for (int i = 0; headers[i] != NULL; i += 2) {
            total_length += strlen(headers[i]) + 2 + strlen(headers[i+1]) + 2; /* "Name: Value\r\n" */
        }
    }
    
    /* 添加空行 */
    total_length += 2;
    
    /* 添加body */
    size_t body_length = body ? strlen(body) : 0;
    total_length += body_length;
    
    /* 分配内存 */
    char* request = (char*)malloc(total_length + 1);
    if (!request) {
        return NULL;
    }
    
    /* 构建请求 */
    size_t pos = 0;
    pos += sprintf(request + pos, "%s %s HTTP/1.1\r\n", method, path);
    
    /* 添加headers */
    if (headers) {
        for (int i = 0; headers[i] != NULL; i += 2) {
            pos += sprintf(request + pos, "%s: %s\r\n", headers[i], headers[i+1]);
        }
    }
    
    /* 添加空行 */
    pos += sprintf(request + pos, "\r\n");
    
    /* 添加body */
    if (body) {
        strcpy(request + pos, body);
        pos += body_length;
    }
    
    *out_length = pos;
    return request;
}

char* uvhttp_test_generate_http_response(int status_code, const char* headers[], 
                                         const char* body, size_t* out_length) {
    if (!out_length) {
        return NULL;
    }
    
    /* 状态文本 */
    const char* status_text = "Unknown";
    switch (status_code) {
        case 200: status_text = "OK"; break;
        case 404: status_text = "Not Found"; break;
        case 500: status_text = "Internal Server Error"; break;
    }
    
    /* 计算所需空间 */
    size_t total_length = 9 + 4 + strlen(status_text) + 2; /* "HTTP/1.1 XXX Text\r\n" */
    
    /* 添加headers */
    if (headers) {
        for (int i = 0; headers[i] != NULL; i += 2) {
            total_length += strlen(headers[i]) + 2 + strlen(headers[i+1]) + 2; /* "Name: Value\r\n" */
        }
    }
    
    /* 添加空行 */
    total_length += 2;
    
    /* 添加body */
    size_t body_length = body ? strlen(body) : 0;
    total_length += body_length;
    
    /* 分配内存 */
    char* response = (char*)malloc(total_length + 1);
    if (!response) {
        return NULL;
    }
    
    /* 构建响应 */
    size_t pos = 0;
    pos += sprintf(response + pos, "HTTP/1.1 %d %s\r\n", status_code, status_text);
    
    /* 添加headers */
    if (headers) {
        for (int i = 0; headers[i] != NULL; i += 2) {
            pos += sprintf(response + pos, "%s: %s\r\n", headers[i], headers[i+1]);
        }
    }
    
    /* 添加空行 */
    pos += sprintf(response + pos, "\r\n");
    
    /* 添加body */
    if (body) {
        strcpy(response + pos, body);
        pos += body_length;
    }
    
    *out_length = pos;
    return response;
}

char* uvhttp_test_generate_random_data(size_t length) {
    char* data = (char*)malloc(length + 1);
    if (!data) {
        return NULL;
    }
    
    for (size_t i = 0; i < length; i++) {
        data[i] = 'A' + (rand() % 26);
    }
    data[length] = '\0';
    
    return data;
}

char* uvhttp_test_generate_text_data(size_t length) {
    char* data = (char*)malloc(length + 1);
    if (!data) {
        return NULL;
    }
    
    const char* pattern = "The quick brown fox jumps over the lazy dog. ";
    size_t pattern_len = strlen(pattern);
    
    for (size_t i = 0; i < length; i++) {
        data[i] = pattern[i % pattern_len];
    }
    data[length] = '\0';
    
    return data;
}

/* ============ 基准测试支持 ============ */

void uvhttp_benchmark_start(uvhttp_benchmark_result_t* result, const char* name, uint64_t iterations) {
    if (!result || !name) {
        return;
    }
    
    memset(result, 0, sizeof(uvhttp_benchmark_result_t));
    result->name = name;
    result->iterations = iterations;
    result->min_time_ns = UINT64_MAX;
}

void uvhttp_benchmark_end(uvhttp_benchmark_result_t* result) {
    if (!result) {
        return;
    }
    
    /* 计算平均时间 */
    if (result->iterations > 0) {
        result->total_time_ns /= result->iterations;
    }
}

void uvhttp_benchmark_print(const uvhttp_benchmark_result_t* result) {
    if (!result) {
        return;
    }
    
    printf("Benchmark: %s\n", result->name);
    printf("  Iterations: %lu\n", (unsigned long)result->iterations);
    printf("  Average time: %lu ns\n", (unsigned long)result->total_time_ns);
    printf("  Min time: %lu ns\n", (unsigned long)result->min_time_ns);
    printf("  Max time: %lu ns\n", (unsigned long)result->max_time_ns);
}