/* uvhttp_request.c NULL参数覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_request.h"

TEST(UvhttpRequestNullCoverageTest, RequestFreeNull) {
    uvhttp_request_free(NULL);
}

TEST(UvhttpRequestNullCoverageTest, RequestCleanupNull) {
    uvhttp_request_cleanup(NULL);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetMethodNull) {
    const char* method = uvhttp_request_get_method(NULL);
    EXPECT_EQ(method, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetUrlNull) {
    const char* url = uvhttp_request_get_url(NULL);
    EXPECT_EQ(url, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetPathNull) {
    const char* path = uvhttp_request_get_path(NULL);
    EXPECT_EQ(path, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetQueryStringNull) {
    const char* query = uvhttp_request_get_query_string(NULL);
    EXPECT_EQ(query, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetQueryParamNull) {
    const char* param = uvhttp_request_get_query_param(NULL, "test");
    EXPECT_EQ(param, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetClientIpNull) {
    const char* ip = uvhttp_request_get_client_ip(NULL);
    EXPECT_EQ(ip, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetHeaderNull) {
    const char* header = uvhttp_request_get_header(NULL, "test");
    EXPECT_EQ(header, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetBodyNull) {
    const char* body = uvhttp_request_get_body(NULL);
    EXPECT_EQ(body, nullptr);
}

TEST(UvhttpRequestNullCoverageTest, RequestGetBodyLengthNull) {
    size_t len = uvhttp_request_get_body_length(NULL);
    EXPECT_EQ(len, 0);
}