/*
 * Directory listing regression tests: href encoding and HTML escaping.
 * A file name is attacker-controlled content; the generated listing must not
 * be able to inject markup or a javascript: URL through it.
 */

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_static.h"

#    include "uvhttp.h"

#    include <gtest/gtest.h>
#    include <stdio.h>
#    include <string.h>
#    include <string>
#    include <sys/stat.h>
#    include <unistd.h>
#    include <uv.h>

class StaticDirectoryListingTest : public ::testing::Test {
   protected:
    uv_loop_t loop_;
    uv_tcp_t client_;
    uvhttp_static_context_t* ctx_ = nullptr;
    std::string dir_;

    static void write_file(const std::string& path, const char* content) {
        FILE* f = fopen(path.c_str(), "wb");
        ASSERT_NE(f, nullptr) << path;
        fputs(content, f);
        fclose(f);
    }

    void SetUp() override {
        ASSERT_EQ(uv_loop_init(&loop_), 0);
        ASSERT_EQ(uv_tcp_init(&loop_, &client_), 0);

        char tmpl[] = "/tmp/uvhttp_listing_XXXXXX";
        char* d = mkdtemp(tmpl);
        ASSERT_NE(d, nullptr);
        dir_ = d;

        write_file(dir_ + "/a b.txt", "space");
        write_file(dir_ + "/<svg onload=alert(1)>.txt", "xss");
        ASSERT_EQ(mkdir((dir_ + "/sub").c_str(), 0755), 0);

        uvhttp_static_config_t cfg;
        memset(&cfg, 0, sizeof(cfg));
        cfg.max_cache_size = 1024 * 1024;
        cfg.max_file_size = 1024 * 1024;
        cfg.cache_ttl = 0;
        cfg.max_cache_entries = 10;
        cfg.enable_directory_listing = 1;
        snprintf(cfg.root_directory, sizeof(cfg.root_directory), "%s",
                 dir_.c_str());
        snprintf(cfg.index_file, sizeof(cfg.index_file), "index.html");

        ASSERT_EQ(uvhttp_static_create(&cfg, &ctx_), UVHTTP_OK);
        ASSERT_NE(ctx_, nullptr);
    }

    void TearDown() override {
        if (ctx_) {
            uvhttp_static_free(ctx_);
            ctx_ = nullptr;
        }
        unlink((dir_ + "/a b.txt").c_str());
        unlink((dir_ + "/<svg onload=alert(1)>.txt").c_str());
        rmdir((dir_ + "/sub").c_str());
        rmdir(dir_.c_str());

        uv_close((uv_handle_t*)&client_, nullptr);
        uv_run(&loop_, UV_RUN_NOWAIT);
        uv_loop_close(&loop_);
    }

    uvhttp_result_t serve(uvhttp_static_context_t* ctx, const char* url,
                          uvhttp_response_t* response, std::string* body) {
        uvhttp_request_t request;
        EXPECT_EQ(uvhttp_request_init(&request, &client_), UVHTTP_OK);
        snprintf(request.url, sizeof(request.url), "%s", url);
        EXPECT_EQ(uvhttp_response_init(response, &client_), UVHTTP_OK);

        uvhttp_result_t result =
            uvhttp_static_handle_request(ctx, &request, response);
        uvhttp_request_cleanup(&request);

        if (body && response->body) {
            body->assign(response->body, response->body_length);
        }
        return result;
    }
};

TEST_F(StaticDirectoryListingTest, EncodesHrefAndEscapesText) {
    uvhttp_response_t response;
    std::string body;

    ASSERT_EQ(serve(ctx_, "/", &response, &body), UVHTTP_OK);
    EXPECT_EQ(response.status_code, 200);

    /* Plain names are untouched. */
    EXPECT_NE(body.find("a b.txt"), std::string::npos);

    /* The href is percent-encoded: spaces, quotes and angle brackets cannot
     * break out of the attribute, and a javascript: prefix cannot survive as
     * a scheme. */
    EXPECT_NE(body.find("a%20b.txt"), std::string::npos);
    EXPECT_NE(body.find("%3Csvg%20onload%3Dalert%281%29%3E.txt"),
              std::string::npos);

    /* The visible text is HTML-escaped: no raw markup reaches the parser. */
    EXPECT_EQ(body.find("<svg"), std::string::npos);
    EXPECT_NE(body.find("&lt;svg"), std::string::npos);

    /* Subdirectories keep a trailing slash in the href. */
    EXPECT_NE(body.find("href=\"sub/\""), std::string::npos);

    uvhttp_response_cleanup(&response);
}

TEST_F(StaticDirectoryListingTest, MissingFileIsNotFound) {
    uvhttp_response_t response;
    ASSERT_EQ(serve(ctx_, "/does-not-exist.txt", &response, nullptr),
              UVHTTP_ERROR_NOT_FOUND);
    EXPECT_EQ(response.status_code, 404);
    uvhttp_response_cleanup(&response);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
