/*
 * sendfile + keep-alive wire ordering.
 *
 * The header block of a static response used to be sent with the response
 * attached to its uv_write. That write's completion restarted HTTP reads, so
 * a second request pipelined behind the first could be dispatched while
 * uv_fs_sendfile was still streaming the first body. The bytes then arrived
 * as [H1][H2][B2][rest of B1] and the client desynchronized on Content-Length.
 *
 * This drives one real socket: request 1, wait for its headers, then send
 * request 2 before the body finishes. It asserts the full wire order
 * [H1][B1][H2][B2] byte for byte. A curl-per-request test cannot see this.
 */

#include "uvhttp_features.h"

#if UVHTTP_FEATURE_STATIC_FILES

#    include "uvhttp_allocator.h"
#    include "uvhttp_constants.h"
#    include "uvhttp_error.h"
#    include "uvhttp_request.h"
#    include "uvhttp_response.h"
#    include "uvhttp_router.h"
#    include "uvhttp_server.h"
#    include "uvhttp_static.h"

#    include <arpa/inet.h>
#    include <gtest/gtest.h>
#    include <netinet/in.h>
#    include <poll.h>
#    include <stdio.h>
#    include <stdlib.h>
#    include <string.h>
#    include <sys/socket.h>
#    include <unistd.h>
#    include <uv.h>

namespace {

/* 4MB in 4KB chunks keeps sendfile busy for hundreds of fs callbacks, so the
 * second request is guaranteed to be dispatched while body 1 is in flight
 * when the header write (wrongly) restarts reads. */
const size_t kFileSize = 4 * 1024 * 1024;
const size_t kChunkSize = 4 * 1024;

uvhttp_static_context_t* g_static_ctx = nullptr;

int StaticHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    (void)req;
    (void)resp;
    return uvhttp_static_handle_request(g_static_ctx, req, resp) == UVHTTP_OK
               ? 0
               : -1;
}

char PatternByte(size_t i, unsigned seed) {
    return (char)('a' + (int)((i + seed * 7) % 26));
}

void FillPattern(char* buf, size_t n, unsigned seed) {
    for (size_t i = 0; i < n; ++i) {
        buf[i] = PatternByte(i, seed);
    }
}

std::string TempDir() {
    char tmpl[] = "/tmp/uvhttp_sf_ka_XXXXXX";
    char* dir = mkdtemp(tmpl);
    return dir ? std::string(dir) : std::string();
}

void WriteFile(const std::string& dir, const char* name, unsigned seed) {
    std::string path = dir + "/" + name;
    FILE* f = fopen(path.c_str(), "wb");
    if (!f) {
        return;
    }
    char* buf = (char*)malloc(kFileSize);
    FillPattern(buf, kFileSize, seed);
    fwrite(buf, 1, kFileSize, f);
    fclose(f);
    free(buf);
}

int ConnectTo(int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        return -1;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((uint16_t)port);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
    if (connect(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

void StopLoop(uv_timer_t* t) {
    uv_stop((uv_loop_t*)t->data);
}

void Pump(uv_loop_t* loop) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, StopLoop, 5, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

/* Read until at least `want` bytes are buffered, pumping the loop so the
 * server can run its fs callbacks. Returns false on EOF or timeout. */
bool ReadAtLeast(uv_loop_t* loop, int fd, std::string* buf, size_t want) {
    char chunk[16384];
    for (int i = 0; i < 400 && buf->size() < want; ++i) {
        Pump(loop);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        int ready = poll(&pfd, 1, 20);
        if (ready < 0) {
            return false;
        }
        if (ready == 0) {
            continue;
        }
        ssize_t n = recv(fd, chunk, sizeof(chunk), 0);
        if (n <= 0) {
            return false;
        }
        buf->append(chunk, (size_t)n);
    }
    return buf->size() >= want;
}

size_t HeaderEnd(const std::string& raw, size_t from) {
    size_t at = raw.find("\r\n\r\n", from);
    return at == std::string::npos ? std::string::npos : at + 4;
}

/* A header block can arrive in several TCP segments. Wait for its terminator
 * instead of guessing a byte count (the real block is ~140 bytes). */
bool ReadHeader(uv_loop_t* loop, int fd, std::string* raw, size_t from) {
    char buf[8192];
    for (int i = 0; i < 400 && HeaderEnd(*raw, from) == std::string::npos;
         ++i) {
        Pump(loop);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 20) <= 0) {
            continue;
        }
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            return false;
        }
        raw->append(buf, (size_t)n);
    }
    return HeaderEnd(*raw, from) != std::string::npos;
}

size_t DeclaredLength(const std::string& raw, size_t from, size_t header_end) {
    const std::string key = "Content-Length: ";
    size_t at = raw.find(key, from);
    if (at == std::string::npos || at >= header_end) {
        return 0;
    }
    return (size_t)atol(raw.c_str() + at + key.size());
}

}  // namespace

class StaticSendfileKeepAliveTest : public ::testing::Test {
   protected:
    void SetUp() override {
        dir_ = TempDir();
        ASSERT_FALSE(dir_.empty());
        WriteFile(dir_, "a.bin", 1);
        WriteFile(dir_, "b.bin", 9);

        uvhttp_static_config_t config;
        memset(&config, 0, sizeof(config));
        strncpy(config.root_directory, dir_.c_str(),
                sizeof(config.root_directory) - 1);
        config.max_cache_size = 1024 * 1024;
        config.cache_ttl = 3600;
        config.max_cache_entries = 100;
        ASSERT_EQ(uvhttp_static_create(&config, &g_static_ctx), UVHTTP_OK);
        ASSERT_EQ(uvhttp_static_set_sendfile_config(g_static_ctx, 5000, 2,
                                                    kChunkSize),
                  UVHTTP_OK);

        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);
        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/a.bin", StaticHandler),
                  UVHTTP_OK);
        ASSERT_EQ(uvhttp_router_add_route(router, "/b.bin", StaticHandler),
                  UVHTTP_OK);
        uvhttp_server_take_router(server_, router);
        ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
        struct sockaddr_in addr;
        int namelen = sizeof(addr);
        memset(&addr, 0, sizeof(addr));
        ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                     (struct sockaddr*)&addr, &namelen),
                  0);
        port_ = ntohs(addr.sin_port);
    }
    void TearDown() override {
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        if (loop_) {
            uv_run(loop_, UV_RUN_NOWAIT);
            uv_loop_close(loop_);
            uvhttp_free(loop_);
        }
        if (g_static_ctx) {
            uvhttp_static_free(g_static_ctx);
            g_static_ctx = nullptr;
        }
        for (const char* name : {"a.bin", "b.bin"}) {
            std::string path = dir_ + "/" + name;
            unlink(path.c_str());
        }
        if (!dir_.empty()) {
            rmdir(dir_.c_str());
        }
    }
    std::string dir_;
    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
    int port_ = 0;
};

TEST_F(StaticSendfileKeepAliveTest, SecondRequestDoesNotInterleaveFirstBody) {
    int fd = ConnectTo(port_);
    ASSERT_GE(fd, 0);

    const char req1[] =
        "GET /a.bin HTTP/1.1\r\nHost: localhost\r\nConnection: keep-alive\r\n\r\n";
    ASSERT_EQ(send(fd, req1, strlen(req1), 0), (ssize_t)strlen(req1));

    std::string raw;
    ASSERT_TRUE(ReadHeader(loop_, fd, &raw, 0));
    size_t h1_end = HeaderEnd(raw, 0);
    ASSERT_NE(h1_end, std::string::npos);
    size_t len1 = DeclaredLength(raw, 0, h1_end);
    ASSERT_EQ(len1, kFileSize);

    /* Send request 2 while body 1 is still streaming. */
    const char req2[] =
        "GET /b.bin HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    ASSERT_EQ(send(fd, req2, strlen(req2), 0), (ssize_t)strlen(req2));

    /* Collect response 1's full body next. Any interleaved response-2 bytes
     * land here and fail the byte-for-byte comparison below. */
    ASSERT_TRUE(ReadAtLeast(loop_, fd, &raw, h1_end + kFileSize));
    for (size_t i = 0; i < kFileSize; ++i) {
        ASSERT_EQ(raw[h1_end + i], PatternByte(i, 1))
            << "response 1 body byte " << i << " is interleaved";
    }

    /* Then response 2 must start cleanly and carry its own full body. */
    size_t h2_start = h1_end + kFileSize;
    ASSERT_TRUE(ReadHeader(loop_, fd, &raw, h2_start));
    size_t h2_rel = HeaderEnd(raw, h2_start);
    ASSERT_NE(h2_rel, std::string::npos);
    size_t len2 = DeclaredLength(raw, h2_start, h2_rel);
    ASSERT_EQ(len2, kFileSize);
    ASSERT_TRUE(ReadAtLeast(loop_, fd, &raw, h2_rel + kFileSize));
    for (size_t i = 0; i < kFileSize; ++i) {
        ASSERT_EQ(raw[h2_rel + i], PatternByte(i, 9))
            << "response 2 body byte " << i << " differs";
    }
    close(fd);
}

TEST_F(StaticSendfileKeepAliveTest, HeadDeclaresFileSizeButStreamsNoBody) {
    int fd = ConnectTo(port_);
    ASSERT_GE(fd, 0);
    const char req[] =
        "HEAD /a.bin HTTP/1.1\r\nHost: localhost\r\nConnection: close\r\n\r\n";
    ASSERT_EQ(send(fd, req, strlen(req), 0), (ssize_t)strlen(req));

    std::string raw;
    ASSERT_TRUE(ReadHeader(loop_, fd, &raw, 0));
    size_t header_end = HeaderEnd(raw, 0);
    ASSERT_NE(header_end, std::string::npos);
    EXPECT_EQ(DeclaredLength(raw, 0, header_end), kFileSize);

    /* Give any misplaced sendfile bytes a chance to arrive. */
    for (int i = 0; i < 20; ++i) {
        Pump(loop_);
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        if (poll(&pfd, 1, 10) <= 0) {
            continue;
        }
        char buf[4096];
        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n > 0) {
            raw.append(buf, (size_t)n);
        }
    }
    EXPECT_EQ(raw.size(), header_end)
        << "HEAD response streamed " << (raw.size() - header_end) << " bytes";
    close(fd);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */
