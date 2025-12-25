/* UVHTTP 测试辅助函数和模拟对象支持 */

#ifndef UVHTTP_TEST_HELPERS_H
#define UVHTTP_TEST_HELPERS_H

#include "uvhttp.h"
#include "uvhttp_response.h"
#include "uvhttp_request.h"
#include "uvhttp_connection.h"
#include "uvhttp_network.h"
#include "uvhttp_context.h"
#include <uv.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ============ 测试模式检查 ============ */
#ifndef UVHTTP_TEST_MODE
#error "This file should only be included when UVHTTP_TEST_MODE is defined"
#endif

/* ============ 内存跟踪支持 ============ */

/* 内存分配记录结构 */
typedef struct uvhttp_memory_record {
    void* ptr;
    size_t size;
    const char* file;
    int line;
    struct uvhttp_memory_record* next;
} uvhttp_memory_record_t;

/* 内存跟踪器状态 */
typedef struct {
    uvhttp_memory_record_t* records;
    size_t total_allocated;
    size_t peak_allocated;
    size_t allocation_count;
    int leak_detection_enabled;
} uvhttp_memory_tracker_t;

/* 全局内存跟踪器 */
extern uvhttp_memory_tracker_t* g_uvhttp_memory_tracker;

/* 内存跟踪函数 */
void uvhttp_test_memory_tracker_init(void);
void uvhttp_test_memory_tracker_cleanup(void);
void uvhttp_test_memory_tracker_enable_leak_detection(int enable);
void uvhttp_test_memory_tracker_add_record(void* ptr, size_t size, const char* file, int line);
void uvhttp_test_memory_tracker_remove_record(void* ptr);
size_t uvhttp_test_memory_tracker_get_allocated_bytes(void);
size_t uvhttp_test_memory_tracker_get_allocation_count(void);
int uvhttp_test_memory_tracker_has_leaks(void);
void uvhttp_test_memory_tracker_print_leaks(void);

/* 内存跟踪版本的分配函数 */
void* uvhttp_test_malloc(size_t size, const char* file, int line);
void uvhttp_test_free(void* ptr, const char* file, int line);
void* uvhttp_test_realloc(void* ptr, size_t size, const char* file, int line);

/* ============ 模拟对象支持 ============ */

/* 模拟客户端连接 */
typedef struct {
    uv_tcp_t tcp_handle;
    char* received_data;
    size_t received_length;
    size_t capacity;
    int send_result;
    int close_called;
    void* user_data;
} uvhttp_mock_client_t;

/* 模拟请求对象 */
typedef struct {
    uvhttp_request_t base;
    char* method;
    char* url;
    char* headers[MAX_HEADERS];
    char* header_values[MAX_HEADERS];
    size_t header_count;
    char* body;
    size_t body_length;
    int parse_error;
} uvhttp_mock_request_t;

/* 模拟响应对象 */
typedef struct {
    uvhttp_response_t base;
    char* sent_data;
    size_t sent_length;
    int send_called;
    int send_result;
} uvhttp_mock_response_t;

/* 模拟连接对象 */
typedef struct {
    uvhttp_connection_t base;
    int is_closed;
    int close_called;
    uvhttp_mock_client_t* mock_client;
} uvhttp_mock_connection_t;

/* ============ 模拟对象创建和销毁 ============ */

/* 模拟客户端 */
uvhttp_mock_client_t* uvhttp_mock_client_create(uv_loop_t* loop);
void uvhttp_mock_client_destroy(uvhttp_mock_client_t* client);
void uvhttp_mock_client_set_send_result(uvhttp_mock_client_t* client, int result);
void uvhttp_mock_client_clear_received(uvhttp_mock_client_t* client);
const char* uvhttp_mock_client_get_received_data(uvhttp_mock_client_t* client, size_t* length);
int uvhttp_mock_client_was_closed(uvhttp_mock_client_t* client);

/* 模拟请求 */
uvhttp_mock_request_t* uvhttp_mock_request_create(uvhttp_mock_client_t* client);
void uvhttp_mock_request_destroy(uvhttp_mock_request_t* request);
void uvhttp_mock_request_set_method(uvhttp_mock_request_t* request, const char* method);
void uvhttp_mock_request_set_url(uvhttp_mock_request_t* request, const char* url);
void uvhttp_mock_request_add_header(uvhttp_mock_request_t* request, const char* name, const char* value);
void uvhttp_mock_request_set_body(uvhttp_mock_request_t* request, const char* body, size_t length);
void uvhttp_mock_request_set_parse_error(uvhttp_mock_request_t* request, int error);

/* 模拟响应 */
uvhttp_mock_response_t* uvhttp_mock_response_create(uvhttp_mock_client_t* client);
void uvhttp_mock_response_destroy(uvhttp_mock_response_t* response);
int uvhttp_mock_response_was_sent(uvhttp_mock_response_t* response);
const char* uvhttp_mock_response_get_sent_data(uvhttp_mock_response_t* response, size_t* length);
void uvhttp_mock_response_set_send_result(uvhttp_mock_response_t* response, int result);

/* 模拟连接 */
uvhttp_mock_connection_t* uvhttp_mock_connection_create(uv_loop_t* loop);
void uvhttp_mock_connection_destroy(uvhttp_mock_connection_t* connection);
int uvhttp_mock_connection_is_closed(uvhttp_mock_connection_t* connection);

/* ============ 测试场景模拟 ============ */

/* 网络错误模拟 */
void uvhttp_test_simulate_network_error(int error_code);
void uvhttp_test_simulate_connection_timeout(void);
void uvhttp_test_simulate_connection_reset(void);
void uvhttp_test_simulate_memory_exhaustion(void);

/* 性能测试辅助 */
uint64_t uvhttp_test_get_timestamp(void);
void uvhttp_test_sleep_ms(uint64_t ms);

/* 负载测试辅助 */
typedef struct {
    int request_count;
    int concurrent_requests;
    uint64_t start_time;
    uint64_t end_time;
    int successful_requests;
    int failed_requests;
    size_t total_bytes_received;
    size_t total_bytes_sent;
} uvhttp_load_test_stats_t;

void uvhttp_load_test_stats_init(uvhttp_load_test_stats_t* stats);
void uvhttp_load_test_stats_start(uvhttp_load_test_stats_t* stats);
void uvhttp_load_test_stats_end(uvhttp_load_test_stats_t* stats);
void uvhttp_load_test_stats_record_request(uvhttp_load_test_stats_t* stats, int success, 
                                          size_t bytes_sent, size_t bytes_received);
void uvhttp_load_test_stats_print(const uvhttp_load_test_stats_t* stats);

/* ============ 断言和验证宏 ============ */

/* 增强的断言宏 */
#define UVHTTP_TEST_ASSERT_EQ(expected, actual) \
    UVHTTP_TEST_ASSERT((expected) == (actual), \
        "Expected %ld, got %ld", (long)(expected), (long)(actual))

#define UVHTTP_TEST_ASSERT_NE(expected, actual) \
    UVHTTP_TEST_ASSERT((expected) != (actual), \
        "Expected not equal to %ld, but got %ld", (long)(expected), (long)(actual))

#define UVHTTP_TEST_ASSERT_STR_EQ(expected, actual) \
    UVHTTP_TEST_ASSERT(strcmp((expected), (actual)) == 0, \
        "Expected \"%s\", got \"%s\"", (expected), (actual))

#define UVHTTP_TEST_ASSERT_NULL(ptr) \
    UVHTTP_TEST_ASSERT((ptr) == NULL, "Expected NULL, got %p", (ptr))

#define UVHTTP_TEST_ASSERT_NOT_NULL(ptr) \
    UVHTTP_TEST_ASSERT((ptr) != NULL, "Expected non-NULL, got NULL")

#define UVHTTP_TEST_ASSERT_SUCCESS(err) \
    UVHTTP_TEST_ASSERT((err) == UVHTTP_OK, "Expected success, got error %d", (err))

/* HTTP特定断言 */
#define UVHTTP_TEST_ASSERT_STATUS_CODE(response, expected_code) \
    UVHTTP_TEST_ASSERT_EQ(expected_code, (response)->status_code)

#define UVHTTP_TEST_ASSERT_HEADER(response, name, expected_value) \
    do { \
        const char* value = uvhttp_response_get_header((response), (name)); \
        UVHTTP_TEST_ASSERT_NOT_NULL(value); \
        UVHTTP_TEST_ASSERT_STR_EQ(expected_value, value); \
    } while(0)

#define UVHTTP_TEST_ASSERT_BODY(response, expected_body) \
    do { \
        const char* body = uvhttp_response_get_body((response)); \
        UVHTTP_TEST_ASSERT_NOT_NULL(body); \
        UVHTTP_TEST_ASSERT_STR_EQ(expected_body, body); \
    } while(0)

/* ============ 测试环境设置和清理 ============ */

/* 测试环境初始化 */
int uvhttp_test_env_init(void);
void uvhttp_test_env_cleanup(void);

/* 测试用例设置和清理 */
int uvhttp_test_setup(uv_loop_t** loop);
void uvhttp_test_teardown(uv_loop_t* loop);

/* 测试上下文管理 */
int uvhttp_test_context_setup(uv_loop_t* loop);
void uvhttp_test_context_teardown(void);

/* 网络接口测试设置 */
int uvhttp_test_network_setup(uv_loop_t* loop, uvhttp_network_type_t type);
void uvhttp_test_network_teardown(void);

/* ============ 测试数据生成 ============ */

/* 生成测试用的HTTP请求数据 */
char* uvhttp_test_generate_http_request(const char* method, const char* path, 
                                        const char* headers[], const char* body, 
                                        size_t* out_length);

/* 生成测试用的HTTP响应数据 */
char* uvhttp_test_generate_http_response(int status_code, const char* headers[], 
                                         const char* body, size_t* out_length);

/* 生成随机数据用于测试 */
char* uvhttp_test_generate_random_data(size_t length);
char* uvhttp_test_generate_text_data(size_t length);

/* ============ 基准测试支持 ============ */

typedef struct {
    const char* name;
    uint64_t iterations;
    uint64_t total_time_ns;
    uint64_t min_time_ns;
    uint64_t max_time_ns;
} uvhttp_benchmark_result_t;

void uvhttp_benchmark_start(uvhttp_benchmark_result_t* result, const char* name, uint64_t iterations);
void uvhttp_benchmark_end(uvhttp_benchmark_result_t* result);
void uvhttp_benchmark_print(const uvhttp_benchmark_result_t* result);

/* ============ 集成测试辅助 ============ */

/* 创建完整的测试服务器 */
typedef struct {
    uvhttp_server_t* server;
    uvhttp_router_t* router;
    uv_loop_t* loop;
    int port;
    int running;
} uvhttp_test_server_t;

uvhttp_test_server_t* uvhttp_test_server_create(int port);
void uvhttp_test_server_destroy(uvhttp_test_server_t* server);
int uvhttp_test_server_start(uvhttp_test_server_t* server);
void uvhttp_test_server_stop(uvhttp_test_server_t* server);
void uvhttp_test_server_add_route(uvhttp_test_server_t* server, const char* path, 
                                  uvhttp_route_handler_t handler);

/* 测试HTTP客户端 */
typedef struct {
    uv_tcp_t tcp_handle;
    uv_connect_t connect_req;
    uv_write_t write_req;
    char* response_data;
    size_t response_length;
    int response_received;
    int error_code;
} uvhttp_test_client_t;

uvhttp_test_client_t* uvhttp_test_client_create(uv_loop_t* loop);
void uvhttp_test_client_destroy(uvhttp_test_client_t* client);
int uvhttp_test_client_connect(uvhttp_test_client_t* client, const char* host, int port);
int uvhttp_test_client_send_request(uvhttp_test_client_t* client, const char* request, size_t length);
int uvhttp_test_client_wait_response(uvhttp_test_client_t* client, uint64_t timeout_ms);
const char* uvhttp_test_client_get_response(uvhttp_test_client_t* client, size_t* length);

#ifdef __cplusplus
}
#endif

#endif /* UVHTTP_TEST_HELPERS_H */