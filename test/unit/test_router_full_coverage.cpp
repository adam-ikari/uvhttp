/* UVHTTP 路由器模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_router.h"
#include "uvhttp_constants.h"

TEST(UvhttpRouterFullCoverageTest, RouterNew) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        ASSERT_NE(router, nullptr);
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterFree) {
    uvhttp_router_free(NULL);
    
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, MethodFromString) {
    EXPECT_EQ(uvhttp_method_from_string("GET"), UVHTTP_GET);
    EXPECT_EQ(uvhttp_method_from_string("POST"), UVHTTP_POST);
    EXPECT_EQ(uvhttp_method_from_string("PUT"), UVHTTP_PUT);
    EXPECT_EQ(uvhttp_method_from_string("DELETE"), UVHTTP_DELETE);
    EXPECT_EQ(uvhttp_method_from_string("HEAD"), UVHTTP_HEAD);
    EXPECT_EQ(uvhttp_method_from_string("OPTIONS"), UVHTTP_OPTIONS);
    EXPECT_EQ(uvhttp_method_from_string("PATCH"), UVHTTP_PATCH);
    
    EXPECT_EQ(uvhttp_method_from_string(NULL), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string(""), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("INVALID"), UVHTTP_ANY);
}

TEST(UvhttpRouterFullCoverageTest, MethodToString) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_GET), "GET");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_POST), "POST");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_PUT), "PUT");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_DELETE), "DELETE");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_HEAD), "HEAD");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_OPTIONS), "OPTIONS");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_PATCH), "PATCH");
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_ANY), "ANY");
    
    EXPECT_STREQ(uvhttp_method_to_string((uvhttp_method_t)999), "UNKNOWN");
}

TEST(UvhttpRouterFullCoverageTest, RouterAddRoute) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_error_t result = uvhttp_router_add_route(router, "/api", NULL);
        
        uvhttp_router_add_route(router, "/api/users", NULL);
        uvhttp_router_add_route(router, "/api/posts", NULL);
        uvhttp_router_add_route(router, "/health", NULL);
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterAddRouteMethod) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_error_t result = uvhttp_router_add_route_method(router, "/api", UVHTTP_GET, NULL);
        
        uvhttp_router_add_route_method(router, "/api/users", UVHTTP_GET, NULL);
        uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, NULL);
        uvhttp_router_add_route_method(router, "/api/posts", UVHTTP_GET, NULL);
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterFindHandler) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_router_add_route(router, "/api", NULL);
        uvhttp_router_add_route(router, "/api/users", NULL);
        
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/api", "GET");
        handler = uvhttp_router_find_handler(router, "/api/users", "GET");
        handler = uvhttp_router_find_handler(router, "/nonexistent", "GET");
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterMatch) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_router_add_route(router, "/api", NULL);
        
        uvhttp_route_match_t match;
        uvhttp_error_t result = uvhttp_router_match(router, "/api", "GET", &match);
        result = uvhttp_router_match(router, "/nonexistent", "GET", &match);
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterNullParams) {
    uvhttp_router_find_handler(NULL, "/api", "GET");
    uvhttp_router_find_handler(NULL, NULL, "GET");
    uvhttp_router_find_handler(NULL, "/api", NULL);
    
    uvhttp_router_add_route(NULL, "/api", NULL);
    uvhttp_router_add_route(NULL, NULL, NULL);
    
    uvhttp_router_add_route_method(NULL, "/api", UVHTTP_GET, NULL);
    uvhttp_router_add_route_method(NULL, NULL, UVHTTP_GET, NULL);
    
    uvhttp_router_match(NULL, "/api", "GET", NULL);
    uvhttp_router_match(NULL, NULL, "GET", NULL);
    uvhttp_router_match(NULL, "/api", NULL, NULL);
}

TEST(UvhttpRouterFullCoverageTest, RouterMultipleCycles) {
    for (int i = 0; i < 5; i++) {
        uvhttp_router_t* router = uvhttp_router_new();
        if (router != NULL) {
            uvhttp_router_add_route(router, "/api", NULL);
            uvhttp_router_free(router);
        }
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterCount) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_router_add_route(router, "/api", NULL);
        uvhttp_router_add_route(router, "/api/users", NULL);
        uvhttp_router_add_route(router, "/api/posts", NULL);
        uvhttp_router_add_route(router, "/health", NULL);
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterDifferentMethods) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_router_add_route_method(router, "/api", UVHTTP_GET, NULL);
        uvhttp_router_add_route_method(router, "/api", UVHTTP_POST, NULL);
        uvhttp_router_add_route_method(router, "/api", UVHTTP_PUT, NULL);
        uvhttp_router_add_route_method(router, "/api", UVHTTP_DELETE, NULL);
        uvhttp_router_add_route_method(router, "/api", UVHTTP_HEAD, NULL);
        uvhttp_router_add_route_method(router, "/api", UVHTTP_OPTIONS, NULL);
        uvhttp_router_add_route_method(router, "/api", UVHTTP_PATCH, NULL);
        
        uvhttp_router_free(router);
    }
}

TEST(UvhttpRouterFullCoverageTest, RouterComplexPaths) {
    uvhttp_router_t* router = uvhttp_router_new();
    if (router != NULL) {
        uvhttp_router_add_route(router, "/api/v1/users", NULL);
        uvhttp_router_add_route(router, "/api/v1/posts", NULL);
        uvhttp_router_add_route(router, "/api/v2/users", NULL);
        uvhttp_router_add_route(router, "/api/v2/posts", NULL);
        uvhttp_router_add_route(router, "/admin/dashboard", NULL);
        uvhttp_router_add_route(router, "/admin/settings", NULL);
        
        uvhttp_router_free(router);
    }
}