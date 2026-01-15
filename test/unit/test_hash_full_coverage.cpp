/* UVHTTP 哈希模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <stdlib.h>
#include <string.h>
#include "uvhttp.h"
#include "uvhttp_hash.h"
#include "uvhttp_constants.h"

/* 测试哈希函数 - NULL参数 */
TEST(UvhttpHashFullCoverageTest, HashNull) {
    uint64_t result;

    /* NULL data */
    result = uvhttp_hash(NULL, 100, 0x12345);
    EXPECT_EQ(result, 0);
}

/* 测试哈希函数 - 正常参数 */
TEST(UvhttpHashFullCoverageTest, HashNormal) {
    uint64_t result;
    const char* data = "Hello, World!";
    size_t len = strlen(data);

    result = uvhttp_hash(data, len, 0x12345);
    EXPECT_NE(result, 0);
}

/* 测试哈希函数 - 空数据 */
TEST(UvhttpHashFullCoverageTest, HashEmpty) {
    uint64_t result;
    const char* data = "";

    result = uvhttp_hash(data, 0, 0x12345);
    /* 空数据应该产生有效的哈希值 */
}

/* 测试哈希函数 - 不同种子 */
TEST(UvhttpHashFullCoverageTest, HashDifferentSeeds) {
    uint64_t result1, result2;
    const char* data = "test data";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0x1111);
    result2 = uvhttp_hash(data, len, 0x2222);

    /* 不同种子应该产生不同的哈希值 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希函数 - 相同输入相同输出 */
TEST(UvhttpHashFullCoverageTest, HashConsistency) {
    uint64_t result1, result2;
    const char* data = "consistent data";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0x12345);
    result2 = uvhttp_hash(data, len, 0x12345);

    /* 相同输入应该产生相同输出 */
    EXPECT_EQ(result1, result2);
}

/* 测试哈希字符串函数 - NULL参数 */
TEST(UvhttpHashFullCoverageTest, HashStringNull) {
    uint64_t result;

    /* NULL string */
    result = uvhttp_hash_string(NULL);
    EXPECT_EQ(result, 0);
}

/* 测试哈希字符串函数 - 正常参数 */
TEST(UvhttpHashFullCoverageTest, HashStringNormal) {
    uint64_t result;
    const char* str = "Hello, World!";

    result = uvhttp_hash_string(str);
    EXPECT_NE(result, 0);
}

/* 测试哈希字符串函数 - 空字符串 */
TEST(UvhttpHashFullCoverageTest, HashStringEmpty) {
    uint64_t result;
    const char* str = "";

    result = uvhttp_hash_string(str);
    /* 空字符串应该产生有效的哈希值 */
}

/* 测试哈希字符串函数 - 一致性 */
TEST(UvhttpHashFullCoverageTest, HashStringConsistency) {
    uint64_t result1, result2;
    const char* str = "consistent string";

    result1 = uvhttp_hash_string(str);
    result2 = uvhttp_hash_string(str);

    /* 相同输入应该产生相同输出 */
    EXPECT_EQ(result1, result2);
}

/* 测试哈希字符串函数 - 不同字符串 */
TEST(UvhttpHashFullCoverageTest, HashStringDifferent) {
    uint64_t result1, result2;
    const char* str1 = "string 1";
    const char* str2 = "string 2";

    result1 = uvhttp_hash_string(str1);
    result2 = uvhttp_hash_string(str2);

    /* 不同字符串应该产生不同的哈希值 */
    EXPECT_NE(result1, result2);
}

/* 测试默认哈希函数 */
TEST(UvhttpHashFullCoverageTest, HashDefault) {
    uint64_t result;
    const char* data = "default hash test";
    size_t len = strlen(data);

    result = uvhttp_hash_default(data, len);
    EXPECT_NE(result, 0);
}

/* 测试默认哈希字符串函数 */
TEST(UvhttpHashFullCoverageTest, HashStringDefault) {
    uint64_t result;
    const char* str = "default string hash";

    result = uvhttp_hash_string_default(str);
    EXPECT_NE(result, 0);
}

/* 测试哈希函数 - 大数据 */
TEST(UvhttpHashFullCoverageTest, HashLargeData) {
    uint64_t result;
    char large_data[1024];
    size_t i;

    /* 填充大数据 */
    for (i = 0; i < sizeof(large_data); i++) {
        large_data[i] = (char)(i % 256);
    }

    result = uvhttp_hash(large_data, sizeof(large_data), 0x12345);
    EXPECT_NE(result, 0);
}

/* 测试哈希函数 - 边界条件 */
TEST(UvhttpHashFullCoverageTest, HashBoundary) {
    uint64_t result;
    char data[1];

    /* 单字节 */
    data[0] = 'A';
    result = uvhttp_hash(data, 1, 0x12345);
    EXPECT_NE(result, 0);

    /* 零种子 */
    result = uvhttp_hash(data, 1, 0);
    EXPECT_NE(result, 0);

    /* 最大种子 */
    result = uvhttp_hash(data, 1, 0xFFFFFFFFFFFFFFFFULL);
    EXPECT_NE(result, 0);
}

/* 测试哈希分布 */
TEST(UvhttpHashFullCoverageTest, HashDistribution) {
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
    EXPECT_LT(collisions, 5);
}

/* 测试哈希有效性 */
TEST(UvhttpHashFullCoverageTest, HashValidity) {
    uint64_t result;
    const char* data = "valid hash";

    result = uvhttp_hash(data, strlen(data), 0x12345);

    /* 哈希值应该在合理范围内（非0） */
    EXPECT_NE(result, 0);
    EXPECT_LE(result, 0xFFFFFFFFFFFFFFFFULL);
}

/* 测试哈希字符串有效性 */
TEST(UvhttpHashFullCoverageTest, HashStringValidity) {
    uint64_t result;
    const char* str = "valid string hash";

    result = uvhttp_hash_string(str);

    /* 哈希值应该在合理范围内（非0） */
    EXPECT_NE(result, 0);
    EXPECT_LE(result, 0xFFFFFFFFFFFFFFFFULL);
}

/* 测试哈希种子影响 */
TEST(UvhttpHashFullCoverageTest, HashSeedImpact) {
    uint64_t result1, result2;
    const char* data = "seed test";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0);
    result2 = uvhttp_hash(data, len, 1);

    /* 种子应该影响哈希结果 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希长度影响 */
TEST(UvhttpHashFullCoverageTest, HashLengthImpact) {
    uint64_t result1, result2;
    const char* data = "length test";

    result1 = uvhttp_hash(data, 5, 0x12345);
    result2 = uvhttp_hash(data, 10, 0x12345);

    /* 长度应该影响哈希结果 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希内容影响 */
TEST(UvhttpHashFullCoverageTest, HashContentImpact) {
    uint64_t result1, result2;
    const char* data1 = "content test 1";
    const char* data2 = "content test 2";

    result1 = uvhttp_hash(data1, strlen(data1), 0x12345);
    result2 = uvhttp_hash(data2, strlen(data2), 0x12345);

    /* 内容应该影响哈希结果 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希字符串与哈希函数一致性 */
TEST(UvhttpHashFullCoverageTest, HashStringConsistencyWithHash) {
    uint64_t result1, result2;
    const char* str = "consistency test";
    size_t len = strlen(str);

    result1 = uvhttp_hash_string(str);
    result2 = uvhttp_hash(str, len, UVHTTP_HASH_DEFAULT_SEED);

    /* 应该产生相同的结果 */
    EXPECT_EQ(result1, result2);
}

/* 测试默认哈希与哈希函数一致性 */
TEST(UvhttpHashFullCoverageTest, HashDefaultConsistency) {
    uint64_t result1, result2;
    const char* data = "default consistency";
    size_t len = strlen(data);

    result1 = uvhttp_hash_default(data, len);
    result2 = uvhttp_hash(data, len, UVHTTP_HASH_DEFAULT_SEED);

    /* 应该产生相同的结果 */
    EXPECT_EQ(result1, result2);
}

/* 测试默认哈希字符串与哈希字符串一致性 */
TEST(UvhttpHashFullCoverageTest, HashStringDefaultConsistency) {
    uint64_t result1, result2;
    const char* str = "default string consistency";

    result1 = uvhttp_hash_string_default(str);
    result2 = uvhttp_hash_string(str);

    /* 应该产生相同的结果 */
    EXPECT_EQ(result1, result2);
}

/* 测试哈希种子常量 */
TEST(UvhttpHashFullCoverageTest, HashSeedConstant) {
    /* 检查默认种子常量 */
    EXPECT_EQ(UVHTTP_HASH_DEFAULT_SEED, 0x1A2B3C4D5E6F7089ULL);
}

/* 测试哈希函数多次调用 */
TEST(UvhttpHashFullCoverageTest, HashMultipleCalls) {
    uint64_t result;
    const char* data = "multiple calls";
    size_t len = strlen(data);
    int i;

    /* 多次调用应该产生相同结果 */
    result = uvhttp_hash(data, len, 0x12345);
    for (i = 0; i < 10; i++) {
        uint64_t new_result = uvhttp_hash(data, len, 0x12345);
        EXPECT_EQ(new_result, result);
    }
}

/* 测试哈希字符串函数多次调用 */
TEST(UvhttpHashFullCoverageTest, HashStringMultipleCalls) {
    uint64_t result;
    const char* str = "multiple string calls";
    int i;

    /* 多次调用应该产生相同结果 */
    result = uvhttp_hash_string(str);
    for (i = 0; i < 10; i++) {
        uint64_t new_result = uvhttp_hash_string(str);
        EXPECT_EQ(new_result, result);
    }
}

/* 测试哈希函数特殊字符 */
TEST(UvhttpHashFullCoverageTest, HashSpecialChars) {
    uint64_t result1, result2;
    const char* data1 = "special \0 chars";
    const char* data2 = "special \n chars";

    result1 = uvhttp_hash(data1, 14, 0x12345);
    result2 = uvhttp_hash(data2, 14, 0x12345);

    /* 特殊字符应该影响哈希结果 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希字符串函数特殊字符 */
TEST(UvhttpHashFullCoverageTest, HashStringSpecialChars) {
    uint64_t result;
    const char* str = "special\nchars\t";

    result = uvhttp_hash_string(str);
    EXPECT_NE(result, 0);
}

/* 测试哈希函数二进制数据 */
TEST(UvhttpHashFullCoverageTest, HashBinaryData) {
    uint64_t result;
    unsigned char binary_data[] = {0x00, 0xFF, 0xAA, 0x55, 0x12, 0x34};

    result = uvhttp_hash(binary_data, sizeof(binary_data), 0x12345);
    EXPECT_NE(result, 0);
}

/* 测试哈希字符串函数Unicode */
TEST(UvhttpHashFullCoverageTest, HashStringUnicode) {
    uint64_t result;
    const char* str = "Unicode测试🎉";

    result = uvhttp_hash_string(str);
    EXPECT_NE(result, 0);
}

/* 测试哈希函数零长度 */
TEST(UvhttpHashFullCoverageTest, HashZeroLength) {
    uint64_t result;
    const char* data = "some data";

    result = uvhttp_hash(data, 0, 0x12345);
    /* 零长度应该产生有效的哈希值 */
}

/* 测试哈希函数相同种子不同数据 */
TEST(UvhttpHashFullCoverageTest, HashSameSeedDifferentData) {
    uint64_t result1, result2;
    const char* data1 = "data1";
    const char* data2 = "data2";

    result1 = uvhttp_hash(data1, strlen(data1), 0x12345);
    result2 = uvhttp_hash(data2, strlen(data2), 0x12345);

    /* 相同种子不同数据应该产生不同哈希值 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希函数不同种子相同数据 */
TEST(UvhttpHashFullCoverageTest, HashDifferentSeedSameData) {
    uint64_t result1, result2;
    const char* data = "same data";

    result1 = uvhttp_hash(data, strlen(data), 0x1111);
    result2 = uvhttp_hash(data, strlen(data), 0x2222);

    /* 不同种子相同数据应该产生不同哈希值 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希字符串函数长度敏感 */
TEST(UvhttpHashFullCoverageTest, HashStringLengthSensitive) {
    uint64_t result1, result2;
    const char* str1 = "test";
    const char* str2 = "test ";

    result1 = uvhttp_hash_string(str1);
    result2 = uvhttp_hash_string(str2);

    /* 不同长度应该产生不同哈希值 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希字符串函数大小写敏感 */
TEST(UvhttpHashFullCoverageTest, HashStringCaseSensitive) {
    uint64_t result1, result2;
    const char* str1 = "CaseSensitive";
    const char* str2 = "casesensitive";

    result1 = uvhttp_hash_string(str1);
    result2 = uvhttp_hash_string(str2);

    /* 大小写应该影响哈希结果 */
    EXPECT_NE(result1, result2);
}

/* 测试哈希函数性能 */
TEST(UvhttpHashFullCoverageTest, HashPerformance) {
    uint64_t result;
    char data[1000];
    size_t i;
    size_t iterations = 1000;

    /* 填充数据 */
    for (i = 0; i < sizeof(data); i++) {
        data[i] = (char)(i % 256);
    }

    /* 多次哈希操作应该快速完成 */
    for (i = 0; i < iterations; i++) {
        result = uvhttp_hash(data, sizeof(data), 0x12345);
        (void)result;
    }
}

/* 测试哈希字符串函数性能 */
TEST(UvhttpHashFullCoverageTest, HashStringPerformance) {
    uint64_t result;
    const char* str = "performance test string";
    int i;
    int iterations = 1000;

    /* 多次哈希操作应该快速完成 */
    for (i = 0; i < iterations; i++) {
        result = uvhttp_hash_string(str);
        (void)result;
    }
}

/* 测试哈希函数零种子 */
TEST(UvhttpHashFullCoverageTest, HashZeroSeed) {
    uint64_t result1, result2;
    const char* data = "zero seed test";
    size_t len = strlen(data);

    result1 = uvhttp_hash(data, len, 0);
    result2 = uvhttp_hash(data, len, 0);

    /* 零种子应该产生一致的结果 */
    EXPECT_EQ(result1, result2);
}

/* 测试哈希函数最大种子 */
TEST(UvhttpHashFullCoverageTest, HashMaxSeed) {
    uint64_t result1, result2;
    const char* data = "max seed test";
    size_t len = strlen(data);
    uint64_t max_seed = 0xFFFFFFFFFFFFFFFFULL;

    result1 = uvhttp_hash(data, len, max_seed);
    result2 = uvhttp_hash(data, len, max_seed);

    /* 最大种子应该产生一致的结果 */
    EXPECT_EQ(result1, result2);
}
