/* UVHTTP 依赖注入模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_deps.h"
#include "uvhttp_constants.h"

TEST(UvhttpDepsFullCoverageTest, DepsNew) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    
    if (deps != nullptr) {
        EXPECT_EQ(deps->get_loop, nullptr);
        EXPECT_EQ(deps->malloc, nullptr);
        EXPECT_EQ(deps->free, nullptr);
        EXPECT_EQ(deps->fopen, nullptr);
        EXPECT_EQ(deps->access, nullptr);
        EXPECT_EQ(deps->loop_provider, nullptr);
        EXPECT_EQ(deps->memory_provider, nullptr);
        EXPECT_EQ(deps->network_provider, nullptr);
        EXPECT_EQ(deps->file_provider, nullptr);
        EXPECT_EQ(deps->owns_providers, 0);
        EXPECT_NE(deps->cleanup, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsCreateDefault) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->loop_provider, nullptr);
        EXPECT_NE(deps->memory_provider, nullptr);
        EXPECT_NE(deps->network_provider, nullptr);
        EXPECT_NE(deps->file_provider, nullptr);
        EXPECT_EQ(deps->owns_providers, 1);
        EXPECT_NE(deps->cleanup, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsCreateTest) {
    uvhttp_deps_t* deps = uvhttp_deps_create_test();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->loop_provider, nullptr);
        EXPECT_NE(deps->memory_provider, nullptr);
        EXPECT_NE(deps->network_provider, nullptr);
        EXPECT_NE(deps->file_provider, nullptr);
        EXPECT_EQ(deps->owns_providers, 1);
        EXPECT_NE(deps->cleanup, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsCreateDestroy) {
    for (int i = 0; i < 10; i++) {
        uvhttp_deps_t* deps = uvhttp_deps_new();
        if (deps != nullptr) {
            uvhttp_deps_free(deps);
        }
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsDifferentCreates) {
    uvhttp_deps_t* deps1 = uvhttp_deps_new();
    uvhttp_deps_t* deps2 = uvhttp_deps_create_default();
    uvhttp_deps_t* deps3 = uvhttp_deps_create_test();
    
    if (deps1 != nullptr) {
        uvhttp_deps_free(deps1);
    }
    if (deps2 != nullptr) {
        uvhttp_deps_free(deps2);
    }
    if (deps3 != nullptr) {
        uvhttp_deps_free(deps3);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsProviderConsistency) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->loop_provider, nullptr);
        EXPECT_NE(deps->memory_provider, nullptr);
        EXPECT_NE(deps->network_provider, nullptr);
        EXPECT_NE(deps->file_provider, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsOwnership) {
    uvhttp_deps_t* deps1 = uvhttp_deps_new();
    if (deps1 != nullptr) {
        EXPECT_EQ(deps1->owns_providers, 0);
        uvhttp_deps_free(deps1);
    }
    
    uvhttp_deps_t* deps2 = uvhttp_deps_create_default();
    if (deps2 != nullptr) {
        EXPECT_EQ(deps2->owns_providers, 1);
        uvhttp_deps_free(deps2);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsNullParams) {
    uvhttp_deps_free(nullptr);
}

TEST(UvhttpDepsFullCoverageTest, DepsCleanup) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->cleanup, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsProviderFields) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->loop_provider, nullptr);
        EXPECT_NE(deps->memory_provider, nullptr);
        EXPECT_NE(deps->network_provider, nullptr);
        EXPECT_NE(deps->file_provider, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsDefaultVsTest) {
    uvhttp_deps_t* default_deps = uvhttp_deps_create_default();
    uvhttp_deps_t* test_deps = uvhttp_deps_create_test();
    
    if (default_deps != nullptr && test_deps != nullptr) {
        EXPECT_NE(default_deps->loop_provider, nullptr);
        EXPECT_NE(test_deps->loop_provider, nullptr);
        EXPECT_NE(default_deps->memory_provider, nullptr);
        EXPECT_NE(test_deps->memory_provider, nullptr);
        EXPECT_NE(default_deps->network_provider, nullptr);
        EXPECT_NE(test_deps->network_provider, nullptr);
        EXPECT_NE(default_deps->file_provider, nullptr);
        EXPECT_NE(test_deps->file_provider, nullptr);
        
        EXPECT_EQ(default_deps->owns_providers, 1);
        EXPECT_EQ(test_deps->owns_providers, 1);
        
        uvhttp_deps_free(default_deps);
        uvhttp_deps_free(test_deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsMultipleCycles) {
    for (int i = 0; i < 5; i++) {
        uvhttp_deps_t* deps = uvhttp_deps_create_default();
        if (deps != nullptr) {
            uvhttp_deps_free(deps);
        }
    }
    
    for (int i = 0; i < 5; i++) {
        uvhttp_deps_t* deps = uvhttp_deps_create_test();
        if (deps != nullptr) {
            uvhttp_deps_free(deps);
        }
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsEmptyContainer) {
    uvhttp_deps_t* deps = uvhttp_deps_new();
    
    if (deps != nullptr) {
        EXPECT_EQ(deps->get_loop, nullptr);
        EXPECT_EQ(deps->malloc, nullptr);
        EXPECT_EQ(deps->free, nullptr);
        EXPECT_EQ(deps->fopen, nullptr);
        EXPECT_EQ(deps->access, nullptr);
        EXPECT_EQ(deps->loop_provider, nullptr);
        EXPECT_EQ(deps->memory_provider, nullptr);
        EXPECT_EQ(deps->network_provider, nullptr);
        EXPECT_EQ(deps->file_provider, nullptr);
        EXPECT_EQ(deps->owns_providers, 0);
        EXPECT_NE(deps->cleanup, nullptr);
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsDefaultContainer) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->loop_provider, nullptr);
        EXPECT_NE(deps->memory_provider, nullptr);
        EXPECT_NE(deps->network_provider, nullptr);
        EXPECT_NE(deps->file_provider, nullptr);
        EXPECT_EQ(deps->owns_providers, 1);
        
        EXPECT_EQ(deps->get_loop, nullptr);
        EXPECT_EQ(deps->malloc, nullptr);
        EXPECT_EQ(deps->free, nullptr);
        EXPECT_EQ(deps->fopen, nullptr);
        EXPECT_EQ(deps->access, nullptr);
        
        uvhttp_deps_free(deps);
    }
}

TEST(UvhttpDepsFullCoverageTest, DepsTestContainer) {
    uvhttp_deps_t* deps = uvhttp_deps_create_test();
    
    if (deps != nullptr) {
        EXPECT_NE(deps->loop_provider, nullptr);
        EXPECT_NE(deps->memory_provider, nullptr);
        EXPECT_NE(deps->network_provider, nullptr);
        EXPECT_NE(deps->file_provider, nullptr);
        EXPECT_EQ(deps->owns_providers, 1);
        
        EXPECT_EQ(deps->get_loop, nullptr);
        EXPECT_EQ(deps->malloc, nullptr);
        EXPECT_EQ(deps->free, nullptr);
        EXPECT_EQ(deps->fopen, nullptr);
        EXPECT_EQ(deps->access, nullptr);
        
        uvhttp_deps_free(deps);
    }
}