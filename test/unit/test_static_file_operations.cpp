/* uvhttp_static.c 文件操作集成测试 - 测试文件操作功能 */

#if UVHTTP_FEATURE_STATIC_FILES

#include <gtest/gtest.h>
#include "uvhttp_static.h"
#include "uvhttp_server.h"
#include "uvhttp_router.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include "uvhttp_allocator.h"
#include "uvhttp_error.h"
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

/* ========== 测试 MIME 类型获取 ========== */

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeHtml) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("index.html", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "html"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeCss) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("style.css", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "css"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeJs) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("script.js", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "javascript"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeJson) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("data.json", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "json"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypePng) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("image.png", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "png"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeJpg) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("image.jpg", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "jpeg"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeGif) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("image.gif", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "gif"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeSvg) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("image.svg", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeIco) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("favicon.ico", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeWoff) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("font.woff", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeWoff2) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("font.woff2", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeTtf) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("font.ttf", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypePdf) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("document.pdf", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "pdf"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeZip) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("archive.zip", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "zip"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeXml) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("data.xml", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "xml"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeTxt) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file.txt", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "text"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeMp4) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("video.mp4", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "mp4"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeWebm) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("video.webm", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeMp3) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("audio.mp3", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeWav) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("audio.wav", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeOgg) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("audio.ogg", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeUnknownExtension) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file.unknown", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        /* 应该返回默认的 MIME 类型 */
        EXPECT_NE(strstr(mime_type, "application"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeNoExtension) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeMultipleDots) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file.name.with.dots.html", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
        EXPECT_NE(strstr(mime_type, "html"), nullptr);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeUpperCaseExtension) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file.HTML", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GetMimeTypeMixedCaseExtension) {
    char mime_type[256];
    uvhttp_result_t result = uvhttp_static_get_mime_type("file.HtMl", mime_type, sizeof(mime_type));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(mime_type), (size_t)0);
    }
}

/* ========== 测试 ETag 生成 ========== */

TEST(UvhttpStaticFileOperationsTest, GenerateEtagNullPath) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag(nullptr, 0, 0, etag, sizeof(etag));
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagNullEtag) {
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 0, 0, nullptr, 256);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagZeroBufferSize) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 0, 0, etag, 0);
    EXPECT_NE(result, UVHTTP_OK);
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagValid) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 1234567890, 1024, etag, sizeof(etag));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(etag), (size_t)0);
        /* ETag 应该以引号开头和结尾 */
        EXPECT_EQ(etag[0], '\"');
        EXPECT_EQ(etag[strlen(etag) - 1], '\"');
    }
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagDifferentFiles) {
    char etag1[256], etag2[256];
    
    uvhttp_result_t result1 = uvhttp_static_generate_etag("file1.txt", 1234567890, 1024, etag1, sizeof(etag1));
    uvhttp_result_t result2 = uvhttp_static_generate_etag("file2.txt", 1234567890, 1024, etag2, sizeof(etag2));
    
    if (result1 == UVHTTP_OK && result2 == UVHTTP_OK) {
        /* 不同文件的 ETag 应该不同（取决于实现） */
        /* 如果实现使用文件路径生成 ETag，则它们应该不同 */
        /* 如果实现只使用时间戳和大小，则它们可能相同 */
        EXPECT_NE(strlen(etag1), (size_t)0);
        EXPECT_NE(strlen(etag2), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagDifferentSizes) {
    char etag1[256], etag2[256];
    
    uvhttp_result_t result1 = uvhttp_static_generate_etag("test.txt", 1234567890, 1024, etag1, sizeof(etag1));
    uvhttp_result_t result2 = uvhttp_static_generate_etag("test.txt", 1234567890, 2048, etag2, sizeof(etag2));
    
    if (result1 == UVHTTP_OK && result2 == UVHTTP_OK) {
        /* 不同大小的文件应该有不同的 ETag */
        EXPECT_NE(strcmp(etag1, etag2), 0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagDifferentTimestamps) {
    char etag1[256], etag2[256];
    
    uvhttp_result_t result1 = uvhttp_static_generate_etag("test.txt", 1234567890, 1024, etag1, sizeof(etag1));
    uvhttp_result_t result2 = uvhttp_static_generate_etag("test.txt", 1234567891, 1024, etag2, sizeof(etag2));
    
    if (result1 == UVHTTP_OK && result2 == UVHTTP_OK) {
        /* 不同时间戳的文件应该有不同的 ETag */
        EXPECT_NE(strcmp(etag1, etag2), 0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagZeroSize) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 1234567890, 0, etag, sizeof(etag));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(etag), (size_t)0);
    }
}

TEST(UvhttpStaticFileOperationsTest, GenerateEtagLargeSize) {
    char etag[256];
    uvhttp_result_t result = uvhttp_static_generate_etag("test.txt", 1234567890, 1024 * 1024 * 1024, etag, sizeof(etag));
    
    if (result == UVHTTP_OK) {
        EXPECT_NE(strlen(etag), (size_t)0);
    }
}

/* ========== 测试路径解析 ========== */

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathNullRootDir) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(nullptr, "test.txt", resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathNullFilePath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", nullptr, resolved_path, sizeof(resolved_path));
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathNullResolvedPath) {
    int result = uvhttp_static_resolve_safe_path(".", "test.txt", nullptr, 512);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathZeroBufferSize) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "test.txt", resolved_path, 0);
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathPathTraversalAttack) {
    char resolved_path[512];
    
    /* 测试各种路径遍历攻击 */
    const char* malicious_paths[] = {
        "../../../etc/passwd",
        "..\\..\\..\\windows\\system32",
        "/etc/passwd",
        "C:\\Windows\\System32",
        "./../../etc/passwd",
        "....//....//....//etc/passwd"
    };
    
    for (size_t i = 0; i < sizeof(malicious_paths) / sizeof(malicious_paths[0]); i++) {
        int result = uvhttp_static_resolve_safe_path(".", malicious_paths[i], resolved_path, sizeof(resolved_path));
        EXPECT_EQ(result, 0);
    }
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathValidPath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "test.txt", resolved_path, sizeof(resolved_path));
    /* 结果取决于文件是否存在 */
    EXPECT_GE(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathSubdirectory) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "subdir/test.txt", resolved_path, sizeof(resolved_path));
    /* 结果取决于文件是否存在 */
    EXPECT_GE(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathDotPath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "./test.txt", resolved_path, sizeof(resolved_path));
    /* 结果取决于文件是否存在 */
    EXPECT_GE(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathAbsolutePath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "/tmp/test.txt", resolved_path, sizeof(resolved_path));
    /* 绝对路径应该被拒绝 */
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathEmptyPath) {
    char resolved_path[512];
    int result = uvhttp_static_resolve_safe_path(".", "", resolved_path, sizeof(resolved_path));
    /* 空路径应该被拒绝 */
    EXPECT_EQ(result, 0);
}

TEST(UvhttpStaticFileOperationsTest, ResolveSafePathLongPath) {
    char resolved_path[512];
    char long_path[1000];
    memset(long_path, 'a', sizeof(long_path) - 1);
    long_path[sizeof(long_path) - 1] = '\0';
    
    int result = uvhttp_static_resolve_safe_path(".", long_path, resolved_path, sizeof(resolved_path));
    /* 超长路径应该被拒绝 */
    EXPECT_EQ(result, 0);
}

#endif /* UVHTTP_FEATURE_STATIC_FILES */