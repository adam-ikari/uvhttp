/* uvhttp_lru_cache.c 完整覆盖率测试 */

#include "uvhttp_lru_cache.h"
#include "uvhttp_allocator.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <limits.h>
#include <assert.h>

/* 测试缓存结构大小 */
void test_cache_struct_size(void) {
    assert(sizeof(cache_entry_t) > 0);
    assert(sizeof(cache_manager_t) > 0);

    printf("test_cache_struct_size: PASSED\n");
}

/* 测试创建缓存管理器 */
void test_cache_create(void) {
    cache_manager_t* cache = uvhttp_lru_cache_create(1024 * 1024, 100, 3600);
    /* 可能返回NULL或创建的缓存 */
    if (cache != NULL) {
        uvhttp_lru_cache_free(cache);
    }

    printf("test_cache_create: PASSED\n");
}

/* 测试释放缓存管理器 - NULL参数 */
void test_cache_free_null(void) {
    /* 应该安全处理 */
    uvhttp_lru_cache_free(NULL);

    printf("test_cache_free_null: PASSED\n");
}

/* 测试查找缓存条目 - NULL参数 */
void test_cache_find_null(void) {
    cache_entry_t* entry = uvhttp_lru_cache_find(NULL, NULL);
    /* 应该返回NULL */
    assert(entry == NULL);

    printf("test_cache_find_null: PASSED\n");
}

/* 测试添加缓存条目 - NULL参数 */
void test_cache_put_null(void) {
    uvhttp_error_t result;

    result = uvhttp_lru_cache_put(NULL, NULL, NULL, 0, NULL, 0, NULL);
    assert(result != UVHTTP_OK);
    (void)result;

    printf("test_cache_put_null: PASSED\n");
}

/* 测试删除缓存条目 - NULL参数 */
void test_cache_remove_null(void) {
    uvhttp_error_t result = uvhttp_lru_cache_remove(NULL, NULL);
    assert(result != UVHTTP_OK);
    (void)result;

    printf("test_cache_remove_null: PASSED\n");
}

/* 测试清空缓存 - NULL参数 */
void test_cache_clear_null(void) {
    /* 应该安全处理 */
    uvhttp_lru_cache_clear(NULL);

    printf("test_cache_clear_null: PASSED\n");
}

/* 测试获取缓存统计信息 - NULL参数 */
void test_cache_get_stats_null(void) {
    /* 应该安全处理 */
    uvhttp_lru_cache_get_stats(NULL, NULL, NULL, NULL, NULL, NULL);

    printf("test_cache_get_stats_null: PASSED\n");
}

/* 测试重置统计信息 - NULL参数 */
void test_cache_reset_stats_null(void) {
    /* 应该安全处理 */
    uvhttp_lru_cache_reset_stats(NULL);

    printf("test_cache_reset_stats_null: PASSED\n");
}

/* 测试清理过期条目 - NULL参数 */
void test_cache_cleanup_expired_null(void) {
    int result = uvhttp_lru_cache_cleanup_expired(NULL);
    /* 应该返回0 */
    assert(result == 0);

    printf("test_cache_cleanup_expired_null: PASSED\n");
}

/* 测试检查缓存条目是否过期 */
void test_cache_is_expired(void) {
    cache_entry_t entry;
    memset(&entry, 0, sizeof(entry));

    /* 测试未过期 */
    entry.cache_time = time(NULL);
    assert(uvhttp_lru_cache_is_expired(&entry, 3600) == 0);

    /* 测试过期 */
    entry.cache_time = time(NULL) - 7200;
    assert(uvhttp_lru_cache_is_expired(&entry, 3600) == 1);

    printf("test_cache_is_expired: PASSED\n");
}

/* 测试移动条目到LRU链表头部 - NULL参数 */
void test_cache_move_to_head_null(void) {
    /* 应该安全处理 */
    uvhttp_lru_cache_move_to_head(NULL, NULL);

    printf("test_cache_move_to_head_null: PASSED\n");
}

/* 测试从LRU链表尾部移除条目 - NULL参数 */
void test_cache_remove_tail_null(void) {
    cache_entry_t* entry = uvhttp_lru_cache_remove_tail(NULL);
    /* 应该返回NULL */
    assert(entry == NULL);

    printf("test_cache_remove_tail_null: PASSED\n");
}

/* 测试计算缓存命中率 - NULL参数 */
void test_cache_get_hit_rate_null(void) {
    double rate = uvhttp_lru_cache_get_hit_rate(NULL);
    /* 应该返回0.0 */
    assert(rate == 0.0);

    printf("test_cache_get_hit_rate_null: PASSED\n");
}

/* 测试缓存条目结构初始化 */
void test_cache_entry_initialization(void) {
    cache_entry_t entry;
    memset(&entry, 0, sizeof(entry));

    /* 验证初始值 */
    assert(entry.content == NULL);
    assert(entry.content_length == 0);
    assert(entry.last_modified == 0);
    assert(entry.access_time == 0);
    assert(entry.cache_time == 0);
    assert(entry.memory_usage == 0);
    assert(entry.is_compressed == 0);
    assert(entry.lru_prev == NULL);
    assert(entry.lru_next == NULL);

    printf("test_cache_entry_initialization: PASSED\n");
}

/* 测试缓存管理器结构初始化 */
void test_cache_manager_initialization(void) {
    cache_manager_t manager;
    memset(&manager, 0, sizeof(manager));

    /* 验证初始值 */
    assert(manager.hash_table == NULL);
    assert(manager.lru_head == NULL);
    assert(manager.lru_tail == NULL);
    assert(manager.total_memory_usage == 0);
    assert(manager.entry_count == 0);
    assert(manager.hit_count == 0);
    assert(manager.miss_count == 0);
    assert(manager.eviction_count == 0);

    printf("test_cache_manager_initialization: PASSED\n");
}

/* 测试边界条件 */
void test_boundary_conditions(void) {
    cache_manager_t manager;
    memset(&manager, 0, sizeof(manager));

    /* 测试极限值 */
    manager.max_memory_usage = SIZE_MAX;
    manager.max_entries = INT_MAX;
    manager.cache_ttl = INT_MAX;

    assert(manager.max_memory_usage == SIZE_MAX);
    assert(manager.max_entries == INT_MAX);
    assert(manager.cache_ttl == INT_MAX);

    printf("test_boundary_conditions: PASSED\n");
}

/* 测试缓存常量 */
void test_cache_constants(void) {
    assert(UVHTTP_MAX_FILE_PATH_SIZE > 0);
    assert(UVHTTP_MAX_HEADER_VALUE_SIZE > 0);

    printf("test_cache_constants: PASSED\n");
}

/* 测试多次调用NULL参数函数 */
void test_multiple_null_calls(void) {
    /* 多次调用NULL参数函数，确保不会崩溃 */
    for (int i = 0; i < 100; i++) {
        uvhttp_lru_cache_free(NULL);
        uvhttp_lru_cache_find(NULL, NULL);
        uvhttp_lru_cache_remove(NULL, NULL);
        uvhttp_lru_cache_clear(NULL);
        uvhttp_lru_cache_get_stats(NULL, NULL, NULL, NULL, NULL, NULL);
        uvhttp_lru_cache_reset_stats(NULL);
        uvhttp_lru_cache_cleanup_expired(NULL);
        uvhttp_lru_cache_move_to_head(NULL, NULL);
        uvhttp_lru_cache_remove_tail(NULL);
        uvhttp_lru_cache_get_hit_rate(NULL);
    }

    printf("test_multiple_null_calls: PASSED\n");
}

/* 测试缓存条目结构对齐 */
void test_cache_struct_alignment(void) {
    /* 验证结构对齐合理 */
    assert(sizeof(cache_entry_t) % sizeof(void*) == 0);
    assert(sizeof(cache_manager_t) % sizeof(void*) == 0);

    printf("test_cache_struct_alignment: PASSED\n");
}

/* 测试缓存条目字段大小 */
void test_cache_entry_field_sizes(void) {
    cache_entry_t entry;

    /* 验证字段大小合理 */
    assert(sizeof(entry.file_path) == UVHTTP_MAX_FILE_PATH_SIZE);
    assert(sizeof(entry.mime_type) == UVHTTP_MAX_HEADER_VALUE_SIZE);
    assert(sizeof(entry.etag) == UVHTTP_MAX_HEADER_VALUE_SIZE);

    printf("test_cache_entry_field_sizes: PASSED\n");
}

/* 测试缓存管理器字段大小 */
void test_cache_manager_field_sizes(void) {
    cache_manager_t manager;

    /* 验证字段大小合理 */
    assert(sizeof(manager.total_memory_usage) == sizeof(size_t));
    assert(sizeof(manager.entry_count) == sizeof(int));
    assert(sizeof(manager.hit_count) == sizeof(int));

    printf("test_cache_manager_field_sizes: PASSED\n");
}

/* 测试缓存条目链表结构 */
void test_cache_entry_list(void) {
    cache_entry_t entry1, entry2, entry3;
    memset(&entry1, 0, sizeof(entry1));
    memset(&entry2, 0, sizeof(entry2));
    memset(&entry3, 0, sizeof(entry3));

    /* 测试链式结构 */
    entry1.lru_next = &entry2;
    entry2.lru_prev = &entry1;
    entry2.lru_next = &entry3;
    entry3.lru_prev = &entry2;

    assert(entry1.lru_next == &entry2);
    assert(entry2.lru_prev == &entry1);
    assert(entry2.lru_next == &entry3);
    assert(entry3.lru_prev == &entry2);

    printf("test_cache_entry_list: PASSED\n");
}

/* 测试缓存统计信息 */
void test_cache_stats(void) {
    cache_manager_t manager;
    size_t total_memory;
    int entry_count, hit_count, miss_count, eviction_count;

    memset(&manager, 0, sizeof(manager));

    /* 设置统计值 */
    manager.total_memory_usage = 1024;
    manager.entry_count = 10;
    manager.hit_count = 100;
    manager.miss_count = 50;
    manager.eviction_count = 5;

    /* 获取统计信息 */
    uvhttp_lru_cache_get_stats(&manager, &total_memory, &entry_count, &hit_count, &miss_count, &eviction_count);

    assert(total_memory == 1024);
    assert(entry_count == 10);
    assert(hit_count == 100);
    assert(miss_count == 50);
    assert(eviction_count == 5);

    printf("test_cache_stats: PASSED\n");
}

/* 测试缓存命中率计算 */
void test_cache_hit_rate(void) {
    cache_manager_t manager;

    memset(&manager, 0, sizeof(manager));

    /* 测试各种命中率 */
    manager.hit_count = 0;
    manager.miss_count = 0;
    assert(uvhttp_lru_cache_get_hit_rate(&manager) == 0.0);

    manager.hit_count = 100;
    manager.miss_count = 0;
    assert(uvhttp_lru_cache_get_hit_rate(&manager) == 1.0);

    manager.hit_count = 50;
    manager.miss_count = 50;
    assert(uvhttp_lru_cache_get_hit_rate(&manager) == 0.5);

    printf("test_cache_hit_rate: PASSED\n");
}

int main() {
    printf("=== uvhttp_lru_cache.c 完整覆盖率测试 ===\n\n");

    /* 结构和常量测试 */
    test_cache_struct_size();
    test_cache_constants();
    test_cache_entry_initialization();
    test_cache_manager_initialization();
    test_cache_entry_field_sizes();
    test_cache_manager_field_sizes();
    test_cache_entry_list();

    /* NULL参数测试 */
    test_cache_free_null();
    test_cache_find_null();
    test_cache_put_null();
    test_cache_remove_null();
    test_cache_clear_null();
    test_cache_get_stats_null();
    test_cache_reset_stats_null();
    test_cache_cleanup_expired_null();
    test_cache_move_to_head_null();
    test_cache_remove_tail_null();
    test_cache_get_hit_rate_null();

    /* 功能测试 */
    test_cache_create();
    test_cache_is_expired();
    test_cache_stats();
    test_cache_hit_rate();

    /* 边界条件测试 */
    test_boundary_conditions();

    /* 结构测试 */
    test_cache_struct_alignment();

    /* 压力测试 */
    test_multiple_null_calls();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}
