# xxHash 哈希集成指南

## 概述

UVHTTP 使用 **xxHash** 作为哈希算法，用于路由查找、缓存键生成和数据完整性校验。

## 性能特性

### 优势对比

| 特性 | xxHash | CRC32 | FNV-1a | MD5 |
|------|--------|-------|--------|-----|
| **速度** | 极快 | 快 | 中等 | 慢 |
| **冲突率** | 低 | 中 | 中 | 极低 |
| **分布质量** | 优秀 | 良好 | 中等 | 优秀 |
| **适用场景** | 路由、缓存、哈希表 | 校验和、简单哈希 | 字符串哈希 | 安全哈希 |
| **跨平台** | 全平台 | 全平台 | 全平台 | 全平台 |

### 性能数据

- 比 CRC32 快 3-5 倍
- 接近 RAM 速度限制
- 零分配，无内存开销
- 良好的缓存局部性

## API 接口

### 基础哈希函数

```c
#include "uvhttp_hash.h"

// 计算任意数据的哈希值
uint64_t uvhttp_hash(const void* data, size_t length, uint64_t seed);

// 计算字符串的哈希值（推荐）
uint64_t uvhttp_hash_string(const char* str);

// 使用默认种子的便捷函数
uint64_t uvhttp_hash_default(const void* data, size_t length);
```

### 使用示例

#### 基础用法

```c
#include "uvhttp_hash.h"

// 字符串哈希
const char* username = "john_doe";
uint64_t user_hash = uvhttp_hash_string(username);

// 二进制数据哈希
struct user_data data;
uint64_t data_hash = uvhttp_hash(&data, sizeof(data), UVHTTP_HASH_DEFAULT_SEED);

// 使用默认种子
uint64_t simple_hash = uvhttp_hash_default("hello", 5);
```

#### 缓存键生成

```c
// 生成用户缓存键
char cache_key[128];
snprintf(cache_key, sizeof(cache_key), "user:%s:profile", username);
uint64_t cache_hash = uvhttp_hash_string(cache_key);

// 会话键生成
char session_key[256];
snprintf(session_key, sizeof(session_key), "%s:%ld:%s", 
         user_id, timestamp, session_token);
uint64_t session_hash = uvhttp_hash_string(session_key);
```

## 内部集成

### 路由缓存

`src/uvhttp_router_cache.c` 用一个文件内静态函数对路由路径做哈希：

```c
static inline uint32_t route_hash(const char* str) {
    if (!str)
        return 0;

    size_t len = strlen(str);
    if (len > UVHTTP_MAX_ROUTE_PATH_LEN - 1)
        len = UVHTTP_MAX_ROUTE_PATH_LEN - 1;

    return (uint32_t)XXH64(str, len, UVHTTP_HASH_DEFAULT_SEED);
}
```

`UVHTTP_MAX_ROUTE_PATH_LEN` 为 256（`include/uvhttp_constants.h`）。长度上限限制了攻击者可控路径的哈希开销。`route_hash` 只在该编译单元内使用，不属于公开 API。

### gzip 缓存键

`src/uvhttp_response.c` 用响应体哈希加长度作为压缩体缓存的键：

```c
uvhttp_hash_default(response->body, response->body_length)
```

同一份 body 只压缩一次，后续响应复用缓存的压缩结果。

## 最佳实践

### 1. 字符串哈希

```c
// 推荐：使用专用字符串哈希函数
uint64_t hash = uvhttp_hash_string("user_data");

// 避免：手动计算长度
uint64_t hash = uvhttp_hash("user_data", strlen("user_data"), seed);
```

### 2. 默认种子使用

```c
// 推荐：使用默认种子确保一致性
uint64_t hash = uvhttp_hash_default(data, length);

// 可选：自定义种子用于特定场景
uint64_t hash = uvhttp_hash(data, length, custom_seed);
```

### 3. 性能优化

```c
// 推荐：缓存频繁使用的哈希值
static uint64_t cached_route_hash = 0;
if (!cached_route_hash) {
    cached_route_hash = uvhttp_hash_string("/api/users");
}

// 推荐：批量处理时重用种子
uint64_t seed = UVHTTP_HASH_DEFAULT_SEED;
for (int i = 0; i < count; i++) {
    hashes[i] = uvhttp_hash(data[i], lengths[i], seed);
}
```

### 4. 错误处理

```c
// 推荐：输入验证
if (!data || length == 0) {
    return 0; // 或适当的错误值
}

uint64_t hash = uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
```

## 安全考虑

### 适用场景

xxHash 适用于以下非加密场景：

- 路由查找和匹配
- 缓存键生成
- 数据完整性校验
- 负载均衡
- 哈希表实现

### 不适用场景

xxHash 不适用于以下加密场景：

- 密码存储
- 数字签名
- 加密密钥生成
- 安全令牌

### 安全增强

需要更高安全性时，可以加盐：

```c
// 结合盐值使用
const char* salt = "random_salt_value";
char combined_data[256];
snprintf(combined_data, sizeof(combined_data), "%s%s", salt, input_data);
uint64_t secure_hash = uvhttp_hash_string(combined_data);
```

## 性能调优

### 编译优化

在 Release 模式下编译以获得最佳性能：

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
endif()
```

### 内存对齐

xxHash 对内存对齐敏感，需对齐数据结构：

```c
// 推荐：对齐的数据结构
typedef struct __attribute__((aligned(8))) {
    char data[64];
} aligned_data_t;

// 避免非对齐访问
```

### 批量处理

大量数据的哈希计算宜批量处理：

```c
// 批量哈希计算示例
void batch_hash(const char** strings, int count, uint64_t* results) {
    uint64_t seed = UVHTTP_HASH_DEFAULT_SEED;
    for (int i = 0; i < count; i++) {
        results[i] = uvhttp_hash_string(strings[i]);
    }
}
```

## 故障排除

### 常见问题

1. **哈希值不一致**
   - 检查是否使用了相同的种子
   - 确认数据完全相同（包括长度）

2. **性能不如预期**
   - 确保在 Release 模式下编译
   - 检查数据是否对齐
   - 考虑批量处理减少函数调用开销

3. **哈希冲突过多**
   - 检查数据分布是否均匀
   - 考虑使用不同的种子值
   - 评估是否需要更长的哈希值

### 调试技巧

```c
// 调试哈希计算
void debug_hash(const void* data, size_t length) {
    uint64_t hash = uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
    printf("Data: %.*s, Length: %zu, Hash: %llu\n", 
           (int)length, (char*)data, length, (unsigned long long)hash);
}
```

## 参考资料

- [xxHash 官方文档](https://github.com/Cyan4973/xxHash)
- [xxHash 性能基准测试](https://github.com/Cyan4973/xxHash/wiki/Performance-comparison)
- [UVHTTP 架构设计](./ARCHITECTURE.md)
- [UVHTTP API 参考](../../api/API_REFERENCE.md)