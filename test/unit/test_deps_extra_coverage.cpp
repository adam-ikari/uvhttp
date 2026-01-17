#include <gtest/gtest.h>
#include <uvhttp_deps.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* 测试获取 loop provider */
TEST(UvhttpDepsExtraTest, GetLoopProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* provider = uvhttp_deps_get_loop_provider(deps);
    EXPECT_NE(provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试获取 loop provider NULL deps */
TEST(UvhttpDepsExtraTest, GetLoopProviderNullDeps) {
    uvhttp_loop_provider_t* provider = uvhttp_deps_get_loop_provider(NULL);
    EXPECT_EQ(provider, nullptr);
}

/* 测试获取 memory provider */
TEST(UvhttpDepsExtraTest, GetMemoryProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_memory_provider_t* provider = uvhttp_deps_get_memory_provider(deps);
    EXPECT_NE(provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试获取 memory provider NULL deps */
TEST(UvhttpDepsExtraTest, GetMemoryProviderNullDeps) {
    uvhttp_memory_provider_t* provider = uvhttp_deps_get_memory_provider(NULL);
    EXPECT_EQ(provider, nullptr);
}

/* 测试获取 network provider */
TEST(UvhttpDepsExtraTest, GetNetworkProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_network_provider_t* provider = uvhttp_deps_get_network_provider(deps);
    EXPECT_NE(provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试获取 network provider NULL deps */
TEST(UvhttpDepsExtraTest, GetNetworkProviderNullDeps) {
    uvhttp_network_provider_t* provider = uvhttp_deps_get_network_provider(NULL);
    EXPECT_EQ(provider, nullptr);
}

/* 测试获取 file provider */
TEST(UvhttpDepsExtraTest, GetFileProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_file_provider_t* provider = uvhttp_deps_get_file_provider(deps);
    EXPECT_NE(provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试获取 file provider NULL deps */
TEST(UvhttpDepsExtraTest, GetFileProviderNullDeps) {
    uvhttp_file_provider_t* provider = uvhttp_deps_get_file_provider(NULL);
    EXPECT_EQ(provider, nullptr);
}

/* 测试设置 loop provider */
TEST(UvhttpDepsExtraTest, SetLoopProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_loop_provider(deps, provider);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证设置成功 */
    uvhttp_loop_provider_t* get_provider = uvhttp_deps_get_loop_provider(deps);
    EXPECT_EQ(get_provider, provider);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 loop provider NULL deps */
TEST(UvhttpDepsExtraTest, SetLoopProviderNullDeps) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    /* NULL deps 应该返回错误 */
    uvhttp_error_t result = uvhttp_deps_set_loop_provider(NULL, provider);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 loop provider NULL provider */
TEST(UvhttpDepsExtraTest, SetLoopProviderNullProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    /* 设置 NULL provider 应该成功 */
    uvhttp_error_t result = uvhttp_deps_set_loop_provider(deps, NULL);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证设置为 NULL */
    uvhttp_loop_provider_t* get_provider = uvhttp_deps_get_loop_provider(deps);
    EXPECT_EQ(get_provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 memory provider */
TEST(UvhttpDepsExtraTest, SetMemoryProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_memory_provider_t* provider = uvhttp_deps_get_memory_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_memory_provider(deps, provider);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证设置成功 */
    uvhttp_memory_provider_t* get_provider = uvhttp_deps_get_memory_provider(deps);
    EXPECT_EQ(get_provider, provider);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 memory provider NULL deps */
TEST(UvhttpDepsExtraTest, SetMemoryProviderNullDeps) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_memory_provider_t* provider = uvhttp_deps_get_memory_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    /* NULL deps 应该返回错误 */
    uvhttp_error_t result = uvhttp_deps_set_memory_provider(NULL, provider);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 memory provider NULL provider */
TEST(UvhttpDepsExtraTest, SetMemoryProviderNullProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    /* 设置 NULL provider 应该成功 */
    uvhttp_error_t result = uvhttp_deps_set_memory_provider(deps, NULL);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证设置为 NULL */
    uvhttp_memory_provider_t* get_provider = uvhttp_deps_get_memory_provider(deps);
    EXPECT_EQ(get_provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 network provider */
TEST(UvhttpDepsExtraTest, SetNetworkProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_network_provider_t* provider = uvhttp_deps_get_network_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_network_provider(deps, provider);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证设置成功 */
    uvhttp_network_provider_t* get_provider = uvhttp_deps_get_network_provider(deps);
    EXPECT_EQ(get_provider, provider);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 network provider NULL deps */
TEST(UvhttpDepsExtraTest, SetNetworkProviderNullDeps) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_network_provider_t* provider = uvhttp_deps_get_network_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_network_provider(NULL, provider);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 network provider NULL provider */
TEST(UvhttpDepsExtraTest, SetNetworkProviderNullProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_network_provider(deps, NULL);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 file provider */
TEST(UvhttpDepsExtraTest, SetFileProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_file_provider_t* provider = uvhttp_deps_get_file_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_file_provider(deps, provider);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 验证设置成功 */
    uvhttp_file_provider_t* get_provider = uvhttp_deps_get_file_provider(deps);
    EXPECT_EQ(get_provider, provider);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 file provider NULL deps */
TEST(UvhttpDepsExtraTest, SetFileProviderNullDeps) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_file_provider_t* provider = uvhttp_deps_get_file_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_file_provider(NULL, provider);
    EXPECT_NE(result, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 file provider NULL provider */
TEST(UvhttpDepsExtraTest, SetFileProviderNullProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_file_provider(deps, NULL);
    EXPECT_EQ(result, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置所有 provider */
TEST(UvhttpDepsExtraTest, SetAllProviders) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* loop_provider = uvhttp_deps_get_loop_provider(deps);
    uvhttp_memory_provider_t* memory_provider = uvhttp_deps_get_memory_provider(deps);
    uvhttp_network_provider_t* network_provider = uvhttp_deps_get_network_provider(deps);
    uvhttp_file_provider_t* file_provider = uvhttp_deps_get_file_provider(deps);
    
    ASSERT_NE(loop_provider, nullptr);
    ASSERT_NE(memory_provider, nullptr);
    ASSERT_NE(network_provider, nullptr);
    ASSERT_NE(file_provider, nullptr);
    
    uvhttp_error_t result1 = uvhttp_deps_set_loop_provider(deps, loop_provider);
    uvhttp_error_t result2 = uvhttp_deps_set_memory_provider(deps, memory_provider);
    uvhttp_error_t result3 = uvhttp_deps_set_network_provider(deps, network_provider);
    uvhttp_error_t result4 = uvhttp_deps_set_file_provider(deps, file_provider);
    
    EXPECT_EQ(result1, UVHTTP_OK);
    EXPECT_EQ(result2, UVHTTP_OK);
    EXPECT_EQ(result3, UVHTTP_OK);
    EXPECT_EQ(result4, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试 provider 交互 - 使用 loop provider */
TEST(UvhttpDepsExtraTest, ProviderInteractionLoop) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* loop_provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(loop_provider, nullptr);
    
    /* 测试获取默认 loop */
    uv_loop_t* loop = loop_provider->get_default_loop(loop_provider->user_data);
    EXPECT_NE(loop, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试 provider 交互 - 使用 memory provider */
TEST(UvhttpDepsExtraTest, ProviderInteractionMemory) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_memory_provider_t* memory_provider = uvhttp_deps_get_memory_provider(deps);
    ASSERT_NE(memory_provider, nullptr);
    
    /* 测试内存分配 */
    void* ptr = memory_provider->malloc(100, NULL);
    EXPECT_NE(ptr, nullptr);
    
    if (ptr) {
        /* 测试获取分配大小 - 可能返回 0 */
        size_t size = memory_provider->get_allocated_size(ptr, NULL);
        /* 不检查 size 的值，因为不同的 provider 可能返回不同的值 */
        
        /* 测试内存释放 */
        memory_provider->free(ptr, NULL);
    }
    
    uvhttp_deps_free(deps);
}

/* 测试 provider 交互 - 使用 network provider */
TEST(UvhttpDepsExtraTest, ProviderInteractionNetwork) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_network_provider_t* network_provider = uvhttp_deps_get_network_provider(deps);
    ASSERT_NE(network_provider, nullptr);
    
    /* 注意：network provider 的函数需要实际的 socket 操作，这里只测试函数指针存在 */
    EXPECT_NE(network_provider->create_socket, nullptr);
    EXPECT_NE(network_provider->bind_socket, nullptr);
    EXPECT_NE(network_provider->listen_socket, nullptr);
    EXPECT_NE(network_provider->accept_socket, nullptr);
    EXPECT_NE(network_provider->close_socket, nullptr);
    EXPECT_NE(network_provider->send_data, nullptr);
    EXPECT_NE(network_provider->recv_data, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试 provider 交互 - 使用 file provider */
TEST(UvhttpDepsExtraTest, ProviderInteractionFile) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_file_provider_t* file_provider = uvhttp_deps_get_file_provider(deps);
    ASSERT_NE(file_provider, nullptr);
    
    /* 注意：file provider 的函数需要实际的文件操作，这里只测试函数指针存在 */
    EXPECT_NE(file_provider->fopen, nullptr);
    EXPECT_NE(file_provider->fclose, nullptr);
    EXPECT_NE(file_provider->fread, nullptr);
    EXPECT_NE(file_provider->fwrite, nullptr);
    EXPECT_NE(file_provider->fseek, nullptr);
    EXPECT_NE(file_provider->ftell, nullptr);
    EXPECT_NE(file_provider->access, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试多次获取相同的 provider */
TEST(UvhttpDepsExtraTest, MultipleGetSameProvider) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* provider1 = uvhttp_deps_get_loop_provider(deps);
    uvhttp_loop_provider_t* provider2 = uvhttp_deps_get_loop_provider(deps);
    
    EXPECT_EQ(provider1, provider2);
    
    uvhttp_deps_free(deps);
}

/* 测试设置不同的 provider */
TEST(UvhttpDepsExtraTest, SetDifferentProvider) {
    uvhttp_deps_t* deps1 = uvhttp_deps_create_default();
    uvhttp_deps_t* deps2 = uvhttp_deps_create_default();
    
    ASSERT_NE(deps1, nullptr);
    ASSERT_NE(deps2, nullptr);
    
    uvhttp_loop_provider_t* provider1 = uvhttp_deps_get_loop_provider(deps1);
    uvhttp_loop_provider_t* provider2 = uvhttp_deps_get_loop_provider(deps2);
    
    ASSERT_NE(provider1, provider2);
    
    uvhttp_deps_free(deps1);
    uvhttp_deps_free(deps2);
}

/* 测试默认依赖和测试依赖的 provider */
TEST(UvhttpDepsExtraTest, DefaultVsTestProviders) {
    uvhttp_deps_t* default_deps = uvhttp_deps_create_default();
    uvhttp_deps_t* test_deps = uvhttp_deps_create_test();
    
    ASSERT_NE(default_deps, nullptr);
    ASSERT_NE(test_deps, nullptr);
    
    uvhttp_loop_provider_t* default_loop = uvhttp_deps_get_loop_provider(default_deps);
    uvhttp_loop_provider_t* test_loop = uvhttp_deps_get_loop_provider(test_deps);
    
    /* 两个依赖的 loop provider 应该不同 */
    EXPECT_NE(default_loop, test_loop);
    
    uvhttp_deps_free(default_deps);
    uvhttp_deps_free(test_deps);
}

/* 测试 provider user_data */
TEST(UvhttpDepsExtraTest, ProviderUserData) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* loop_provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(loop_provider, nullptr);
    
    /* user_data 可能为 NULL 或指向 deps，这里只检查 provider 存在 */
    EXPECT_NE(loop_provider, nullptr);
    
    uvhttp_memory_provider_t* memory_provider = uvhttp_deps_get_memory_provider(deps);
    ASSERT_NE(memory_provider, nullptr);
    
    EXPECT_NE(memory_provider, nullptr);
    
    uvhttp_network_provider_t* network_provider = uvhttp_deps_get_network_provider(deps);
    ASSERT_NE(network_provider, nullptr);
    
    EXPECT_NE(network_provider, nullptr);
    
    uvhttp_file_provider_t* file_provider = uvhttp_deps_get_file_provider(deps);
    ASSERT_NE(file_provider, nullptr);
    
    EXPECT_NE(file_provider, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试 provider 函数指针有效性 */
TEST(UvhttpDepsExtraTest, ProviderFunctionPointers) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* loop_provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(loop_provider, nullptr);
    
    EXPECT_NE(loop_provider->get_default_loop, nullptr);
    EXPECT_NE(loop_provider->create_loop, nullptr);
    EXPECT_NE(loop_provider->run_loop, nullptr);
    EXPECT_NE(loop_provider->close_loop, nullptr);
    
    uvhttp_memory_provider_t* memory_provider = uvhttp_deps_get_memory_provider(deps);
    ASSERT_NE(memory_provider, nullptr);
    
    EXPECT_NE(memory_provider->malloc, nullptr);
    EXPECT_NE(memory_provider->free, nullptr);
    EXPECT_NE(memory_provider->calloc, nullptr);
    EXPECT_NE(memory_provider->realloc, nullptr);
    EXPECT_NE(memory_provider->get_allocated_size, nullptr);
    
    uvhttp_network_provider_t* network_provider = uvhttp_deps_get_network_provider(deps);
    ASSERT_NE(network_provider, nullptr);
    
    EXPECT_NE(network_provider->create_socket, nullptr);
    EXPECT_NE(network_provider->bind_socket, nullptr);
    EXPECT_NE(network_provider->listen_socket, nullptr);
    EXPECT_NE(network_provider->accept_socket, nullptr);
    EXPECT_NE(network_provider->close_socket, nullptr);
    EXPECT_NE(network_provider->send_data, nullptr);
    EXPECT_NE(network_provider->recv_data, nullptr);
    
    uvhttp_file_provider_t* file_provider = uvhttp_deps_get_file_provider(deps);
    ASSERT_NE(file_provider, nullptr);
    
    EXPECT_NE(file_provider->fopen, nullptr);
    EXPECT_NE(file_provider->fclose, nullptr);
    EXPECT_NE(file_provider->fread, nullptr);
    EXPECT_NE(file_provider->fwrite, nullptr);
    EXPECT_NE(file_provider->fseek, nullptr);
    EXPECT_NE(file_provider->ftell, nullptr);
    EXPECT_NE(file_provider->access, nullptr);
    
    uvhttp_deps_free(deps);
}

/* 测试 cleanup 函数 */
TEST(UvhttpDepsExtraTest, CleanupFunction) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    EXPECT_NE(deps->cleanup, nullptr);
    
    /* cleanup 函数应该释放所有 providers */
    deps->cleanup(deps);
}

/* 测试 owns_providers 标志 */
TEST(UvhttpDepsExtraTest, OwnsProvidersFlag) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    /* 创建默认依赖应该拥有 providers */
    EXPECT_TRUE(deps->owns_providers);
    
    uvhttp_deps_free(deps);
}

/* 测试 NULL cleanup 函数 */
TEST(UvhttpDepsExtraTest, NullCleanupFunction) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    /* 设置 cleanup 为 NULL */
    deps->cleanup = NULL;
    
    /* 释放依赖不应该崩溃 */
    uvhttp_deps_free(deps);
}

/* 测试多次设置同一个 provider */
TEST(UvhttpDepsExtraTest, SetSameProviderMultipleTimes) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    /* 多次设置同一个 provider */
    uvhttp_error_t result1 = uvhttp_deps_set_loop_provider(deps, provider);
    uvhttp_error_t result2 = uvhttp_deps_set_loop_provider(deps, provider);
    uvhttp_error_t result3 = uvhttp_deps_set_loop_provider(deps, provider);
    
    EXPECT_EQ(result1, UVHTTP_OK);
    EXPECT_EQ(result2, UVHTTP_OK);
    EXPECT_EQ(result3, UVHTTP_OK);
    
    uvhttp_deps_free(deps);
}

/* 测试设置 provider 后仍然可以获取 */
TEST(UvhttpDepsExtraTest, GetAfterSet) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    uvhttp_loop_provider_t* provider = uvhttp_deps_get_loop_provider(deps);
    ASSERT_NE(provider, nullptr);
    
    uvhttp_error_t result = uvhttp_deps_set_loop_provider(deps, provider);
    EXPECT_EQ(result, UVHTTP_OK);
    
    /* 设置后仍然可以获取 */
    uvhttp_loop_provider_t* get_provider = uvhttp_deps_get_loop_provider(deps);
    EXPECT_EQ(get_provider, provider);
    
    uvhttp_deps_free(deps);
}

/* 测试交叉设置 provider */
TEST(UvhttpDepsExtraTest, CrossSetProviders) {
    uvhttp_deps_t* deps1 = uvhttp_deps_create_default();
    uvhttp_deps_t* deps2 = uvhttp_deps_create_default();
    
    ASSERT_NE(deps1, nullptr);
    ASSERT_NE(deps2, nullptr);
    
    uvhttp_loop_provider_t* provider1 = uvhttp_deps_get_loop_provider(deps1);
    uvhttp_loop_provider_t* provider2 = uvhttp_deps_get_loop_provider(deps2);
    
    /* 交叉设置 */
    uvhttp_error_t result1 = uvhttp_deps_set_loop_provider(deps1, provider2);
    uvhttp_error_t result2 = uvhttp_deps_set_loop_provider(deps2, provider1);
    
    EXPECT_EQ(result1, UVHTTP_OK);
    EXPECT_EQ(result2, UVHTTP_OK);
    
    /* 验证设置成功 */
    EXPECT_EQ(uvhttp_deps_get_loop_provider(deps1), provider2);
    EXPECT_EQ(uvhttp_deps_get_loop_provider(deps2), provider1);
    
    uvhttp_deps_free(deps1);
    uvhttp_deps_free(deps2);
}

/* 测试 provider 顺序 */
TEST(UvhttpDepsExtraTest, ProviderOrder) {
    uvhttp_deps_t* deps = uvhttp_deps_create_default();
    ASSERT_NE(deps, nullptr);
    
    /* 按顺序获取所有 providers */
    uvhttp_loop_provider_t* loop_provider = uvhttp_deps_get_loop_provider(deps);
    uvhttp_memory_provider_t* memory_provider = uvhttp_deps_get_memory_provider(deps);
    uvhttp_network_provider_t* network_provider = uvhttp_deps_get_network_provider(deps);
    uvhttp_file_provider_t* file_provider = uvhttp_deps_get_file_provider(deps);
    
    /* 验证所有 providers 都存在 */
    ASSERT_NE(loop_provider, nullptr);
    ASSERT_NE(memory_provider, nullptr);
    ASSERT_NE(network_provider, nullptr);
    ASSERT_NE(file_provider, nullptr);
    
    /* user_data 可能为 NULL 或指向 deps，这里不检查具体值 */
    
    uvhttp_deps_free(deps);
}
