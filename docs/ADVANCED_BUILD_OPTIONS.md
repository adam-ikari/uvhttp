# Advanced Build Options

This document describes advanced build configuration options for UVHTTP that are typically only needed by advanced users or for specific performance tuning scenarios.

## Overview

Most users should use the default configuration values. These advanced options allow fine-tuning of internal parameters for specific use cases, performance optimization, or resource-constrained environments.

## Memory Allocator Configuration

### UVHTTP_ALLOCATOR_TYPE
- **Type**: STRING
- **Default**: 0
- **Description**: Memory allocator type selection
- **Usage**: Choose the memory allocator implementation
- **Options**:
  - `0`: System allocator (malloc/free)
  - `1`: mimalloc allocator (auto-enables `BUILD_WITH_MIMALLOC=ON`)
  - `2`: Custom allocator (application layer implementation)
- **Impact**:
  - System allocator: Standard performance, no dependencies
  - mimalloc: 30-50% faster memory allocation, reduced fragmentation
  - Custom: Full control over memory management
- **Notes**:
  - Option `1` automatically enables `BUILD_WITH_MIMALLOC=ON` for convenience
  - Option `2` requires application layer to implement custom allocator functions

#### Custom Allocator Implementation

When using `UVHTTP_ALLOCATOR_TYPE=2`, you must implement the following functions in your application:

```c
#include <stddef.h>
#include <stdlib.h>

// Custom allocator implementation
void* uvhttp_custom_alloc(size_t size) {
    // Implement your custom allocation logic
    return malloc(size);
}

void uvhttp_custom_free(void* ptr) {
    // Implement your custom free logic
    free(ptr);
}

void* uvhttp_custom_realloc(void* ptr, size_t size) {
    // Implement your custom realloc logic
    return realloc(ptr, size);
}

void* uvhttp_custom_calloc(size_t nmemb, size_t size) {
    // Implement your custom calloc logic
    return calloc(nmemb, size);
}
```

**Example Use Cases**:
- Memory pool allocation for embedded systems
- Tracking memory usage for debugging
- Implementing custom allocation strategies
- Integrating with application-specific memory managers

**Important Notes**:
- These functions must be implemented before including any UVHTTP headers
- Thread safety is the responsibility of the implementer
- All allocation functions must properly handle NULL returns
- Free must safely handle NULL pointers

## HTTP Protocol Configuration

### UVHTTP_MAX_HEADER_NAME_SIZE
- **Type**: STRING
- **Default**: 256
- **Description**: Maximum HTTP header name length in bytes
- **Usage**: Adjust if you need to handle very long header names
- **Impact**: Reducing this value saves memory but may reject valid requests with long header names

### UVHTTP_MAX_HEADER_VALUE_SIZE
- **Type**: STRING
- **Default**: 4096
- **Description**: Maximum HTTP header value length in bytes
- **Usage**: Adjust for applications that need very long header values
- **Impact**: Reducing this value saves memory but may reject valid requests with long header values

### UVHTTP_MAX_HEADERS
- **Type**: STRING
- **Default**: 64
- **Description**: Maximum number of HTTP headers per request
- **Usage**: Adjust for applications that need many headers
- **Impact**: Reducing this value saves memory but may reject valid requests with many headers

### UVHTTP_INLINE_HEADERS_CAPACITY
- **Type**: STRING
- **Default**: 32
- **Description**: Inline headers capacity for optimization
- **Usage**: Adjust based on typical number of headers in your requests
- **Impact**: Higher values use more stack memory but may improve performance for requests with many headers

### UVHTTP_MAX_URL_SIZE
- **Type**: STRING
- **Default**: 2048
- **Description**: Maximum URL length in bytes
- **Usage**: Adjust for applications with very long URLs
- **Impact**: Reducing this value saves memory but may reject valid requests with long URLs

### UVHTTP_MAX_PATH_SIZE
- **Type**: STRING
- **Default**: 1024
- **Description**: Maximum path length in bytes
- **Usage**: Adjust for applications with very long paths
- **Impact**: Reducing this value saves memory but may reject valid requests with long paths

### UVHTTP_MAX_METHOD_SIZE
- **Type**: STRING
- **Default**: 16
- **Description**: Maximum HTTP method length in bytes
- **Usage**: Rarely needs adjustment
- **Impact**: Minimal memory impact

## Connection Management

### UVHTTP_MAX_CONNECTIONS_DEFAULT
- **Type**: STRING
- **Default**: 2048
- **Description**: Default maximum number of concurrent connections
- **Usage**: Adjust based on expected concurrent connections
- **Impact**: Higher values use more memory but allow more concurrent connections

### UVHTTP_MAX_CONNECTIONS_MAX
- **Type**: STRING
- **Default**: 10000
- **Description**: Recommended maximum number of concurrent connections
- **Usage**: Set to your server's capacity limit
- **Impact**: Higher values use more memory but allow more concurrent connections

### UVHTTP_BACKLOG
- **Type**: STRING
- **Default**: 8192
- **Description**: TCP backlog size
- **Usage**: Adjust based on expected connection burst rate
- **Impact**: Higher values use more kernel memory but handle burst connections better

### UVHTTP_CONNECTION_TIMEOUT_DEFAULT
- **Type**: STRING
- **Default**: 60
- **Description**: Default connection timeout in seconds
- **Usage**: Adjust based on your application's needs
- **Impact**: Longer timeouts keep idle connections alive longer, using more resources

## Buffer Configuration

### UVHTTP_INITIAL_BUFFER_SIZE
- **Type**: STRING
- **Default**: 8192
- **Description**: Initial buffer size in bytes
- **Usage**: Adjust based on typical request/response sizes
- **Impact**: Larger values use more memory but may reduce reallocations

### UVHTTP_MAX_BODY_SIZE
- **Type**: STRING
- **Default**: 1048576 (1MB)
- **Description**: Maximum request body size in bytes
- **Usage**: Adjust based on your application's needs
- **Impact**: Reducing this value saves memory but may reject valid large requests

### UVHTTP_READ_BUFFER_SIZE
- **Type**: STRING
- **Default**: 16384 (16KB)
- **Description**: Read buffer size in bytes
- **Usage**: Adjust based on network conditions and typical data sizes
- **Impact**: Larger values use more memory but may improve throughput

## Async File Operations

### UVHTTP_ASYNC_FILE_BUFFER_SIZE
- **Type**: STRING
- **Default**: 65536 (64KB)
- **Description**: Async file buffer size in bytes
- **Usage**: Adjust based on file sizes and I/O patterns
- **Impact**: Larger values use more memory but may improve file I/O performance

### UVHTTP_ASYNC_FILE_MAX_CONCURRENT
- **Type**: STRING
- **Default**: 64
- **Description**: Maximum number of concurrent file reads
- **Usage**: Adjust based on disk I/O capacity and concurrency requirements
- **Impact**: Higher values use more memory but allow more concurrent file operations

### UVHTTP_ASYNC_FILE_MAX_SIZE
- **Type**: STRING
- **Default**: 10485760 (10MB)
- **Description**: Maximum file size for async operations in bytes
- **Usage**: Files larger than this will use synchronous operations
- **Impact**: Larger values use more memory but allow async operations for larger files

## Static File Service

### UVHTTP_STATIC_MAX_CACHE_SIZE
- **Type**: STRING
- **Default**: 1048576 (1MB)
- **Description**: Static file cache maximum size in bytes
- **Usage**: Adjust based on available memory and file access patterns
- **Impact**: Larger values use more memory but improve cache hit rate

### UVHTTP_STATIC_MAX_PATH_SIZE
- **Type**: STRING
- **Default**: 1024
- **Description**: Maximum static file path length in bytes
- **Usage**: Adjust for applications with very long file paths
- **Impact**: Reducing this value saves memory but may reject valid file paths

### UVHTTP_STATIC_MAX_CONTENT_LENGTH
- **Type**: STRING
- **Default**: 32
- **Description**: Static file Content-Length maximum length
- **Usage**: Rarely needs adjustment
- **Impact**: Minimal memory impact

### UVHTTP_STATIC_MAX_FILE_SIZE
- **Type**: STRING
- **Default**: 1073741824 (1GB)
- **Description**: Maximum static file size in bytes
- **Usage**: Adjust based on your application's needs
- **Impact**: Reducing this value prevents serving very large files

### UVHTTP_STATIC_SMALL_FILE_THRESHOLD
- **Type**: STRING
- **Default**: 4096 (4KB)
- **Description**: Small file threshold in bytes
- **Usage**: Files smaller than this are handled differently
- **Impact**: Adjusting this value affects performance characteristics for small files

## WebSocket Configuration

### UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE
- **Type**: STRING
- **Default**: 16777216 (16MB)
- **Description**: WebSocket default maximum frame size in bytes
- **Usage**: Adjust based on your application's message size requirements
- **Impact**: Reducing this value saves memory but may reject valid large frames

### UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE
- **Type**: STRING
- **Default**: 67108864 (64MB)
- **Description**: WebSocket default maximum message size in bytes
- **Usage**: Adjust based on your application's message size requirements
- **Impact**: Reducing this value saves memory but may reject valid large messages

### UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE
- **Type**: STRING
- **Default**: 65536 (64KB)
- **Description**: WebSocket default receive buffer size in bytes
- **Usage**: Adjust based on typical message sizes
- **Impact**: Larger values use more memory but may improve performance for large messages

### UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL
- **Type**: STRING
- **Default**: 30
- **Description**: WebSocket default ping interval in seconds
- **Usage**: Adjust based on network conditions and application requirements
- **Impact**: Shorter intervals detect dead connections sooner but use more bandwidth

### UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT
- **Type**: STRING
- **Default**: 10
- **Description**: WebSocket default ping timeout in seconds
- **Usage**: Adjust based on network conditions and application requirements
- **Impact**: Shorter timeouts detect dead connections sooner but may cause false positives

## TCP Configuration

### UVHTTP_TCP_KEEPALIVE_TIMEOUT
- **Type**: STRING
- **Default**: 60
- **Description**: TCP keepalive timeout in seconds
- **Usage**: Adjust based on network conditions and application requirements
- **Impact**: Shorter timeouts detect dead connections sooner but may cause false positives

### UVHTTP_CLIENT_IP_BUFFER_SIZE
- **Type**: STRING
- **Default**: 64
- **Description**: Client IP buffer size in bytes
- **Usage**: Adjust for applications that need to store very long client IP addresses
- **Impact**: Minimal memory impact

## Sendfile Configuration

### UVHTTP_SENDFILE_TIMEOUT_MS
- **Type**: STRING
- **Default**: 30000
- **Description**: Sendfile timeout in milliseconds
- **Usage**: Adjust based on network conditions and file sizes
- **Impact**: Longer timeouts may keep connections alive longer for slow transfers

### UVHTTP_SENDFILE_MAX_RETRY
- **Type**: STRING
- **Default**: 2
- **Description**: Sendfile maximum retry count
- **Usage**: Adjust based on network reliability
- **Impact**: Higher values may improve reliability but may delay error detection

### UVHTTP_SENDFILE_CHUNK_SIZE
- **Type**: STRING
- **Default**: 262144 (256KB)
- **Description**: Sendfile chunk size in bytes
- **Usage**: Adjust based on network conditions and file sizes
- **Impact**: Larger values use more memory but may improve throughput

### UVHTTP_SENDFILE_MIN_FILE_SIZE
- **Type**: STRING
- **Default**: 65536 (64KB)
- **Description**: Sendfile minimum file size in bytes
- **Usage**: Files smaller than this use regular file operations
- **Impact**: Adjusting this value affects performance characteristics for small files

## File Size Thresholds

### UVHTTP_FILE_SIZE_SMALL
- **Type**: STRING
- **Default**: 1048576 (1MB)
- **Description**: Small file threshold in bytes
- **Usage**: Used for performance optimization decisions
- **Impact**: Adjusting this value affects performance characteristics for different file sizes

### UVHTTP_FILE_SIZE_MEDIUM
- **Type**: STRING
- **Default**: 10485760 (10MB)
- **Description**: Medium file threshold in bytes
- **Usage**: Used for performance optimization decisions
- **Impact**: Adjusting this value affects performance characteristics for different file sizes

### UVHTTP_FILE_SIZE_LARGE
- **Type**: STRING
- **Default**: 104857600 (100MB)
- **Description**: Large file threshold in bytes
- **Usage**: Used for performance optimization decisions
- **Impact**: Adjusting this value affects performance characteristics for different file sizes

## Chunk Size Configuration

### UVHTTP_CHUNK_SIZE_SMALL
- **Type**: STRING
- **Default**: 65536 (64KB)
- **Description**: Small chunk size in bytes
- **Usage**: Used for performance optimization decisions
- **Impact**: Adjusting this value affects performance characteristics for different chunk sizes

### UVHTTP_CHUNK_SIZE_MEDIUM
- **Type**: STRING
- **Default**: 262144 (256KB)
- **Description**: Medium chunk size in bytes
- **Usage**: Used for performance optimization decisions
- **Impact**: Adjusting this value affects performance characteristics for different chunk sizes

### UVHTTP_CHUNK_SIZE_LARGE
- **Type**: STRING
- **Default**: 1048576 (1MB)
- **Description**: Large chunk size in bytes
- **Usage**: Used for performance optimization decisions
- **Impact**: Adjusting this value affects performance characteristics for different chunk sizes

## Cache Configuration

### UVHTTP_CACHE_DEFAULT_MAX_ENTRIES
- **Type**: STRING
- **Default**: 1000
- **Description**: Cache default maximum number of entries
- **Usage**: Adjust based on available memory and cache access patterns
- **Impact**: Higher values use more memory but improve cache hit rate

### UVHTTP_CACHE_DEFAULT_TTL
- **Type**: STRING
- **Default**: 3600
- **Description**: Cache default TTL in seconds
- **Usage**: Adjust based on data freshness requirements
- **Impact**: Longer TTLs may serve stale data but improve cache hit rate

### UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE
- **Type**: STRING
- **Default**: 10
- **Description**: LRU cache batch eviction size
- **Usage**: Adjust based on cache access patterns
- **Impact**: Larger values use more CPU during eviction but may improve overall cache performance

## Socket Configuration

### UVHTTP_SOCKET_SEND_BUF_SIZE
- **Type**: STRING
- **Default**: 262144 (256KB)
- **Description**: Socket send buffer size in bytes
- **Usage**: Adjust based on network conditions and throughput requirements
- **Impact**: Larger values use more memory but may improve throughput

### UVHTTP_SOCKET_RECV_BUF_SIZE
- **Type**: STRING
- **Default**: 262144 (256KB)
- **Description**: Socket receive buffer size in bytes
- **Usage**: Adjust based on network conditions and throughput requirements
- **Impact**: Larger values use more memory but may improve throughput

## System Configuration

### UVHTTP_PAGE_SIZE
- **Type**: STRING
- **Default**: 4096
- **Description**: Memory page size in bytes
- **Usage**: Should match system page size
- **Impact**: Incorrect values may cause performance issues

### UVHTTP_IP_OCTET_MAX_VALUE
- **Type**: STRING
- **Default**: 255
- **Description**: IP octet maximum value
- **Usage**: Should not be changed
- **Impact**: Changing this value may cause incorrect IP address parsing

## Rate Limit Configuration

### UVHTTP_RATE_LIMIT_MAX_REQUESTS
- **Type**: STRING
- **Default**: 1000000
- **Description**: Rate limit maximum number of requests
- **Usage**: Adjust based on your application's rate limiting needs
- **Impact**: Higher values allow more requests but may increase server load

### UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS
- **Type**: STRING
- **Default**: 86400
- **Description**: Rate limit maximum time window in seconds
- **Usage**: Adjust based on your application's rate limiting needs
- **Impact**: Longer windows allow more flexibility but may reduce rate limiting effectiveness

### UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS
- **Type**: STRING
- **Default**: 10
- **Description**: Rate limit minimum timeout in seconds
- **Usage**: Adjust based on your application's rate limiting needs
- **Impact**: Shorter timeouts may cause more false positives

## How to Use Advanced Options

To configure advanced options, use CMake's `-D` flag:

```bash
cmake -DUVHTTP_MAX_HEADER_NAME_SIZE=512 \
      -DUVHTTP_MAX_BODY_SIZE=5242880 \
      -DUVHTTP_CACHE_DEFAULT_MAX_ENTRIES=5000 \
      ..
```

## Performance Tuning Guidelines

### Memory-Constrained Environments
For memory-constrained environments, consider reducing:
- `UVHTTP_MAX_CONNECTIONS_DEFAULT`
- `UVHTTP_MAX_HEADERS`
- `UVHTTP_MAX_BODY_SIZE`
- `UVHTTP_CACHE_DEFAULT_MAX_ENTRIES`
- `UVHTTP_READ_BUFFER_SIZE`

### High-Throughput Environments
For high-throughput environments, consider increasing:
- `UVHTTP_MAX_CONNECTIONS_DEFAULT`
- `UVHTTP_READ_BUFFER_SIZE`
- `UVHTTP_SOCKET_SEND_BUF_SIZE`
- `UVHTTP_SOCKET_RECV_BUF_SIZE`
- `UVHTTP_CACHE_DEFAULT_MAX_ENTRIES`

### Low-Latency Environments
For low-latency environments, consider:
- Reducing `UVHTTP_CONNECTION_TIMEOUT_DEFAULT`
- Reducing `UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL`
- Increasing `UVHTTP_READ_BUFFER_SIZE`
- Increasing `UVHTTP_SOCKET_SEND_BUF_SIZE`

## Related Documentation

- [Build Configuration Matrix](BUILD_CONFIGURATION_MATRIX.md)
- [Developer Guide](guide/DEVELOPER_GUIDE.md)
- [Performance Benchmark](dev/PERFORMANCE_BENCHMARK.md)