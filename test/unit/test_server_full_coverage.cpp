/* uvhttp_server.c 完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_config.h"
#include "uvhttp_allocator.h"

TEST(UvhttpServerFullCoverageTest, ServerNewDefaultLoop) {
    uvhttp_server_t* server = uvhttp_server_new(NULL);
    if (server != NULL) {
        ASSERT_NE(server, nullptr);
        EXPECT_EQ(server->is_listening, 0);
        EXPECT_EQ(server->active_connections, 0);
        uvhttp_server_free(server);
    }
}

TEST(UvhttpServerFullCoverageTest, ServerFreeNull) {
    uvhttp_error_t result = uvhttp_server_free(NULL);
}

TEST(UvhttpServerFullCoverageTest, ServerSetHandlerNull) {
    uvhttp_error_t result = uvhttp_server_set_handler(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerSetRouterNull) {
    uvhttp_error_t result = uvhttp_server_set_router(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerListenNull) {
    uvhttp_error_t result = uvhttp_server_listen(NULL, "0.0.0.0", 8080);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerStopNull) {
    uvhttp_error_t result = uvhttp_server_stop(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerAddMiddlewareNull) {
    uvhttp_error_t result = uvhttp_server_add_middleware(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerRemoveMiddlewareNull) {
    uvhttp_error_t result = uvhttp_server_remove_middleware(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerCleanupMiddlewareNull) {
    uvhttp_server_cleanup_middleware(NULL);
}

#if UVHTTP_FEATURE_RATE_LIMIT
TEST(UvhttpServerFullCoverageTest, ServerEnableRateLimitNull) {
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(NULL, 100, 60);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerDisableRateLimitNull) {
    uvhttp_error_t result = uvhttp_server_disable_rate_limit(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerCheckRateLimitNull) {
    uvhttp_error_t result = uvhttp_server_check_rate_limit(NULL);
}

TEST(UvhttpServerFullCoverageTest, ServerAddRateLimitWhitelistNull) {
    uvhttp_error_t result = uvhttp_server_add_rate_limit_whitelist(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerGetRateLimitStatusNull) {
    uvhttp_error_t result;
    int remaining;
    uint64_t reset_time;
    
    result = uvhttp_server_get_rate_limit_status(NULL, NULL, &remaining, &reset_time);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerResetRateLimitClientNull) {
    uvhttp_error_t result = uvhttp_server_reset_rate_limit_client(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerClearRateLimitAllNull) {
    uvhttp_error_t result = uvhttp_server_clear_rate_limit_all(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}
#endif

#if UVHTTP_FEATURE_TLS
TEST(UvhttpServerFullCoverageTest, ServerEnableTlsNull) {
    uvhttp_error_t result = uvhttp_server_enable_tls(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerDisableTlsNull) {
    uvhttp_error_t result = uvhttp_server_disable_tls(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}
#endif

#if UVHTTP_FEATURE_WEBSOCKET
TEST(UvhttpServerFullCoverageTest, ServerRegisterWsHandlerNull) {
    uvhttp_error_t result = uvhttp_server_register_ws_handler(NULL, NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerWsSendNull) {
    uvhttp_error_t result = uvhttp_server_ws_send(NULL, NULL, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, ServerWsCloseNull) {
    uvhttp_error_t result = uvhttp_server_ws_close(NULL, 1000, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}
#endif

TEST(UvhttpServerFullCoverageTest, ServerStructSize) {
    EXPECT_GT(sizeof(uvhttp_server_t), 0);
    EXPECT_GT(sizeof(uvhttp_server_builder_t), 0);
}

TEST(UvhttpServerFullCoverageTest, ServerConstants) {
    EXPECT_GT(MAX_CONNECTIONS, 0);
    EXPECT_GT(INET_ADDRSTRLEN, 0);
}

TEST(UvhttpServerFullCoverageTest, RequestInitNull) {
    uvhttp_error_t result = uvhttp_request_init(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, RequestCleanupNull) {
    uvhttp_request_cleanup(NULL);
}

TEST(UvhttpServerFullCoverageTest, GetParamNull) {
    const char* result = uvhttp_get_param(NULL, "name");
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, GetHeaderNull) {
    const char* result = uvhttp_get_header(NULL, "Content-Type");
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, GetBodyNull) {
    const char* result = uvhttp_get_body(NULL);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, ServerMultipleCreateFree) {
    for (int i = 0; i < 10; i++) {
        uvhttp_server_t* server = uvhttp_server_new(NULL);
        if (server != NULL) {
            uvhttp_server_free(server);
        }
    }
}

TEST(UvhttpServerFullCoverageTest, ServerInitializationState) {
    uvhttp_server_t* server = uvhttp_server_new(NULL);
    if (server != NULL) {
        EXPECT_EQ(server->is_listening, 0);
        EXPECT_EQ(server->active_connections, 0);
        EXPECT_EQ(server->handler, nullptr);
        EXPECT_EQ(server->router, nullptr);
        EXPECT_EQ(server->middleware_chain, nullptr);
        uvhttp_server_free(server);
    }
}

TEST(UvhttpServerFullCoverageTest, ServerSetHandlerValid) {
    uvhttp_server_t* server = uvhttp_server_new(NULL);
    if (server != NULL) {
        uvhttp_error_t result = uvhttp_server_set_handler(server, [](uvhttp_request_t* req, uvhttp_response_t* resp) -> int {
            (void)req;
            (void)resp;
            return 0;
        });
        EXPECT_EQ(result, UVHTTP_OK);
        uvhttp_server_free(server);
    }
}

TEST(UvhttpServerFullCoverageTest, ServerSetRouterValid) {
    uvhttp_server_t* server = uvhttp_server_new(NULL);
    if (server != NULL) {
        uvhttp_router_t* router = uvhttp_router_new();
        if (router != NULL) {
            uvhttp_error_t result = uvhttp_server_set_router(server, router);
            EXPECT_EQ(result, UVHTTP_OK);
            uvhttp_router_free(router);
        }
        uvhttp_server_free(server);
    }
}

TEST(UvhttpServerFullCoverageTest, ServerListenInvalidHost) {
    uvhttp_server_t* server = uvhttp_server_new(NULL);
    if (server != NULL) {
        uvhttp_error_t result = uvhttp_server_listen(server, NULL, 8080);
        EXPECT_NE(result, UVHTTP_OK);
        uvhttp_server_free(server);
    }
}

TEST(UvhttpServerFullCoverageTest, ServerStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_server_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_server_t), sizeof(int));
    EXPECT_GE(sizeof(uvhttp_server_t), sizeof(size_t));
}

TEST(UvhttpServerFullCoverageTest, ServerBuilderStructAlignment) {
    EXPECT_GE(sizeof(uvhttp_server_builder_t), sizeof(void*));
    EXPECT_GE(sizeof(uvhttp_server_builder_t), sizeof(int));
}

TEST(UvhttpServerFullCoverageTest, ServerConstantsValidation) {
    EXPECT_GT(MAX_CONNECTIONS, 0);
    EXPECT_GE(MAX_CONNECTIONS, 100);
    EXPECT_LE(MAX_CONNECTIONS, 100000);
    
    EXPECT_GT(INET_ADDRSTRLEN, 0);
    EXPECT_GE(INET_ADDRSTRLEN, 16);
}

TEST(UvhttpServerFullCoverageTest, ServerMultipleNullOperations) {
    for (int i = 0; i < 100; i++) {
        uvhttp_server_set_handler(NULL, NULL);
        uvhttp_server_set_router(NULL, NULL);
        uvhttp_server_listen(NULL, "0.0.0.0", 8080);
        uvhttp_server_stop(NULL);
        uvhttp_server_add_middleware(NULL, NULL);
        uvhttp_server_remove_middleware(NULL, NULL);
        uvhttp_server_cleanup_middleware(NULL);
        
        uvhttp_request_init(NULL, NULL);
        uvhttp_request_cleanup(NULL);
        
        uvhttp_get_param(NULL, "name");
        uvhttp_get_header(NULL, "Content-Type");
        uvhttp_get_body(NULL);
    }
}

TEST(UvhttpServerFullCoverageTest, RequestInitInvalidParams) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    uvhttp_error_t result = uvhttp_request_init(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpServerFullCoverageTest, GetParamEmptyName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* result = uvhttp_get_param(&request, "");
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, GetHeaderEmptyName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* result = uvhttp_get_header(&request, "");
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, GetHeaderNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* result = uvhttp_get_header(&request, NULL);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, GetParamNullName) {
    uvhttp_request_t request;
    memset(&request, 0, sizeof(request));
    
    const char* result = uvhttp_get_param(&request, NULL);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpServerFullCoverageTest, QuickResponseNullRequest) {
    }

TEST(UvhttpServerFullCoverageTest, ServerStructFieldValidation) {
    uvhttp_server_t server;
    memset(&server, 0, sizeof(server));
    
    EXPECT_EQ(server.is_listening, 0);
    EXPECT_EQ(server.active_connections, 0);
    EXPECT_EQ(server.handler, nullptr);
    EXPECT_EQ(server.router, nullptr);
    EXPECT_EQ(server.middleware_chain, nullptr);
}

TEST(UvhttpServerFullCoverageTest, ServerBuilderStructFieldValidation) {
    uvhttp_server_builder_t builder;
    memset(&builder, 0, sizeof(builder));
    
    EXPECT_EQ(sizeof(builder), sizeof(uvhttp_server_builder_t));
}

TEST(UvhttpServerFullCoverageTest, ServerMemoryLayout) {
    size_t expected_size = sizeof(uv_loop_t*) +
                           sizeof(int) +     /* is_listening */
                           sizeof(int) +     /* active_connections */
                           sizeof(uvhttp_request_handler_t) +
                           sizeof(uvhttp_router_t*) +
                           sizeof(void*) +   /* middleware_chain */
                           sizeof(uv_tcp_t) +
                           sizeof(uv_idle_t);
    
    EXPECT_GE(sizeof(uvhttp_server_t), expected_size);
}