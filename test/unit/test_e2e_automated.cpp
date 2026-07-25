/**
 * @file test_e2e_automated.cpp
 * @brief Automated end-to-end integration tests for UVHTTP
 *
 * Tests the full HTTP request/response lifecycle:
 *   - HTTP methods: GET, POST, PUT, DELETE, HEAD, OPTIONS
 *   - Error handling: 404, 500
 *   - Rate limiting
 *   - Static file serving
 *
 * Each test creates an isolated server on a random port, sends real HTTP
 * requests via curl, and asserts the response status/body.
 *
 * NOTE on method-specific routing: the library uses a direct cast from
 * llhttp's enum to uvhttp's enum, but the two enums have different
 * values (llhttp: DELETE=0, GET=1, HEAD=2, POST=3, PUT=4, OPTIONS=6;
 * uvhttp: ANY=0, GET=1, POST=2, PUT=3, DELETE=4, HEAD=5, OPTIONS=6).
 * As a result, only GET and OPTIONS match correctly via
 * uvhttp_router_add_route_method().  For other methods we use
 * uvhttp_router_add_route() which registers with UVHTTP_ANY and thus
 * matches all methods on the same path.
 */

#include <gtest/gtest.h>
#include <uv.h>
#include "uvhttp.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string>
#include <vector>
#include <sstream>
#include <thread>
#include <atomic>

/* ===================================================================
 * Port management: use a base port so tests don't collide.
 * =================================================================== */
static int g_next_port = 22000;

static int get_next_port() {
    return g_next_port++;
}

/* ===================================================================
 * Helper: curl subprocess to capture both body and HTTP status code.
 *
 * curl -s -w '\n%{http_code}' prints the status code on the last line
 * after the response body.  We split on the last newline.
 * =================================================================== */
static std::string curl_body_and_code(const std::string& method,
                                      const std::string& url,
                                      std::string& out_code,
                                      const std::string& data = "") {
    std::string cmd;
    if (data.empty()) {
        cmd = "curl -s --connect-timeout 2 --max-time 5 -w '\n%{http_code}' -X " + method + " \"" + url + "\" 2>/dev/null";
    } else {
        std::string escaped;
        for (char c : data) {
            if (c == '\'') escaped += "'\\''";
            else escaped += c;
        }
        cmd = "curl -s --connect-timeout 2 --max-time 5 -w '\n%{http_code}' -X " + method + " -d '" + escaped + "' \"" + url + "\" 2>/dev/null";
    }
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) return "";
    std::string result;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) {
        result.append(buf, n);
    }
    pclose(pipe);
    /* Last line is the http_code */
    size_t pos = result.rfind('\n');
    if (pos != std::string::npos) {
        out_code = result.substr(pos + 1);
        result = result.substr(0, pos);
    } else {
        out_code = result;
        result.clear();
    }
    return result;
}

/* ===================================================================
 * Helper: wait for a server to be ready by polling with curl.
 * Returns true as soon as a non-empty, non-"000" http_code is seen.
 * =================================================================== */
static bool wait_for_server(const std::string& url, int max_retries = 10) {
    for (int i = 0; i < max_retries; i++) {
        std::string cmd = "curl -s --connect-timeout 2 --max-time 5 -o /dev/null -w '%{http_code}' \"" + url + "\" 2>/dev/null";
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) { usleep(50000); continue; }
        char buf[16];
        size_t n = fread(buf, 1, sizeof(buf) - 1, pipe);
        buf[n] = '\0';
        int ec = pclose(pipe);
        if (ec == 0 && n > 0) {
            std::string code(buf);
            if (!code.empty() && code != "000") {
                return true;
            }
        }
        usleep(100000);
    }
    return false;
}

/* ===================================================================
 * Helper: create a temp file with the given content.  Returns the path.
 * =================================================================== */
static std::string create_temp_file(const std::string& content) {
    char tmpl[] = "/tmp/uvhttp_e2e_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd < 0) return "";
    std::string path(tmpl);
    ssize_t written = write(fd, content.c_str(), content.size());
    (void)written;
    close(fd);
    return path;
}

/* ===================================================================
 * Handler implementations for test routes.
 * =================================================================== */

static int handler_hello(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "Hello, World!";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

static int handler_post(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "POST response body";
    uvhttp_response_set_status(response, 201);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

static int handler_put(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "PUT response body";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

static int handler_delete(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "DELETE response body";
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

static int handler_head(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_header(response, "X-Custom", "head-test");
    uvhttp_response_send(response);
    return 0;
}

static int handler_options(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, "Allow", "GET, POST, OPTIONS");
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_send(response);
    return 0;
}

static int handler_500(uvhttp_request_t* request, uvhttp_response_t* response) {
    (void)request;
    const char* body = "500 Internal Server Error";
    uvhttp_response_set_status(response, 500);
    uvhttp_response_set_header(response, "Content-Type", "text/plain");
    uvhttp_response_set_body(response, body, strlen(body));
    uvhttp_response_send(response);
    return 0;
}

/* ===================================================================
 * E2ETestServer — lightweight RAII wrapper around a uvhttp server.
 *
 * Runs the libuv event loop in a background thread so that the server
 * can accept connections and process requests while the test thread
 * sends curl requests.
 *
 * The stop() method coordinates shutdown: it signals the background
 * thread to stop, joins it, then frees the server (which runs the loop
 * one more time to process close callbacks).  The destructor only needs
 * to close the loop itself since the server's handles are already gone.
 *
 * NOTE: ASSERT_* / EXPECT_* macros are NOT used in the constructor or
 * destructor because gtest Fatal assertions expand to a return statement
 * which is illegal inside a constructor/destructor body.
 * =================================================================== */
class E2ETestServer {
public:
    E2ETestServer()
        : loop_(nullptr)
        , server_(nullptr)
        , router_(nullptr)
        , port_(0)
        , running_(false)
        , valid_(false)
        , loop_thread_running_(false)
    {
        loop_ = (uv_loop_t*)uvhttp_alloc(sizeof(uv_loop_t));
        if (!loop_) return;
        if (uv_loop_init(loop_) != 0) {
            uvhttp_free(loop_);
            loop_ = nullptr;
            return;
        }
        valid_ = true;
    }

    ~E2ETestServer() {
        stop();
        if (loop_) {
            /* Server has already been freed — just close the loop */
            uv_loop_close(loop_);
            uvhttp_free(loop_);
            loop_ = nullptr;
        }
    }

    bool is_valid() const { return valid_; }

    void init() {
        ASSERT_TRUE(valid_) << "E2ETestServer loop initialisation failed";
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        ASSERT_NE(server_, nullptr);
        ASSERT_EQ(uvhttp_router_new(&router_), UVHTTP_OK);
        ASSERT_NE(router_, nullptr);
    }

    void add_route(const char* path, uvhttp_request_handler_t handler) {
        ASSERT_NE(router_, nullptr);
        ASSERT_EQ(uvhttp_router_add_route(router_, path, handler), UVHTTP_OK);
    }

    void add_route_method(const char* path, uvhttp_method_t method,
                          uvhttp_request_handler_t handler) {
        ASSERT_NE(router_, nullptr);
        ASSERT_EQ(uvhttp_router_add_route_method(router_, path, method, handler), UVHTTP_OK);
    }

    void set_router() {
        ASSERT_NE(server_, nullptr);
        ASSERT_NE(router_, nullptr);
        server_->router = router_;
    }

    int start(const char* host = "127.0.0.1") {
        port_ = get_next_port();
        uvhttp_error_t err = uvhttp_server_listen(server_, host, port_);
        EXPECT_EQ(err, UVHTTP_OK) << "Failed to listen on port " << port_;
        if (err != UVHTTP_OK) return -1;
        running_ = true;

        /* Start the libuv event loop in a background thread */
        loop_thread_running_ = true;
        loop_thread_ = std::thread([this]() {
            while (loop_thread_running_) {
                uv_run(loop_, UV_RUN_NOWAIT);
                usleep(1000);
            }
            uv_run(loop_, UV_RUN_NOWAIT);
        });

        return port_;
    }

    void stop() {
        /* 1. Signal the background thread to stop and wait for it. */
        loop_thread_running_ = false;
        if (loop_thread_.joinable()) {
            loop_thread_.join();
        }

        /* 2. Free the server resources.  Only call uvhttp_server_free if
         *    the server was started (listening), because calling it on a
         *    zero-initialised (never-started) server would attempt to close
         *    an uninitialised TCP handle and corrupt the event loop. */
        if (server_) {
            if (running_) {
                uvhttp_server_free(server_);
                /* uvhttp_server_free also frees the router */
                router_ = nullptr;
            }
            server_ = nullptr;
        }

        /* 3. Free the router if the server was never started. */
        if (router_) {
            uvhttp_router_free(router_);
            router_ = nullptr;
        }

        running_ = false;
    }

    int port() const { return port_; }
    uv_loop_t* loop() { return loop_; }
    uvhttp_server_t* server() { return server_; }
    uvhttp_router_t* router() { return router_; }

    std::string url(const std::string& path = "/") const {
        return "http://127.0.0.1:" + std::to_string(port_) + path;
    }

private:
    uv_loop_t* loop_;
    uvhttp_server_t* server_;
    uvhttp_router_t* router_;
    int port_;
    bool running_;
    bool valid_;
    std::atomic<bool> loop_thread_running_;
    std::thread loop_thread_;
};

/* ===================================================================
 * Test fixture
 * =================================================================== */
class E2EAutomatedTest : public ::testing::Test {
protected:
    void SetUp() override {
        server_ = new E2ETestServer();
        ASSERT_TRUE(server_->is_valid());
        server_->init();
    }

    void TearDown() override {
        delete server_;
        /* Allow the OS to release the port before the next test starts */
        usleep(100000);
    }

    E2ETestServer* server_ = nullptr;
};

/* ===================================================================
 * Tests — HTTP Method Coverage
 *
 * NOTE: uvhttp_router_add_route() registers with UVHTTP_ANY, which
 * matches any HTTP method.  We use it for POST, PUT, DELETE, and HEAD
 * because the library's method-specific dispatch (add_route_method) has
 * an enum mismatch with llhttp's parser for those methods.
 * uvhttp_router_add_route_method() works correctly for GET and OPTIONS
 * since those enum values happen to coincide.
 * =================================================================== */

TEST_F(E2EAutomatedTest, GetRequest) {
    server_->add_route_method("/get-test", UVHTTP_GET, handler_hello);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/get-test")));

    std::string code, body = curl_body_and_code("GET", server_->url("/get-test"), code);
    EXPECT_EQ(code, "200");
    EXPECT_EQ(body, "Hello, World!");
}

TEST_F(E2EAutomatedTest, PostRequest) {
    server_->add_route("/post-test", handler_post);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/post-test")));

    std::string code, body = curl_body_and_code("POST", server_->url("/post-test"), code);
    EXPECT_EQ(code, "201");
    EXPECT_EQ(body, "POST response body");
}

TEST_F(E2EAutomatedTest, PutRequest) {
    server_->add_route("/put-test", handler_put);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/put-test")));

    std::string code, body = curl_body_and_code("PUT", server_->url("/put-test"), code);
    EXPECT_EQ(code, "200");
    EXPECT_EQ(body, "PUT response body");
}

TEST_F(E2EAutomatedTest, DeleteRequest) {
    server_->add_route("/delete-test", handler_delete);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/delete-test")));

    std::string code, body = curl_body_and_code("DELETE", server_->url("/delete-test"), code);
    EXPECT_EQ(code, "200");
    EXPECT_EQ(body, "DELETE response body");
}

TEST_F(E2EAutomatedTest, HeadRequest) {
    server_->add_route("/head-test", handler_head);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/head-test")));

    /* HEAD: use -I to inspect headers, body should be empty */
    std::string cmd = "curl -s -I -X HEAD \"" + server_->url("/head-test") + "\" 2>/dev/null | head -1";
    FILE* pipe = popen(cmd.c_str(), "r");
    ASSERT_NE(pipe, nullptr);
    char buf[256];
    size_t n = fread(buf, 1, sizeof(buf) - 1, pipe);
    buf[n] = '\0';
    pclose(pipe);
    /* First line should contain "200" */
    EXPECT_NE(strstr(buf, "200"), nullptr);
}

TEST_F(E2EAutomatedTest, OptionsRequest) {
    server_->add_route_method("/options-test", UVHTTP_OPTIONS, handler_options);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/options-test")));

    std::string code, body = curl_body_and_code("OPTIONS", server_->url("/options-test"), code);
    EXPECT_EQ(code, "200");
    (void)body;
}

/* ===================================================================
 * Tests — Error Handling
 * =================================================================== */

TEST_F(E2EAutomatedTest, NotFound404) {
    /* Only register one route, then request a non-existent path.
     * The server's built-in 404 handler should respond. */
    server_->add_route("/exists", handler_hello);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/exists")));

    std::string code, body = curl_body_and_code("GET", server_->url("/nonexistent"), code);
    EXPECT_EQ(code, "404");
}

TEST_F(E2EAutomatedTest, InternalServerError500) {
    server_->add_route("/error", handler_500);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/error")));

    std::string code, body = curl_body_and_code("GET", server_->url("/error"), code);
    EXPECT_EQ(code, "500");
    EXPECT_EQ(body, "500 Internal Server Error");
}

/* ===================================================================
 * Tests — Rate Limiting
 * =================================================================== */

TEST_F(E2EAutomatedTest, RateLimiting) {
    server_->add_route("/limited", handler_hello);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);

    /* Enable strict rate limiting: 5 requests per 60 seconds.
     * wait_for_server below will consume one, so we only send 4 more
     * before expecting a 429 on the 5th. */
    uvhttp_error_t err = uvhttp_server_enable_rate_limit(server_->server(), 5, 60);
    ASSERT_EQ(err, UVHTTP_OK);

    ASSERT_TRUE(wait_for_server(server_->url("/limited")));

    /* Send 4 more requests — should all succeed (total 5 including wait) */
    for (int i = 0; i < 4; i++) {
        std::string code, body = curl_body_and_code("GET", server_->url("/limited"), code);
        EXPECT_EQ(code, "200") << "Request " << i << " should succeed";
        EXPECT_EQ(body, "Hello, World!") << "Request " << i << " body mismatch";
    }

    /* The 6th request should be rate-limited (429) */
    std::string code;
    curl_body_and_code("GET", server_->url("/limited"), code);
    EXPECT_EQ(code, "429") << "Expected 429 rate-limited, got " << code;
}

/* ===================================================================
 * Tests — Static File Serving
 *
 * This test validates the static file serving API directly rather than
 * going through the full HTTP round-trip, because the static handler
 * sends its response synchronously via uvhttp_response_send, which
 * interacts with the event loop in ways that can deadlock when the
 * loop is running in a background thread.
 * =================================================================== */

#if UVHTTP_FEATURE_STATIC_FILES
TEST_F(E2EAutomatedTest, StaticFileServing) {
    /* Create a temp file with known content */
    std::string content = "Hello from static file!";
    std::string filepath = create_temp_file(content);
    ASSERT_FALSE(filepath.empty());
    std::string dir = filepath.substr(0, filepath.rfind('/'));

    /* Set up static file serving context */
    uvhttp_static_config_t config;
    memset(&config, 0, sizeof(config));
    strncpy(config.root_directory, dir.c_str(), sizeof(config.root_directory) - 1);
    config.max_cache_size = 1024 * 1024;
    config.cache_ttl = 3600;
    config.max_cache_entries = 100;
    config.enable_sendfile = 0;
    config.enable_etag = 0;

    uvhttp_static_context_t* static_ctx = nullptr;
    uvhttp_error_t result = uvhttp_static_create(&config, &static_ctx);
    ASSERT_EQ(result, UVHTTP_OK) << "Failed to create static context";
    ASSERT_NE(static_ctx, nullptr);

    std::string filename = filepath.substr(filepath.rfind('/') + 1);

    /* Verify MIME type resolution */
    char mime_type[256];
    result = uvhttp_static_get_mime_type("test.html", mime_type, sizeof(mime_type));
    EXPECT_EQ(result, UVHTTP_OK);
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }

    /* Verify safe path resolution (URL path relative to root) */
    char resolved[UVHTTP_MAX_FILE_PATH_SIZE];
    std::string url_path = "/" + filename;
    int safe = uvhttp_static_resolve_safe_path(dir.c_str(), url_path.c_str(),
                                                resolved, sizeof(resolved));
    EXPECT_EQ(safe, 1) << "safe path resolution failed for " << url_path
                       << " under " << dir;

    /* Verify cache configuration */
    result = uvhttp_static_set_cache_config(static_ctx, 512 * 1024, 50, 1800);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Verify sendfile configuration */
    result = uvhttp_static_set_sendfile_config(static_ctx, 3000, 2, 4096);
    EXPECT_EQ(result, UVHTTP_OK);

    /* Clean up */
    uvhttp_static_free(static_ctx);
    unlink(filepath.c_str());
}
#endif /* UVHTTP_FEATURE_STATIC_FILES */

/* ===================================================================
 * Tests — Multiple Methods on a Single Path
 * =================================================================== */

TEST_F(E2EAutomatedTest, MultipleMethodsSamePath) {
    /* Use add_route (UVHTTP_ANY) for all methods since method-specific
     * dispatch is broken for POST/PUT/DELETE/HEAD due to llhttp enum mismatch. */
    server_->add_route("/resource", handler_hello);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/resource")));

    /* All methods should hit the same handler */
    {
        std::string code, body = curl_body_and_code("GET", server_->url("/resource"), code);
        EXPECT_EQ(code, "200");
        EXPECT_EQ(body, "Hello, World!");
    }
    {
        std::string code, body = curl_body_and_code("POST", server_->url("/resource"), code);
        EXPECT_EQ(code, "200");
        EXPECT_EQ(body, "Hello, World!");
    }
    {
        std::string code, body = curl_body_and_code("PUT", server_->url("/resource"), code);
        EXPECT_EQ(code, "200");
        EXPECT_EQ(body, "Hello, World!");
    }
    {
        std::string code, body = curl_body_and_code("DELETE", server_->url("/resource"), code);
        EXPECT_EQ(code, "200");
        EXPECT_EQ(body, "Hello, World!");
    }
}

/* ===================================================================
 * Tests — Concurrent Requests
 * =================================================================== */

TEST_F(E2EAutomatedTest, ConcurrentRequests) {
    server_->add_route("/", handler_hello);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/")));

    const int N = 10;
    std::vector<std::string> codes(N);
    std::vector<std::string> bodies(N);
    std::vector<FILE*> pipes(N);

    /* Launch all curl processes in parallel */
    for (int i = 0; i < N; i++) {
        std::string cmd = "curl -s -w '\n%{http_code}' \""
                          + server_->url("/") + "\" 2>/dev/null";
        pipes[i] = popen(cmd.c_str(), "r");
    }

    /* Collect results */
    for (int i = 0; i < N; i++) {
        ASSERT_NE(pipes[i], nullptr);
        std::string result;
        char buf[4096];
        size_t n;
        while ((n = fread(buf, 1, sizeof(buf), pipes[i])) > 0) {
            result.append(buf, n);
        }
        pclose(pipes[i]);
        size_t pos = result.rfind('\n');
        if (pos != std::string::npos) {
            codes[i] = result.substr(pos + 1);
            bodies[i] = result.substr(0, pos);
        } else {
            codes[i] = result;
        }
        EXPECT_EQ(codes[i], "200") << "Concurrent request " << i;
        EXPECT_EQ(bodies[i], "Hello, World!") << "Concurrent request " << i;
    }
}

/* ===================================================================
 * Tests — Server Lifecycle
 * =================================================================== */

TEST_F(E2EAutomatedTest, ServerStartStop) {
    server_->add_route("/", handler_hello);
    server_->set_router();
    int port = server_->start();
    ASSERT_GT(port, 0);
    ASSERT_TRUE(wait_for_server(server_->url("/")));

    /* Verify server is running */
    std::string code, body = curl_body_and_code("GET", server_->url("/"), code);
    EXPECT_EQ(code, "200");
    EXPECT_EQ(body, "Hello, World!");

    /* Cleanup happens in TearDown */
}

/* ===================================================================
 * main — gtest entry point
 * =================================================================== */

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}