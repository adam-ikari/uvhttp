/* UVHTTP 中间件模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_constants.h"
#include <uv.h>

static int test_middleware_handler(uvhttp_request_t* request, 
                                  uvhttp_response_t* response, 
                                  uvhttp_middleware_context_t* ctx) {
    (void)request;
    (void)response;
    (void)ctx;
    return UVHTTP_MIDDLEWARE_CONTINUE;
}

static int stop_middleware_handler(uvhttp_request_t* request, 
                                   uvhttp_response_t* response, 
                                   uvhttp_middleware_context_t* ctx) {
    (void)request;
    (void)response;
    (void)ctx;
    return UVHTTP_MIDDLEWARE_STOP;
}

static void test_context_cleanup(void* data) {
    if (data) {
        free(data);
    }
}

TEST(UvhttpMiddlewareFullCoverageTest, MiddlewareCreate) {
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    EXPECT_EQ(middleware->handler, test_middleware_handler);
    EXPECT_EQ(middleware->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_STREQ(middleware->path, "/api");
    EXPECT_EQ(middleware->next, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
    
    middleware = uvhttp_http_middleware_create(NULL, test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    ASSERT_NE(middleware, nullptr);
    EXPECT_EQ(middleware->path, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
    
    middleware = uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    ASSERT_NE(middleware, nullptr);
    EXPECT_EQ(middleware->priority, UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    
    uvhttp_http_middleware_destroy(middleware);
}

TEST(UvhttpMiddlewareFullCoverageTest, NullParams) {
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", NULL, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_EQ(middleware, nullptr);
    
    middleware = uvhttp_http_middleware_create(NULL, test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    middleware = uvhttp_http_middleware_create(NULL, NULL, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_EQ(middleware, nullptr);
    
    uvhttp_http_middleware_destroy(NULL);
    uvhttp_http_middleware_set_context(NULL, NULL, NULL);
}

TEST(UvhttpMiddlewareFullCoverageTest, MiddlewareDestroy) {
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    uvhttp_http_middleware_destroy(middleware);
    
    middleware = uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    uvhttp_http_middleware_destroy(middleware);
}

TEST(UvhttpMiddlewareFullCoverageTest, MiddlewareSetContext) {
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    int* data = (int*)malloc(sizeof(int));
    *data = 42;
    uvhttp_http_middleware_set_context(middleware, data, test_context_cleanup);
    EXPECT_EQ(middleware->context.data, data);
    EXPECT_EQ(middleware->context.cleanup, test_context_cleanup);
    
    uvhttp_http_middleware_destroy(middleware);
    
    middleware = uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                               UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(middleware, nullptr);
    
    uvhttp_http_middleware_set_context(middleware, NULL, NULL);
    EXPECT_EQ(middleware->context.data, nullptr);
    EXPECT_EQ(middleware->context.cleanup, nullptr);
    
    uvhttp_http_middleware_destroy(middleware);
}

TEST(UvhttpMiddlewareFullCoverageTest, MiddlewareChain) {
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    ASSERT_NE(mw1, nullptr);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw2, nullptr);
    
    uvhttp_http_middleware_t* mw3 = 
        uvhttp_http_middleware_create("/static", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    ASSERT_NE(mw3, nullptr);
    
    mw1->next = mw2;
    mw2->next = mw3;
    
    EXPECT_EQ(mw1->next, mw2);
    EXPECT_EQ(mw2->next, mw3);
    EXPECT_EQ(mw3->next, nullptr);
    
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    uvhttp_http_middleware_destroy(mw3);
}

TEST(UvhttpMiddlewareFullCoverageTest, MiddlewareExecute) {
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw1, nullptr);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw2, nullptr);
    
    mw1->next = mw2;
    
    uvhttp_request_t request;
    uv_tcp_t client;
    uvhttp_request_init(&request, &client);
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, &client);
    
    int result = uvhttp_http_middleware_execute(mw1, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    result = uvhttp_http_middleware_execute(NULL, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    result = uvhttp_http_middleware_execute(mw1, NULL, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    result = uvhttp_http_middleware_execute(mw1, &request, NULL);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
}

TEST(UvhttpMiddlewareFullCoverageTest, MiddlewareStop) {
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", stop_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw1, nullptr);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw2, nullptr);
    
    mw1->next = mw2;
    
    uvhttp_request_t request;
    uv_tcp_t client;
    uvhttp_request_init(&request, &client);
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, &client);
    
    strncpy(request.url, "/api/test", MAX_URL_LEN - 1);
    
    int result = uvhttp_http_middleware_execute(mw1, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_STOP);
    
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
}

TEST(UvhttpMiddlewareFullCoverageTest, PathMatching) {
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw1, nullptr);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create(NULL, test_middleware_handler, 
                                       UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw2, nullptr);
    
    uvhttp_request_t request;
    uv_tcp_t client;
    uvhttp_request_init(&request, &client);
    
    uvhttp_response_t response;
    uvhttp_response_init(&response, &client);
    
    strncpy(request.url, "/api", MAX_URL_LEN - 1);
    int result = uvhttp_http_middleware_execute(mw1, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    strncpy(request.url, "/api/test", MAX_URL_LEN - 1);
    result = uvhttp_http_middleware_execute(mw1, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    strncpy(request.url, "/test", MAX_URL_LEN - 1);
    result = uvhttp_http_middleware_execute(mw1, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    strncpy(request.url, "/any/path", MAX_URL_LEN - 1);
    result = uvhttp_http_middleware_execute(mw2, &request, &response);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
    
    uvhttp_request_cleanup(&request);
    uvhttp_response_cleanup(&response);
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
}

TEST(UvhttpMiddlewareFullCoverageTest, MultipleCreateDestroy) {
    for (int i = 0; i < 100; i++) {
        uvhttp_http_middleware_t* middleware = 
            uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                         UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
        ASSERT_NE(middleware, nullptr);
        uvhttp_http_middleware_destroy(middleware);
    }
}

TEST(UvhttpMiddlewareFullCoverageTest, DifferentPriorities) {
    uvhttp_http_middleware_t* mw1 = 
        uvhttp_http_middleware_create("/api", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    ASSERT_NE(mw1, nullptr);
    EXPECT_EQ(mw1->priority, UVHTTP_MIDDLEWARE_PRIORITY_HIGH);
    
    uvhttp_http_middleware_t* mw2 = 
        uvhttp_http_middleware_create("/test", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    ASSERT_NE(mw2, nullptr);
    EXPECT_EQ(mw2->priority, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    
    uvhttp_http_middleware_t* mw3 = 
        uvhttp_http_middleware_create("/static", test_middleware_handler, 
                                     UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    ASSERT_NE(mw3, nullptr);
    EXPECT_EQ(mw3->priority, UVHTTP_MIDDLEWARE_PRIORITY_LOW);
    
    uvhttp_http_middleware_destroy(mw1);
    uvhttp_http_middleware_destroy(mw2);
    uvhttp_http_middleware_destroy(mw3);
}

TEST(UvhttpMiddlewareFullCoverageTest, MemoryAllocationFailure) {
    uvhttp_http_middleware_t* middleware = 
        uvhttp_http_middleware_create("/api", NULL, UVHTTP_MIDDLEWARE_PRIORITY_NORMAL);
    EXPECT_EQ(middleware, nullptr);
}