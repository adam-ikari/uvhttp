/*
 * UVHTTP 分派前 request filter 接缝测试
 *
 * v3.0.0 删掉了内置限流（per-server 单计数器会让一个客户端打满全站 429），
 * 换成一个库只负责「跑」、策略完全归调用方的接缝：
 * uvhttp_server_set_request_filter() 与 on_message_complete 里的调用点
 * （src/uvhttp_request.c:362）。被删的限流测试没有留下任何等价覆盖，
 * 而这条路径上四件事都可能出错且都不显眼：
 *
 *   1. 接缝必须早于协议升级探测 —— 否则被拒的请求仍会到达用户的 detector，
 *      在一个已判定要拒绝的连接上去做升级。
 *   2. filter 自己发响应时库不得重复发（双发会让连接直接坏掉）。
 *   3. filter 只设状态不发送时，库的兜底必须保留那个状态；若 filter 什么都没
 *      设，兜底得产出合法的 403 而不是 `HTTP/1.1 0 ` —— 连接复用时
 *      uvhttp_connection.c 把 status_code 重置为 0，而 build_data 原样序列化。
 *   4. 拒绝不能终结 keep-alive 连接。
 *
 * 回调每请求恰好一次、user_data 原样透传、client_ip 在 filter 里可见
 * （per-client 策略据此才可实现）—— 一并断言。
 */

extern "C" {
#include "uvhttp_protocol_upgrade.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_router.h"
#include "uvhttp_server.h"
}

#include <atomic>
#include <cstddef>
#include <cstring>
#include <gtest/gtest.h>
#include <string>
#include <thread>
#include <unistd.h>
#include <uv.h>

namespace {

/* 观测点：每个用例开始前 Reset() */
std::atomic<int> g_filter_calls{0};
std::atomic<int> g_handler_calls{0};
std::atomic<int> g_detector_calls{0};
std::atomic<int> g_upgrade_handler_calls{0};
void* g_filter_user_data = nullptr;
std::string g_filter_path;
std::string g_filter_client_ip;
uvhttp_request_t* g_filter_request = nullptr;
uvhttp_response_t* g_filter_response = nullptr;

void Reset() {
    g_filter_calls = 0;
    g_handler_calls = 0;
    g_detector_calls = 0;
    g_upgrade_handler_calls = 0;
    g_filter_user_data = nullptr;
    g_filter_path.clear();
    g_filter_client_ip.clear();
    g_filter_request = nullptr;
    g_filter_response = nullptr;
}

/* 放行，同时把可见信息录下来 */
uvhttp_error_t AllowFilter(uvhttp_request_t* request,
                           uvhttp_response_t* response, void* user_data) {
    g_filter_calls++;
    g_filter_user_data = user_data;
    g_filter_request = request;
    g_filter_response = response;
    const char* path = uvhttp_request_get_path(request);
    const char* ip = uvhttp_request_get_client_ip(request);
    g_filter_path = path ? path : "";
    g_filter_client_ip = ip ? ip : "";
    return UVHTTP_OK;
}

/* 拒绝并已把响应发完 —— 库不得再兜底 */
uvhttp_error_t DenyWithOwnResponse(uvhttp_request_t*,
                                   uvhttp_response_t* response, void*) {
    g_filter_calls++;
    uvhttp_response_set_status(response, 429);
    uvhttp_response_set_header(response, "Retry-After", "1");
    uvhttp_response_set_body(response, "slow down", strlen("slow down"));
    uvhttp_response_send(response);
    return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
}

/* 拒绝，只设状态没发送 —— 兜底应保留 429 */
uvhttp_error_t DenyStatusOnly(uvhttp_request_t*, uvhttp_response_t* response,
                              void*) {
    g_filter_calls++;
    uvhttp_response_set_status(response, 429);
    return UVHTTP_ERROR_RATE_LIMIT_EXCEEDED;
}

/* 拒绝，什么都不设 —— 兜底应给出合法的 403。
 * 返回码故意不是限流码：任何非 OK 都应当停止分派。 */
uvhttp_error_t DenyWithoutResponse(uvhttp_request_t*, uvhttp_response_t*,
                                   void*) {
    g_filter_calls++;
    return UVHTTP_ERROR_CONNECTION_LIMIT;
}

int HandleRequest(uvhttp_request_t*, uvhttp_response_t* response) {
    g_handler_calls++;
    uvhttp_response_set_status(response, 200);
    uvhttp_response_set_header(response, UVHTTP_HEADER_CONTENT_TYPE,
                               UVHTTP_CONTENT_TYPE_TEXT);
    uvhttp_response_set_body(response, "handled", 7);
    uvhttp_response_send(response);
    return 0;
}

/* 协议升级：detector 记录是否被调用。返回 1 让框架调 handler，handler 返回错误
 * 使框架发出 400 —— 客户端能观测到「升级路径确实走到了」。 */
int ProbeDetector(uvhttp_request_t*, char* protocol_name, size_t len,
                  const char* upgrade_header, const char*) {
    g_detector_calls++;
    if (upgrade_header && len > 0) {
        strncpy(protocol_name, upgrade_header, len - 1);
        protocol_name[len - 1] = '\0';
    }
    return 1;
}

uvhttp_error_t RejectUpgrade(uvhttp_connection_t*, const char*, void*) {
    g_upgrade_handler_calls++;
    return UVHTTP_ERROR_NOT_SUPPORTED;
}

std::string run_curl(const std::string& cmd) {
    FILE* pipe = popen(cmd.c_str(), "r");
    if (!pipe) {
        return "";
    }
    std::string out;
    char buf[4096];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), pipe)) > 0) {
        out.append(buf, n);
    }
    pclose(pipe);
    return out;
}

/* 返回 "<code>|<body>" */
std::string fetch(const std::string& url,
                  const std::string& extra_headers = "") {
    std::string result = run_curl(
        "curl -s --connect-timeout 2 --max-time 5 -w '\\n%{http_code}'" +
        extra_headers + " '" + url + "' 2>/dev/null");
    size_t pos = result.rfind('\n');
    if (pos == std::string::npos) {
        return "000|" + result;
    }
    return result.substr(pos + 1) + "|" + result.substr(0, pos);
}

std::string code_of(const std::string& fetched) {
    return fetched.substr(0, fetched.find('|'));
}

std::string body_of(const std::string& fetched) {
    size_t pos = fetched.find('|');
    return pos == std::string::npos ? "" : fetched.substr(pos + 1);
}

/* 状态行出现次数：>1 说明同一连接上发了两遍响应 */
int status_line_count(const std::string& raw) {
    int n = 0;
    size_t pos = 0;
    while ((pos = raw.find("HTTP/1.1 ", pos)) != std::string::npos) {
        n++;
        pos += 9;
    }
    return n;
}

/* 起一个真实监听的服务端：port 0 让内核选端口，再从 tcp_handle 读回实际端口，
 * 不占用固定端口号（并发/重跑时不会撞车）。 */
class LiveServer {
   public:
    LiveServer() = default;
    ~LiveServer() {
        stop();
    }

    LiveServer(const LiveServer&) = delete;
    LiveServer& operator=(const LiveServer&) = delete;

    void start() {
        ASSERT_EQ(uv_loop_init(&loop_), 0);
        loop_ready_ = true;
        ASSERT_EQ(uvhttp_server_new(&loop_, &server_), UVHTTP_OK);
        ASSERT_NE(server_, nullptr);
        ASSERT_EQ(uvhttp_server_set_handler(server_, HandleRequest), UVHTTP_OK);

        /* 空 router 是必需的，不是冗余：on_message_complete 的分派块整体在
         * `if (server->router)` 里，没有 router 时请求直接落到硬编码的 200
         * 默认响应，server->handler 根本不会被调用（uvhttp_request.c:564）。
         * 挂一个不匹配任何路径的 router，find_handler 返回 NULL，才走到
         * server->handler 这条 catch-all 分支。 */
        uvhttp_router_t* router = nullptr;
        ASSERT_EQ(uvhttp_router_new(&router), UVHTTP_OK);
        ASSERT_NE(router, nullptr);
        ASSERT_EQ(uvhttp_server_take_router(server_, router), UVHTTP_OK);

        ASSERT_EQ(uvhttp_server_listen(server_, "127.0.0.1", 0), UVHTTP_OK);
        struct sockaddr_in addr {};
        int len = static_cast<int>(sizeof(addr));
        ASSERT_EQ(uv_tcp_getsockname(&server_->tcp_handle,
                                     (struct sockaddr*)&addr, &len),
                  0);
        port_ = ntohs(addr.sin_port);
        ASSERT_GT(port_, 0);
        base_url_ = "http://127.0.0.1:" + std::to_string(port_);

        running_ = true;
        thread_ = std::thread([this] {
            while (running_) {
                uv_run(&loop_, UV_RUN_NOWAIT);
                usleep(1000);
            }
        });

        ASSERT_TRUE(wait_until_ready())
            << "server on port " << port_ << " never came up";
        /* wait_until_ready 自己发过探测请求：不清零的话，探测那一次
         * handler/filter 调用会混进用例断言里。 */
        Reset();
    }

    void stop() {
        if (thread_.joinable()) {
            running_ = false;
            thread_.join();
        }
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        if (loop_ready_) {
            /* drain close callbacks before closing, or uv_loop_close returns
             * UV_EBUSY and the leaksanitizer run reports the whole server */
            uv_run(&loop_, UV_RUN_DEFAULT);
            uv_loop_close(&loop_);
            loop_ready_ = false;
        }
    }

    uvhttp_server_t* server() {
        return server_;
    }

    std::string url(const std::string& path = "/") {
        return base_url_ + path;
    }

   private:
    bool wait_until_ready() {
        for (int i = 0; i < 100; i++) {
            std::string code = run_curl("curl -s -o /dev/null --max-time 2 -w "
                                        "'%{http_code}' " +
                                        url("/") + " 2>/dev/null");
            if (!code.empty() && code != "000") {
                return true;
            }
            usleep(50000);
        }
        return false;
    }

    uv_loop_t loop_{};
    bool loop_ready_ = false;
    uvhttp_server_t* server_ = nullptr;
    std::thread thread_;
    std::atomic<bool> running_{false};
    int port_ = 0;
    std::string base_url_;
};

}  // namespace

class RequestFilter : public ::testing::Test {
   protected:
    void SetUp() override {
        Reset();
        ASSERT_EQ(uv_loop_init(&loop_), 0);
        ASSERT_EQ(uvhttp_server_new(&loop_, &server_), UVHTTP_OK);
        ASSERT_NE(server_, nullptr);
    }

    void TearDown() override {
        if (server_) {
            uvhttp_server_free(server_);
            server_ = nullptr;
        }
        uv_run(&loop_, UV_RUN_DEFAULT);
        uv_loop_close(&loop_);
    }

    uv_loop_t loop_{};
    uvhttp_server_t* server_ = nullptr;
};

// ============================================================================
// setter 契约
// ============================================================================

TEST_F(RequestFilter, SetRequestFilterRejectsNullServer) {
    int marker = 7;
    EXPECT_EQ(uvhttp_server_set_request_filter(nullptr, AllowFilter, &marker),
              UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(RequestFilter, SetAndClearRequestFilter) {
    int marker = 7;
    ASSERT_EQ(uvhttp_server_set_request_filter(server_, AllowFilter, &marker),
              UVHTTP_OK);
    EXPECT_EQ(server_->request_filter, AllowFilter);
    EXPECT_EQ(server_->request_filter_user_data, &marker);

    /* NULL filter 清除接缝；user_data 是借用的，库不 free */
    ASSERT_EQ(uvhttp_server_set_request_filter(server_, nullptr, nullptr),
              UVHTTP_OK);
    EXPECT_EQ(server_->request_filter, nullptr);
    EXPECT_EQ(server_->request_filter_user_data, nullptr);
}

TEST_F(RequestFilter, ServerNewLeavesSeamClosed) {
    /* 未显式设置时不得有残留指针——否则每个请求都会跳进未定义函数 */
    EXPECT_EQ(server_->request_filter, nullptr);
    EXPECT_EQ(server_->request_filter_user_data, nullptr);
}

TEST_F(RequestFilter, SeamFieldsShareOneCacheLine) {
    /* 布局断言写成相对形式：TLS/WebSocket 等 feature 会改变前面的 cache line
     * 大小，硬编码 256 会在别的构建组合下假红。 */
    size_t a = offsetof(uvhttp_server_t, request_filter);
    size_t b = offsetof(uvhttp_server_t, request_filter_user_data);
    EXPECT_EQ(b - a, sizeof(void*));
    EXPECT_EQ(a / 64, b / 64) << "接缝两个字段应落在同一条 cache line";
}

// ============================================================================
// 活体请求路径
// ============================================================================

TEST_F(RequestFilter, NullFilterDispatchesNormally) {
    LiveServer srv;
    srv.start();

    auto got = fetch(srv.url("/anything"));
    EXPECT_EQ(code_of(got), "200");
    EXPECT_EQ(body_of(got), "handled");
    EXPECT_EQ(g_handler_calls, 1);
}

TEST_F(RequestFilter, FilterRunsOncePerRequestWithUsableContext) {
    LiveServer srv;
    srv.start();
    int marker = 99;
    ASSERT_EQ(
        uvhttp_server_set_request_filter(srv.server(), AllowFilter, &marker),
        UVHTTP_OK);

    auto got = fetch(srv.url("/widgets"));
    EXPECT_EQ(code_of(got), "200");
    EXPECT_EQ(g_filter_calls, 1) << "每请求恰好一次";
    EXPECT_EQ(g_handler_calls, 1) << "放行后照常分派";
    EXPECT_EQ(g_filter_user_data, &marker) << "user_data 原样透传";
    EXPECT_EQ(g_filter_path, "/widgets");
    /* per-client 策略的前提：filter 里拿得到对端地址（IPv6 与 XFF 由
     * uvhttp_request_get_client_ip 内部处理） */
    EXPECT_EQ(g_filter_client_ip, "127.0.0.1");
    EXPECT_NE(g_filter_request, nullptr);
    EXPECT_NE(g_filter_response, nullptr);
}

TEST_F(RequestFilter, DenyWithOwnResponseReachesClient) {
    LiveServer srv;
    srv.start();
    ASSERT_EQ(uvhttp_server_set_request_filter(srv.server(),
                                               DenyWithOwnResponse, nullptr),
              UVHTTP_OK);

    auto got = fetch(srv.url("/"));
    EXPECT_EQ(code_of(got), "429");
    EXPECT_EQ(body_of(got), "slow down");
    EXPECT_EQ(g_handler_calls, 0) << "被拒请求不得进入 handler";

    /* filter 自己发的头应原样送出，且库没有重复发送 */
    auto raw =
        run_curl("curl -s -i --max-time 5 '" + srv.url("/") + "' 2>/dev/null");
    EXPECT_NE(raw.find("Retry-After: 1"), std::string::npos) << raw;
    EXPECT_EQ(status_line_count(raw), 1)
        << "同一连接上响应只应出现一次：" << raw;
}

TEST_F(RequestFilter, FallbackKeepsStatusSetByFilter) {
    LiveServer srv;
    srv.start();
    ASSERT_EQ(
        uvhttp_server_set_request_filter(srv.server(), DenyStatusOnly, nullptr),
        UVHTTP_OK);

    /* filter 设了 429 但没发送：兜底不得把状态改回 403 */
    auto got = fetch(srv.url("/"));
    EXPECT_EQ(code_of(got), "429");
    EXPECT_EQ(g_handler_calls, 0);
}

TEST_F(RequestFilter, FallbackEmitsLegalForbidden) {
    LiveServer srv;
    srv.start();
    ASSERT_EQ(uvhttp_server_set_request_filter(srv.server(),
                                               DenyWithoutResponse, nullptr),
              UVHTTP_OK);

    auto got = fetch(srv.url("/"));
    EXPECT_EQ(code_of(got), "403");

    /* 响应行必须是合法状态行。连接复用时 status_code 被重置为 0，
     * 少了这个哨兵检查就会发出 "HTTP/1.1 0 " */
    auto raw =
        run_curl("curl -s -i --max-time 5 '" + srv.url("/") + "' 2>/dev/null");
    EXPECT_EQ(raw.compare(0, 13, "HTTP/1.1 403 "), 0) << raw;
}

TEST_F(RequestFilter, DenialGatesProtocolUpgrade) {
    LiveServer srv;
    srv.start();
    ASSERT_EQ(uvhttp_server_register_protocol_upgrade(srv.server(), "probe",
                                                      "probe", ProbeDetector,
                                                      RejectUpgrade, nullptr),
              UVHTTP_OK);

    const std::string upgrade = " -H 'Connection: Upgrade' -H 'Upgrade: probe'";

    /* 基线：没有 filter 时 detector 确实被调用（否则下面的断言是假的） */
    auto allowed = fetch(srv.url("/"), upgrade);
    EXPECT_EQ(code_of(allowed), "400") << "detector+handler 走通后框架发 400";
    EXPECT_EQ(g_detector_calls, 1);
    EXPECT_EQ(g_upgrade_handler_calls, 1);

    /* 接缝在探测之前：被拒的请求不得到达用户的 detector */
    g_detector_calls = 0;
    g_upgrade_handler_calls = 0;
    ASSERT_EQ(uvhttp_server_set_request_filter(srv.server(),
                                               DenyWithOwnResponse, nullptr),
              UVHTTP_OK);
    auto denied = fetch(srv.url("/"), upgrade);
    EXPECT_EQ(code_of(denied), "429");
    EXPECT_EQ(g_detector_calls, 0) << "filter 必须先于协议升级探测";
    EXPECT_EQ(g_upgrade_handler_calls, 0);
}

TEST_F(RequestFilter, KeepAliveSurvivesDenial) {
    /* 同一 curl 进程发两个 URL → 复用同一条 TCP 连接。
     * 若拒绝把连接打死，第二个请求拿不到预期码。 */
    LiveServer srv;
    srv.start();
    ASSERT_EQ(uvhttp_server_set_request_filter(srv.server(),
                                               DenyWithoutResponse, nullptr),
              UVHTTP_OK);

    /* 两个 URL 必须各配一个 -o，否则 curl 只把第一份响应丢弃，第二份打到
     * stdout 混进 -w 的输出里 */
    std::string codes =
        run_curl("curl -s --connect-timeout 2 --max-time 5 -o /dev/null "
                 "-o /dev/null -w '%{http_code} ' " +
                 srv.url("/first") + " " + srv.url("/second") + " 2>/dev/null");
    EXPECT_EQ(codes, "403 403 ") << "两次都走 filter 兜底，连接未被拒死";
    EXPECT_EQ(g_filter_calls, 2);

    /* 放开接缝后同一路径仍可服务 */
    ASSERT_EQ(uvhttp_server_set_request_filter(srv.server(), nullptr, nullptr),
              UVHTTP_OK);
    auto got = fetch(srv.url("/third"));
    EXPECT_EQ(code_of(got), "200");
    EXPECT_EQ(body_of(got), "handled");
}
