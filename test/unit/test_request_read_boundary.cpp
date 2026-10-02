/**
 * @file test_request_read_boundary.cpp
 * @brief llhttp 跨 TCP 读边界的续写路径测试（Fix 3/4/5）
 *
 * 为什么需要独立测试：src/uvhttp_request.c 的三个解析回调都实现了
 * 「同一 token 被 llhttp 分多次回调」时的续写逻辑——TCP 读边界落在
 * URL / header 名 / header 值中间时，llhttp 会把同一个 token 拆成多段
 * 分别回调。续写逻辑依赖累积偏移而非从 0 覆写：
 *
 *   on_url (Fix 3):        used = strlen(request->url); memcpy(url + used, ...)
 *   on_header_field (Fix 4): parsing_header_field == 1 时从
 * current_header_field_len 续写 on_header_value (Fix 5): parsing_header_field
 * == 2 时续段 append 进已存 header 的 value
 *
 * 若这些回调改为从 0 覆写（去掉 used/field_len 偏移），则跨边界请求的
 * URL 与 header 会被截成第一段——而一次性喂入的请求完全正常，
 * 该缺陷在现有测试（全部一次性喂完整请求）下完全不可见。
 *
 * 真实 TCP 场景中请求分片是常态（Nagle、MTU 边界、慢客户端），
 * 所以这不是人造边界，而是生产环境的常见路径。
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_connection.h"
#include "uvhttp_protocol_upgrade.h"
#include "uvhttp_request.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
}

#include <string.h>
#include <string>

class ReadBoundaryTest : public ::testing::Test {
   protected:
    uv_loop_t loop{};
    uvhttp_server_t* server = nullptr;
    uvhttp_connection_t* conn = nullptr;

    void SetUp() override {
        ASSERT_EQ(uv_loop_init(&loop), 0);
        ASSERT_EQ(uvhttp_server_new(&loop, &server), UVHTTP_OK);
        ASSERT_NE(server, nullptr);
        ASSERT_EQ(uvhttp_connection_new(server, &conn), UVHTTP_OK);
        ASSERT_NE(conn, nullptr);
        ASSERT_NE(conn->request, nullptr);
        ASSERT_NE(conn->request->parser, nullptr);
    }

    void TearDown() override {
        if (conn) {
            uvhttp_connection_free(conn);
            conn = nullptr;
            uv_run(&loop, UV_RUN_DEFAULT);
        }
        if (server) {
            uvhttp_server_free(server);
        }
        uv_loop_close(&loop);
    }

    /* 喂一段原始字节（模拟一次 TCP 读） */
    void Feed(const std::string& chunk) {
        llhttp_execute(conn->request->parser, chunk.data(), chunk.size());
    }

    /* 把完整请求在指定偏移处切成两段，分别喂入 */
    void FeedSplit(const std::string& raw, size_t split_at) {
        ASSERT_LT(split_at, raw.size());
        Feed(raw.substr(0, split_at));
        Feed(raw.substr(split_at));
    }
};

// ============================================================================
// on_url 续写（Fix 3）
// ============================================================================

TEST_F(ReadBoundaryTest, UrlSplitAcrossReadsIsConcatenated) {
    Feed("GET /abc");
    Feed("def HTTP/1.1\r\nHost: example.com\r\n\r\n");
    EXPECT_STREQ(conn->request->url, "/abcdef");
}

TEST_F(ReadBoundaryTest, UrlSplitMidPathKeepsQueryIntact) {
    Feed("GET /api/v1/users");
    Feed("?page=2&size=10 HTTP/1.1\r\nHost: x\r\n\r\n");
    EXPECT_STREQ(conn->request->url, "/api/v1/users?page=2&size=10");
}

/* 在 URL 每一个偏移处切一刀都应得到完整 URL——覆盖所有切点而非个别 */
TEST_F(ReadBoundaryTest, UrlSplitAtEveryOffsetStaysIntact) {
    const std::string raw =
        "GET /path/to/resource?q=1 HTTP/1.1\r\nHost: x\r\n\r\n";
    const size_t url_start = 5;           /* "GET /" 之后 */
    const size_t url_end = raw.find(' '); /* URL 结束处 */
    for (size_t at = url_start; at <= url_end; at++) {
        /* 每个切点用独立 connection，避免状态串扰 */
        uvhttp_connection_t* c2 = nullptr;
        ASSERT_EQ(uvhttp_connection_new(server, &c2), UVHTTP_OK);
        llhttp_execute(c2->request->parser, raw.data(), at);
        llhttp_execute(c2->request->parser, raw.data() + at, raw.size() - at);
        EXPECT_STREQ(c2->request->url, "/path/to/resource?q=1")
            << "切点偏移 " << at << " 处 URL 损坏";
        uvhttp_connection_free(c2);
        uv_run(&loop, UV_RUN_DEFAULT);
    }
}

/* URL 跨读后仍可被正确解析出 path 与 query（续写不是仅拼接字符串） */
TEST_F(ReadBoundaryTest, SplitUrlStillParsesPathAndQuery) {
    Feed("GET /api/users");
    Feed("?id=42 HTTP/1.1\r\nHost: x\r\n\r\n");
    EXPECT_STREQ(uvhttp_request_get_path(conn->request), "/api/users");
    EXPECT_STREQ(uvhttp_request_get_query_string(conn->request), "id=42");
}

// ============================================================================
// on_header_value 续写（Fix 5）
// ============================================================================

TEST_F(ReadBoundaryTest, HeaderValueSplitAcrossReadsIsConcatenated) {
    Feed("GET / HTTP/1.1\r\nHost: abc");
    Feed("def\r\n\r\n");
    const char* v = uvhttp_request_get_header(conn->request, "Host");
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "abcdef");
}

/* 多个 header 各自跨读，验证续写不会串到相邻 header */
TEST_F(ReadBoundaryTest, MultipleSplitHeadersDoNotBleedIntoEachOther) {
    Feed("GET / HTTP/1.1\r\nHost: exa");
    Feed("mple.com\r\n");
    Feed("X-First: one");
    Feed("-two\r\n");
    Feed("X-Second: th");
    Feed("ree\r\n\r\n");

    const char* host = uvhttp_request_get_header(conn->request, "Host");
    const char* first = uvhttp_request_get_header(conn->request, "X-First");
    const char* second = uvhttp_request_get_header(conn->request, "X-Second");
    ASSERT_NE(host, nullptr);
    ASSERT_NE(first, nullptr);
    ASSERT_NE(second, nullptr);
    EXPECT_STREQ(host, "example.com");
    EXPECT_STREQ(first, "one-two");
    EXPECT_STREQ(second, "three");
}

/* 跨读不应产生重复条目——续段应 append 进已存 header 而非新增一个 */
TEST_F(ReadBoundaryTest, SplitHeaderValueDoesNotCreateDuplicateEntries) {
    Feed("GET / HTTP/1.1\r\nHost: ab");
    Feed("cdef\r\n\r\n");
    /* 只有一个 Host，值完整；若续段被误当新 header 存入，
     * get_header 只返回第一个片段，值会短于 "abcdef" */
    const char* v = uvhttp_request_get_header(conn->request, "Host");
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "abcdef");
    size_t count = 0;
    for (size_t i = 0; i < uvhttp_request_get_header_count(conn->request);
         i++) {
        uvhttp_header_t* h = uvhttp_request_get_header_at(conn->request, i);
        if (h && strcmp(h->name, "Host") == 0) {
            count++;
        }
    }
    EXPECT_EQ(count, 1u) << "续段不应产生重复 header 条目";
}

/* 分段处正好落在 value 中间的多段续写（不止一次回调） */
TEST_F(ReadBoundaryTest, HeaderValueSplitIntoThreeSegments) {
    Feed("GET / HTTP/1.1\r\nX-Multi: a");
    Feed("bc");
    Feed("def\r\n\r\n");
    const char* v = uvhttp_request_get_header(conn->request, "X-Multi");
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "abcdef");
}

// ============================================================================
// on_header_field 续写（Fix 4）
// ============================================================================

TEST_F(ReadBoundaryTest, HeaderFieldNameSplitAcrossReadsIsConcatenated) {
    Feed("GET / HTTP/1.1\r\nX-Cus");
    Feed("tom: value\r\n\r\n");
    const char* v = uvhttp_request_get_header(conn->request, "X-Custom");
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "value");
}

/* 多个 header 名各自跨读——验证 parsing_header_field 状态机在
 * 「上一对已提交（状态 2）后新名重新开始」时不错乱 */
TEST_F(ReadBoundaryTest, MultipleSplitHeaderNamesDoNotBleed) {
    Feed("GET / HTTP/1.1\r\nHost: h\r\nX-Al");
    Feed("pha: a\r\n");
    Feed("X-Be");
    Feed("ta: b\r\n\r\n");

    const char* host = uvhttp_request_get_header(conn->request, "Host");
    const char* alpha = uvhttp_request_get_header(conn->request, "X-Alpha");
    const char* beta = uvhttp_request_get_header(conn->request, "X-Beta");
    ASSERT_NE(host, nullptr);
    ASSERT_NE(alpha, nullptr);
    ASSERT_NE(beta, nullptr);
    EXPECT_STREQ(host, "h");
    EXPECT_STREQ(alpha, "a");
    EXPECT_STREQ(beta, "b");
}

/* header 名与值同时跨读——两个状态机的组合 */
TEST_F(ReadBoundaryTest, HeaderNameAndValueBothSplit) {
    Feed("GET / HTTP/1.1\r\nX-Bot");
    Feed("h: par");
    Feed("tial-value\r\n\r\n");
    const char* v = uvhttp_request_get_header(conn->request, "X-Both");
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "partial-value");
}

// ============================================================================
// 对照：一次性喂入同样正确（证明续写不破坏常规路径）
// ============================================================================

TEST_F(ReadBoundaryTest, SingleFeedStillWorksAsBaseline) {
    Feed("GET /single HTTP/1.1\r\nHost: full.example\r\n\r\n");
    EXPECT_STREQ(conn->request->url, "/single");
    const char* v = uvhttp_request_get_header(conn->request, "Host");
    ASSERT_NE(v, nullptr);
    EXPECT_STREQ(v, "full.example");
}