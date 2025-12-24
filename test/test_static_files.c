/*
 * UVHTTP 静态文件服务测试
 */

#include "uvhttp_test_framework.h"
#include "uvhttp_static.h"
#include "uvhttp_request.h"
#include "uvhttp_response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* 测试用的静态文件服务上下文 */
static uvhttp_static_context_t* test_ctx = NULL;

/* 测试文件路径 */
static const char* test_file_path = "./test_data/test.txt";
static const char* test_content = "Hello, UVHTTP Static File Server!";

/**
 * 测试设置：创建测试文件和上下文
 */
int setup_static_tests() {
    /* 创建测试目录 */
    system("mkdir -p ./test_data");
    
    /* 创建测试文件 */
    FILE* file = fopen(test_file_path, "w");
    if (!file) {
        printf("Failed to create test file\n");
        return -1;
    }
    fprintf(file, "%s", test_content);
    fclose(file);
    
    /* 配置静态文件服务 */
    uvhttp_static_config_t config = {
        .root_directory = "./test_data",
        .index_file = "index.html",
        .enable_directory_listing = 0,
        .enable_etag = 1,
        .enable_last_modified = 1,
        .max_cache_size = 1024 * 1024,  /* 1MB */
        .cache_ttl = 60,                /* 1分钟 */
        .custom_headers = ""
    };
    
    /* 创建静态文件服务上下文 */
    test_ctx = uvhttp_static_create(&config);
    if (!test_ctx) {
        printf("Failed to create static context\n");
        return -1;
    }
    
    return 0;
}

/**
 * 测试清理
 */
void cleanup_static_tests() {
    if (test_ctx) {
        uvhttp_static_free(test_ctx);
        test_ctx = NULL;
    }
    
    /* 删除测试文件 */
    unlink(test_file_path);
    rmdir("./test_data");
}

/**
 * 测试MIME类型检测
 */
int test_mime_type_detection() {
    printf("Testing MIME type detection...\n");
    
    char mime_type[256];
    
    /* 测试HTML文件 */
    TEST_ASSERT_EQ(0, uvhttp_static_get_mime_type("test.html", mime_type, sizeof(mime_type)));
    TEST_ASSERT_STR_EQ("text/html", mime_type);
    
    /* 测试CSS文件 */
    TEST_ASSERT_EQ(0, uvhttp_static_get_mime_type("style.css", mime_type, sizeof(mime_type)));
    TEST_ASSERT_STR_EQ("text/css", mime_type);
    
    /* 测试JavaScript文件 */
    TEST_ASSERT_EQ(0, uvhttp_static_get_mime_type("script.js", mime_type, sizeof(mime_type)));
    TEST_ASSERT_STR_EQ("application/javascript", mime_type);
    
    /* 测试PNG图片 */
    TEST_ASSERT_EQ(0, uvhttp_static_get_mime_type("image.png", mime_type, sizeof(mime_type)));
    TEST_ASSERT_STR_EQ("image/png", mime_type);
    
    /* 测试未知文件类型 */
    TEST_ASSERT_EQ(0, uvhttp_static_get_mime_type("unknown.xyz", mime_type, sizeof(mime_type)));
    TEST_ASSERT_STR_EQ("application/octet-stream", mime_type);
    
    printf("✅ MIME type detection tests passed\n");
    return 0;
}

/**
 * 测试路径安全验证
 */
int test_path_security() {
    printf("Testing path security validation...\n");
    
    char resolved_path[512];
    
    /* 测试正常路径 */
    TEST_ASSERT_EQ(1, uvhttp_static_resolve_safe_path("./test_data", "test.txt", 
                                                   resolved_path, sizeof(resolved_path)));
    TEST_ASSERT_STR_EQ("./test_data/test.txt", resolved_path);
    
    /* 测试路径遍历攻击 */
    TEST_ASSERT_EQ(0, uvhttp_static_resolve_safe_path("./test_data", "../etc/passwd", 
                                                   resolved_path, sizeof(resolved_path)));
    
    /* 测试绝对路径 */
    TEST_ASSERT_EQ(0, uvhttp_static_resolve_safe_path("./test_data", "/etc/passwd", 
                                                   resolved_path, sizeof(resolved_path)));
    
    /* 测试空路径 */
    TEST_ASSERT_EQ(1, uvhttp_static_resolve_safe_path("./test_data", "", 
                                                   resolved_path, sizeof(resolved_path)));
    TEST_ASSERT_STR_EQ("./test_data/", resolved_path);
    
    printf("✅ Path security validation tests passed\n");
    return 0;
}

/**
 * 测试ETag生成
 */
int test_etag_generation() {
    printf("Testing ETag generation...\n");
    
    char etag[256];
    time_t now = time(NULL);
    
    /* 测试ETag生成 */
    TEST_ASSERT_EQ(0, uvhttp_static_generate_etag("test.txt", now, 1024, 
                                                  etag, sizeof(etag)));
    
    /* 验证ETag格式（应该被引号包围） */
    TEST_ASSERT_EQ('"', etag[0]);
    TEST_ASSERT_EQ('"', etag[strlen(etag) - 1]);
    
    /* 测试相同参数生成相同ETag */
    char etag2[256];
    TEST_ASSERT_EQ(0, uvhttp_static_generate_etag("test.txt", now, 1024, 
                                                  etag2, sizeof(etag2)));
    TEST_ASSERT_STR_EQ(etag, etag2);
    
    printf("✅ ETag generation tests passed\n");
    return 0;
}

/**
 * 测试文件缓存
 */
int test_file_caching() {
    printf("Testing file caching...\n");
    
    /* 检查初始缓存状态 */
    TEST_ASSERT_EQ(0, test_ctx->cache_count);
    TEST_ASSERT_EQ(0, test_ctx->current_cache_size);
    
    /* 测试缓存清理 */
    uvhttp_static_clear_cache(test_ctx);
    TEST_ASSERT_EQ(0, test_ctx->cache_count);
    TEST_ASSERT_EQ(0, test_ctx->current_cache_size);
    
    printf("✅ File caching tests passed\n");
    return 0;
}

/**
 * 运行所有静态文件测试
 */
int run_static_file_tests() {
    printf("=== Running UVHTTP Static File Tests ===\n\n");
    
    int failed_tests = 0;
    
    /* 设置测试环境 */
    if (setup_static_tests() != 0) {
        printf("❌ Failed to setup test environment\n");
        return -1;
    }
    
    /* 运行测试 */
    failed_tests += test_mime_type_detection();
    failed_tests += test_path_security();
    failed_tests += test_etag_generation();
    failed_tests += test_file_caching();
    
    /* 清理测试环境 */
    cleanup_static_tests();
    
    printf("\n=== Static File Tests Summary ===\n");
    if (failed_tests == 0) {
        printf("🎉 All static file tests passed!\n");
    } else {
        printf("❌ %d test(s) failed\n", failed_tests);
    }
    
    return failed_tests;
}

/* 如果直接运行此文件，执行测试 */
#ifdef STATIC_TEST_MAIN
int main() {
    return run_static_file_tests();
}
#endif