# xxHash 高性能哈希集成指南

## 概述

UVHTTP 项目集成了 **xxHash** 作为核心哈希算法，以提供极高性能的路由查找、缓存键生成和数据完整性校验功能。

## 性能特性

### 优势对比

| 特性 | xxHash | CRC32 | FNV-1a | MD5 |
|------|--------|-------|--------|-----|
| **速度** | ⚡⚡⚡ 极快 | ⚡⚡ 快 | ⚡ 中等 | ⚡ 慢 |
| **冲突率** | 低 | 中 | 中 | 极低 |
| **分布质量** | 优秀 | 良好 | 中等 | 优秀 |
| **适用场景** | 路由、缓存、哈希表 | 校验和、简单哈希 | 字符串哈希 | 安全哈希 |
| **跨平台** | ✅ 全平台 | ✅ 全平台 | ✅ 全平台 | ✅ 全平台 |

### 性能数据

- **比 CRC32 快 3-5 倍**
- **接近 RAM 速度限制**
- **零分配设计，无内存开销**
- **优秀的缓存局部性**

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

### 路由系统优化

xxHash 在路由系统中用于：

1. **路由哈希表查找**
   ```c
   // 路由节点哈希（内部使用）
   uint32_t route_hash = uvhttp_route_hash("/api/users", UVHTTP_GET);
   ```

2. **缓存键生成**
   ```c
   // 路径缓存键
   uint64_t path_cache_key = uvhttp_hash_string("/static/css/style.css");
   ```

3. **参数提取优化**
   ```c
   // URL参数哈希
   uint64_t param_hash = uvhttp_hash_string("user_id=123");
   ```

### 字符串池优化

字符串池使用 xxHash 进行：

```c
// 字符串池中的快速哈希查找
static inline uint32_t uvhttp_string_hash(const char* str, size_t length) {
    return (uint32_t)uvhttp_hash_string(str);
}
```

### 安全特性

#### 哈希冲突攻击防护

```c
static inline uint32_t route_hash(const char* str) {
    if (!str) return 0;
    
    // 限制最大字符串长度以防止哈希冲突攻击
    size_t len = strlen(str);
    if (len > 1024) {
        len = 1024;  // 截断过长的字符串
    }
    
    return (uint32_t)XXH64(str, len, UVHTTP_HASH_DEFAULT_SEED);
}
```

## 最佳实践

### 1. 字符串哈希

```c
// ✅ 推荐：使用专用字符串哈希函数
uint64_t hash = uvhttp_hash_string("user_data");

// ❌ 避免：手动计算长度
uint64_t hash = uvhttp_hash("user_data", strlen("user_data"), seed);
```

### 2. 默认种子使用

```c
// ✅ 推荐：使用默认种子确保一致性
uint64_t hash = uvhttp_hash_default(data, length);

// ✅ 可选：自定义种子用于特定场景
uint64_t hash = uvhttp_hash(data, length, custom_seed);
```

### 3. 性能优化

```c
// ✅ 推荐：缓存频繁使用的哈希值
static uint64_t cached_route_hash = 0;
if (!cached_route_hash) {
    cached_route_hash = uvhttp_hash_string("/api/users");
}

// ✅ 推荐：批量处理时重用种子
uint64_t seed = UVHTTP_HASH_DEFAULT_SEED;
for (int i = 0; i < count; i++) {
    hashes[i] = uvhttp_hash(data[i], lengths[i], seed);
}
```

### 4. 错误处理

```c
// ✅ 推荐：输入验证
if (!data || length == 0) {
    return 0; // 或适当的错误值
}

uint64_t hash = uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
```

## 安全考虑

### 适用场景

xxHash 适用于以下**非加密**场景：

- ✅ 路由查找和匹配
- ✅ 缓存键生成
- ✅ 数据完整性校验
- ✅ 负载均衡
- ✅ 哈希表实现

### 不适用场景

xxHash **不适用于**以下加密场景：

- ❌ 密码存储
- ❌ 数字签名
- ❌ 加密密钥生成
- ❌ 安全令牌

### 安全增强

对于需要更高安全性的场景，可以考虑：

```c
// 结合盐值使用
const char* salt = "random_salt_value";
char combined_data[256];
snprintf(combined_data, sizeof(combined_data), "%s%s", salt, input_data);
uint64_t secure_hash = uvhttp_hash_string(combined_data);
```

## 性能调优

### 编译优化

确保在 Release 模式下编译以获得最佳性能：

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
endif()
```

### 内存对齐

xxHash 对内存对齐敏感，确保数据结构对齐：

```c
// ✅ 推荐：对齐的数据结构
typedef struct __attribute__((aligned(8))) {
    char data[64];
} aligned_data_t;

// 避免非对齐访问
```

### 批量处理

对于大量数据的哈希计算，考虑批量处理：

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
- [UVHTTP API 参考](./API_REFERENCE.md)