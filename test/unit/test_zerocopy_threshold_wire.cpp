/*
 * Zero-copy writev threshold — wire equivalence test
 *
 * UVHTTP_ZEROCOPY_MIN_BODY (default 4096) switches a non-TLS response between
 * two send routes inside uvhttp_response_send():
 *
 *   body_length <  threshold -> copy path  : build_data() concatenates
 *                                             header+body into one buffer
 *   body_length >= threshold -> zero-copy   : prepare() yields header and body
 *                                             as two iovecs for one uv_write
 *
 * Both routes must put identical bytes on the wire. The threshold is an
 * off-by-one away from truncating or duplicating a body, and the zero-copy
 * route carries the body as a second iovec, so framing bugs hide exactly at
 * the switch. This drives a real server over a real socket at the sizes that
 * straddle the boundary and compares the raw response bytes.
 *
 * The body is a non-uniform repeating pattern, so a body that came back
 * short, long, or shifted is distinguishable from a correct one.
 */

#include "uvhttp_allocator.h"
#include "uvhttp_constants.h"
#include "uvhttp_features.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
#if UVHTTP_FEATURE_COMPRESSION
/* Only the compression-interaction tests below need a decoder, and the
 * headers ship with the feature. The no-compression build matrix entry must
 * still compile this file. */
#    include "miniz.h"
#    include "miniz_tinfl.h"
#endif

#include <arpa/inet.h>
#include <gtest/gtest.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <sys/socket.h>
#include <unistd.h>
#include <uv.h>
#include <vector>

namespace {

/* Largest body used below: one byte over the threshold. */
const size_t kMaxBody = UVHTTP_ZEROCOPY_MIN_BODY + 1;

/* Deterministic, non-uniform filler so truncation or duplication of the body
 * cannot masquerade as a correct response. */
char PatternByte(size_t i) {
    return (char)('a' + (int)(i % 26));
}

void FillPattern(char* buf, size_t n) {
    for (size_t i = 0; i < n; i++) {
        buf[i] = PatternByte(i);
    }
}

/* Body size is carried in the path so each request is self-describing and no
 * handler state has to survive across the loop pumps below. A `/gzip/` prefix
 * additionally turns on response compression, which makes the body shrink
 * below the zero-copy threshold — the interaction between the two size
 * decisions is what these tests exercise. */
int BodyHandler(uvhttp_request_t* req, uvhttp_response_t* resp) {
    const char* path = uvhttp_request_get_path(req);
    int gzip = 0;
    if (strncmp(path, "/gzip/", 6) == 0) {
        gzip = 1;
        path += 5; /* keep the '/' so strrchr below still finds the digits */
    }
    const char* digits = strrchr(path, '/');
    long size = digits ? atol(digits + 1) : 0;

    if (size <= 0 || (size_t)size > kMaxBody) {
        uvhttp_response_set_status(resp, 400);
        uvhttp_response_set_header(resp, "Content-Type", "text/plain");
        uvhttp_response_set_body(resp, "bad size", 8);
        return uvhttp_response_send(resp);
    }

    if (gzip) {
        uvhttp_response_set_compress(resp, 1);
        /* Force the compression path regardless of the default 1KB floor, so
         * the size under test is the only variable. */
        uvhttp_response_set_compress_threshold(resp, 1);
    }

    /* set_body copies into a response-owned buffer, so a stack buffer is
     * enough even though the zero-copy route writes asynchronously. */
    char body[kMaxBody];
    FillPattern(body, (size_t)size);

    uvhttp_response_set_status(resp, 200);
    uvhttp_response_set_header(resp, "Content-Type",
                               "application/octet-stream");
    uvhttp_response_set_body(resp, body, (size_t)size);
    return uvhttp_response_send(resp);
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

/* Pump the loop for a bounded slice so the server can accept, parse, and
 * write without the test ever blocking on the event loop. */
void Pump(uv_loop_t* loop, int ms) {
    uv_timer_t timer;
    uv_timer_init(loop, &timer);
    timer.data = loop;
    uv_timer_start(&timer, StopLoop, ms, 0);
    uv_run(loop, UV_RUN_DEFAULT);
    uv_close((uv_handle_t*)&timer, NULL);
    uv_run(loop, UV_RUN_NOWAIT);
}

/* Raw response bytes as observed on the socket. */
struct WireResponse {
    std::string raw;
    std::string header_block;
    std::string body;
    /* The Content-Length value the server declared, kept separately so a test
     * can assert it against what actually arrived. Comparing body.size() to
     * the raw framing would be circular — FetchRaw reads exactly this many
     * bytes, so they agree by construction even when the value is wrong. */
    size_t declared_length = 0;
};

/* Issue one GET and read until Content-Length bytes of body have arrived.
 * Returns false if the exchange did not complete within the budget. */
bool FetchRaw(uv_loop_t* loop, int port, const std::string& path,
              WireResponse* out) {
    int fd = ConnectTo(port);
    if (fd < 0) {
        return false;
    }

    char request[256];
    int request_len =
        snprintf(request, sizeof(request),
                 "GET %s HTTP/1.1\r\nHost: localhost\r\n\r\n", path.c_str());
    if (send(fd, request, (size_t)request_len, 0) != request_len) {
        close(fd);
        return false;
    }

    /* Read until the body is complete, pumping the loop between reads so the
     * server's write can actually be issued. */
    out->raw.clear();
    char buf[8192];
    /* Bytes the whole response occupies on the wire: header block, blank-line
     * separator, and the declared body. raw.size() already includes the
     * headers, so comparing it against the body length alone would let this
     * loop exit while the body is still short — by up to one header block —
     * whenever TCP delivers the response in more than one segment. */
    size_t expected_total = 0;
    bool have_length = false;
    const int kMaxPolls = 200;

    for (int i = 0;
         i < kMaxPolls && !(have_length && out->raw.size() >= expected_total);
         i++) {
        Pump(loop, 10);

        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        int ready = poll(&pfd, 1, 20);
        if (ready <= 0) {
            continue;
        }

        ssize_t n = recv(fd, buf, sizeof(buf), 0);
        if (n <= 0) {
            break;
        }
        out->raw.append(buf, (size_t)n);

        if (!have_length) {
            size_t sep = out->raw.find("\r\n\r\n");
            if (sep == std::string::npos) {
                continue;
            }
            out->header_block = out->raw.substr(0, sep);
            const std::string kKey = "Content-Length: ";
            size_t cl = out->header_block.find(kKey);
            if (cl == std::string::npos) {
                close(fd);
                return false; /* framed by something other than a length */
            }
            /* header block + blank-line separator + declared body */
            out->declared_length =
                (size_t)atol(out->header_block.c_str() + cl + kKey.size());
            expected_total = sep + 4 + out->declared_length;
            have_length = true;
        }
    }

    close(fd);

    if (!have_length) {
        return false;
    }
    out->body = out->raw.substr(out->raw.find("\r\n\r\n") + 4);
    return true;
}

/* Replace the Content-Length value so two header blocks can be compared for
 * equality independent of the body size they describe. */
std::string NormalizeHeaderBlock(const std::string& block) {
    const std::string kKey = "Content-Length: ";
    size_t at = block.find(kKey);
    if (at == std::string::npos) {
        return block;
    }
    size_t value_start = at + kKey.size();
    size_t value_end = block.find("\r\n", value_start);
    if (value_end == std::string::npos) {
        return block;
    }
    return block.substr(0, value_start) + "<n>" + block.substr(value_end);
}

std::string BodyPath(size_t size) {
    char path[64];
    snprintf(path, sizeof(path), "/body/%zu", size);
    return std::string(path);
}

std::string GzipPath(size_t size) {
    char path[64];
    snprintf(path, sizeof(path), "/gzip/%zu", size);
    return std::string(path);
}

}  // namespace

class ZerocopyThresholdWireTest : public ::testing::Test {
   protected:
    void SetUp() override {
        loop_ = uv_loop_new();
        ASSERT_NE(loop_, nullptr);

        ASSERT_EQ(uvhttp_server_new(loop_, &server_), UVHTTP_OK);
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        /* Exact paths rather than a prefix route: the handler reads the size
         * back out of the path, so registration stays derived from the
         * threshold macro instead of hard-coded byte counts. */
        const size_t sizes[] = {UVHTTP_ZEROCOPY_MIN_BODY - 1,
                                UVHTTP_ZEROCOPY_MIN_BODY,
                                UVHTTP_ZEROCOPY_MIN_BODY + 1};
        for (size_t size : sizes) {
            ASSERT_EQ(uvhttp_router_add_route(router, BodyPath(size).c_str(),
                                              BodyHandler),
                      UVHTTP_OK);
            ASSERT_EQ(uvhttp_router_add_route(router, GzipPath(size).c_str(),
                                              BodyHandler),
                      UVHTTP_OK);
        }
        uvhttp_server_set_router(server_, router);

        ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);

        struct sockaddr_in addr;
        int namelen = sizeof(addr);
        memset(&addr, 0, sizeof(addr));
        ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                     (struct sockaddr*)&addr, &namelen),
                  0);
        port_ = ntohs(addr.sin_port);
        ASSERT_GT(port_, 0);
    }

    void TearDown() override {
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        uv_run(loop_, UV_RUN_NOWAIT);
        uv_loop_close(loop_);
        uvhttp_free(loop_);
    }

    uv_loop_t* loop_ = nullptr;
    uvhttp_server_t* server_ = nullptr;
    int port_ = 0;
};

/* A body at each side of the threshold must arrive whole, with the length the
 * server declared. The copy route (4095) and the zero-copy route (4096/4097)
 * differ in how the bytes are framed into uv_write, so this is where a dropped
 * second iovec or an off-by-one in the comparison would show up. */
TEST_F(ZerocopyThresholdWireTest, BodyIsIntactAcrossThreshold) {
    const size_t sizes[] = {UVHTTP_ZEROCOPY_MIN_BODY - 1,
                            UVHTTP_ZEROCOPY_MIN_BODY,
                            UVHTTP_ZEROCOPY_MIN_BODY + 1};

    for (size_t size : sizes) {
        WireResponse resp;
        ASSERT_TRUE(FetchRaw(loop_, port_, BodyPath(size), &resp))
            << "no complete response for body size " << size;

        /* Exactly the declared body, no more and no less. */
        EXPECT_EQ(resp.body.size(), size) << "body size " << size;
        EXPECT_EQ(resp.raw.size(), resp.header_block.size() + 4 + size)
            << "framing differs for body size " << size;

        /* Byte-for-byte correct, not merely the right length. */
        for (size_t i = 0; i < size && i < resp.body.size(); i++) {
            ASSERT_EQ(resp.body[i], PatternByte(i))
                << "body byte " << i << " differs at body size " << size;
        }
    }
}

/* The threshold only decides how the body is framed; it must not change the
 * header block that describes it. Identical normalized headers on both sides
 * of the boundary is what makes the two routes wire-equivalent. */
TEST_F(ZerocopyThresholdWireTest, HeaderBlockIsIdenticalAcrossThreshold) {
    WireResponse below, at, above;
    ASSERT_TRUE(
        FetchRaw(loop_, port_, BodyPath(UVHTTP_ZEROCOPY_MIN_BODY - 1), &below));
    ASSERT_TRUE(
        FetchRaw(loop_, port_, BodyPath(UVHTTP_ZEROCOPY_MIN_BODY), &at));
    ASSERT_TRUE(
        FetchRaw(loop_, port_, BodyPath(UVHTTP_ZEROCOPY_MIN_BODY + 1), &above));

    const std::string normalized = NormalizeHeaderBlock(below.header_block);
    EXPECT_EQ(NormalizeHeaderBlock(at.header_block), normalized);
    EXPECT_EQ(NormalizeHeaderBlock(above.header_block), normalized);

    /* The value that was normalized away must still be the real one. */
    EXPECT_NE(below.header_block, at.header_block)
        << "Content-Length did not track the body size";
}

#if UVHTTP_FEATURE_COMPRESSION
/* ---------------- compression × zero-copy interaction ---------------- */

/* Decompress a gzip stream and return the byte count, or -1 if the bytes are
 * not a valid stream.
 *
 * UVHTTP builds gzip by hand: a 10-byte header, then a RAW deflate stream
 * (windowBits = -15, no zlib wrapper), then CRC32 + ISIZE. The bundled miniz
 * tinfl decodes raw deflate only, so the header and trailer are stripped here
 * rather than handed to it. Decoding with zlib instead would be a different
 * container than what actually went on the wire. */
long GunzipSize(const std::string& in, size_t expect) {
    const size_t kHeader = 10; /* magic, CM, FLG, MTIME, XFL, OS */
    const size_t kTrailer = 8; /* CRC32 + ISIZE */
    if (in.size() <= kHeader + kTrailer) {
        return -1;
    }
    if (in.compare(0, 2, std::string("\x1f\x8b", 2)) != 0) {
        return -1; /* not a gzip stream at all */
    }

    std::vector<unsigned char> out(expect + 64);
    const char* raw = in.data() + kHeader;
    size_t raw_len = in.size() - kHeader - kTrailer;
    size_t n =
        tinfl_decompress_mem_to_mem(out.data(), out.size(), raw, raw_len,
                                    TINFL_FLAG_USING_NON_WRAPPING_OUTPUT_BUF);
    if (n == TINFL_DECOMPRESS_MEM_TO_MEM_FAILED) {
        return -1;
    }
    return static_cast<long>(n);
}

/* A compressible body at/above the zero-copy threshold shrinks below it, so
 * the two size decisions disagree: the send path chooses zero-copy from the
 * PRE-compression length while the wire carries the POST-compression bytes.
 * Whatever route is chosen, the response must be self-consistent —
 * Content-Encoding announces gzip, Content-Length matches what actually
 * arrives, and the bytes inflate back to the original body. */
TEST_F(ZerocopyThresholdWireTest, CompressedBodyIsSelfConsistent) {
    for (size_t size :
         {UVHTTP_ZEROCOPY_MIN_BODY, UVHTTP_ZEROCOPY_MIN_BODY + 1}) {
        WireResponse resp;
        ASSERT_TRUE(FetchRaw(loop_, port_, GzipPath(size), &resp))
            << "no complete response for gzip body size " << size;

        EXPECT_NE(resp.header_block.find("Content-Encoding: gzip"),
                  std::string::npos)
            << "compressible body was not compressed at size " << size;

        /* The declared length must be what actually arrived — and this must
         * be checked against the header VALUE, not against the raw framing:
         * FetchRaw reads exactly declared_length bytes, so framing agreement
         * holds by construction even when the declared value is wrong. */
        EXPECT_EQ(resp.declared_length, resp.body.size())
            << "Content-Length describes " << resp.declared_length
            << " bytes but " << resp.body.size() << " arrived for body size "
            << size;
        EXPECT_NE(resp.declared_length, size)
            << "Content-Length still describes the uncompressed body size "
            << size;

        /* Compression must have actually shrunk it, otherwise this test is
         * not exercising the size-disagreement case at all. */
        EXPECT_LT(resp.body.size(), size)
            << "body did not shrink below the original size " << size;

        /* And the bytes must decode back to exactly what the handler set. */
        EXPECT_EQ(GunzipSize(resp.body, size), static_cast<long>(size))
            << "gzip payload does not inflate to the original body size "
            << size;
    }
}

/* The same body uncompressed must arrive as plain bytes with no
 * Content-Encoding — the compression switch is what distinguishes the two
 * routes, so without it this would only prove the handler echoes input. */
TEST_F(ZerocopyThresholdWireTest, UncompressedBodyCarriesNoContentEncoding) {
    WireResponse resp;
    ASSERT_TRUE(
        FetchRaw(loop_, port_, BodyPath(UVHTTP_ZEROCOPY_MIN_BODY), &resp));

    EXPECT_EQ(resp.header_block.find("Content-Encoding"), std::string::npos);
    EXPECT_EQ(resp.body.size(), UVHTTP_ZEROCOPY_MIN_BODY);
    for (size_t i = 0; i < resp.body.size(); i++) {
        ASSERT_EQ(resp.body[i], PatternByte(i))
            << "plain body byte " << i << " differs";
    }
}
#endif /* UVHTTP_FEATURE_COMPRESSION */
