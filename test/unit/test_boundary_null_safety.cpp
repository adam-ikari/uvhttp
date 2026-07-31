/*
 * Boundary and null pointer safety tests.
 * Exercises edge cases: empty inputs, oversized inputs, integer overflow,
 * zero-length buffers, and NULL parameter paths.
 */

#include <gtest/gtest.h>
#include "uvhttp_server.h"
#include "uvhttp_connection.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_config.h"
#include "uvhttp_context.h"
#include "uvhttp_error.h"
#include "uvhttp_allocator.h"
#include <string.h>
#include <limits.h>

/* ========== Null-pointer safety for public APIs ========== */

TEST(UvhttpNullSafetyTest, ServerNewNull) {
    uvhttp_server_t* srv = NULL;
    uvhttp_error_t err = uvhttp_server_new(NULL, &srv);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpNullSafetyTest, ConnectionNewNullServer) {
    uvhttp_connection_t* conn = NULL;
    uvhttp_error_t err = uvhttp_connection_new(NULL, &conn);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpNullSafetyTest, ConnectionNewNullOut) {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_server_t* srv = NULL;
    uvhttp_server_new(loop, &srv);
    uvhttp_error_t err = uvhttp_connection_new(srv, NULL);
    EXPECT_NE(err, UVHTTP_OK);
    uvhttp_server_free(srv);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

TEST(UvhttpNullSafetyTest, ConnectionFreeNull) {
    uvhttp_connection_free(NULL);
}

TEST(UvhttpNullSafetyTest, ServerFreeNull) {
    uvhttp_server_free(NULL);
}

TEST(UvhttpNullSafetyTest, RouterFreeNull) {
    uvhttp_router_free(NULL);
}

TEST(UvhttpNullSafetyTest, ConfigFreeNull) {
    uvhttp_config_free(NULL);
}

/* ========== Router boundary cases ========== */

TEST(UvhttpNullSafetyTest, RouterAddRouteNullRouter) {
    uvhttp_error_t err = uvhttp_router_add_route(NULL, "/test", NULL);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpNullSafetyTest, RouterAddRouteNullPath) {
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_error_t err = uvhttp_router_add_route(router, NULL, NULL);
    EXPECT_NE(err, UVHTTP_OK);
    uvhttp_router_free(router);
}

TEST(UvhttpNullSafetyTest, RouterAddRouteNullHandler) {
    uvhttp_router_t* router = NULL;
    uvhttp_router_new(&router);
    uvhttp_error_t err = uvhttp_router_add_route(router, "/test", NULL);
    EXPECT_NE(err, UVHTTP_OK);
    uvhttp_router_free(router);
}

/* ========== Response boundary cases ========== */

TEST(UvhttpResponseBoundaryTest, SetHeaderNullResponse) {
    uvhttp_error_t err = uvhttp_response_set_header(NULL, "X-Test", "value");
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpResponseBoundaryTest, SetHeaderNullName) {
    uvhttp_response_t resp;
    memset(&resp, 0, sizeof(resp));
    uvhttp_error_t err = uvhttp_response_set_header(&resp, NULL, "value");
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpResponseBoundaryTest, SetHeaderNullValue) {
    uvhttp_response_t resp;
    memset(&resp, 0, sizeof(resp));
    uvhttp_error_t err = uvhttp_response_set_header(&resp, "X-Test", NULL);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpResponseBoundaryTest, SetStatusNullResponse) {
    uvhttp_error_t err = uvhttp_response_set_status(NULL, 200);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpResponseBoundaryTest, SetBodyNullResponse) {
    uvhttp_error_t err = uvhttp_response_set_body(NULL, "test", 4);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpResponseBoundaryTest, SetBodyNullData) {
    uvhttp_response_t resp;
    memset(&resp, 0, sizeof(resp));
    uvhttp_error_t err = uvhttp_response_set_body(&resp, NULL, 4);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpResponseBoundaryTest, SetBodyZeroLength) {
    uvhttp_response_t resp;
    memset(&resp, 0, sizeof(resp));
    uvhttp_error_t err = uvhttp_response_set_body(&resp, "", 0);
    /* Zero-length body is valid (used for SSE) */
}

TEST(UvhttpResponseBoundaryTest, SetBodyLargeSize) {
    uvhttp_response_t resp;
    memset(&resp, 0, sizeof(resp));
    const char* data = "test";
    uvhttp_error_t err = uvhttp_response_set_body(&resp, data, 4);
    /* Clean up to prevent leak */
    if (err == UVHTTP_OK) {
        uvhttp_response_cleanup(&resp);
    }
}

/* ========== Config boundary cases ========== */

TEST(UvhttpConfigBoundaryTest, ConfigNewNull) {
    uvhttp_error_t err = uvhttp_config_new(NULL);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConfigBoundaryTest, ConfigSetDefaultsNull) {
    uvhttp_config_set_defaults(NULL);
}

/* ========== Request API null safety ========== */

TEST(UvhttpRequestNullTest, GetMethodNull) {
    const char* result = uvhttp_request_get_method(NULL);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpRequestNullTest, GetUrlNull) {
    const char* result = uvhttp_request_get_url(NULL);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpRequestNullTest, GetPathNull) {
    const char* result = uvhttp_request_get_path(NULL);
    EXPECT_EQ(result, nullptr);
}

TEST(UvhttpRequestNullTest, GetHeaderNull) {
    const char* result = uvhttp_request_get_header(NULL, "Host");
    EXPECT_EQ(result, nullptr);
}

/* ========== Connection API boundary ========== */

TEST(UvhttpConnectionBoundaryTest, CloseNull) {
    uvhttp_connection_close(NULL);
}

TEST(UvhttpConnectionBoundaryTest, StartNull) {
    uvhttp_error_t err = uvhttp_connection_start(NULL);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpConnectionBoundaryTest, StartTimeoutNull) {
    uvhttp_error_t err = uvhttp_connection_start_timeout(NULL);
    EXPECT_NE(err, UVHTTP_OK);
}

/* ========== Server listen boundary ========== */

TEST(UvhttpServerBoundaryTest, ListenNullServer) {
    uvhttp_error_t err = uvhttp_server_listen(NULL, "127.0.0.1", 8080);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpServerBoundaryTest, ListenNullHost) {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_server_t* srv = NULL;
    uvhttp_server_new(loop, &srv);
    uvhttp_error_t err = uvhttp_server_listen(srv, NULL, 8080);
    EXPECT_NE(err, UVHTTP_OK);
    uvhttp_server_free(srv);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

TEST(UvhttpServerBoundaryTest, ListenPortZero) {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_server_t* srv = NULL;
    uvhttp_server_new(loop, &srv);
    /* Port 0 should auto-assign, should succeed */
    uvhttp_error_t err = uvhttp_server_listen(srv, "127.0.0.1", 0);
    EXPECT_EQ(err, UVHTTP_OK);
    uvhttp_server_free(srv);
    uv_run(loop, UV_RUN_NOWAIT);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

/* ========== Context boundary ========== */

TEST(UvhttpContextBoundaryTest, CreateNullLoop) {
    uvhttp_context_t* ctx = NULL;
    uvhttp_error_t err = uvhttp_context_create(NULL, &ctx);
    EXPECT_NE(err, UVHTTP_OK);
}

TEST(UvhttpContextBoundaryTest, CreateNullOut) {
    uv_loop_t* loop = uv_loop_new();
    uvhttp_error_t err = uvhttp_context_create(loop, NULL);
    EXPECT_NE(err, UVHTTP_OK);
    uv_loop_close(loop);
    uvhttp_free(loop);
}

TEST(UvhttpContextBoundaryTest, DestroyNull) {
    uvhttp_context_destroy(NULL);
}