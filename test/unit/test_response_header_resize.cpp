/*
 * UVHTTP 响应头缓冲区扩容测试
 *
 * 为什么需要独立测试：uvhttp_response_prepare 的 header 缓冲区扩容分支
 * （src/uvhttp_response.c:733-745）：
 *
 *     size_t headers_size = UVHTTP_INITIAL_BUFFER_SIZE * 2;   // 8192*2 = 16384
 *     build_response_headers(response, headers_buffer, &headers_length);
 *     if (headers_length >= headers_size) {
 *         free(headers_buffer);
 *         headers_size = headers_length + UVHTTP_RESPONSE_HEADER_SAFETY_MARGIN;
 *         headers_buffer = alloc(headers_size);
 *         headers_length = headers_size;
 *         build_response_headers(response, headers_buffer, &headers_length); //
 * 重写
 *     }
 *
 * 该分支的特别之处在于 SNAPPEND 宏（src/uvhttp_response.c:200）：
 *
 *     size_t _rem = (pos < *length) ? (*length - pos) : 0;
 *     pos += snprintf(buffer + pos, _rem, fmt, ...);
 *
 * snprintf 溢出时不写入但**仍返回应写入的长度**，所以 pos 会继续增长并
 * 超过缓冲容量——这正是 headers_length >= headers_size 的检测依据。
 * 扩容后必须重新构建一次 header，否则先前溢出的部分永久丢失。
 *
 * 现有测试的 header 数量/长度都不足以超过 16384 字节，该分支从未被执行。
 */

#include <gtest/gtest.h>
#include <string.h>
#include <string>
#include <vector>

extern "C" {
#include "uvhttp_response.h"
}

class HeaderBufferResize : public ::testing::Test {
   protected:
    uvhttp_response_t res_{};

    void SetUp() override {
        memset(&res_, 0, sizeof(res_));
        ASSERT_EQ(uvhttp_response_init(&res_, (void*)0x1234), UVHTTP_OK);
        ASSERT_EQ(uvhttp_response_set_status(&res_, 200), UVHTTP_OK);
    }

    void TearDown() override {
        uvhttp_response_cleanup(&res_);
    }

    /* 添加 n 个各约 value_size 字节的自定义 header */
    void AddHeaders(int n, size_t value_size) {
        std::string value(value_size, 'v');
        for (int i = 0; i < n; i++) {
            char name[32];
            snprintf(name, sizeof(name), "X-Probe-%d", i);
            ASSERT_EQ(uvhttp_response_set_header(&res_, name, value.c_str()),
                      UVHTTP_OK)
                << "第 " << i << " 个 header 设置失败";
        }
    }

    /* 构建并返回完整响应数据（失败时返回空串，由调用方断言） */
    std::string Build() {
        char* data = nullptr;
        size_t len = 0;
        if (uvhttp_response_build_data(&res_, &data, &len) != UVHTTP_OK ||
            data == nullptr) {
            return std::string();
        }
        std::string out(data, len);
        free(data);
        return out;
    }
};

/* 初始 header 缓冲 = UVHTTP_INITIAL_BUFFER_SIZE(8192) * 2 = 16384 字节 */
static constexpr size_t kInitialBuffer = 16384;

/* ========== 不触发扩容（对照组） ========== */

/* 5 个 2000 字节 header ≈ 10.1KB < 16384 → 不扩容，输出应完整 */
TEST_F(HeaderBufferResize, BelowThresholdDoesNotNeedResize) {
    AddHeaders(5, 2000);
    std::string out = Build();
    EXPECT_GT(out.size(), 9000u);
    EXPECT_LT(out.size(), kInitialBuffer) << "此用例不应触发扩容";
    /* header 内容必须完整 */
    EXPECT_NE(out.find("X-Probe-0:"), std::string::npos);
    EXPECT_NE(out.find("X-Probe-4:"), std::string::npos);
}

/* ========== 触发扩容 ========== */

/* 10 个 2000 字节 header ≈ 20.1KB > 16384 → 触发扩容 */
TEST_F(HeaderBufferResize, ExceedingInitialBufferTriggersResize) {
    AddHeaders(10, 2000);
    std::string out = Build();
    EXPECT_GE(out.size(), kInitialBuffer) << "应超过初始缓冲以触发扩容";
}

/* 扩容分支的核心断言：重写后所有 header 都在，无截断丢失。
 * 若扩容后未重新 build_response_headers，先前溢出的 header 会永久丢失。 */
TEST_F(HeaderBufferResize, AllHeadersPresentAfterResize) {
    AddHeaders(10, 2000);
    std::string out = Build();

    /* 每个 header 的名字与完整 value 都必须在输出中 */
    for (int i = 0; i < 10; i++) {
        char name[32];
        snprintf(name, sizeof(name), "X-Probe-%d", i);
        std::string needle = std::string(name) + ": ";
        size_t pos = out.find(needle);
        ASSERT_NE(pos, std::string::npos) << "扩容后 header 丢失：" << name;
        /* value 应为 2000 个 'v'，确认未被截断 */
        size_t vstart = pos + needle.size();
        size_t vcount = 0;
        while (vstart + vcount < out.size() && out[vstart + vcount] == 'v') {
            vcount++;
        }
        EXPECT_EQ(vcount, 2000u)
            << name << " 的 value 被截断（长度 " << vcount << "，期望 2000）";
    }
}

/* ========== 极端扩容：单个 header 就超阈值 ========== */

/* 单个 header 的 value 接近 header
 * 值上限（UVHTTP_MAX_HEADER_VALUE_SIZE=4096）， 多个叠加后远超
 * 16384，验证深度扩容路径同样正确 */
TEST_F(HeaderBufferResize, DeepResizeKeepsStandardHeadersIntact) {
    AddHeaders(30, 2000);
    std::string out = Build();
    EXPECT_GT(out.size(), 60000u) << "20×4000 应远超初始缓冲";

    /* 标准头必须在——它们由 build_response_headers 统一生成，
     * 扩容重写时不能丢失 */
    EXPECT_NE(out.find("Content-Length:"), std::string::npos)
        << "扩容后标准头 Content-Length 丢失";
    EXPECT_NE(out.find("\r\n\r\n"), std::string::npos)
        << "header/body 分隔符丢失，响应结构损坏";
}

/* ========== 状态行正确性 ========== */

/* 扩容后状态行仍须正确（重写是完整重建，不是追加） */
TEST_F(HeaderBufferResize, StatusLineCorrectAfterResize) {
    ASSERT_EQ(uvhttp_response_set_status(&res_, 404), UVHTTP_OK);
    AddHeaders(10, 2000);
    std::string out = Build();
    EXPECT_EQ(out.compare(0, 12, "HTTP/1.1 404"), 0)
        << "扩容后状态行错误：" << out.substr(0, 20);
}

/* ========== 边界：恰好接近阈值 ========== */

/* 逐渐增加 header 数量，找到触发扩容的临界点，验证判定用的是 >= 而非 >
 * （SNAPPEND 的 pos 记账使 headers_length 可能恰等于 headers_size） */
TEST_F(HeaderBufferResize, ResizeTriggeredAtOrAboveThreshold) {
    /* 逐步逼近阈值，记录输出长度随 header 数量单调增长——
     * 这保证扩容不会截断任何内容（无论是否跨过阈值） */
    size_t prev = 0;
    for (int n : {4, 6, 9, 12}) {
        memset(&res_, 0, sizeof(res_));
        uvhttp_response_init(&res_, (void*)0x1234);
        uvhttp_response_set_status(&res_, 200);
        AddHeaders(n, 2000);
        std::string out = Build();
        EXPECT_GT(out.size(), prev) << "header 增加后输出应增长";
        EXPECT_NE(out.find("X-Probe-0:"), std::string::npos)
            << "n=" << n << " 时首个 header 丢失";
        prev = out.size();
        uvhttp_response_cleanup(&res_);
    }
}