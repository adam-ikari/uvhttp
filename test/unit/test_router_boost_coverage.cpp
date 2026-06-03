/**
 * @file test_router_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_router module
 *
 * Tests router API functions including route addition, matching,
 * parameter extraction, method parsing, and trie/array mode switching.
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_router.h"
}

#include <string.h>

static int dummy_handler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return 0;
}

static int dummy_handler2(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return 1;
}

class RouterBoostTest : public ::testing::Test {
protected:
    uvhttp_router_t* router = nullptr;

    void SetUp() override {
        uvhttp_error_t err = uvhttp_router_new(&router);
        ASSERT_EQ(err, UVHTTP_OK);
        ASSERT_NE(router, nullptr);
    }

    void TearDown() override {
        if (router) {
            uvhttp_router_free(router);
            router = nullptr;
        }
    }
};

// ========== uvhttp_router_new ==========

TEST_F(RouterBoostTest, New_NullOutput_ReturnsError) {
    EXPECT_EQ(uvhttp_router_new(nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, New_Valid_CreatesRouter) {
    // Already done in SetUp
    EXPECT_NE(router, nullptr);
    EXPECT_EQ(router->route_count, 0u);
}

// ========== uvhttp_router_free ==========

TEST(RouterFreeTest, Free_Null_DoesNotCrash) {
    uvhttp_router_free(nullptr);
}

// ========== uvhttp_router_add_route ==========

TEST_F(RouterBoostTest, AddRoute_NullRouter_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route(nullptr, "/test", dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRoute_NullPath_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route(router, nullptr, dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRoute_NullHandler_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route(router, "/test", nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRoute_EmptyPath_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route(router, "", dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRoute_QueryString_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route(router, "/test?q=1", dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRoute_ValidPath_Success) {
    EXPECT_EQ(uvhttp_router_add_route(router, "/api/users", dummy_handler), UVHTTP_OK);
    EXPECT_EQ(router->route_count, 1u);
}

TEST_F(RouterBoostTest, AddRoute_MultipleRoutes_Success) {
    EXPECT_EQ(uvhttp_router_add_route(router, "/a", dummy_handler), UVHTTP_OK);
    EXPECT_EQ(uvhttp_router_add_route(router, "/b", dummy_handler2), UVHTTP_OK);
    EXPECT_EQ(router->route_count, 2u);
}

// ========== uvhttp_router_add_route_method ==========

TEST_F(RouterBoostTest, AddRouteMethod_NullRouter_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route_method(nullptr, "/test", UVHTTP_GET, dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRouteMethod_NullPath_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, nullptr, UVHTTP_GET, dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRouteMethod_NullHandler_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_GET, nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRouteMethod_EmptyPath_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "", UVHTTP_GET, dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, AddRouteMethod_WithPathParam_MigratesToTrie) {
    // Adding a route with :param should trigger trie migration
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, dummy_handler), UVHTTP_OK);
    EXPECT_EQ(router->use_trie, 1);
}

TEST_F(RouterBoostTest, AddRouteMethod_GET_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_GET, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_POST_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_POST, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_PUT_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_PUT, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_DELETE_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_DELETE, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_HEAD_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_HEAD, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_OPTIONS_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_OPTIONS, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_PATCH_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_PATCH, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_ANY_Success) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test", UVHTTP_ANY, dummy_handler), UVHTTP_OK);
}

TEST_F(RouterBoostTest, AddRouteMethod_QueryString_ReturnsError) {
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/test?q=1", UVHTTP_GET, dummy_handler), UVHTTP_ERROR_INVALID_PARAM);
}

// ========== uvhttp_router_find_handler ==========

TEST_F(RouterBoostTest, FindHandler_NullRouter_ReturnsNull) {
    EXPECT_EQ(uvhttp_router_find_handler(nullptr, "/test", "GET"), nullptr);
}

TEST_F(RouterBoostTest, FindHandler_NullPath_ReturnsNull) {
    EXPECT_EQ(uvhttp_router_find_handler(router, nullptr, "GET"), nullptr);
}

TEST_F(RouterBoostTest, FindHandler_NullMethod_ReturnsNull) {
    EXPECT_EQ(uvhttp_router_find_handler(router, "/test", nullptr), nullptr);
}

TEST_F(RouterBoostTest, FindHandler_ExactMatch_ArrayMode) {
    uvhttp_router_add_route(router, "/api/users", dummy_handler);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/api/users", "GET");
    EXPECT_EQ(found, dummy_handler);
}

TEST_F(RouterBoostTest, FindHandler_NotFound_ArrayMode) {
    uvhttp_router_add_route(router, "/api/users", dummy_handler);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/api/other", "GET");
    EXPECT_EQ(found, nullptr);
}

TEST_F(RouterBoostTest, FindHandler_MethodMismatch_ArrayMode) {
    uvhttp_router_add_route_method(router, "/api/users", UVHTTP_POST, dummy_handler);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/api/users", "GET");
    EXPECT_EQ(found, nullptr);
}

TEST_F(RouterBoostTest, FindHandler_ANYMethod_MatchesAll) {
    uvhttp_router_add_route(router, "/api/users", dummy_handler);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/api/users", "POST");
    EXPECT_EQ(found, dummy_handler);
}

TEST_F(RouterBoostTest, FindHandler_TrieMode_ExactMatch) {
    // Force trie mode by adding a parameter route
    uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, dummy_handler);
    uvhttp_router_add_route_method(router, "/items/:id", UVHTTP_GET, dummy_handler2);

    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/items/42", "GET");
    EXPECT_EQ(found, dummy_handler2);
}

TEST_F(RouterBoostTest, FindHandler_TrieMode_ParamMatch) {
    uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, dummy_handler);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/users/42", "GET");
    EXPECT_EQ(found, dummy_handler);
}

TEST_F(RouterBoostTest, FindHandler_TrieMode_NotFound) {
    uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, dummy_handler);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/posts/42", "GET");
    EXPECT_EQ(found, nullptr);
}

// ========== uvhttp_router_match ==========

TEST_F(RouterBoostTest, Match_NullRouter_ReturnsError) {
    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(nullptr, "/test", "GET", &match), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, Match_NullPath_ReturnsError) {
    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, nullptr, "GET", &match), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, Match_NullMethod_ReturnsError) {
    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, "/test", nullptr, &match), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, Match_NullMatch_ReturnsError) {
    EXPECT_EQ(uvhttp_router_match(router, "/test", "GET", nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RouterBoostTest, Match_ExactMatch_ArrayMode) {
    uvhttp_router_add_route(router, "/api/users", dummy_handler);
    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, "/api/users", "GET", &match), UVHTTP_OK);
    EXPECT_EQ(match.handler, dummy_handler);
    EXPECT_EQ(match.param_count, 0u);
}

TEST_F(RouterBoostTest, Match_NotFound_ArrayMode) {
    uvhttp_router_add_route(router, "/api/users", dummy_handler);
    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, "/api/other", "GET", &match), UVHTTP_ERROR_NOT_FOUND);
}

TEST_F(RouterBoostTest, Match_WithParams_TrieMode) {
    uvhttp_router_add_route_method(router, "/users/:id/posts/:pid", UVHTTP_GET, dummy_handler);
    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, "/users/42/posts/99", "GET", &match), UVHTTP_OK);
    EXPECT_EQ(match.handler, dummy_handler);
    EXPECT_EQ(match.param_count, 2u);
    EXPECT_STREQ(match.params[0].value, "42");
    EXPECT_STREQ(match.params[1].value, "99");
}

TEST_F(RouterBoostTest, Match_TrieMode_NoParams) {
    uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, dummy_handler);
    uvhttp_router_add_route_method(router, "/api/data", UVHTTP_GET, dummy_handler2);

    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, "/api/data", "GET", &match), UVHTTP_OK);
    EXPECT_EQ(match.handler, dummy_handler2);
}

// ========== uvhttp_parse_path_params ==========

TEST_F(RouterBoostTest, ParsePathParams_NullPath_ReturnsError) {
    uvhttp_param_t params[4];
    size_t count = 0;
    EXPECT_EQ(uvhttp_parse_path_params(nullptr, params, &count), -1);
}

TEST_F(RouterBoostTest, ParsePathParams_NullParams_ReturnsError) {
    size_t count = 0;
    EXPECT_EQ(uvhttp_parse_path_params("/test", nullptr, &count), -1);
}

TEST_F(RouterBoostTest, ParsePathParams_NullCount_ReturnsError) {
    uvhttp_param_t params[4];
    EXPECT_EQ(uvhttp_parse_path_params("/test", params, nullptr), -1);
}

TEST_F(RouterBoostTest, ParsePathParams_NoParams_ReturnsZero) {
    uvhttp_param_t params[4];
    size_t count = 99;
    EXPECT_EQ(uvhttp_parse_path_params("/api/users", params, &count), 0);
    EXPECT_EQ(count, 0u);
}

TEST_F(RouterBoostTest, ParsePathParams_WithParams_ExtractsNames) {
    // parse_path_params expects :name:value format (colon-separated pairs)
    uvhttp_param_t params[4];
    size_t count = 0;
    EXPECT_EQ(uvhttp_parse_path_params("/:id:42/:name:john", params, &count), 0);
    EXPECT_EQ(count, 2u);
    EXPECT_STREQ(params[0].name, "id");
    EXPECT_STREQ(params[0].value, "42");
    EXPECT_STREQ(params[1].name, "name");
    EXPECT_STREQ(params[1].value, "john");
}

// ========== uvhttp_method_from_string ==========

TEST(RouterMethodTest, FromString_Null_ReturnsAny) {
    EXPECT_EQ(uvhttp_method_from_string(nullptr), UVHTTP_ANY);
}

TEST(RouterMethodTest, FromString_Empty_ReturnsAny) {
    EXPECT_EQ(uvhttp_method_from_string(""), UVHTTP_ANY);
}

TEST(RouterMethodTest, FromString_GET_ReturnsGet) {
    EXPECT_EQ(uvhttp_method_from_string("GET"), UVHTTP_GET);
}

TEST(RouterMethodTest, FromString_POST_ReturnsPost) {
    EXPECT_EQ(uvhttp_method_from_string("POST"), UVHTTP_POST);
}

TEST(RouterMethodTest, FromString_PUT_ReturnsPut) {
    EXPECT_EQ(uvhttp_method_from_string("PUT"), UVHTTP_PUT);
}

TEST(RouterMethodTest, FromString_DELETE_ReturnsDelete) {
    EXPECT_EQ(uvhttp_method_from_string("DELETE"), UVHTTP_DELETE);
}

TEST(RouterMethodTest, FromString_HEAD_ReturnsHead) {
    EXPECT_EQ(uvhttp_method_from_string("HEAD"), UVHTTP_HEAD);
}

TEST(RouterMethodTest, FromString_OPTIONS_ReturnsOptions) {
    EXPECT_EQ(uvhttp_method_from_string("OPTIONS"), UVHTTP_OPTIONS);
}

TEST(RouterMethodTest, FromString_PATCH_ReturnsPatch) {
    EXPECT_EQ(uvhttp_method_from_string("PATCH"), UVHTTP_PATCH);
}

TEST(RouterMethodTest, FromString_Unknown_ReturnsAny) {
    EXPECT_EQ(uvhttp_method_from_string("TRACE"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("CONNECT"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("INVALID"), UVHTTP_ANY);
}

TEST(RouterMethodTest, FromString_PartialMatch_ReturnsAny) {
    EXPECT_EQ(uvhttp_method_from_string("GE"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("GETX"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("POS"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("POSTX"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("PU"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("PUTX"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("PA"), UVHTTP_ANY);
    EXPECT_EQ(uvhttp_method_from_string("PAT"), UVHTTP_ANY);
}

// ========== uvhttp_method_to_string ==========

TEST(RouterMethodTest, ToString_Get_ReturnsGET) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_GET), "GET");
}

TEST(RouterMethodTest, ToString_Post_ReturnsPOST) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_POST), "POST");
}

TEST(RouterMethodTest, ToString_Put_ReturnsPUT) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_PUT), "PUT");
}

TEST(RouterMethodTest, ToString_Delete_ReturnsDELETE) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_DELETE), "DELETE");
}

TEST(RouterMethodTest, ToString_Head_ReturnsHEAD) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_HEAD), "HEAD");
}

TEST(RouterMethodTest, ToString_Options_ReturnsOPTIONS) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_OPTIONS), "OPTIONS");
}

TEST(RouterMethodTest, ToString_Patch_ReturnsPATCH) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_PATCH), "PATCH");
}

TEST(RouterMethodTest, ToString_Any_ReturnsANY) {
    EXPECT_STREQ(uvhttp_method_to_string(UVHTTP_ANY), "ANY");
}

TEST(RouterMethodTest, ToString_Unknown_ReturnsUNKNOWN) {
    EXPECT_STREQ(uvhttp_method_to_string((uvhttp_method_t)999), "UNKNOWN");
    EXPECT_STREQ(uvhttp_method_to_string((uvhttp_method_t)-1), "UNKNOWN");
}

// ========== Hybrid threshold migration ==========

TEST_F(RouterBoostTest, AddManyRoutes_TriggersTrieMigration) {
    // Add routes with parameters to trigger trie migration
    for (int i = 0; i < 5; i++) {
        char path[64];
        snprintf(path, sizeof(path), "/api/:param%d/items", i);
        EXPECT_EQ(uvhttp_router_add_route(router, path, dummy_handler), UVHTTP_OK);
    }
    EXPECT_EQ(router->use_trie, 1);
    EXPECT_EQ(router->route_count, 5u);
}

// ========== Node pool expansion ==========

TEST_F(RouterBoostTest, TrieMode_MultipleParamRoutes) {
    // Add multiple parameterized routes
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/users/:id", UVHTTP_GET, dummy_handler), UVHTTP_OK);
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/posts/:id", UVHTTP_GET, dummy_handler2), UVHTTP_OK);
    EXPECT_EQ(uvhttp_router_add_route_method(router, "/items/:id/details", UVHTTP_GET, dummy_handler), UVHTTP_OK);

    // Verify all routes work
    EXPECT_EQ(uvhttp_router_find_handler(router, "/users/1", "GET"), dummy_handler);
    EXPECT_EQ(uvhttp_router_find_handler(router, "/posts/2", "GET"), dummy_handler2);
    EXPECT_EQ(uvhttp_router_find_handler(router, "/items/3/details", "GET"), dummy_handler);
}

// ========== Route with deep path ==========

TEST_F(RouterBoostTest, AddRoute_DeepPath_Success) {
    EXPECT_EQ(uvhttp_router_add_route(router, "/a/b/c/d/e/f/g/h", dummy_handler), UVHTTP_OK);
    uvhttp_request_handler_t found = uvhttp_router_find_handler(router, "/a/b/c/d/e/f/g/h", "GET");
    EXPECT_EQ(found, dummy_handler);
}

// ========== Route match with method filtering ==========

TEST_F(RouterBoostTest, Match_MethodFiltering_TrieMode) {
    uvhttp_router_add_route_method(router, "/resource", UVHTTP_GET, dummy_handler);
    uvhttp_router_add_route_method(router, "/resource", UVHTTP_POST, dummy_handler2);

    uvhttp_route_match_t match;
    EXPECT_EQ(uvhttp_router_match(router, "/resource", "GET", &match), UVHTTP_OK);
    EXPECT_EQ(match.handler, dummy_handler);

    EXPECT_EQ(uvhttp_router_match(router, "/resource", "POST", &match), UVHTTP_OK);
    EXPECT_EQ(match.handler, dummy_handler2);

    EXPECT_EQ(uvhttp_router_match(router, "/resource", "DELETE", &match), UVHTTP_ERROR_NOT_FOUND);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
