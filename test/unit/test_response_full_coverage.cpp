/* UVHTTP 响应处理模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_response.h"
#include "uvhttp_constants.h"

TEST(UvhttpResponseFullCoverageTest, ResponseInit) {
    uvhttp_response_t response;
    
    uvhttp_error_t result = uvhttp_response_init(&response, NULL);
    
    result = uvhttp_response_init(NULL, NULL);
}

TEST(UvhttpResponseFullCoverageTest, ResponseSetStatus) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_error_t result = uvhttp_response_set_status(&response, UVHTTP_STATUS_OK);
    
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_CREATED);
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_NO_CONTENT);
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_BAD_REQUEST);
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_UNAUTHORIZED);
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_FORBIDDEN);
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_NOT_FOUND);
    result = uvhttp_response_set_status(&response, UVHTTP_STATUS_INTERNAL_ERROR);
    
    result = uvhttp_response_set_status(NULL, UVHTTP_STATUS_OK);
}

TEST(UvhttpResponseFullCoverageTest, ResponseSetHeader) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_error_t result = uvhttp_response_set_header(&response, "Content-Type", "text/html");
    
    result = uvhttp_response_set_header(&response, "Content-Length", "123");
    result = uvhttp_response_set_header(&response, "Connection", "keep-alive");
    result = uvhttp_response_set_header(&response, "Cache-Control", "no-cache");
    result = uvhttp_response_set_header(&response, "Server", "uvhttp/1.0");
    
    result = uvhttp_response_set_header(NULL, "Content-Type", "text/html");
    result = uvhttp_response_set_header(&response, NULL, "text/html");
    result = uvhttp_response_set_header(&response, "Content-Type", NULL);
}

TEST(UvhttpResponseFullCoverageTest, ResponseSetBody) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    const char* body = "Hello, World!";
    uvhttp_error_t result = uvhttp_response_set_body(&response, body, strlen(body));
    
    result = uvhttp_response_set_body(&response, "", 0);
    
    result = uvhttp_response_set_body(NULL, body, strlen(body));
    result = uvhttp_response_set_body(&response, NULL, 0);
}

TEST(UvhttpResponseFullCoverageTest, ResponseBuildData) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    response.status_code = UVHTTP_STATUS_OK;
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_body(&response, "Hello, World!", 13);
    
    char* data = NULL;
    size_t length = 0;
    uvhttp_error_t result = uvhttp_response_build_data(&response, &data, &length);
    
    result = uvhttp_response_build_data(NULL, &data, &length);
    result = uvhttp_response_build_data(&response, NULL, &length);
    result = uvhttp_response_build_data(&response, &data, NULL);
}

TEST(UvhttpResponseFullCoverageTest, ResponseCleanup) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    response.status_code = UVHTTP_STATUS_OK;
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_body(&response, "Hello, World!", 13);
    
    uvhttp_response_cleanup(&response);
    
    uvhttp_response_cleanup(NULL);
}

TEST(UvhttpResponseFullCoverageTest, ResponseFree) {
    uvhttp_response_free(NULL);
}

TEST(UvhttpResponseFullCoverageTest, ResponseSend) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    response.status_code = UVHTTP_STATUS_OK;
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_body(&response, "Hello, World!", 13);
    
    uvhttp_error_t result = uvhttp_response_send(&response);
    
    result = uvhttp_response_send(NULL);
}

TEST(UvhttpResponseFullCoverageTest, ResponseFullFlow) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_response_init(&response, NULL);
    
    uvhttp_response_set_status(&response, UVHTTP_STATUS_OK);
    
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_header(&response, "Content-Length", "13");
    uvhttp_response_set_header(&response, "Connection", "keep-alive");
    
    uvhttp_response_set_body(&response, "Hello, World!", 13);
    
    char* data = NULL;
    size_t length = 0;
    uvhttp_response_build_data(&response, &data, &length);
    
    uvhttp_response_cleanup(&response);
}

TEST(UvhttpResponseFullCoverageTest, ResponseDifferentStatus) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_response_set_status(&response, UVHTTP_STATUS_OK);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_CREATED);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_NO_CONTENT);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_BAD_REQUEST);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_UNAUTHORIZED);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_FORBIDDEN);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_NOT_FOUND);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_METHOD_NOT_ALLOWED);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_INTERNAL_ERROR);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_NOT_IMPLEMENTED);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_BAD_GATEWAY);
    uvhttp_response_set_status(&response, UVHTTP_STATUS_SERVICE_UNAVAILABLE);
}

TEST(UvhttpResponseFullCoverageTest, ResponseMultipleHeaders) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_header(&response, "Content-Length", "123");
    uvhttp_response_set_header(&response, "Connection", "keep-alive");
    uvhttp_response_set_header(&response, "Cache-Control", "no-cache");
    uvhttp_response_set_header(&response, "Server", "uvhttp/1.0");
    uvhttp_response_set_header(&response, "Date", "Mon, 01 Jan 2026 00:00:00 GMT");
    uvhttp_response_set_header(&response, "Expires", "Mon, 01 Jan 2026 00:00:00 GMT");
    uvhttp_response_set_header(&response, "Last-Modified", "Mon, 01 Jan 2026 00:00:00 GMT");
}

TEST(UvhttpResponseFullCoverageTest, ResponseEmptyBody) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    response.status_code = UVHTTP_STATUS_NO_CONTENT;
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_body(&response, NULL, 0);
}

TEST(UvhttpResponseFullCoverageTest, ResponseLargeBody) {
    uvhttp_response_t response;
    memset(&response, 0, sizeof(response));
    
    char large_body[1024];
    memset(large_body, 'A', sizeof(large_body));
    large_body[sizeof(large_body) - 1] = '\0';
    
    response.status_code = UVHTTP_STATUS_OK;
    uvhttp_response_set_header(&response, "Content-Type", "text/html");
    uvhttp_response_set_body(&response, large_body, sizeof(large_body) - 1);
}