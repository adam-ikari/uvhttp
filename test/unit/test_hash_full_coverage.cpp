/* UVHTTP 哈希模块完整覆盖率测试 */

#include <gtest/gtest.h>
#include <string.h>
#include "uvhttp_hash.h"

/* 测试基础哈希函数 - 正常情况 */
TEST(UvhttpHashTest, HashNormalData) {
    const char* data = "Hello, World!";
    size_t length = strlen(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试基础哈希函数 - 空数据 */
TEST(UvhttpHashTest, HashEmptyData) {
    const char* data = "";
    size_t length = 0;
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 空数据的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试基础哈希函数 - NULL指针 */
TEST(UvhttpHashTest, HashNullPointer) {
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(NULL, 100, seed);
    
    /* NULL指针的哈希值应该为0 */
    EXPECT_EQ(hash, 0ULL);
}

/* 测试基础哈希函数 - 不同种子 */
TEST(UvhttpHashTest, HashDifferentSeeds) {
    const char* data = "Hello, World!";
    size_t length = strlen(data);
    
    uint64_t seed1 = 0x123456789ABCDEF0ULL;
    uint64_t seed2 = 0xFEDCBA9876543210ULL;
    
    uint64_t hash1 = uvhttp_hash(data, length, seed1);
    uint64_t hash2 = uvhttp_hash(data, length, seed2);
    
    /* 不同种子应该产生不同的哈希值 */
    EXPECT_NE(hash1, hash2);
}

/* 测试基础哈希函数 - 相同数据相同种子 */
TEST(UvhttpHashTest, HashSameDataSameSeed) {
    const char* data = "Hello, World!";
    size_t length = strlen(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash1 = uvhttp_hash(data, length, seed);
    uint64_t hash2 = uvhttp_hash(data, length, seed);
    
    /* 相同数据和种子应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
}

/* 测试基础哈希函数 - 不同数据 */
TEST(UvhttpHashTest, HashDifferentData) {
    const char* data1 = "Hello, World!";
    const char* data2 = "Hello, Universe!";
    size_t length1 = strlen(data1);
    size_t length2 = strlen(data2);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash1 = uvhttp_hash(data1, length1, seed);
    uint64_t hash2 = uvhttp_hash(data2, length2, seed);
    
    /* 不同数据应该产生不同的哈希值 */
    EXPECT_NE(hash1, hash2);
}

/* 测试基础哈希函数 - 大数据 */
TEST(UvhttpHashTest, HashLargeData) {
    char large_data[10000];
    memset(large_data, 'A', sizeof(large_data));
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(large_data, sizeof(large_data), seed);
    
    /* 大数据的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试基础哈希函数 - 单字节 */
TEST(UvhttpHashTest, HashSingleByte) {
    char data = 'X';
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(&data, 1, seed);
    
    /* 单字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试基础哈希函数 - 零种子 */
TEST(UvhttpHashTest, HashZeroSeed) {
    const char* data = "Hello, World!";
    size_t length = strlen(data);
    uint64_t seed = 0;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 零种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试基础哈希函数 - 默认种子 */
TEST(UvhttpHashTest, HashDefaultSeed) {
    const char* data = "Hello, World!";
    size_t length = strlen(data);
    
    uint64_t hash = uvhttp_hash_default(data, length);
    
    /* 默认种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试字符串哈希函数 - 正常字符串 */
TEST(UvhttpHashTest, HashStringNormal) {
    const char* str = "Hello, World!";
    
    uint64_t hash = uvhttp_hash_string(str);
    
    /* 字符串哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试字符串哈希函数 - 空字符串 */
TEST(UvhttpHashTest, HashStringEmpty) {
    const char* str = "";
    
    uint64_t hash = uvhttp_hash_string(str);
    
    /* 空字符串的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试字符串哈希函数 - NULL指针 */
TEST(UvhttpHashTest, HashStringNullPointer) {
    uint64_t hash = uvhttp_hash_string(NULL);
    
    /* NULL指针的哈希值应该为0 */
    EXPECT_EQ(hash, 0ULL);
}

/* 测试字符串哈希函数 - 相同字符串 */
TEST(UvhttpHashTest, HashStringSameString) {
    const char* str = "Hello, World!";
    
    uint64_t hash1 = uvhttp_hash_string(str);
    uint64_t hash2 = uvhttp_hash_string(str);
    
    /* 相同字符串应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
}

/* 测试字符串哈希函数 - 不同字符串 */
TEST(UvhttpHashTest, HashStringDifferentString) {
    const char* str1 = "Hello, World!";
    const char* str2 = "Hello, Universe!";
    
    uint64_t hash1 = uvhttp_hash_string(str1);
    uint64_t hash2 = uvhttp_hash_string(str2);
    
    /* 不同字符串应该产生不同的哈希值 */
    EXPECT_NE(hash1, hash2);
}

/* 测试字符串哈希函数 - 长字符串 */
TEST(UvhttpHashTest, HashStringLongString) {
    char long_str[1000];
    memset(long_str, 'A', sizeof(long_str) - 1);
    long_str[sizeof(long_str) - 1] = '\0';
    
    uint64_t hash = uvhttp_hash_string(long_str);
    
    /* 长字符串的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试字符串哈希函数 - 默认种子 */
TEST(UvhttpHashTest, HashStringDefaultSeed) {
    const char* str = "Hello, World!";
    
    uint64_t hash1 = uvhttp_hash_string(str);
    uint64_t hash2 = uvhttp_hash_string_default(str);
    
    /* 默认种子的哈希值应该相同 */
    EXPECT_EQ(hash1, hash2);
}

/* 测试哈希值的一致性 - 相同输入相同输出 */
TEST(UvhttpHashTest, HashConsistency) {
    const char* data = "Test data for consistency";
    size_t length = strlen(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash1 = uvhttp_hash(data, length, seed);
    uint64_t hash2 = uvhttp_hash(data, length, seed);
    uint64_t hash3 = uvhttp_hash(data, length, seed);
    
    /* 多次调用应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
}

/* 测试哈希值的一致性 - 字符串 */
TEST(UvhttpHashTest, HashStringConsistency) {
    const char* str = "Test string for consistency";
    
    uint64_t hash1 = uvhttp_hash_string(str);
    uint64_t hash2 = uvhttp_hash_string(str);
    uint64_t hash3 = uvhttp_hash_string(str);
    
    /* 多次调用应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
    EXPECT_EQ(hash2, hash3);
}

/* 测试哈希值的分布 - 不同输入不同输出 */
TEST(UvhttpHashTest, HashDistribution) {
    const char* data1 = "Data 1";
    const char* data2 = "Data 2";
    const char* data3 = "Data 3";
    size_t length1 = strlen(data1);
    size_t length2 = strlen(data2);
    size_t length3 = strlen(data3);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash1 = uvhttp_hash(data1, length1, seed);
    uint64_t hash2 = uvhttp_hash(data2, length2, seed);
    uint64_t hash3 = uvhttp_hash(data3, length3, seed);
    
    /* 不同输入应该产生不同的哈希值 */
    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash2, hash3);
    EXPECT_NE(hash1, hash3);
}

/* 测试哈希值的分布 - 字符串 */
TEST(UvhttpHashTest, HashStringDistribution) {
    const char* str1 = "String 1";
    const char* str2 = "String 2";
    const char* str3 = "String 3";
    
    uint64_t hash1 = uvhttp_hash_string(str1);
    uint64_t hash2 = uvhttp_hash_string(str2);
    uint64_t hash3 = uvhttp_hash_string(str3);
    
    /* 不同字符串应该产生不同的哈希值 */
    EXPECT_NE(hash1, hash2);
    EXPECT_NE(hash2, hash3);
    EXPECT_NE(hash1, hash3);
}

/* 测试哈希函数的边界值 - 最大种子 */
TEST(UvhttpHashTest, HashMaxSeed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = UINT64_MAX;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 最大种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 最小种子 */
TEST(UvhttpHashTest, HashMinSeed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = 1;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 最小种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的性能 - 多次调用 */
TEST(UvhttpHashTest, HashPerformance) {
    const char* data = "Performance test data";
    size_t length = strlen(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    /* 调用多次以确保性能 */
    for (int i = 0; i < 1000; i++) {
        uint64_t hash = uvhttp_hash(data, length, seed);
        EXPECT_NE(hash, 0ULL);
    }
}

/* 测试字符串哈希函数的性能 - 多次调用 */
TEST(UvhttpHashTest, HashStringPerformance) {
    const char* str = "Performance test string";
    
    /* 调用多次以确保性能 */
    for (int i = 0; i < 1000; i++) {
        uint64_t hash = uvhttp_hash_string(str);
        EXPECT_NE(hash, 0ULL);
    }
}

/* 测试哈希函数的雪崩效应 - 微小变化产生巨大差异 */
TEST(UvhttpHashTest, HashAvalancheEffect) {
    const char* data1 = "Hello, World!";
    const char* data2 = "Hello, World?";
    size_t length = strlen(data1);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash1 = uvhttp_hash(data1, length, seed);
    uint64_t hash2 = uvhttp_hash(data2, length, seed);
    
    /* 微小变化应该产生巨大的哈希值差异 */
    EXPECT_NE(hash1, hash2);
    
    /* 计算哈希值的汉明距离 */
    uint64_t xor_result = hash1 ^ hash2;
    int bit_count = 0;
    while (xor_result) {
        bit_count += xor_result & 1;
        xor_result >>= 1;
    }
    
    /* 汉明距离应该大于0 */
    EXPECT_GT(bit_count, 0);
}

/* 测试哈希函数的雪崩效应 - 字符串 */
TEST(UvhttpHashTest, HashStringAvalancheEffect) {
    const char* str1 = "Hello, World!";
    const char* str2 = "Hello, World?";
    
    uint64_t hash1 = uvhttp_hash_string(str1);
    uint64_t hash2 = uvhttp_hash_string(str2);
    
    /* 微小变化应该产生巨大的哈希值差异 */
    EXPECT_NE(hash1, hash2);
    
    /* 计算哈希值的汉明距离 */
    uint64_t xor_result = hash1 ^ hash2;
    int bit_count = 0;
    while (xor_result) {
        bit_count += xor_result & 1;
        xor_result >>= 1;
    }
    
    /* 汉明距离应该大于0 */
    EXPECT_GT(bit_count, 0);
}

/* 测试哈希函数的确定性 - 相同输入相同输出（跨函数） */
TEST(UvhttpHashTest, HashDeterminismAcrossFunctions) {
    const char* str = "Test string";
    size_t length = strlen(str);
    uint64_t seed = UVHTTP_HASH_DEFAULT_SEED;
    
    uint64_t hash1 = uvhttp_hash(str, length, seed);
    uint64_t hash2 = uvhttp_hash_string(str);
    
    /* 相同输入应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
}

/* 测试哈希函数的错误处理 - NULL指针 */
TEST(UvhttpHashTest, HashErrorHandlingNullPointer) {
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(NULL, 100, seed);
    
    /* NULL指针应该返回0 */
    EXPECT_EQ(hash, 0ULL);
}

/* 测试字符串哈希函数的错误处理 - NULL指针 */
TEST(UvhttpHashTest, HashStringErrorHandlingNullPointer) {
    uint64_t hash = uvhttp_hash_string(NULL);
    
    /* NULL指针应该返回0 */
    EXPECT_EQ(hash, 0ULL);
}

/* 测试哈希函数的零长度数据 */
TEST(UvhttpHashTest, HashZeroLengthData) {
    const char* data = "Test data";
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, 0, seed);
    
    /* 零长度数据的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - UINT64_MAX种子 */
TEST(UvhttpHashTest, HashUint64MaxSeed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = UINT64_MAX;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* UINT64_MAX种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 0xFFFFFFFFFFFFFFFF种子 */
TEST(UvhttpHashTest, HashFFFFFFFFFFFFFFFFSeed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = 0xFFFFFFFFFFFFFFFFULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 0xFFFFFFFFFFFFFFFF种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 0x8000000000000000种子 */
TEST(UvhttpHashTest, Hash8000000000000000Seed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = 0x8000000000000000ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 0x8000000000000000种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 0x1种子 */
TEST(UvhttpHashTest, Hash01Seed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = 0x1ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 0x1种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 0x0种子 */
TEST(UvhttpHashTest, Hash00Seed) {
    const char* data = "Test data";
    size_t length = strlen(data);
    uint64_t seed = 0x0ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 0x0种子的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 默认种子 */
TEST(UvhttpHashTest, HashDefaultSeedValue) {
    const char* data = "Test data";
    size_t length = strlen(data);
    
    uint64_t hash1 = uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
    uint64_t hash2 = uvhttp_hash_default(data, length);
    
    /* 默认种子应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
}

/* 测试字符串哈希函数的边界值 - 默认种子 */
TEST(UvhttpHashTest, HashStringDefaultSeedValue) {
    const char* str = "Test string";
    
    uint64_t hash1 = uvhttp_hash_string(str);
    uint64_t hash2 = uvhttp_hash_string_default(str);
    
    /* 默认种子应该产生相同的哈希值 */
    EXPECT_EQ(hash1, hash2);
}

/* 测试哈希函数的边界值 - 特殊字符 */
TEST(UvhttpHashTest, HashSpecialCharacters) {
    const char* data = "!@#$%^&*()_+-=[]{}|;':\",./<>?";
    size_t length = strlen(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 特殊字符的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试字符串哈希函数的边界值 - 特殊字符 */
TEST(UvhttpHashTest, HashStringSpecialCharacters) {
    const char* str = "!@#$%^&*()_+-=[]{}|;':\",./<>?";
    
    uint64_t hash = uvhttp_hash_string(str);
    
    /* 特殊字符的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - Unicode字符 */
TEST(UvhttpHashTest, HashUnicodeCharacters) {
    const char* data = "Hello 世界 🌍";
    size_t length = strlen(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* Unicode字符的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试字符串哈希函数的边界值 - Unicode字符 */
TEST(UvhttpHashTest, HashStringUnicodeCharacters) {
    const char* str = "Hello 世界 🌍";
    
    uint64_t hash = uvhttp_hash_string(str);
    
    /* Unicode字符的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 二进制数据 */
TEST(UvhttpHashTest, HashBinaryData) {
    unsigned char data[] = {0x00, 0x01, 0x02, 0x03, 0xFF, 0xFE, 0xFD, 0xFC};
    size_t length = sizeof(data);
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, length, seed);
    
    /* 二进制数据的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 所有零字节 */
TEST(UvhttpHashTest, HashAllZeroBytes) {
    unsigned char data[100];
    memset(data, 0, sizeof(data));
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 所有零字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 所有0xFF字节 */
TEST(UvhttpHashTest, HashAllFFBytes) {
    unsigned char data[100];
    memset(data, 0xFF, sizeof(data));
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 所有0xFF字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 交替字节 */
TEST(UvhttpHashTest, HashAlternatingBytes) {
    unsigned char data[100];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (i % 2) ? 0xAA : 0x55;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 交替字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 递增字节 */
TEST(UvhttpHashTest, HashIncrementingBytes) {
    unsigned char data[256];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 递增字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 递减字节 */
TEST(UvhttpHashTest, HashDecrementingBytes) {
    unsigned char data[256];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)(255 - i);
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 递减字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 最大长度 */
TEST(UvhttpHashTest, HashMaxLength) {
    unsigned char data[10000];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)(i % 256);
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 最大长度的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 单字节最大值 */
TEST(UvhttpHashTest, HashSingleByteMaxValue) {
    unsigned char data = 0xFF;
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(&data, 1, seed);
    
    /* 单字节最大值的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 单字节最小值 */
TEST(UvhttpHashTest, HashSingleByteMinValue) {
    unsigned char data = 0x00;
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(&data, 1, seed);
    
    /* 单字节最小值的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 单字节中间值 */
TEST(UvhttpHashTest, HashSingleByteMidValue) {
    unsigned char data = 0x80;
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(&data, 1, seed);
    
    /* 单字节中间值的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 双字节 */
TEST(UvhttpHashTest, HashTwoBytes) {
    unsigned char data[] = {0x12, 0x34};
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 双字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 四字节 */
TEST(UvhttpHashTest, HashFourBytes) {
    unsigned char data[] = {0x12, 0x34, 0x56, 0x78};
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 四字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 八字节 */
TEST(UvhttpHashTest, HashEightBytes) {
    unsigned char data[] = {0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0};
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 八字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 十六字节 */
TEST(UvhttpHashTest, HashSixteenBytes) {
    unsigned char data[] = {
        0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0,
        0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88
    };
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 十六字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 三十二字节 */
TEST(UvhttpHashTest, HashThirtyTwoBytes) {
    unsigned char data[32];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 三十二字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 六十四字节 */
TEST(UvhttpHashTest, HashSixtyFourBytes) {
    unsigned char data[64];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 六十四字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 一百二十八字节 */
TEST(UvhttpHashTest, HashOneHundredTwentyEightBytes) {
    unsigned char data[128];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 一百二十八字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 二百五十六字节 */
TEST(UvhttpHashTest, HashTwoHundredFiftySixBytes) {
    unsigned char data[256];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 二百五十六字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 五百一十二字节 */
TEST(UvhttpHashTest, HashFiveHundredTwelveBytes) {
    unsigned char data[512];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 五百一十二字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 一千零二十四字节 */
TEST(UvhttpHashTest, HashOneThousandTwentyFourBytes) {
    unsigned char data[1024];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 一千零二十四字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 二千零四十八字节 */
TEST(UvhttpHashTest, HashTwoThousandFortyEightBytes) {
    unsigned char data[2048];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 二千零四十八字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 四千零九十六字节 */
TEST(UvhttpHashTest, HashFourThousandNinetySixBytes) {
    unsigned char data[4096];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 四千零九十六字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}

/* 测试哈希函数的边界值 - 八千一百九十二字节 */
TEST(UvhttpHashTest, HashEightThousandOneHundredNinetyTwoBytes) {
    unsigned char data[8192];
    for (size_t i = 0; i < sizeof(data); i++) {
        data[i] = (unsigned char)i;
    }
    uint64_t seed = 0x123456789ABCDEF0ULL;
    
    uint64_t hash = uvhttp_hash(data, sizeof(data), seed);
    
    /* 八千一百九十二字节的哈希值不应该为0 */
    EXPECT_NE(hash, 0ULL);
}
