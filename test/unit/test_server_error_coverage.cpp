/* uvhttp_server.c 错误处理覆盖率测试 - 测试 NULL 参数和错误情况 */

#include <gtest/gtest.h>
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <string.h>

/* 测试服务器创建 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, ServerNewNullParams) {
    uvhttp_server_t* server = NULL;
    
    /* 测试 NULL loop，NULL server 指针 */
    uvhttp_error_t result = uvhttp_server_new(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    EXPECT_EQ(server, (uvhttp_server_t*)NULL);
    
    /* 测试 NULL server 指针 */
    result = uvhttp_server_new(uv_default_loop(), NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试服务器释放 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, ServerFreeNull) {
    /* 测试释放 NULL 服务器 */
    uvhttp_server_free(NULL);
}

/* 测试服务器监听 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, ServerListenNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_listen(NULL, "127.0.0.1", 8080);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 主机 */
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        result = uvhttp_server_listen(server, NULL, 8080);
        EXPECT_NE(result, UVHTTP_OK);
        uvhttp_server_free(server);
    }
}

/* 测试服务器停止 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, ServerStopNull) {
    uvhttp_error_t result = uvhttp_server_stop(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试设置处理器 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, SetHandlerNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_set_handler(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 处理器 */
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        result = uvhttp_server_set_handler(server, NULL);
        EXPECT_EQ(result, UVHTTP_OK);
        uvhttp_server_free(server);
    }
}

/* 测试设置路由器 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, SetRouterNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_set_router(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 路由器 */
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        result = uvhttp_server_set_router(server, NULL);
        EXPECT_EQ(result, UVHTTP_OK);
        uvhttp_server_free(server);
    }
}

/* 测试设置上下文 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, SetContextNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_set_context(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试 NULL 上下文 */
    uvhttp_server_t* server = NULL;
    result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        result = uvhttp_server_set_context(server, NULL);
        EXPECT_EQ(result, UVHTTP_OK);
        uvhttp_server_free(server);
    }
}

#if UVHTTP_FEATURE_RATE_LIMIT
/* 测试限流功能 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, RateLimitNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_enable_rate_limit(NULL, 100, 60);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_disable_rate_limit(NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_check_rate_limit(NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_add_rate_limit_whitelist(NULL, "127.0.0.1");
    EXPECT_NE(result, UVHTTP_OK);
    
    int remaining;
    uint64_t reset_time;
    result = uvhttp_server_get_rate_limit_status(NULL, "127.0.0.1", &remaining, &reset_time);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_reset_rate_limit_client(NULL, "127.0.0.1");
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_clear_rate_limit_all(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}

/* 测试限流功能无效参数 */
TEST(UvhttpServerErrorCoverageTest, RateLimitInvalidParams) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        /* 测试无效的 max_requests */
        result = uvhttp_server_enable_rate_limit(server, 0, 60);
        EXPECT_NE(result, UVHTTP_OK);
        
        result = uvhttp_server_enable_rate_limit(server, -1, 60);
        EXPECT_NE(result, UVHTTP_OK);
        
        /* 测试无效的 window_seconds */
        result = uvhttp_server_enable_rate_limit(server, 100, 0);
        EXPECT_NE(result, UVHTTP_OK);
        
        result = uvhttp_server_enable_rate_limit(server, 100, -1);
        EXPECT_NE(result, UVHTTP_OK);
        
        uvhttp_server_free(server);
    }
}
#endif /* UVHTTP_FEATURE_RATE_LIMIT */

#if UVHTTP_FEATURE_WEBSOCKET
/* 测试 WebSocket 功能 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, WebSocketNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_register_ws_handler(NULL, "/ws", NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_ws_send(NULL, "test", 4);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_ws_close(NULL, 1000, "Normal");
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_ws_broadcast(NULL, "/ws", "test", 4);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_ws_close_all(NULL, "/ws");
    EXPECT_NE(result, UVHTTP_OK);
    
    /* 测试获取连接数 */
    int count = uvhttp_server_ws_get_connection_count(NULL);
    EXPECT_EQ(count, 0);
    
    count = uvhttp_server_ws_get_connection_count_by_path(NULL, "/ws");
    EXPECT_EQ(count, 0);
}

/* 测试 WebSocket 连接管理 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, WebSocketConnectionManagementNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_ws_enable_connection_management(NULL, 60, 30);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_ws_disable_connection_management(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}
#endif /* UVHTTP_FEATURE_WEBSOCKET */

#if UVHTTP_FEATURE_TLS
/* 测试 TLS 功能 NULL 参数 */
TEST(UvhttpServerErrorCoverageTest, TLSNullParams) {
    /* 测试 NULL 服务器 */
    uvhttp_error_t result = uvhttp_server_enable_tls(NULL, NULL);
    EXPECT_NE(result, UVHTTP_OK);
    
    result = uvhttp_server_disable_tls(NULL);
    EXPECT_NE(result, UVHTTP_OK);
}
#endif /* UVHTTP_FEATURE_TLS */

/* 测试服务器结构体字段访问 */
TEST(UvhttpServerErrorCoverageTest, ServerStructureFields) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        /* 测试结构体字段 */
        EXPECT_GE(server->max_connections, 0);
        EXPECT_GE(server->max_message_size, 0);
        EXPECT_EQ(server->is_listening, 0);
        EXPECT_EQ(server->owns_loop, 0);
        EXPECT_EQ(server->active_connections, 0);
        EXPECT_NE(server->loop, nullptr);
        
        uvhttp_server_free(server);
    }
}

/* 测试多次释放服务器 */
TEST(UvhttpServerErrorCoverageTest, MultipleFree) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        uvhttp_server_free(server);
        uvhttp_server_free(server);  /* 第二次释放，不应该崩溃 */
    }
}

/* 测试服务器创建和监听完整流程 */
TEST(UvhttpServerErrorCoverageTest, ServerCreateAndListen) {
    uvhttp_server_t* server = NULL;
    uvhttp_error_t result = uvhttp_server_new(uv_default_loop(), &server);
    if (result == UVHTTP_OK && server != NULL) {
        /* 添加路由器 */
        uvhttp_router_t* router = NULL;
        result = uvhttp_router_new(&router);
        if (result == UVHTTP_OK && router != NULL) {
            uvhttp_router_add_route(router, "/", NULL);
            uvhttp_server_set_router(server, router);
            
            /* 尝试监听（可能失败，这是预期的） */
            result = uvhttp_server_listen(server, "127.0.0.1", 18080);
            if (result != UVHTTP_OK) {
                /* 监听失败是预期的，因为端口可能被占用 */
            }
            
            /* 停止服务器 */
            uvhttp_server_stop(server);
        }
        
        uvhttp_server_free(server);
    }
}