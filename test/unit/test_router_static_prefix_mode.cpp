/* UVHTTP Router static_prefix mode testing
 *
 * This test verifies that static_prefix check works correctly in both
 * Array and Trie modes. Before the fix, static_prefix was only checked
 * in Trie mode, causing static file requests to fail in Array mode.
 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_router.h"
#include "uvhttp_constants.h"

// Dummy handler for testing
static void dummy_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    (void)req;
    (void)res;
}

/* Test: static_prefix check in Array mode (default, < 100 routes) */
TEST(RouterStaticPrefixModeTest, ArrayModeStaticPrefixCheck) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add a few routes to stay in Array mode (threshold is 100)
    for (int i = 0; i < 10; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, dummy_handler);
    }

    // Verify we are in Array mode
    EXPECT_EQ(router->use_trie, 0);
    EXPECT_LT(router->array_route_count, 100);

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/static", "./public");

    // Test finding static file handler in Array mode
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/static/index.html", "GET");
    EXPECT_NE(handler, nullptr) << "static_prefix should be checked in Array mode";

    // Test uvhttp_router_match also works
    uvhttp_route_match_t match;
    uvhttp_error_t result = uvhttp_router_match(router, "/static/index.html", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK) << "uvhttp_router_match should find static files in Array mode";
    EXPECT_NE(match.handler, nullptr);

    // Test non-static path still works
    handler = uvhttp_router_find_handler(router, "/api/route0", "GET");
    EXPECT_NE(handler, nullptr) << "Regular routes should still work in Array mode";

    uvhttp_router_free(router);
}

/* Test: static_prefix check in Trie mode (>= 100 routes or has params) */
TEST(RouterStaticPrefixModeTest, TrieModeStaticPrefixCheck) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add enough routes to trigger Trie mode (threshold is 100)
    for (int i = 0; i < 150; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, dummy_handler);
    }

    // Verify we are in Trie mode
    EXPECT_EQ(router->use_trie, 1);

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/static", "./public");

    // Test finding static file handler in Trie mode
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/static/index.html", "GET");
    EXPECT_NE(handler, nullptr) << "static_prefix should be checked in Trie mode";

    // Test uvhttp_router_match also works
    uvhttp_route_match_t match;
    uvhttp_error_t result = uvhttp_router_match(router, "/static/index.html", "GET", &match);
    EXPECT_EQ(result, UVHTTP_OK) << "uvhttp_router_match should find static files in Trie mode";
    EXPECT_NE(match.handler, nullptr);

    // Test non-static path still works
    handler = uvhttp_router_find_handler(router, "/api/route0", "GET");
    EXPECT_NE(handler, nullptr) << "Regular routes should still work in Trie mode";

    uvhttp_router_free(router);
}

/* Test: static_prefix with path parameters (immediately switches to Trie) */
TEST(RouterStaticPrefixModeTest, TrieModeWithParamsAndStaticPrefix) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add a route with parameters (should trigger Trie mode immediately)
    uvhttp_router_add_route(router, "/users/:id", dummy_handler);
    uvhttp_router_add_route(router, "/posts/:postId/comments/:commentId", dummy_handler);

    // Verify we are in Trie mode
    EXPECT_EQ(router->use_trie, 1);

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/static", "./public");

    // Test finding static file handler
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/static/style.css", "GET");
    EXPECT_NE(handler, nullptr) << "static_prefix should work with parameter routes";

    // Test parameter routes still work
    handler = uvhttp_router_find_handler(router, "/users/123", "GET");
    EXPECT_NE(handler, nullptr) << "Parameter routes should still work";

    uvhttp_router_free(router);
}

/* Test: static_prefix different paths */
TEST(RouterStaticPrefixModeTest, StaticPrefixVariousPaths) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add routes to stay in Array mode
    uvhttp_router_add_route(router, "/api", dummy_handler);

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/assets", "./public");

    // Test various static file paths
    const char* static_paths[] = {
        "/assets/index.html",
        "/assets/css/style.css",
        "/assets/js/app.js",
        "/assets/images/logo.png",
        "/assets/vendor/jquery.min.js"
    };

    for (size_t i = 0; i < sizeof(static_paths) / sizeof(static_paths[0]); i++) {
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, static_paths[i], "GET");
        EXPECT_NE(handler, nullptr) << "Should find static file: " << static_paths[i];
    }

    // Test non-matching path
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/other/path", "GET");
    EXPECT_EQ(handler, nullptr) << "Non-matching path should return NULL";

    uvhttp_router_free(router);
}

/* Test: static_prefix with different HTTP methods */
TEST(RouterStaticPrefixModeTest, StaticPrefixWithMethods) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add routes to stay in Array mode
    uvhttp_router_add_route(router, "/api", dummy_handler);

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/static", "./public");

    // Test static files with different methods
    const char* methods[] = {"GET", "HEAD", "POST", "PUT", "DELETE"};

    for (size_t i = 0; i < sizeof(methods) / sizeof(methods[0]); i++) {
        uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/static/file.html", methods[i]);
        EXPECT_NE(handler, nullptr) << "Static file should be found for method: " << methods[i];
    }

    uvhttp_router_free(router);
}

/* Test: Array mode to Trie mode migration preserves static_prefix */
TEST(RouterStaticPrefixModeTest, ModeMigrationPreservesStaticPrefix) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Start in Array mode
    for (int i = 0; i < 50; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, dummy_handler);
    }
    EXPECT_EQ(router->use_trie, 0);

    // Set static prefix in Array mode
    uvhttp_router_set_static_prefix(router, "/static", "./public");

    // Verify static_prefix works in Array mode
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/static/file.html", "GET");
    EXPECT_NE(handler, nullptr) << "static_prefix should work in Array mode";

    // Add more routes to trigger migration to Trie mode
    for (int i = 50; i < 150; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, dummy_handler);
    }
    EXPECT_EQ(router->use_trie, 1);

    // Verify static_prefix still works after migration
    handler = uvhttp_router_find_handler(router, "/static/file.html", "GET");
    EXPECT_NE(handler, nullptr) << "static_prefix should work after migration to Trie mode";

    uvhttp_router_free(router);
}

/* Test: NULL and empty static_prefix handling */
TEST(RouterStaticPrefixModeTest, NullAndEmptyStaticPrefix) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add routes to stay in Array mode
    uvhttp_router_add_route(router, "/api", dummy_handler);

    // Don't set static prefix (NULL by default)
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/static/file.html", "GET");
    EXPECT_EQ(handler, nullptr) << "Should return NULL when static_prefix is not set";

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/static", "./public");
    handler = uvhttp_router_find_handler(router, "/static/file.html", "GET");
    EXPECT_NE(handler, nullptr) << "Should find static file when prefix is set";

    uvhttp_router_free(router);
}

/* Test: fallback_context when no routes match */
TEST(RouterStaticPrefixModeTest, FallbackContextBehavior) {
    uvhttp_router_t* router = NULL;
    ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
    ASSERT_NE(router, nullptr);

    // Add routes to stay in Array mode
    uvhttp_router_add_route(router, "/api", dummy_handler);

    // Set fallback context
    uvhttp_router_set_fallback_static(router, "./fallback");

    // Test non-matching path returns fallback handler
    uvhttp_request_handler_t handler = uvhttp_router_find_handler(router, "/unknown/path", "GET");
    EXPECT_NE(handler, nullptr) << "Should return fallback handler for non-matching paths";

    uvhttp_router_free(router);
}