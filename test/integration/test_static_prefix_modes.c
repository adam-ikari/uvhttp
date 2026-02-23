/* End-to-end test for static file serving in Array and Trie router modes
 *
 * This test verifies that static files are served correctly regardless of
 * whether the router is in Array mode or Trie mode.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <uv.h>
#include "uvhttp.h"

#define TEST_PORT 18888
#define TEST_HOST "127.0.0.1"

static uv_loop_t* loop;
static uvhttp_server_t* server;
static uvhttp_router_t* router;

/* Simple handler for API routes */
static void api_handler(uvhttp_request_t* req, uvhttp_response_t* res) {
    uvhttp_response_set_status(res, 200);
    uvhttp_response_set_header(res, "Content-Type", "application/json");
    uvhttp_response_set_body(res, "{\"status\":\"ok\"}");
    uvhttp_response_send(res);
}

/* Test helper: create test file */
static void create_test_file(const char* path, const char* content) {
    FILE* f = fopen(path, "w");
    if (f) {
        fwrite(content, 1, strlen(content), f);
        fclose(f);
        printf("Created test file: %s\n", path);
    }
}

/* Test helper: remove test files */
static void cleanup_test_files() {
    system("rm -rf /tmp/uvhttp_test_static");
}

/* Test helper: make HTTP GET request */
static int make_http_request(const char* path, char* response, size_t response_size) {
    uv_tcp_t tcp;
    uv_connect_t connect_req;
    int result = 0;

    uv_tcp_init(loop, &tcp);

    struct sockaddr_in addr;
    uv_ip4_addr(TEST_HOST, TEST_PORT, &addr);

    struct request_data {
        char response[4096];
        size_t response_len;
        uv_buf_t write_buf;
        int done;
    } data = {0};

    tcp.data = &data;

    uv_connect(&connect_req, (uv_stream_t*)&tcp, (struct sockaddr*)&addr,
               [](uv_connect_t* req, int status) {
                   if (status < 0) {
                       printf("Connection failed: %s\n", uv_strerror(status));
                       uv_close((uv_stream_t*)req->handle, NULL);
                       return;
                   }

                   struct request_data* data = (struct request_data*)req->handle->data;
                   char request[256];
                   int len = snprintf(request, sizeof(request),
                                     "GET /%s HTTP/1.1\r\n"
                                     "Host: localhost\r\n"
                                     "Connection: close\r\n\r\n",
                                     data->write_buf.base);
                   data->write_buf.len = len;

                   uv_write_t write_req;
                   uv_write(&write_req, req->handle, &data->write_buf, 1,
                            [](uv_write_t* req, int status) {
                                if (status < 0) {
                                    uv_close((uv_stream_t*)req->handle, NULL);
                                    return;
                                }
                                uv_read_start(req->handle,
                                             [](uv_handle_t* handle, size_t suggested_size, uv_buf_t* buf) {
                                                 static char buffer[4096];
                                                 buf->base = buffer;
                                                 buf->len = sizeof(buffer);
                                             },
                                             [](uv_stream_t* stream, ssize_t nread, const uv_buf_t* buf) {
                                                 struct request_data* data = (struct request_data*)stream->data;
                                                 if (nread > 0) {
                                                     size_t copy_len = nread < sizeof(data->response) - data->response_len
                                                                         ? nread
                                                                         : sizeof(data->response) - data->response_len;
                                                     memcpy(data->response + data->response_len, buf->base, copy_len);
                                                     data->response_len += copy_len;
                                                 }
                                                 if (nread <= 0 || data->response_len > 0) {
                                                     uv_close(stream, NULL);
                                                 }
                                             });
                            });
               }, &connect_req);

    data.write_buf.base = (char*)path;
    uv_run(loop, UV_RUN_DEFAULT);

    if (data.response_len > 0 && response_size > 0) {
        size_t copy_len = data.response_len < response_size - 1 ? data.response_len : response_size - 1;
        memcpy(response, data.response, copy_len);
        response[copy_len] = '\0';
        result = 1;
    }

    return result;
}

/* Test: Array mode static file serving */
static void test_array_mode_static_files() {
    printf("\n=== Test 1: Array mode static file serving ===\n");

    // Create router in Array mode (add < 100 routes)
    router = uvhttp_router_new();
    server = uvhttp_server_new(loop);
    server->router = router;

    // Add API routes (stay in Array mode)
    uvhttp_router_add_route(router, "/api", api_handler);
    uvhttp_router_add_route(router, "/api/users", api_handler);
    uvhttp_router_add_route(router, "/health", api_handler);

    printf("Router mode: %s\n", router->use_trie ? "Trie" : "Array");
    printf("Route count: %zu\n", router->array_route_count + router->route_count);

    // Create test directory and files
    system("mkdir -p /tmp/uvhttp_test_static");
    create_test_file("/tmp/uvhttp_test_static/index.html",
                     "<html><body>Array Mode Test</body></html>");
    create_test_file("/tmp/uvhttp_test_static/test.txt", "Hello from Array mode!");

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/static", "/tmp/uvhttp_test_static");

    // Start server
    if (uvhttp_server_listen(server, TEST_HOST, TEST_PORT) != UVHTTP_OK) {
        printf("Failed to start server\n");
        return;
    }

    printf("Server started on port %d\n", TEST_PORT);

    // Test static file request
    char response[4096];
    if (make_http_request("static/index.html", response, sizeof(response))) {
        printf("Response:\n%.*s\n", (int)200, response);
        if (strstr(response, "200 OK") && strstr(response, "Array Mode Test")) {
            printf("✅ PASS: Static file served correctly in Array mode\n");
        } else {
            printf("❌ FAIL: Static file not served correctly in Array mode\n");
        }
    } else {
        printf("❌ FAIL: No response received\n");
    }

    // Test API route still works
    if (make_http_request("api", response, sizeof(response))) {
        if (strstr(response, "200 OK") && strstr(response, "{\"status\":\"ok\"}")) {
            printf("✅ PASS: API route works in Array mode\n");
        } else {
            printf("❌ FAIL: API route not working\n");
        }
    }

    // Cleanup
    uvhttp_server_free(server);
    uvhttp_router_free(router);
}

/* Test: Trie mode static file serving */
static void test_trie_mode_static_files() {
    printf("\n=== Test 2: Trie mode static file serving ===\n");

    // Create router in Trie mode (add >= 100 routes)
    router = uvhttp_router_new();
    server = uvhttp_server_new(loop);
    server->router = router;

    // Add many routes to trigger Trie mode
    for (int i = 0; i < 150; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, api_handler);
    }

    printf("Router mode: %s\n", router->use_trie ? "Trie" : "Array");
    printf("Route count: %zu\n", router->array_route_count + router->route_count);

    // Create test directory and files
    system("mkdir -p /tmp/uvhttp_test_static");
    create_test_file("/tmp/uvhttp_test_static/index.html",
                     "<html><body>Trie Mode Test</body></html>");
    create_test_file("/tmp/uvhttp_test_static/data.json", "{\"mode\":\"trie\"}");

    // Set static prefix
    uvhttp_router_set_static_prefix(router, "/assets", "/tmp/uvhttp_test_static");

    // Start server
    if (uvhttp_server_listen(server, TEST_HOST, TEST_PORT) != UVHTTP_OK) {
        printf("Failed to start server\n");
        return;
    }

    printf("Server started on port %d\n", TEST_PORT);

    // Test static file request
    char response[4096];
    if (make_http_request("assets/index.html", response, sizeof(response))) {
        printf("Response:\n%.*s\n", (int)200, response);
        if (strstr(response, "200 OK") && strstr(response, "Trie Mode Test")) {
            printf("✅ PASS: Static file served correctly in Trie mode\n");
        } else {
            printf("❌ FAIL: Static file not served correctly in Trie mode\n");
        }
    } else {
        printf("❌ FAIL: No response received\n");
    }

    // Test API route still works
    if (make_http_request("api/route0", response, sizeof(response))) {
        if (strstr(response, "200 OK") && strstr(response, "{\"status\":\"ok\"}")) {
            printf("✅ PASS: API route works in Trie mode\n");
        } else {
            printf("❌ FAIL: API route not working\n");
        }
    }

    // Cleanup
    uvhttp_server_free(server);
    uvhttp_router_free(router);
}

/* Test: Mode migration with static files */
static void test_mode_migration() {
    printf("\n=== Test 3: Mode migration with static files ===\n");

    router = uvhttp_router_new();
    server = uvhttp_server_new(loop);
    server->router = router;

    // Start in Array mode
    for (int i = 0; i < 50; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, api_handler);
    }

    printf("Initial router mode: %s\n", router->use_trie ? "Trie" : "Array");

    // Set static prefix
    system("mkdir -p /tmp/uvhttp_test_static");
    create_test_file("/tmp/uvhttp_test_static/test.html",
                     "<html><body>Migration Test</body></html>");
    uvhttp_router_set_static_prefix(router, "/static", "/tmp/uvhttp_test_static");

    // Start server
    if (uvhttp_server_listen(server, TEST_HOST, TEST_PORT) != UVHTTP_OK) {
        printf("Failed to start server\n");
        return;
    }

    // Test static file in Array mode
    char response[4096];
    if (make_http_request("static/test.html", response, sizeof(response))) {
        if (strstr(response, "200 OK") && strstr(response, "Migration Test")) {
            printf("✅ PASS: Static file works in Array mode\n");
        } else {
            printf("❌ FAIL: Static file not working in Array mode\n");
        }
    }

    // Stop server and migrate to Trie mode
    uvhttp_server_free(server);

    // Add more routes to trigger migration
    for (int i = 50; i < 150; i++) {
        char route[64];
        snprintf(route, sizeof(route), "/api/route%d", i);
        uvhttp_router_add_route(router, route, api_handler);
    }

    printf("After migration router mode: %s\n", router->use_trie ? "Trie" : "Array");

    // Restart server
    server = uvhttp_server_new(loop);
    server->router = router;

    if (uvhttp_server_listen(server, TEST_HOST, TEST_PORT) != UVHTTP_OK) {
        printf("Failed to restart server\n");
        return;
    }

    // Test static file still works after migration
    if (make_http_request("static/test.html", response, sizeof(response))) {
        if (strstr(response, "200 OK") && strstr(response, "Migration Test")) {
            printf("✅ PASS: Static file works after migration to Trie mode\n");
        } else {
            printf("❌ FAIL: Static file not working after migration\n");
        }
    }

    // Cleanup
    uvhttp_server_free(server);
    uvhttp_router_free(router);
}

int main() {
    printf("=== Static File Serving Mode Tests ===\n");

    loop = uv_default_loop();

    // Run tests
    test_array_mode_static_files();
    test_trie_mode_static_files();
    test_mode_migration();

    // Cleanup
    cleanup_test_files();

    printf("\n=== All tests completed ===\n");
    return 0;
}
