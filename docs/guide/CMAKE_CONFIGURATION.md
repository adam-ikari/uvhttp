# CMake Configuration Guide

## Overview

UVHTTP supports configuring various compile-time constants through CMake, allowing you to adjust the library's behavior and performance to suit your needs.

## Configuration Methods

### 1. Basic Configuration

Edit the `option()` or `set()` configuration constants in `CMakeLists.txt`, then run `make build`:

```bash
make build
```

Preconfigure the build in `CMakeLists.txt`:

```cmake
set(UVHTTP_MAX_HEADER_NAME_SIZE 512 CACHE STRING "Max HTTP header name size")
set(UVHTTP_MAX_HEADER_VALUE_SIZE 8192 CACHE STRING "Max HTTP header value size")
```

## Configurable Constants

### HTTP Related

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_MAX_HEADER_NAME_SIZE` | 256 | Maximum HTTP header name length | 256-512 |
| `UVHTTP_MAX_HEADER_VALUE_SIZE` | 4096 | Maximum HTTP header value length | 4096-8192 |
| `UVHTTP_MAX_HEADERS` | 64 | Maximum number of HTTP headers | 32-128 |
| `UVHTTP_MAX_URL_SIZE` | 2048 | Maximum URL length | 2048-4096 |
| `UVHTTP_MAX_PATH_SIZE` | 1024 | Maximum path length | 512-2048 |
| `UVHTTP_MAX_METHOD_SIZE` | 16 | Maximum HTTP method length | 16 |
| `UVHTTP_MAX_BODY_SIZE` | 1048576 | Maximum request body size (bytes) | Adjust per requirements |

### Connection Management

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_MAX_CONNECTIONS_DEFAULT` | 2048 | Default maximum number of connections | 1024-4096 |
| `UVHTTP_MAX_CONNECTIONS_MAX` | 10000 | Maximum recommended number of connections | 10000-100000 |
| `UVHTTP_BACKLOG` | 8192 | TCP backlog size | 1024-8192 |
| `UVHTTP_CONNECTION_TIMEOUT_DEFAULT` | 60 | Connection timeout (seconds) | 30-120 |
| `UVHTTP_TCP_KEEPALIVE_TIMEOUT` | 60 | TCP keepalive timeout (seconds) | 30-120 |

### Buffer Management

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_INLINE_HEADERS_CAPACITY` | 32 | Inline header capacity | 16-64 |
| `UVHTTP_INITIAL_BUFFER_SIZE` | 8192 | Initial buffer size (bytes) | 8192-16384 |
| `UVHTTP_READ_BUFFER_SIZE` | 16384 | Read buffer size (bytes) | 16384-65536 |

### Static File Serving

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_STATIC_MAX_CACHE_SIZE` | 1048576 | Static file cache size (bytes) | Adjust per memory |
| `UVHTTP_STATIC_MAX_PATH_SIZE` | 1024 | Maximum static file path length | 512-2048 |
| `UVHTTP_STATIC_MAX_FILE_SIZE` | 10485760 | Maximum static file size (bytes) | Adjust per requirements |
| `UVHTTP_STATIC_SMALL_FILE_THRESHOLD` | 4096 | Small file threshold (bytes) | 4096-8192 |

### WebSocket

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_WEBSOCKET_DEFAULT_MAX_FRAME_SIZE` | 16777216 | Maximum WebSocket frame size (bytes) | Adjust per requirements |
| `UVHTTP_WEBSOCKET_DEFAULT_MAX_MESSAGE_SIZE` | 67108864 | Maximum WebSocket message size (bytes) | Adjust per requirements |
| `UVHTTP_WEBSOCKET_DEFAULT_RECV_BUFFER_SIZE` | 65536 | WebSocket receive buffer size (bytes) | 32768-131072 |
| `UVHTTP_WEBSOCKET_DEFAULT_PING_INTERVAL` | 30 | WebSocket ping interval (seconds) | 10-60 |
| `UVHTTP_WEBSOCKET_DEFAULT_PING_TIMEOUT` | 10 | WebSocket ping timeout (seconds) | 5-30 |

### Asynchronous File Operations

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_ASYNC_FILE_BUFFER_SIZE` | 65536 | Async file buffer size (bytes) | 32768-131072 |
| `UVHTTP_ASYNC_FILE_MAX_CONCURRENT` | 64 | Maximum concurrent file reads | 32-128 |
| `UVHTTP_ASYNC_FILE_MAX_SIZE` | 10485760 | Maximum async file size (bytes) | Adjust per requirements |

### Performance Optimization

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_SENDFILE_CHUNK_SIZE` | 65536 | sendfile chunk size (bytes) | 32768-131072 |
| `UVHTTP_SENDFILE_TIMEOUT_MS` | 30000 | sendfile timeout (milliseconds) | 10000-60000 |
| `UVHTTP_STATIC_MAX_CACHE_SIZE` | 10485760 | Static file cache size (bytes, 10MB) | 5242880-52428800 |
| `UVHTTP_LRU_CACHE_BATCH_EVICTION_SIZE` | 2 | LRU cache batch eviction size | 1-10 |

### Rate Limiting

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_RATE_LIMIT_MAX_REQUESTS` | 1000000 | Maximum number of rate-limit requests | Adjust per requirements |
| `UVHTTP_RATE_LIMIT_MAX_WINDOW_SECONDS` | 86400 | Maximum rate-limit time window (seconds) | Adjust per requirements |
| `UVHTTP_RATE_LIMIT_MIN_TIMEOUT_SECONDS` | 10 | Minimum rate-limit timeout (seconds) | 5-30 |

### Other

| Constant | Default | Description | Recommended |
|----------|---------|-------------|-------------|
| `UVHTTP_CLIENT_IP_BUFFER_SIZE` | 64 | Client IP buffer size | 64 |
| `UVHTTP_IP_OCTET_MAX_VALUE` | 255 | Maximum IP octet value | 255 |

## Configuration Examples

### Example 1: High Concurrency Scenario

Edit `CMakeLists.txt`, add or modify the following configuration, then run `make build`:

```bash
make build
```

### Example 2: Large File Transfer

Edit `CMakeLists.txt`, add or modify the following configuration, then run `make build`:

```bash
make build
```

### Example 3: Memory-Constrained Environments

Edit `CMakeLists.txt`, add or modify the following configuration, then run `make build`:

```bash
make build
```

### Example 4: WebSocket Optimization

Edit `CMakeLists.txt`, add or modify the following configuration, then run `make build`:

```bash
make build
```

## Notes

1. **Memory Impact**: Increasing buffer and cache sizes increases memory usage
2. **Performance Trade-off**: Larger buffers can improve performance but increase memory usage
3. **Platform Limitations**: Some values are constrained by operating system limits (such as the number of file descriptors)
4. **Test Verification**: Conduct thorough performance testing after modifying configuration
5. **Documentation Updates**: If you change default values, please update the relevant documentation

## Verifying Configuration

After compilation, you can verify your configuration in the following ways:

```bash
# Build the project
make build

# View macro definitions in the compile commands
grep UVHTTP_MAX_HEADER_NAME_SIZE build/CMakeCache.txt

# Run tests to verify
./dist/bin/uvhttp_unit_tests
```

## Related Documentation

- [API Reference](../../api/API_REFERENCE.md)
- [Performance Benchmark](../dev/PERFORMANCE_BENCHMARK.md)
- [Contributor Guide](DEVELOPER_GUIDE.md)
