/**
 * @file test_response_boost_coverage.cpp
 * @brief Coverage boost tests for uvhttp_response module
 *
 * Tests public response API functions by directly manipulating struct fields,
 * focusing on pure functions and state management without network I/O.
 */

#include <gtest/gtest.h>

extern "C" {
#include "uvhttp_allocator.h"
#include "uvhttp_response.h"
}

#include <string.h>

class ResponseBoostTest : public ::testing::Test {
protected:
    uvhttp_response_t* resp = nullptr;

    void SetUp() override {
        resp = (uvhttp_response_t*)uvhttp_calloc(1, sizeof(uvhttp_response_t));
        ASSERT_NE(resp, nullptr);
        resp->headers_capacity = UVHTTP_INLINE_HEADERS_CAPACITY;
        resp->status_code = 200;
        resp->keepalive = 1;
    }

    void TearDown() override {
        if (resp) {
            if (resp->body) {
                uvhttp_free(resp->body);
                resp->body = nullptr;
            }
            if (resp->headers_extra) {
                uvhttp_free(resp->headers_extra);
                resp->headers_extra = nullptr;
            }
            uvhttp_free(resp);
            resp = nullptr;
        }
    }

    void set_body(const char* text) {
        if (resp->body) {
            uvhttp_free(resp->body);
        }
        size_t len = strlen(text);
        resp->body = (char*)uvhttp_alloc(len);
        ASSERT_NE(resp->body, nullptr);
        memcpy(resp->body, text, len);
        resp->body_length = len;
    }
};

// ========== uvhttp_response_init ==========

TEST_F(ResponseBoostTest, Init_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_init(nullptr, (void*)0x1), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, Init_NullClient_ReturnsError) {
    EXPECT_EQ(uvhttp_response_init(resp, nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, Init_Valid_SetsDefaults) {
    uvhttp_response_t local;
    // Use stack address as fake client pointer
    uvhttp_error_t err = uvhttp_response_init(&local, (void*)0x1234);
    EXPECT_EQ(err, UVHTTP_OK);
    EXPECT_EQ(local.status_code, 200);
    EXPECT_EQ(local.keepalive, 1);
    EXPECT_EQ(local.sent, 0);
    EXPECT_EQ(local.finished, 0);
    EXPECT_EQ(local.client, (void*)0x1234);
}

// ========== uvhttp_response_set_status ==========

TEST_F(ResponseBoostTest, SetStatus_Null_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_status(nullptr, 200), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetStatus_BelowMin_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 99), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetStatus_AboveMax_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 600), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetStatus_Valid_SetsCode) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 404), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 404);
}

TEST_F(ResponseBoostTest, SetStatus_200_OK) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 200), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 200);
}

TEST_F(ResponseBoostTest, SetStatus_201_Created) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 201), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 201);
}

TEST_F(ResponseBoostTest, SetStatus_204_NoContent) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 204), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 204);
}

TEST_F(ResponseBoostTest, SetStatus_400_BadRequest) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 400), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 400);
}

TEST_F(ResponseBoostTest, SetStatus_401_Unauthorized) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 401), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 401);
}

TEST_F(ResponseBoostTest, SetStatus_403_Forbidden) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 403), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 403);
}

TEST_F(ResponseBoostTest, SetStatus_500_InternalError) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 500), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 500);
}

TEST_F(ResponseBoostTest, SetStatus_501_NotImplemented) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 501), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 501);
}

TEST_F(ResponseBoostTest, SetStatus_502_BadGateway) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 502), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 502);
}

TEST_F(ResponseBoostTest, SetStatus_503_ServiceUnavailable) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 503), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 503);
}

TEST_F(ResponseBoostTest, SetStatus_405_MethodNotAllowed) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 405), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 405);
}

TEST_F(ResponseBoostTest, SetStatus_100_Continue) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 100), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 100);
}

TEST_F(ResponseBoostTest, SetStatus_599_Max) {
    EXPECT_EQ(uvhttp_response_set_status(resp, 599), UVHTTP_OK);
    EXPECT_EQ(resp->status_code, 599);
}

// ========== uvhttp_response_set_header ==========

TEST_F(ResponseBoostTest, SetHeader_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_header(nullptr, "Name", "Value"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetHeader_NullName_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_header(resp, nullptr, "Value"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetHeader_NullValue_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_header(resp, "Name", nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetHeader_Valid_AddsHeader) {
    EXPECT_EQ(uvhttp_response_set_header(resp, "Content-Type", "text/html"), UVHTTP_OK);
    EXPECT_EQ(resp->header_count, 1u);
}

TEST_F(ResponseBoostTest, SetHeader_ControlChars_ReturnsError) {
    // Header value with control characters (newline) should be rejected
    EXPECT_EQ(uvhttp_response_set_header(resp, "X-Test", "bad\r\nvalue"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetHeader_MultipleHeaders_AllAdded) {
    EXPECT_EQ(uvhttp_response_set_header(resp, "Content-Type", "text/html"), UVHTTP_OK);
    EXPECT_EQ(uvhttp_response_set_header(resp, "X-Custom", "test"), UVHTTP_OK);
    EXPECT_EQ(resp->header_count, 2u);
}

// ========== uvhttp_response_set_body ==========

TEST_F(ResponseBoostTest, SetBody_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_body(nullptr, "body", 4), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetBody_NullBody_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_body(resp, nullptr, 4), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetBody_ZeroLength_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_body(resp, "body", 0), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetBody_Valid_SetsBody) {
    EXPECT_EQ(uvhttp_response_set_body(resp, "hello", 5), UVHTTP_OK);
    EXPECT_NE(resp->body, nullptr);
    EXPECT_EQ(resp->body_length, 5u);
    EXPECT_EQ(memcmp(resp->body, "hello", 5), 0);
}

TEST_F(ResponseBoostTest, SetBody_ReplacesExisting) {
    EXPECT_EQ(uvhttp_response_set_body(resp, "first", 5), UVHTTP_OK);
    EXPECT_EQ(uvhttp_response_set_body(resp, "second", 6), UVHTTP_OK);
    EXPECT_EQ(resp->body_length, 6u);
    EXPECT_EQ(memcmp(resp->body, "second", 6), 0);
}

// ========== uvhttp_response_cleanup ==========

TEST_F(ResponseBoostTest, Cleanup_Null_DoesNotCrash) {
    uvhttp_response_cleanup(nullptr);
}

TEST_F(ResponseBoostTest, Cleanup_WithBody_FreesBody) {
    resp->body = (char*)uvhttp_alloc(64);
    resp->body_length = 64;
    uvhttp_response_cleanup(resp);
    EXPECT_EQ(resp->body, nullptr);
    EXPECT_EQ(resp->body_length, 0u);
}

TEST_F(ResponseBoostTest, Cleanup_WithHeadersExtra_FreesExtra) {
    resp->headers_extra = (uvhttp_header_t*)uvhttp_alloc(128);
    uvhttp_response_cleanup(resp);
    EXPECT_EQ(resp->headers_extra, nullptr);
}

// ========== uvhttp_response_free ==========

TEST_F(ResponseBoostTest, Free_Null_DoesNotCrash) {
    uvhttp_response_free(nullptr);
}

// ========== uvhttp_response_get_header_count ==========

TEST_F(ResponseBoostTest, GetHeaderCount_Null_ReturnsZero) {
    EXPECT_EQ(uvhttp_response_get_header_count(nullptr), 0u);
}

TEST_F(ResponseBoostTest, GetHeaderCount_Empty_ReturnsZero) {
    EXPECT_EQ(uvhttp_response_get_header_count(resp), 0u);
}

TEST_F(ResponseBoostTest, GetHeaderCount_WithHeaders_ReturnsCount) {
    uvhttp_response_set_header(resp, "Host", "example.com");
    uvhttp_response_set_header(resp, "Accept", "text/html");
    EXPECT_EQ(uvhttp_response_get_header_count(resp), 2u);
}

// ========== uvhttp_response_get_header_at ==========

TEST_F(ResponseBoostTest, GetHeaderAt_Null_ReturnsNull) {
    EXPECT_EQ(uvhttp_response_get_header_at(nullptr, 0), nullptr);
}

TEST_F(ResponseBoostTest, GetHeaderAt_OutOfRange_ReturnsNull) {
    EXPECT_EQ(uvhttp_response_get_header_at(resp, 0), nullptr);
}

TEST_F(ResponseBoostTest, GetHeaderAt_Valid_ReturnsHeader) {
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    uvhttp_header_t* h = uvhttp_response_get_header_at(resp, 0);
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(h->name, "Content-Type");
    EXPECT_STREQ(h->value, "application/json");
}

TEST_F(ResponseBoostTest, GetHeaderAt_DynamicExpansion_ReturnsHeader) {
    // Fill inline capacity
    for (int i = 0; i < UVHTTP_INLINE_HEADERS_CAPACITY; i++) {
        char name[32], value[32];
        snprintf(name, sizeof(name), "H%d", i);
        snprintf(value, sizeof(value), "V%d", i);
        EXPECT_EQ(uvhttp_response_set_header(resp, name, value), UVHTTP_OK);
    }
    // Add one more to trigger expansion
    EXPECT_EQ(uvhttp_response_set_header(resp, "Extra", "Value"), UVHTTP_OK);

    uvhttp_header_t* h = uvhttp_response_get_header_at(resp, UVHTTP_INLINE_HEADERS_CAPACITY);
    ASSERT_NE(h, nullptr);
    EXPECT_STREQ(h->name, "Extra");
    EXPECT_STREQ(h->value, "Value");
}

// ========== uvhttp_response_foreach_header ==========

struct RespHeaderCollector {
    int count;
    char names[32][64];
    char values[32][256];
};

static void collect_resp_header(const char* name, const char* value, void* user_data) {
    auto* col = static_cast<RespHeaderCollector*>(user_data);
    if (col->count < 32) {
        strncpy(col->names[col->count], name, sizeof(col->names[0]) - 1);
        col->names[col->count][sizeof(col->names[0]) - 1] = '\0';
        strncpy(col->values[col->count], value, sizeof(col->values[0]) - 1);
        col->values[col->count][sizeof(col->values[0]) - 1] = '\0';
        col->count++;
    }
}

TEST_F(ResponseBoostTest, ForeachHeader_NullResponse_DoesNotCrash) {
    RespHeaderCollector col = {0};
    uvhttp_response_foreach_header(nullptr, collect_resp_header, &col);
    EXPECT_EQ(col.count, 0);
}

TEST_F(ResponseBoostTest, ForeachHeader_NullCallback_DoesNotCrash) {
    uvhttp_response_set_header(resp, "Host", "example.com");
    uvhttp_response_foreach_header(resp, nullptr, nullptr);
}

TEST_F(ResponseBoostTest, ForeachHeader_IteratesAll) {
    uvhttp_response_set_header(resp, "Host", "example.com");
    uvhttp_response_set_header(resp, "Accept", "text/html");
    uvhttp_response_set_header(resp, "Connection", "keep-alive");

    RespHeaderCollector col = {0};
    uvhttp_response_foreach_header(resp, collect_resp_header, &col);
    EXPECT_EQ(col.count, 3);
    EXPECT_STREQ(col.names[0], "Host");
    EXPECT_STREQ(col.values[0], "example.com");
}

// ========== uvhttp_response_build_data (pure function) ==========

TEST_F(ResponseBoostTest, BuildData_NullResponse_ReturnsError) {
    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(nullptr, &data, &len), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, BuildData_NullOutData_ReturnsError) {
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, nullptr, &len), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, BuildData_NullOutLength_ReturnsError) {
    char* data = nullptr;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, BuildData_AlreadySent_ReturnsEmpty) {
    resp->sent = 1;
    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    EXPECT_EQ(data, nullptr);
    EXPECT_EQ(len, 0u);
}

TEST_F(ResponseBoostTest, BuildData_Valid_ProducesHTTPResponse) {
    resp->status_code = 200;
    uvhttp_response_set_header(resp, "Content-Type", "text/plain");
    set_body("Hello, World!");

    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_GT(len, 0u);

    // Verify it starts with HTTP/1.1 200
    EXPECT_NE(strstr(data, "HTTP/1.1 200 OK"), nullptr);
    // Verify body is included
    EXPECT_NE(strstr(data, "Hello, World!"), nullptr);

    uvhttp_free(data);
}

TEST_F(ResponseBoostTest, BuildData_NoBody_IncludesContentLengthZero) {
    resp->status_code = 204;
    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_NE(strstr(data, "Content-Length: 0"), nullptr);
    uvhttp_free(data);
}

TEST_F(ResponseBoostTest, BuildData_KeepAlive_IncludesKeepAliveHeader) {
    resp->keepalive = 1;
    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_NE(strstr(data, "Connection: keep-alive"), nullptr);
    uvhttp_free(data);
}

TEST_F(ResponseBoostTest, BuildData_CloseConnection_IncludesCloseHeader) {
    resp->keepalive = 0;
    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_NE(strstr(data, "Connection: close"), nullptr);
    uvhttp_free(data);
}

TEST_F(ResponseBoostTest, BuildData_CustomContentType_UsedInHeaders) {
    uvhttp_response_set_header(resp, "Content-Type", "application/json");
    set_body("{\"ok\":true}");

    char* data = nullptr;
    size_t len = 0;
    EXPECT_EQ(uvhttp_response_build_data(resp, &data, &len), UVHTTP_OK);
    ASSERT_NE(data, nullptr);
    EXPECT_NE(strstr(data, "Content-Type: application/json"), nullptr);
    uvhttp_free(data);
}

// ========== Compression helper functions ==========
#if UVHTTP_FEATURE_COMPRESSION

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Null_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension(nullptr), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Empty_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension(""), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_NoDot_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("nodot"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Html_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("index.html"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Htm_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("page.htm"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Css_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("style.css"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Js_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("app.js"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Json_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("data.json"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Xml_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("feed.xml"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Txt_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("readme.txt"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Md_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("README.md"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Jpg_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("photo.jpg"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Png_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("image.png"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Mp4_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("video.mp4"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Zip_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("archive.zip"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Exe_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("program.exe"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Pdf_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("document.pdf"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Svg_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("icon.svg"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_Unknown_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("file.xyz123"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByExtension_PathWithDir_Works) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("/var/www/index.html"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("/var/www/image.jpg"), 0);
}

// ========== Content type compression helpers ==========

TEST_F(ResponseBoostTest, ShouldCompressByContentType_Null_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type(nullptr), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_Empty_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type(""), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_TextHtml_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/html"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_TextPlain_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/plain"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_TextCss_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/css"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationJson_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/json"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationXml_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/xml"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationJavascript_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/javascript"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationXhtml_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/xhtml+xml"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationRss_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/rss+xml"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationAtom_ReturnsOne) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/atom+xml"), 1);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ImageJpeg_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("image/jpeg"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ImagePng_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("image/png"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_VideoMp4_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("video/mp4"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_AudioMpeg_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("audio/mpeg"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationZip_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/zip"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationGzip_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/gzip"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationXGzip_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/x-gzip"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationXCompressed_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/x-compressed"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationPdf_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/pdf"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_ApplicationVnd_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/vnd.ms-excel"), 0);
}

TEST_F(ResponseBoostTest, ShouldCompressByContentType_Unknown_ReturnsZero) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/octet-stream"), 0);
}

// ========== Compression API ==========

TEST_F(ResponseBoostTest, SetCompressByFilename_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_compress_by_filename(nullptr, "test.html"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompressByFilename_Compressible_EnablesCompress) {
    EXPECT_EQ(uvhttp_response_set_compress_by_filename(resp, "index.html"), UVHTTP_OK);
    EXPECT_EQ(resp->compress, 1);
    EXPECT_EQ(resp->compress_threshold, 1024);
}

TEST_F(ResponseBoostTest, SetCompressByFilename_NonCompressible_DisablesCompress) {
    resp->compress = 1;
    EXPECT_EQ(uvhttp_response_set_compress_by_filename(resp, "image.jpg"), UVHTTP_OK);
    EXPECT_EQ(resp->compress, 0);
}

TEST_F(ResponseBoostTest, SetCompressByContentType_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_compress_by_content_type(nullptr, "text/html"), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompressByContentType_Compressible_EnablesCompress) {
    EXPECT_EQ(uvhttp_response_set_compress_by_content_type(resp, "text/html"), UVHTTP_OK);
    EXPECT_EQ(resp->compress, 1);
    EXPECT_EQ(resp->compress_threshold, 1024);
}

TEST_F(ResponseBoostTest, SetCompressByContentType_NonCompressible_DisablesCompress) {
    resp->compress = 1;
    EXPECT_EQ(uvhttp_response_set_compress_by_content_type(resp, "image/jpeg"), UVHTTP_OK);
    EXPECT_EQ(resp->compress, 0);
}

TEST_F(ResponseBoostTest, SetCompress_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_compress(nullptr, 1), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompress_Enable_SetsFields) {
    EXPECT_EQ(uvhttp_response_set_compress(resp, 1), UVHTTP_OK);
    EXPECT_EQ(resp->compress, 1);
    EXPECT_EQ(resp->compress_threshold, 1024);
}

TEST_F(ResponseBoostTest, SetCompress_Disable_ResetsFields) {
    resp->compress = 1;
    resp->compress_algorithm = 1;
    resp->compress_threshold = 2048;
    EXPECT_EQ(uvhttp_response_set_compress(resp, 0), UVHTTP_OK);
    EXPECT_EQ(resp->compress, 0);
    EXPECT_EQ(resp->compress_algorithm, 0);
    EXPECT_EQ(resp->compress_threshold, 0);
}

TEST_F(ResponseBoostTest, SetCompress_Algorithm_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(nullptr, 1), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompress_Algorithm_NotEnabled_ReturnsError) {
    resp->compress = 0;
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(resp, 1), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompress_Algorithm_InvalidRange_ReturnsError) {
    resp->compress = 1;
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(resp, -1), UVHTTP_ERROR_INVALID_PARAM);
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(resp, 2), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompress_Algorithm_Valid_SetsValue) {
    resp->compress = 1;
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(resp, 0), UVHTTP_OK);
    EXPECT_EQ(resp->compress_algorithm, 0);
    EXPECT_EQ(uvhttp_response_set_compress_algorithm(resp, 1), UVHTTP_OK);
    EXPECT_EQ(resp->compress_algorithm, 1);
}

TEST_F(ResponseBoostTest, SetCompress_Threshold_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_compress_threshold(nullptr, 1024), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompress_Threshold_TooLarge_ReturnsError) {
    EXPECT_EQ(uvhttp_response_set_compress_threshold(resp, UVHTTP_MAX_BODY_SIZE + 1), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, SetCompress_Threshold_Valid_SetsValue) {
    EXPECT_EQ(uvhttp_response_set_compress_threshold(resp, 2048), UVHTTP_OK);
    EXPECT_EQ(resp->compress_threshold, 2048);
}

#endif /* UVHTTP_FEATURE_COMPRESSION */

// ========== Response send (null check only) ==========

TEST_F(ResponseBoostTest, Send_NullResponse_ReturnsError) {
    EXPECT_EQ(uvhttp_response_send(nullptr), UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(ResponseBoostTest, Send_AlreadySent_ReturnsOK) {
    resp->sent = 1;
    EXPECT_EQ(uvhttp_response_send(resp), UVHTTP_OK);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
