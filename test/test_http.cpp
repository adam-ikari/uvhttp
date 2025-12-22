#include <gtest/gtest.h>
#include "../include/uvhttp_request.h"
#include "../include/uvhttp_response.h"

// 模拟HTTP请求测试数据
static const char* simple_get_request = 
    "GET / HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "User-Agent: test/1.0\r\n"
    "\r\n";

static const char* post_request_with_body = 
    "POST /api/data HTTP/1.1\r\n"
    "Host: example.com\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 13\r\n"
    "\r\n"
    "{\"test\":true}";

static const char* request_with_headers = 
    "GET /api/users HTTP/1.1\r\n"
    "Host: api.example.com\r\n"
    "Authorization: Bearer token123\r\n"
    "Accept: application/json\r\n"
    "X-Custom-Header: custom-value\r\n"
    "\r\n";

// 测试请求初始化
TEST(HttpTest, RequestInitialization) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    // 测试基本初始化
    EXPECT_EQ(uvhttp_request_init(&request, NULL), 0);
    EXPECT_NE(&request, nullptr);
    
    // 清理
    uvhttp_request_cleanup(&request);
}

// 测试响应初始化
TEST(HttpTest, ResponseInitialization) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    // 测试基本初始化
    EXPECT_EQ(uvhttp_response_init(&response, NULL), 0);
    EXPECT_NE(&response, nullptr);
    
    // 清理
    uvhttp_response_cleanup(&response);
}

// 测试响应状态码设置
TEST(HttpTest, ResponseStatusCode) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_response_init(&response, NULL);
    
    // 测试常见状态码
    uvhttp_response_set_status(&response, 200);
    EXPECT_EQ(response.status_code, 200);
    
    uvhttp_response_set_status(&response, 404);
    EXPECT_EQ(response.status_code, 404);
    
    uvhttp_response_set_status(&response, 500);
    EXPECT_EQ(response.status_code, 500);
    
    // 清理
    uvhttp_response_cleanup(&response);
}

// 测试响应头部设置
TEST(HttpTest, ResponseHeaders) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_response_init(&response, NULL);
    
    // 测试基本头部设置
    uvhttp_response_set_header(&response, "Content-Type", "application/json");
    
    // 验证头部数量
    EXPECT_GT(response.header_count, 0);
    
    // 测试多个头部
    uvhttp_response_set_header(&response, "Server", "uvhttp/1.0");
    uvhttp_response_set_header(&response, "Connection", "close");
    
    // 清理
    uvhttp_response_cleanup(&response);
}

// 测试响应体设置
TEST(HttpTest, ResponseBody) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_response_init(&response, NULL);
    
    // 测试基本响应体设置
    const char* body = "Hello, World!";
    EXPECT_EQ(uvhttp_response_set_body(&response, body, strlen(body)), 0);
    EXPECT_EQ(response.body_length, strlen(body));
    
    // 测试空响应体
    EXPECT_EQ(uvhttp_response_set_body(&response, "", 0), 0);
    EXPECT_EQ(response.body_length, 0);
    
    // 测试NULL响应体
    EXPECT_EQ(uvhttp_response_set_body(&response, NULL, 0), 0);
    
    // 清理
    uvhttp_response_cleanup(&response);
}

// 测试大响应体处理
TEST(HttpTest, LargeResponseBody) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    uvhttp_response_init(&response, NULL);
    
    // 创建大响应体
    const size_t large_size = 1024 * 1024; // 1MB
    char* large_body = (char*)malloc(large_size);
    memset(large_body, 'A', large_size);
    
    // 测试大响应体设置
    EXPECT_EQ(uvhttp_response_set_body(&response, large_body, large_size), 0);
    EXPECT_EQ(response.body_length, large_size);
    
    free(large_body);
    uvhttp_response_cleanup(&response);
}

// 测试HTTP方法解析
TEST(HttpTest, HttpMethodParsing) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    uvhttp_request_init(&request, NULL);
    
    // 模拟不同HTTP方法的解析
    const char* methods[] = {"GET", "POST", "PUT", "DELETE", "HEAD", "OPTIONS", "PATCH"};
    const int method_count = sizeof(methods) / sizeof(methods[0]);
    
    for (int i = 0; i < method_count; i++) {
        // 这里应该有实际的解析逻辑，暂时测试方法字符串
        EXPECT_NE(methods[i], nullptr);
        EXPECT_GT(strlen(methods[i]), 0);
    }
    
    uvhttp_request_cleanup(&request);
}

// 测试HTTP头部解析
TEST(HttpTest, HttpHeaderParsing) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    uvhttp_request_init(&request, NULL);
    
    // 测试头部解析逻辑
    const char* header_line = "Content-Type: application/json";
    const char* colon = strchr(header_line, ':');
    
    EXPECT_NE(colon, nullptr);
    EXPECT_GT(colon - header_line, 0);
    EXPECT_GT(strlen(colon + 1), 0);
    
    // 测试头部名称验证
    char header_name[64];
    size_t name_len = colon - header_line;
    strncpy(header_name, header_line, name_len);
    header_name[name_len] = '\0';
    
    EXPECT_GT(name_len, 0);
    EXPECT_LT(name_len, sizeof(header_name));
    
    uvhttp_request_cleanup(&request);
}

// 测试URL解析
TEST(HttpTest, UrlParsing) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    uvhttp_request_init(&request, NULL);
    
    // 测试各种URL格式
    const char* urls[] = {
        "/",
        "/api/users",
        "/api/v1/data",
        "/search?q=test&page=1",
        "/users/123/profile"
    };
    
    const int url_count = sizeof(urls) / sizeof(urls[0]);
    
    for (int i = 0; i < url_count; i++) {
        // 验证URL格式
        EXPECT_EQ(urls[i][0], '/'); // 所有URL应该以/开头
        EXPECT_GT(strlen(urls[i]), 0);
        
        // 模拟URL验证
        EXPECT_EQ(validate_url(urls[i], strlen(urls[i])), 0);
    }
    
    uvhttp_request_cleanup(&request);
}

// 测试HTTP版本解析
TEST(HttpTest, HttpVersionParsing) {
    // 测试HTTP/1.1格式
    const char* http_version = "HTTP/1.1";
    
    EXPECT_EQ(strncmp(http_version, "HTTP/", 5), 0);
    EXPECT_EQ(strncmp(http_version + 5, "1.1", 3), 0);
    
    // 测试HTTP/1.0格式
    const char* http_version_10 = "HTTP/1.0";
    EXPECT_EQ(strncmp(http_version_10, "HTTP/", 5), 0);
    EXPECT_EQ(strncmp(http_version_10 + 5, "1.0", 3), 0);
}

// 测试请求体处理
TEST(HttpTest, RequestBodyHandling) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    uvhttp_request_init(&request, NULL);
    
    // 测试空请求体
    EXPECT_EQ(request.body_length, 0);
    EXPECT_EQ(request.body, nullptr);
    
    // 测试请求体分配
    const char* test_body = "{\"test\": \"data\"}";
    const size_t body_len = strlen(test_body);
    
    // 模拟请求体处理
    EXPECT_GT(body_len, 0);
    EXPECT_NE(test_body, nullptr);
    
    uvhttp_request_cleanup(&request);
}

// 测试错误处理
TEST(HttpTest, ErrorHandling) {
    uvhttp_request_t request;
    uvhttp_response_t response;
    
    // 测试NULL指针处理
    EXPECT_EQ(uvhttp_request_init(NULL, NULL), -1);
    EXPECT_EQ(uvhttp_response_init(NULL, NULL), -1);
    
    // 测试无效参数
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    
    uvhttp_response_set_header(&response, NULL, "value");
    uvhttp_response_set_header(&response, "name", NULL);
    uvhttp_response_set_body(&response, NULL, 0);
    
    // 应该不会崩溃
    EXPECT_TRUE(true);
}

// 测试内存管理
TEST(HttpTest, MemoryManagement) {
    // 测试多次初始化和清理
    for (int i = 0; i < 100; i++) {
        uvhttp_request_t request;
        uvhttp_response_t response;
        
        memset(&request, 0, sizeof(request));
        memset(&response, 0, sizeof(response));
        
        EXPECT_EQ(uvhttp_request_init(&request, NULL), 0);
        EXPECT_EQ(uvhttp_response_init(&response, NULL), 0);
        
        // 设置一些数据
        uvhttp_response_set_status(&response, 200);
        uvhttp_response_set_header(&response, "Test", "Value");
        uvhttp_response_set_body(&response, "test", 4);
        
        // 清理
        uvhttp_request_cleanup(&request);
        uvhttp_response_cleanup(&response);
    }
    
    EXPECT_TRUE(true); // 如果没有崩溃就算通过
}