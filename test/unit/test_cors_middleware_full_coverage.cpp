/* UVHTTP CORS 中间件完整覆盖率测试 */

#include <gtest/gtest.h>
#include "uvhttp.h"
#include "uvhttp_cors_middleware.h"
#include "uvhttp_constants.h"

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigDefault) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        EXPECT_EQ(config->allow_all_origins, 1);
        EXPECT_EQ(config->allow_credentials_enabled, 0);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigCreate) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com", "GET, POST", "Content-Type");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        EXPECT_EQ(config->allow_all_origins, 0);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigCreateNull) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(NULL, NULL, NULL);
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigDestroyNull) {
    uvhttp_cors_config_destroy(NULL);
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigDestroyNormal) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    
    if (config != NULL) {
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsSetHeadersNull) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    
    if (config != NULL) {
        uvhttp_cors_set_headers(NULL, config, NULL);
        uvhttp_cors_set_headers(NULL, NULL, NULL);
        uvhttp_cors_set_headers(NULL, config, "https://example.com");
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsMiddlewareNull) {
    int result = uvhttp_cors_middleware(NULL, NULL, NULL);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsMiddlewareSimpleNull) {
    int result = uvhttp_cors_middleware_simple(NULL, NULL, NULL);
    EXPECT_EQ(result, UVHTTP_MIDDLEWARE_CONTINUE);
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigAllowAll) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "GET, POST", "Content-Type");
    
    if (config != NULL) {
        EXPECT_EQ(config->allow_all_origins, 1);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigPartialNull) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com", NULL, NULL);
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
    
    config = uvhttp_cors_config_create(NULL, "GET, POST", NULL);
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
    
    config = uvhttp_cors_config_create(NULL, NULL, "Content-Type");
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigEmptyStrings) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("", "", "");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigMultipleCreateDestroy) {
    uvhttp_cors_config_t* config;
    for (int i = 0; i < 10; i++) {
        config = uvhttp_cors_config_default();
        if (config != NULL) {
            uvhttp_cors_config_destroy(config);
        }
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigFields) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_default();
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        ASSERT_NE(config->expose_headers, nullptr);
        ASSERT_NE(config->allow_credentials, nullptr);
        ASSERT_NE(config->max_age, nullptr);
        EXPECT_EQ(config->owns_strings, 0);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigOwnsStrings) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com", "GET, POST", "Content-Type");
    
    if (config != NULL) {
        EXPECT_EQ(config->owns_strings, 1);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigSpecialChars) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com:8080", "GET, POST", "Content-Type, X-Custom-Header");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigUnicode) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://例子.com", "GET, POST", "Content-Type");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigSameInput) {
    uvhttp_cors_config_t* config1 = uvhttp_cors_config_default();
    uvhttp_cors_config_t* config2 = uvhttp_cors_config_default();
    
    if (config1 != NULL && config2 != NULL) {
        EXPECT_STREQ(config1->allow_origin, config2->allow_origin);
        EXPECT_STREQ(config1->allow_methods, config2->allow_methods);
        EXPECT_STREQ(config1->allow_headers, config2->allow_headers);
        uvhttp_cors_config_destroy(config1);
        uvhttp_cors_config_destroy(config2);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigDifferentInput) {
    uvhttp_cors_config_t* config1 = uvhttp_cors_config_create("https://example1.com", "GET", "Content-Type");
    uvhttp_cors_config_t* config2 = uvhttp_cors_config_create("https://example2.com", "POST", "Authorization");
    
    if (config1 != NULL && config2 != NULL) {
        EXPECT_STRNE(config1->allow_origin, config2->allow_origin);
        EXPECT_STRNE(config1->allow_methods, config2->allow_methods);
        EXPECT_STRNE(config1->allow_headers, config2->allow_headers);
        uvhttp_cors_config_destroy(config1);
        uvhttp_cors_config_destroy(config2);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigBoundary) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("*", "G", "C");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigWhitespace) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create(" * ", " GET , POST ", " Content-Type ");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigDuplicateMethods) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com", "GET, GET, POST, POST", "Content-Type");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_methods, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigDuplicateHeaders) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com", "GET, POST", "Content-Type, Content-Type");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}

TEST(UvhttpCorsMiddlewareFullCoverageTest, CorsConfigCommaSeparated) {
    uvhttp_cors_config_t* config = uvhttp_cors_config_create("https://example.com,https://example2.com", "GET,POST,PUT", "Content-Type,Authorization");
    
    if (config != NULL) {
        ASSERT_NE(config->allow_origin, nullptr);
        ASSERT_NE(config->allow_methods, nullptr);
        ASSERT_NE(config->allow_headers, nullptr);
        uvhttp_cors_config_destroy(config);
    }
}