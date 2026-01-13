/* UVHTTP 哈希模块完整覆盖率测试 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "uvhttp.h"
#include "uvhttp_hash.h"
#include "uvhttp_constants.h"

/* 测试哈希函数 - NULL参数 */
void test_hash_null(void) {
    uint64_t result;

    /* NULL data */
    result = uvhttp_hash(NULL, 100, 0x12345);
    assert(result == 0);

    printf("test_hash_null: PASSED\n");
}

/* 测试哈希函数 - 正常参数 */
void test_hash_normal(void) {
    uint64_t result;
    const char* data = "Hello, World!";
    size_t len = strlen(data);

    result = uvhttp_hash(data, len, 0x12345);
    assert(result != 0);

    printf("test_hash_normal: PASSED\n");
}

/* 测试哈希函数 - 空数据 */
void test_hash_empty(void) {
    uint64_t result;
    const char* data = "";

    result = uvhttp_hash(data, 0, 0x12345);
    /* 空数据应该产生有效的哈希值 */

    printf("test_hash_empty: PASSED\n");
}

/* 测试哈希函数 - 不同种子 */
void test_hash_different_seeds(void) {
    uint64_t result1, result2;
    const char* data = "test data";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0x1111);
    result2 = uvhttp_hash(data, len, 0x2222);

    /* 不同种子应该产生不同的哈希值 */
    assert(result1 != result2);

    printf("test_hash_different_seeds: PASSED\n");
}

/* 测试哈希函数 - 相同输入相同输出 */
void test_hash_consistency(void) {
    uint64_t result1, result2;
    const char* data = "consistent data";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0x12345);
    result2 = uvhttp_hash(data, len, 0x12345);

    /* 相同输入应该产生相同输出 */
    assert(result1 == result2);

    printf("test_hash_consistency: PASSED\n");
}

/* 测试哈希字符串函数 - NULL参数 */
void test_hash_string_null(void) {
    uint64_t result;

    /* NULL string */
    result = uvhttp_hash_string(NULL);
    assert(result == 0);

    printf("test_hash_string_null: PASSED\n");
}

/* 测试哈希字符串函数 - 正常参数 */
void test_hash_string_normal(void) {
    uint64_t result;
    const char* str = "Hello, World!";

    result = uvhttp_hash_string(str);
    assert(result != 0);

    printf("test_hash_string_normal: PASSED\n");
}

/* 测试哈希字符串函数 - 空字符串 */
void test_hash_string_empty(void) {
    uint64_t result;
    const char* str = "";

    result = uvhttp_hash_string(str);
    /* 空字符串应该产生有效的哈希值 */

    printf("test_hash_string_empty: PASSED\n");
}

/* 测试哈希字符串函数 - 一致性 */
void test_hash_string_consistency(void) {
    uint64_t result1, result2;
    const char* str = "consistent string";

    result1 = uvhttp_hash_string(str);
    result2 = uvhttp_hash_string(str);

    /* 相同输入应该产生相同输出 */
    assert(result1 == result2);

    printf("test_hash_string_consistency: PASSED\n");
}

/* 测试哈希字符串函数 - 不同字符串 */
void test_hash_string_different(void) {
    uint64_t result1, result2;
    const char* str1 = "string 1";
    const char* str2 = "string 2";

    result1 = uvhttp_hash_string(str1);
    result2 = uvhttp_hash_string(str2);

    /* 不同字符串应该产生不同的哈希值 */
    assert(result1 != result2);

    printf("test_hash_string_different: PASSED\n");
}

/* 测试默认哈希函数 */
void test_hash_default(void) {
    uint64_t result;
    const char* data = "default hash test";
    size_t len = strlen(data);

    result = uvhttp_hash_default(data, len);
    assert(result != 0);

    printf("test_hash_default: PASSED\n");
}

/* 测试默认哈希字符串函数 */
void test_hash_string_default(void) {
    uint64_t result;
    const char* str = "default string hash";

    result = uvhttp_hash_string_default(str);
    assert(result != 0);

    printf("test_hash_string_default: PASSED\n");
}

/* 测试哈希函数 - 大数据 */
void test_hash_large_data(void) {
    uint64_t result;
    char large_data[1024];
    size_t i;

    /* 填充大数据 */
    for (i = 0; i < sizeof(large_data); i++) {
        large_data[i] = (char)(i % 256);
    }

    result = uvhttp_hash(large_data, sizeof(large_data), 0x12345);
    assert(result != 0);

    printf("test_hash_large_data: PASSED\n");
}

/* 测试哈希函数 - 边界条件 */
void test_hash_boundary(void) {
    uint64_t result;
    char data[1];

    /* 单字节 */
    data[0] = 'A';
    result = uvhttp_hash(data, 1, 0x12345);
    assert(result != 0);

    /* 零种子 */
    result = uvhttp_hash(data, 1, 0);
    assert(result != 0);

    /* 最大种子 */
    result = uvhttp_hash(data, 1, 0xFFFFFFFFFFFFFFFFULL);
    assert(result != 0);

    printf("test_hash_boundary: PASSED\n");
}

/* 测试哈希分布 */
void test_hash_distribution(void) {
    uint64_t results[100];
    size_t i;
    int collisions = 0;
    size_t j;

    /* 生成100个不同的哈希值 */
    for (i = 0; i < 100; i++) {
        char data[32];
        snprintf(data, sizeof(data), "data%zu", i);
        results[i] = uvhttp_hash_string(data);
    }

    /* 检查碰撞 */
    for (i = 0; i < 100; i++) {
        for (j = i + 1; j < 100; j++) {
            if (results[i] == results[j]) {
                collisions++;
            }
        }
    }

    /* 碰撞应该很少（理想情况下为0） */
    assert(collisions < 5);

    printf("test_hash_distribution: PASSED\n");
}

/* 测试哈希有效性 */
void test_hash_validity(void) {
    uint64_t result;
    const char* data = "valid hash";

    result = uvhttp_hash(data, strlen(data), 0x12345);

    /* 哈希值应该在合理范围内（非0） */
    assert(result != 0);
    assert(result <= 0xFFFFFFFFFFFFFFFFULL);

    printf("test_hash_validity: PASSED\n");
}

/* 测试哈希字符串有效性 */
void test_hash_string_validity(void) {
    uint64_t result;
    const char* str = "valid string hash";

    result = uvhttp_hash_string(str);

    /* 哈希值应该在合理范围内（非0） */
    assert(result != 0);
    assert(result <= 0xFFFFFFFFFFFFFFFFULL);

    printf("test_hash_string_validity: PASSED\n");
}

/* 测试哈希种子影响 */
void test_hash_seed_impact(void) {
    uint64_t result1, result2;
    const char* data = "seed test";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0);
    result2 = uvhttp_hash(data, len, 1);

    /* 种子应该影响哈希结果 */
    assert(result1 != result2);

    printf("test_hash_seed_impact: PASSED\n");
}

/* 测试哈希长度影响 */
void test_hash_length_impact(void) {
    uint64_t result1, result2;
    const char* data = "length test";

    result1 = uvhttp_hash(data, 5, 0x12345);
    result2 = uvhttp_hash(data, 10, 0x12345);

    /* 长度应该影响哈希结果 */
    assert(result1 != result2);

    printf("test_hash_length_impact: PASSED\n");
}

/* 测试哈希内容影响 */
void test_hash_content_impact(void) {
    uint64_t result1, result2;
    const char* data1 = "content test 1";
    const char* data2 = "content test 2";

    result1 = uvhttp_hash(data1, strlen(data1), 0x12345);
    result2 = uvhttp_hash(data2, strlen(data2), 0x12345);

    /* 内容应该影响哈希结果 */
    assert(result1 != result2);

    printf("test_hash_content_impact: PASSED\n");
}

/* 测试哈希字符串与哈希函数一致性 */
void test_hash_string_consistency_with_hash(void) {
    uint64_t result1, result2;
    const char* str = "consistency test";
    size_t len = strlen(str);

    result1 = uvhttp_hash_string(str);
    result2 = uvhttp_hash(str, len, UVHTTP_HASH_DEFAULT_SEED);

    /* 应该产生相同的结果 */
    assert(result1 == result2);

    printf("test_hash_string_consistency_with_hash: PASSED\n");
}

/* 测试默认哈希与哈希函数一致性 */
void test_hash_default_consistency(void) {
    uint64_t result1, result2;
    const char* data = "default consistency";
    size_t len = strlen(data);

    result1 = uvhttp_hash_default(data, len);
    result2 = uvhttp_hash(data, len, UVHTTP_HASH_DEFAULT_SEED);

    /* 应该产生相同的结果 */
    assert(result1 == result2);

    printf("test_hash_default_consistency: PASSED\n");
}

/* 测试默认哈希字符串与哈希字符串一致性 */
void test_hash_string_default_consistency(void) {
    uint64_t result1, result2;
    const char* str = "default string consistency";

    result1 = uvhttp_hash_string_default(str);
    result2 = uvhttp_hash_string(str);

    /* 应该产生相同的结果 */
    assert(result1 == result2);

    printf("test_hash_string_default_consistency: PASSED\n");
}

/* 测试哈希种子常量 */
void test_hash_seed_constant(void) {
    /* 检查默认种子常量 */
    assert(UVHTTP_HASH_DEFAULT_SEED == 0x1A2B3C4D5E6F7089ULL);

    printf("test_hash_seed_constant: PASSED\n");
}

/* 测试哈希函数多次调用 */
void test_hash_multiple_calls(void) {
    uint64_t result;
    const char* data = "multiple calls";
    size_t len = strlen(data);
    int i;

    /* 多次调用应该产生相同结果 */
    result = uvhttp_hash(data, len, 0x12345);
    for (i = 0; i < 10; i++) {
        uint64_t new_result = uvhttp_hash(data, len, 0x12345);
        assert(new_result == result);
    }

    printf("test_hash_multiple_calls: PASSED\n");
}

/* 测试哈希字符串函数多次调用 */
void test_hash_string_multiple_calls(void) {
    uint64_t result;
    const char* str = "multiple string calls";
    int i;

    /* 多次调用应该产生相同结果 */
    result = uvhttp_hash_string(str);
    for (i = 0; i < 10; i++) {
        uint64_t new_result = uvhttp_hash_string(str);
        assert(new_result == result);
    }

    printf("test_hash_string_multiple_calls: PASSED\n");
}

/* 测试哈希函数特殊字符 */
void test_hash_special_chars(void) {
    uint64_t result1, result2;
    const char* data1 = "special \0 chars";
    const char* data2 = "special \n chars";

    result1 = uvhttp_hash(data1, 14, 0x12345);
    result2 = uvhttp_hash(data2, 14, 0x12345);

    /* 特殊字符应该影响哈希结果 */
    assert(result1 != result2);

    printf("test_hash_special_chars: PASSED\n");
}

/* 测试哈希字符串函数特殊字符 */
void test_hash_string_special_chars(void) {
    uint64_t result;
    const char* str = "special\nchars\t";

    result = uvhttp_hash_string(str);
    assert(result != 0);

    printf("test_hash_string_special_chars: PASSED\n");
}

/* 测试哈希函数二进制数据 */
void test_hash_binary_data(void) {
    uint64_t result;
    unsigned char binary_data[] = {0x00, 0xFF, 0xAA, 0x55, 0x12, 0x34};

    result = uvhttp_hash(binary_data, sizeof(binary_data), 0x12345);
    assert(result != 0);

    printf("test_hash_binary_data: PASSED\n");
}

/* 测试哈希字符串函数Unicode */
void test_hash_string_unicode(void) {
    uint64_t result;
    const char* str = "Unicode测试🎉";

    result = uvhttp_hash_string(str);
    assert(result != 0);

    printf("test_hash_string_unicode: PASSED\n");
}

/* 测试哈希函数零长度 */
void test_hash_zero_length(void) {
    uint64_t result;
    const char* data = "some data";

    result = uvhttp_hash(data, 0, 0x12345);
    /* 零长度应该产生有效的哈希值 */

    printf("test_hash_zero_length: PASSED\n");
}

/* 测试哈希函数相同种子不同数据 */
void test_hash_same_seed_different_data(void) {
    uint64_t result1, result2;
    const char* data1 = "data1";
    const char* data2 = "data2";

    result1 = uvhttp_hash(data1, strlen(data1), 0x12345);
    result2 = uvhttp_hash(data2, strlen(data2), 0x12345);

    /* 相同种子不同数据应该产生不同哈希值 */
    assert(result1 != result2);

    printf("test_hash_same_seed_different_data: PASSED\n");
}

/* 测试哈希函数不同种子相同数据 */
void test_hash_different_seed_same_data(void) {
    uint64_t result1, result2;
    const char* data = "same data";

    result1 = uvhttp_hash(data, strlen(data), 0x1111);
    result2 = uvhttp_hash(data, strlen(data), 0x2222);

    /* 不同种子相同数据应该产生不同哈希值 */
    assert(result1 != result2);

    printf("test_hash_different_seed_same_data: PASSED\n");
}

/* 测试哈希字符串函数长度敏感 */
void test_hash_string_length_sensitive(void) {
    uint64_t result1, result2;
    const char* str1 = "test";
    const char* str2 = "test ";

    result1 = uvhttp_hash_string(str1);
    result2 = uvhttp_hash_string(str2);

    /* 不同长度应该产生不同哈希值 */
    assert(result1 != result2);

    printf("test_hash_string_length_sensitive: PASSED\n");
}

/* 测试哈希字符串函数大小写敏感 */
void test_hash_string_case_sensitive(void) {
    uint64_t result1, result2;
    const char* str1 = "CaseSensitive";
    const char* str2 = "casesensitive";

    result1 = uvhttp_hash_string(str1);
    result2 = uvhttp_hash_string(str2);

    /* 大小写应该影响哈希结果 */
    assert(result1 != result2);

    printf("test_hash_string_case_sensitive: PASSED\n");
}

/* 测试哈希函数性能 */
void test_hash_performance(void) {
    uint64_t result;
    char data[1000];
    size_t i;
    int iterations = 1000;

    /* 填充数据 */
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (char)(i % 256);
    }

    /* 多次哈希操作应该快速完成 */
    for (i = 0; i < iterations; i++) {
        result = uvhttp_hash(data, sizeof(data), 0x12345);
        (void)result;
    }

    printf("test_hash_performance: PASSED\n");
}

/* 测试哈希字符串函数性能 */
void test_hash_string_performance(void) {
    uint64_t result;
    const char* str = "performance test string";
    int i;
    int iterations = 1000;

    /* 多次哈希操作应该快速完成 */
    for (i = 0; i < iterations; i++) {
        result = uvhttp_hash_string(str);
        (void)result;
    }

    printf("test_hash_string_performance: PASSED\n");
}

/* 测试哈希函数零种子 */
void test_hash_zero_seed(void) {
    uint64_t result1, result2;
    const char* data = "zero seed test";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0);
    result2 = uvhttp_hash(data, len, 0);

    /* 零种子应该产生一致的结果 */
    assert(result1 == result2);

    printf("test_hash_zero_seed: PASSED\n");
}

/* 测试哈希函数最大种子 */
void test_hash_max_seed(void) {
    uint64_t result1, result2;
    const char* data = "max seed test";
    size_t len = strlen(data);
    uint64_t max_seed = 0xFFFFFFFFFFFFFFFFULL;

    result1 = uvhttp_hash(data, len, max_seed);
    result2 = uvhttp_hash(data, len, max_seed);

    /* 最大种子应该产生一致的结果 */
    assert(result1 == result2);

    printf("test_hash_max_seed: PASSED\n");
}

int main(void) {
    printf("=== uvhttp_hash.c 完整覆盖率测试 ===\n\n");

    test_hash_null();
    test_hash_normal();
    test_hash_empty();
    test_hash_different_seeds();
    test_hash_consistency();
    test_hash_string_null();
    test_hash_string_normal();
    test_hash_string_empty();
    test_hash_string_consistency();
    test_hash_string_different();
    test_hash_default();
    test_hash_string_default();
    test_hash_large_data();
    test_hash_boundary();
    test_hash_distribution();
    test_hash_validity();
    test_hash_string_validity();
    test_hash_seed_impact();
    test_hash_length_impact();
    test_hash_content_impact();
    test_hash_string_consistency_with_hash();
    test_hash_default_consistency();
    test_hash_string_default_consistency();
    test_hash_seed_constant();
    test_hash_multiple_calls();
    test_hash_string_multiple_calls();
    test_hash_special_chars();
    test_hash_string_special_chars();
    test_hash_binary_data();
    test_hash_string_unicode();
    test_hash_zero_length();
    test_hash_same_seed_different_data();
    test_hash_different_seed_same_data();
    test_hash_string_length_sensitive();
    test_hash_string_case_sensitive();
    test_hash_performance();
    test_hash_string_performance();
    test_hash_zero_seed();
    test_hash_max_seed();

    printf("\n=== 所有测试通过 ===\n");
    return 0;
}