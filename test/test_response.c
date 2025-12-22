/**
 * @file test_response.c
 * @brief HTTP响应模块测试
 */

#include "uvhttp_test_framework.h"
#include "../include/uvhttp_response.h"
#include "../include/uvhttp_constants.h"

TEST_FUNC(response_init_normal) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    TEST_ASSERT_NOT_NULL(mock_client);
    
    int result = uvhttp_response_init(&response, mock_client);
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_EQ(200, response.status_code);
    TEST_ASSERT_EQ(1, response.keep_alive); /* HTTP/1.1 default */
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_init_null_params) {
    uvhttp_response_t response;
    
    int result = uvhttp_response_init(NULL, (void*)0x1);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_init(&response, NULL);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    return 0;
}

TEST_FUNC(response_set_status_valid) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    int result = uvhttp_response_set_status(&response, 404);
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_EQ(404, response.status_code);
    
    result = uvhttp_response_set_status(&response, 500);
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_EQ(500, response.status_code);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_set_status_invalid) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    int result = uvhttp_response_set_status(NULL, 200);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_set_status(&response, 99); /* Too low */
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_set_status(&response, 600); /* Too high */
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_set_header_valid) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    int result = uvhttp_response_set_header(&response, "Content-Type", "text/plain");
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_EQ(1, response.header_count);
    TEST_ASSERT_STREQ("Content-Type", response.headers[0].name);
    TEST_ASSERT_STREQ("text/plain", response.headers[0].value);
    
    /* 添加第二个header */
    result = uvhttp_response_set_header(&response, "Content-Length", "13");
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_EQ(2, response.header_count);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_set_header_invalid) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    int result = uvhttp_response_set_header(NULL, "name", "value");
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_set_header(&response, NULL, "value");
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_set_header(&response, "name", NULL);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_set_body_valid) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    const char* test_data = "Hello, World!";
    size_t test_len = strlen(test_data);
    
    int result = uvhttp_response_set_body(&response, test_data, test_len);
    TEST_ASSERT_EQ(0, result);
    TEST_ASSERT_EQ(test_len, response.body_length);
    TEST_ASSERT_NOT_NULL(response.body);
    TEST_ASSERT_MEMEQ(test_data, response.body, test_len);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_set_body_invalid) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    int result = uvhttp_response_set_body(NULL, "data", 4);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_set_body(&response, NULL, 4);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    result = uvhttp_response_set_body(&response, "data", 0);
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_FUNC(response_set_body_oversize) {
    uvhttp_response_t response;
    void* mock_client = uvhttp_test_create_mock_client();
    
    uvhttp_response_init(&response, mock_client);
    
    /* 尝试设置过大的body */
    char large_data[UVHTTP_MAX_BODY_SIZE + 1];
    memset(large_data, 'A', sizeof(large_data));
    
    int result = uvhttp_response_set_body(&response, large_data, sizeof(large_data));
    TEST_ASSERT_EQ(UVHTTP_ERROR_INVALID_PARAM, result);
    TEST_ASSERT_EQ(0, response.body_length);
    TEST_ASSERT_NULL(response.body);
    
    uvhttp_response_cleanup(&response);
    uvhttp_test_destroy_mock_client(mock_client);
    
    return 0;
}

TEST_SUITE(response) {
    TEST_CASE(response_init_normal);
    TEST_CASE(response_init_null_params);
    TEST_CASE(response_set_status_valid);
    TEST_CASE(response_set_status_invalid);
    TEST_CASE(response_set_header_valid);
    TEST_CASE(response_set_header_invalid);
    TEST_CASE(response_set_body_valid);
    TEST_CASE(response_set_body_invalid);
    TEST_CASE(response_set_body_oversize);
    
    END_TEST_SUITE();
}