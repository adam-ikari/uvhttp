/**
 * @file test_compression_helpers.cpp
 * @brief Test compression helper functions
 */

#include <gtest/gtest.h>
#include <uvhttp.h>
#include <string>

class CompressionHelpersTest : public ::testing::Test {
protected:
    void SetUp() override {
        uvhttp_response_init(&response, NULL);
    }

    void TearDown() override {
        uvhttp_response_cleanup(&response);
    }

    uvhttp_response_t response;
};

/* ========== uvhttp_should_compress_by_extension Tests ========== */

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_TextFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("index.html"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("style.css"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("app.js"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("data.json"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("config.xml"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("README.md"), 1);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_ScriptFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("server.php"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("script.py"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("app.rb"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("script.pl"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("start.sh"), 1);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_ConfigFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("config.ini"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("settings.yaml"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("app.conf"), 1);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_ImageFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("photo.jpg"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("image.png"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("logo.gif"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("banner.webp"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("icon.ico"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_VideoFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("movie.mp4"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("video.avi"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("clip.mkv"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_AudioFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("song.mp3"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("audio.wav"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("music.flac"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_ArchiveFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("data.zip"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("archive.tar.gz"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("file.7z"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_BinaryFile) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("app.exe"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("lib.so"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("data.bin"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_CaseInsensitive) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("INDEX.HTML"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("Style.CSS"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("App.JS"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_extension("Photo.JPG"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("Image.PNG"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_NoExtension) {
    EXPECT_EQ(uvhttp_should_compress_by_extension("Makefile"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension("README"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension(".gitignore"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByExtension_NullFilename) {
    EXPECT_EQ(uvhttp_should_compress_by_extension(NULL), 0);
    EXPECT_EQ(uvhttp_should_compress_by_extension(""), 0);
}

/* ========== uvhttp_should_compress_by_content_type Tests ========== */

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_TextType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/html"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/plain"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/css"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("text/javascript"), 1);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_ApplicationType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/json"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/xml"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/javascript"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/xhtml+xml"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/rss+xml"), 1);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_ImageType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("image/jpeg"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("image/png"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("image/gif"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("image/webp"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_VideoType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("video/mp4"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("video/avi"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("video/webm"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_AudioType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("audio/mpeg"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("audio/wav"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("audio/ogg"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_ArchiveType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/zip"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/gzip"), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("application/x-gzip"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_CaseInsensitive) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type("TEXT/HTML"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("APPLICATION/JSON"), 1);
    EXPECT_EQ(uvhttp_should_compress_by_content_type("IMAGE/JPEG"), 0);
}

TEST_F(CompressionHelpersTest, ShouldCompressByContentType_NullContentType) {
    EXPECT_EQ(uvhttp_should_compress_by_content_type(NULL), 0);
    EXPECT_EQ(uvhttp_should_compress_by_content_type(""), 0);
}

/* ========== uvhttp_response_set_compress_by_filename Tests ========== */

TEST_F(CompressionHelpersTest, SetCompressByFilename_CompressibleFile) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_filename(&response, "index.html");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    EXPECT_EQ(response.compress_threshold, 1024);
}

TEST_F(CompressionHelpersTest, SetCompressByFilename_NonCompressibleFile) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_filename(&response, "photo.jpg");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 0);
}

TEST_F(CompressionHelpersTest, SetCompressByFilename_JsonFile) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_filename(&response, "data.json");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    EXPECT_EQ(response.compress_threshold, 1024);
}

TEST_F(CompressionHelpersTest, SetCompressByFilename_NullResponse) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_filename(NULL, "index.html");
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(CompressionHelpersTest, SetCompressByFilename_WithExistingThreshold) {
    /* 先设置阈值 */
    response.compress_threshold = 2048;
    
    uvhttp_error_t result = uvhttp_response_set_compress_by_filename(&response, "index.html");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    /* 阈值应该保持不变 */
    EXPECT_EQ(response.compress_threshold, 2048);
}

/* ========== uvhttp_response_set_compress_by_content_type Tests ========== */

TEST_F(CompressionHelpersTest, SetCompressByContentType_CompressibleType) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_content_type(&response, "text/html");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    EXPECT_EQ(response.compress_threshold, 1024);
}

TEST_F(CompressionHelpersTest, SetCompressByContentType_NonCompressibleType) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_content_type(&response, "image/jpeg");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 0);
}

TEST_F(CompressionHelpersTest, SetCompressByContentType_JsonType) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_content_type(&response, "application/json");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    EXPECT_EQ(response.compress_threshold, 1024);
}

TEST_F(CompressionHelpersTest, SetCompressByContentType_NullResponse) {
    uvhttp_error_t result = uvhttp_response_set_compress_by_content_type(NULL, "text/html");
    EXPECT_EQ(result, UVHTTP_ERROR_INVALID_PARAM);
}

TEST_F(CompressionHelpersTest, SetCompressByContentType_WithExistingThreshold) {
    /* 先设置阈值 */
    response.compress_threshold = 2048;
    
    uvhttp_error_t result = uvhttp_response_set_compress_by_content_type(&response, "text/html");
    EXPECT_EQ(result, UVHTTP_OK);
    EXPECT_EQ(response.compress, 1);
    /* 阈值应该保持不变 */
    EXPECT_EQ(response.compress_threshold, 2048);
}

/* ========== Integration Tests ========== */

TEST_F(CompressionHelpersTest, Integration_CompressibleHtmlFile) {
    const char* html_content = "<!DOCTYPE html><html><body><h1>Hello</h1></body></html>";
    
    /* 设置响应体 */
    uvhttp_response_set_body(&response, html_content, strlen(html_content));
    
    /* 根据文件名自动设置压缩 */
    uvhttp_response_set_compress_by_filename(&response, "index.html");
    
    /* 验证压缩已启用 */
    EXPECT_EQ(response.compress, 1);
    EXPECT_EQ(response.compress_threshold, 1024);
}

TEST_F(CompressionHelpersTest, Integration_NonCompressibleImageFile) {
    /* 模拟图片数据 */
    const char* image_data = "\x89PNG\r\n\x1a\n";
    
    /* 设置响应体 */
    uvhttp_response_set_body(&response, image_data, 8);
    
    /* 根据文件名自动设置压缩 */
    uvhttp_response_set_compress_by_filename(&response, "photo.png");
    
    /* 验证压缩已禁用 */
    EXPECT_EQ(response.compress, 0);
}

TEST_F(CompressionHelpersTest, Integration_JsonApi) {
    std::string json_body = "{\"status\":\"ok\",\"data\":[{\"id\":1,\"name\":\"test\"}]}";
    
    /* 设置响应体 */
    uvhttp_response_set_body(&response, json_body.c_str(), json_body.length());
    
    /* 根据内容类型自动设置压缩 */
    uvhttp_response_set_compress_by_content_type(&response, "application/json");
    
    /* 验证压缩已启用 */
    EXPECT_EQ(response.compress, 1);
    EXPECT_EQ(response.compress_threshold, 1024);
}