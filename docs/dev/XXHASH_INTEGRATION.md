# xxHash Hash Integration Guide

## Overview

UVHTTP uses **xxHash** as the hashing algorithm for route lookup, cache key generation, and data integrity verification.

## Performance Characteristics

### Comparative Advantages

| Feature | xxHash | CRC32 | FNV-1a | MD5 |
|---------|--------|-------|--------|-----|
| **Speed** | Very fast | Fast | Moderate | Slow |
| **Collision rate** | Low | Medium | Medium | Very low |
| **Distribution quality** | Excellent | Good | Moderate | Excellent |
| **Use cases** | Routing, caching, hash tables | Checksums, simple hashing | String hashing | Secure hashing |
| **Cross-platform** | All platforms | All platforms | All platforms | All platforms |

### Performance Data

- 3-5x faster than CRC32
- Approaches the RAM speed limit
- Zero allocation, no memory overhead
- Good cache locality

## API Interface

### Core Hash Functions

```c
#include "uvhttp_hash.h"

// Compute the hash of arbitrary data
uint64_t uvhttp_hash(const void* data, size_t length, uint64_t seed);

// Compute the hash of a string (recommended)
uint64_t uvhttp_hash_string(const char* str);

// Convenience function using the default seed
uint64_t uvhttp_hash_default(const void* data, size_t length);
```

### Usage Examples

#### Basic Usage

```c
#include "uvhttp_hash.h"

// String hashing
const char* username = "john_doe";
uint64_t user_hash = uvhttp_hash_string(username);

// Binary data hashing
struct user_data data;
uint64_t data_hash = uvhttp_hash(&data, sizeof(data), UVHTTP_HASH_DEFAULT_SEED);

// Using the default seed
uint64_t simple_hash = uvhttp_hash_default("hello", 5);
```

#### Cache Key Generation

```c
// Generate a user cache key
char cache_key[128];
snprintf(cache_key, sizeof(cache_key), "user:%s:profile", username);
uint64_t cache_hash = uvhttp_hash_string(cache_key);

// Session key generation
char session_key[256];
snprintf(session_key, sizeof(session_key), "%s:%ld:%s", 
         user_id, timestamp, session_token);
uint64_t session_hash = uvhttp_hash_string(session_key);
```

## Internal Integration

### Router Cache

`src/uvhttp_router_cache.c` hashes route paths with a file-local helper:

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

`UVHTTP_MAX_ROUTE_PATH_LEN` is 256 (`include/uvhttp_constants.h`). The cap bounds hashing work on attacker-controlled paths. `route_hash` is internal to that translation unit, not part of the public API.

### Gzip Cache Keys

`src/uvhttp_response.c` keys the compressed-body cache by body hash plus length:

```c
uvhttp_hash_default(response->body, response->body_length)
```

The same body compresses once and later responses reuse the cached bytes.

## Best Practices

### 1. String Hashing

```c
// Recommended: use the dedicated string hash function
uint64_t hash = uvhttp_hash_string("user_data");

// Avoid: computing the length manually
uint64_t hash = uvhttp_hash("user_data", strlen("user_data"), seed);
```

### 2. Default Seed Usage

```c
// Recommended: use the default seed for consistency
uint64_t hash = uvhttp_hash_default(data, length);

// Optional: custom seed for specific scenarios
uint64_t hash = uvhttp_hash(data, length, custom_seed);
```

### 3. Performance Optimization

```c
// Recommended: cache frequently used hash values
static uint64_t cached_route_hash = 0;
if (!cached_route_hash) {
    cached_route_hash = uvhttp_hash_string("/api/users");
}

// Recommended: reuse the seed when processing in batch
uint64_t seed = UVHTTP_HASH_DEFAULT_SEED;
for (int i = 0; i < count; i++) {
    hashes[i] = uvhttp_hash(data[i], lengths[i], seed);
}
```

### 4. Error Handling

```c
// Recommended: input validation
if (!data || length == 0) {
    return 0; // or an appropriate error value
}

uint64_t hash = uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
```

## Security Considerations

### Applicable Use Cases

xxHash is suitable for the following non-cryptographic scenarios:

- Route lookup and matching
- Cache key generation
- Data integrity verification
- Load balancing
- Hash table implementations

### Non-Applicable Scenarios

xxHash is not suitable for the following cryptographic scenarios:

- Password storage
- Digital signatures
- Encryption key generation
- Security tokens

### Security Enhancement

For higher security, add a salt:

```c
// Combine with a salt value
const char* salt = "random_salt_value";
char combined_data[256];
snprintf(combined_data, sizeof(combined_data), "%s%s", salt, input_data);
uint64_t secure_hash = uvhttp_hash_string(combined_data);
```

## Performance Tuning

### Compilation Optimization

Build in Release mode for best performance:

```cmake
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -DNDEBUG")
endif()
```

### Memory Alignment

xxHash is sensitive to memory alignment. Align data structures:

```c
// Recommended: aligned data structure
typedef struct __attribute__((aligned(8))) {
    char data[64];
} aligned_data_t;

// Avoid misaligned access
```

### Batch Processing

Hash large volumes in batches:

```c
// Batch hash computation example
void batch_hash(const char** strings, int count, uint64_t* results) {
    uint64_t seed = UVHTTP_HASH_DEFAULT_SEED;
    for (int i = 0; i < count; i++) {
        results[i] = uvhttp_hash_string(strings[i]);
    }
}
```

## Troubleshooting

### Common Issues

1. **Inconsistent hash values**
   - Check that the same seed is used
   - Confirm the data is byte-for-byte identical (including length)

2. **Performance below expectations**
   - Ensure compilation in Release mode
   - Check that the data is aligned
   - Consider batch processing to reduce function call overhead

3. **Too many hash collisions**
   - Check whether the data distribution is uniform
   - Consider using a different seed value
   - Evaluate whether longer hash values are needed

### Debugging Tips

```c
// Debug hash computation
void debug_hash(const void* data, size_t length) {
    uint64_t hash = uvhttp_hash(data, length, UVHTTP_HASH_DEFAULT_SEED);
    printf("Data: %.*s, Length: %zu, Hash: %llu\n", 
           (int)length, (char*)data, length, (unsigned long long)hash);
}
```

## References

- [xxHash Official Documentation](https://github.com/Cyan4973/xxHash)
- [xxHash Performance Benchmarks](https://github.com/Cyan4973/xxHash/wiki/Performance-comparison)
- [UVHTTP Architecture Design](./ARCHITECTURE.md)
- [UVHTTP API Reference](../api/API_REFERENCE.md)
